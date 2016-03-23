#include "env.h"

#include <vector>

#include <assert.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "xdebug.h"


namespace slash {

/*
 * size of initial mmap size
 */
static size_t kMmapBoundSize = 1024 * 1024 * 4;

void SetMmapBoundSize(size_t size) {
  kMmapBoundSize = size;
}

static Status IOError(const std::string& context, int err_number) {
    return Status::IOError(context, strerror(err_number));
}

int CreateDir(const std::string& path) {
  int res = 0;

  if (mkdir(path.c_str(), 0755) != 0) {
    log_warn("mkdir error is %s", strerror(errno));
  }
  return res;
}

int FileExists(const std::string& path) {
  return access(path.c_str(), F_OK) == 0;
}

Status DeleteFile(const std::string& fname) {
  Status result;
  if (unlink(fname.c_str()) != 0) {
    result = IOError(fname, errno);
  }
  return result;
};

int GetChildren(const std::string& dir, std::vector<std::string>& result) {
  int res = 0;
  result.clear();
  DIR* d = opendir(dir.c_str());
  if (d == NULL) {
    return errno;
  }
  struct dirent* entry;
  while ((entry = readdir(d)) != NULL) {
    result.push_back(entry->d_name);
  }
  closedir(d);
  return res;
}

int RenameFile(const std::string& oldname, const std::string& newname) {
  return rename(oldname.c_str(), newname.c_str());
}

int IsDir(const std::string& path) {
  struct stat buf;
  int ret = stat(path.c_str(), &buf);
  if (0 == ret) {
    if (buf.st_mode & S_IFDIR) {
      //folder
      return 0;
    } else {
      //file
      return 1;
    }
  }
  return -1;
}

int DeleteDir(const std::string& path)
{
  char chBuf[256];
  DIR * dir = NULL;
  struct dirent *ptr;
  int ret = 0;
  dir = opendir(path.c_str());
  if (NULL == dir) {
    return -1;
  }
  while((ptr = readdir(dir)) != NULL) {
    ret = strcmp(ptr->d_name, ".");
    if (0 == ret) {
      continue;
    }
    ret = strcmp(ptr->d_name, "..");
    if (0 == ret) {
      continue;
    }
    snprintf(chBuf, 256, "%s/%s", path.c_str(), ptr->d_name);
    ret = IsDir(chBuf);
    if (0 == ret) {
      //is dir
      ret = DeleteDir(chBuf);
      if (0 != ret) {
        return -1;
      }
    }
    else if (1 == ret) {
      //is file
      ret = remove(chBuf);
      if(0 != ret) {
        return -1;
      }
    }
  }
  (void)closedir(dir);
  ret = remove(path.c_str());
  if (0 != ret) {
    return -1;
  }
  return 0;
}

bool DeleteDirIfExist(const std::string& path) {
  if (IsDir(path) == 0 && DeleteDir(path) != 0) {
    return false;
  }
  return true;
}

uint64_t Du(const std::string& filename) {
	struct stat statbuf;
	uint64_t sum;
  if (lstat(filename.c_str(), &statbuf) != 0) {
		return 0;
	}
	if (S_ISLNK(statbuf.st_mode) && stat(filename.c_str(), &statbuf) != 0) {
	  return 0;
	}
  sum = statbuf.st_size;
	if (S_ISDIR(statbuf.st_mode)) {
		DIR *dir = NULL;
		struct dirent *entry;
		std::string newfile;

		dir = opendir(filename.c_str());
		if (!dir) {
			return sum;
		}
		while ((entry = readdir(dir))) {
			if (strcmp(entry->d_name, "..") == 0 || strcmp(entry->d_name, ".") == 0) {
				continue;
      }
			newfile = filename + "/" + entry->d_name;
			sum += Du(newfile);
		}
		closedir(dir);
	}
	return sum;
}

SequentialFile::~SequentialFile() {
}

class PosixSequentialFile: public SequentialFile {
private:
  std::string filename_;
  FILE* file_;

public:
  virtual void setUnBuffer() {
    setbuf(file_, NULL);
  }

  PosixSequentialFile(const std::string& fname, FILE* f)
      : filename_(fname), file_(f) { setbuf(file_, NULL); }
  //virtual ~PosixSequentialFile() { }
  virtual ~PosixSequentialFile() { fclose(file_); }

  virtual Status Read(size_t n, Slice* result, char* scratch) override {
    Status s;
    size_t r = fread_unlocked(scratch, 1, n, file_);

    *result = Slice(scratch, r);

    if (r < n) {
      if (feof(file_)) {
        s = Status::EndFile(filename_, "end file");
        // We leave status as ok if we hit the end of the file
      } else {
        // A partial read with an error: return a non-ok status
        s = IOError(filename_, errno);
      }
    }
    return s;
  }

  virtual Status Skip(uint64_t n) override {
    if (fseek(file_, n, SEEK_CUR)) {
      return IOError(filename_, errno);
    }
    return Status::OK();
  }

  virtual char *ReadLine(char* buf, int n) override {
    return fgets(buf, n, file_);
  }

  virtual Status Close() {
    if (fclose(file_) != 0) {
      return IOError(filename_, errno);
    }
    file_ = NULL;
    return Status::OK();
  }
};

WritableFile::~WritableFile() {
}

// We preallocate up to an extra megabyte and use memcpy to append new
// data to the file.  This is safe since we either properly close the
// file before reading from it, or for log files, the reading code
// knows enough to skip zero suffixes.
class PosixMmapFile : public WritableFile
{
 private:
  std::string filename_;
  int fd_;
  size_t page_size_;
  size_t map_size_;       // How much extra memory to map at a time
  char* base_;            // The mapped region
  char* limit_;           // Limit of the mapped region
  char* dst_;             // Where to write next  (in range [base_,limit_])
  char* last_sync_;       // Where have we synced up to
  uint64_t file_offset_;  // Offset of base_ in file
  uint64_t write_len_;    // The data that written in the file


  // Have we done an munmap of unsynced data?
  bool pending_sync_;

  // Roundup x to a multiple of y
  static size_t Roundup(size_t x, size_t y) {
    return ((x + y - 1) / y) * y;
  }

  static size_t TrimDown(size_t x, size_t y) {
    return (x / y) * y;
  }
  size_t TruncateToPageBoundary(size_t s) {
    s -= (s & (page_size_ - 1));
    assert((s % page_size_) == 0);
    return s;
  }

  bool UnmapCurrentRegion() {
    bool result = true;
    if (base_ != NULL) {
      if (last_sync_ < limit_) {
        // Defer syncing this data until next Sync() call, if any
        pending_sync_ = true;
      }
      if (munmap(base_, limit_ - base_) != 0) {
        result = false;
      }
      file_offset_ += limit_ - base_;
      base_ = NULL;
      limit_ = NULL;
      last_sync_ = NULL;
      dst_ = NULL;

      // Increase the amount we map the next time, but capped at 1MB
      if (map_size_ < (1<<20)) {
        map_size_ *= 2;
      }
    }
    return result;
  }

  bool MapNewRegion() {
    assert(base_ == NULL);
    if (ftruncate(fd_, file_offset_ + map_size_) < 0) {
      log_warn("ftruncate error");
      return false;
    }
    //log_info("map_size %d fileoffset %llu", map_size_, file_offset_);
    void* ptr = mmap(NULL, map_size_, PROT_READ | PROT_WRITE, MAP_SHARED,
                     fd_, file_offset_);
    if (ptr == MAP_FAILED) {
      log_warn("mmap failed");
      return false;
    }
    base_ = reinterpret_cast<char*>(ptr);
    limit_ = base_ + map_size_;
    dst_ = base_ + write_len_;
    write_len_ = 0;
    last_sync_ = base_;
    return true;
  }

 public:
  PosixMmapFile(const std::string& fname, int fd, size_t page_size, uint64_t write_len = 0)
      : filename_(fname),
      fd_(fd),
      page_size_(page_size),
      map_size_(Roundup(kMmapBoundSize, page_size)),
      base_(NULL),
      limit_(NULL),
      dst_(NULL),
      last_sync_(NULL),
      file_offset_(0),
      write_len_(write_len),
      pending_sync_(false) {
        if (write_len_ != 0) {
          while (map_size_ < write_len_) {
            map_size_ += (1024 * 1024);
          }
        }
        assert((page_size & (page_size - 1)) == 0);
      }


  ~PosixMmapFile() {
    if (fd_ >= 0) {
      PosixMmapFile::Close();
    }
  }

  virtual Status Append(const Slice& data) {
    const char* src = data.data();
    size_t left = data.size();
    while (left > 0) {
      assert(base_ <= dst_);
      assert(dst_ <= limit_);
      size_t avail = limit_ - dst_;
      if (avail == 0) {
        if (!UnmapCurrentRegion() || !MapNewRegion()) {
          return IOError(filename_, errno);
        }
      }
      size_t n = (left <= avail) ? left : avail;
      memcpy(dst_, src, n);
      dst_ += n;
      src += n;
      left -= n;
    }
    return Status::OK();
  }

  virtual Status Close() {
    Status s;
    size_t unused = limit_ - dst_;
    if (!UnmapCurrentRegion()) {
      s = IOError(filename_, errno);
    } else if (unused > 0) {
      // Trim the extra space at the end of the file
      if (ftruncate(fd_, file_offset_ - unused) < 0) {
        s = IOError(filename_, errno);
      }
    }

    if (close(fd_) < 0) {
      if (s.ok()) {
        s = IOError(filename_, errno);
      }
    }

    fd_ = -1;
    base_ = NULL;
    limit_ = NULL;
    return s;
  }

  virtual Status Flush() {
    return Status::OK();
  }

  virtual Status Sync() {
    Status s;

    if (pending_sync_) {
      // Some unmapped data was not synced
      pending_sync_ = false;
      if (fdatasync(fd_) < 0) {
        s = IOError(filename_, errno);
      }
    }

    if (dst_ > last_sync_) {
      // Find the beginnings of the pages that contain the first and last
      // bytes to be synced.
      size_t p1 = TruncateToPageBoundary(last_sync_ - base_);
      size_t p2 = TruncateToPageBoundary(dst_ - base_ - 1);
      last_sync_ = dst_;
      if (msync(base_ + p1, p2 - p1 + page_size_, MS_SYNC) < 0) {
        s = IOError(filename_, errno);
      }
    }

    return s;
  }

  virtual uint64_t Filesize() {
    return write_len_ + file_offset_ + (dst_ - base_);
  }
};


RWFile::~RWFile() {
}

class MmapRWFile : public RWFile
{
 public:
  MmapRWFile(const std::string& fname, int fd, size_t page_size)
      : filename_(fname),
      fd_(fd),
      page_size_(page_size),
      map_size_(Roundup(65536, page_size)),
      base_(NULL) {
        DoMapRegion();
      }

  ~MmapRWFile() {
    if (fd_ >= 0) {
      munmap(base_, map_size_);
    }
  }

  bool DoMapRegion() {
    if (ftruncate(fd_, map_size_) < 0) {
      return false;
    }
    void* ptr = mmap(NULL, map_size_, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0);
    if (ptr == MAP_FAILED) {
      return false;
    }
    base_ = reinterpret_cast<char*>(ptr);
    return true;
  }

  char* GetData() { return base_; }
  char* base() { return base_; }

 private:
  static size_t Roundup(size_t x, size_t y) {
    return ((x + y - 1) / y) * y;
  }
  std::string filename_;
  int fd_;
  size_t page_size_;
  size_t map_size_;
  char* base_;
};

Status NewSequentialFile(const std::string& fname, SequentialFile** result) {
  FILE* f = fopen(fname.c_str(), "r");
  if (f == NULL) {
    *result = NULL;
    return IOError(fname, errno);
  } else {
    *result = new PosixSequentialFile(fname, f);
    return Status::OK();
  }
}

Status NewWritableFile(const std::string& fname, WritableFile** result) {
  Status s;
  const int fd = open(fname.c_str(), O_CREAT | O_RDWR | O_TRUNC, 0644);
  if (fd < 0) {
    *result = NULL;
    s = IOError(fname, errno);
  } else {
    *result = new PosixMmapFile(fname, fd, kPageSize);
  }
  return s;
}

Status NewRWFile(const std::string& fname, RWFile** result) {
  Status s;
  const int fd = open(fname.c_str(), O_CREAT | O_RDWR, 0644);
  if (fd < 0) {
    *result = NULL;
    s = IOError(fname, errno);
  } else {
    *result = new MmapRWFile(fname, fd, kPageSize);
  }
  return s;
}

Status AppendWritableFile(const std::string& fname, WritableFile** result, uint64_t write_len) {
  Status s;
  const int fd = open(fname.c_str(), O_RDWR, 0644);
  if (fd < 0) {
    *result = NULL;
    s = IOError(fname, errno);
  } else {
    *result = new PosixMmapFile(fname, fd, kPageSize, write_len);
  }
  return s;
}


}   // namespace slash
