#include "env.h"

#include <vector>

#include <assert.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "xdebug.h"


namespace slash {

static size_t kMmapBoundSize = 1024 * 1024 * 4;
static const size_t kPageSize = getpagesize();

int CreateDir(const std::string& path)
{
  int res = 0;

  if (mkdir(path.c_str(), 0755) != 0) {
    log_warn("mkdir error is %s", strerror(errno));
  }
  return res;
}

int FileExists(const std::string& path)
{
  return access(path.c_str(), F_OK) == 0;
}

int GetChildren(const std::string& dir, std::vector<std::string>& result) 
{
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

int RenameFile(const std::string& oldname, const std::string& newname)
{
  return rename(oldname.c_str(), newname.c_str());
}


SequentialFile::~SequentialFile() {
}

class PosixSequentialFile: public SequentialFile 
{
 public:
  PosixSequentialFile(std::string fname, FILE* f) :
      fname_(fname),
      f_(f)
  {};

  /*
   * if read to the EOF return EOF
   */
  virtual int Read(size_t n, char *&result, char *scratch) override {
    size_t r = 0;
    do {
      r = fread(scratch, 1, n, f_);
    } while (r == 0 && ferror(f_) && errno == EINTR);

    result = scratch;

    if (r < n) {
      if (feof(f_)) {
        return 0;
      } else {
        return errno;
      }
    }
    return r;
  };

  virtual char *ReadLine(char* buf, int n) override {
    return fgets(buf, n, f_);
  }

  virtual int Skip(uint64_t n) override {
    if (fseek(f_, static_cast<long int>(n), SEEK_CUR)) {
      return errno;
    }
    return 0;
  };

  virtual int Close() override {
    int ret = fclose(f_);
    f_ = NULL;
    return ret;
  }

 private:
  std::string fname_;
  FILE* f_;

  // not allow copy and copy assign construct
  PosixSequentialFile(const PosixSequentialFile&);
  void operator =(const PosixSequentialFile&);

};

WritableFile::WritableFile() {
}

WritableFile::~WritableFile() {
}

// We preallocate up to an extra megabyte and use memcpy to append new
// data to the file.  This is safe since we either properly close the
// file before reading from it, or for log files, the reading code
// knows enough to skip zero suffixes.
class PosixMmapFile : public WritableFile {
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
    // log_info("map_size %d fileoffset %llu", map_size_, file_offset_);
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

  virtual int Append(const char* data, int len) override {
    const char* src = data;
    size_t left = len; 
    while (left > 0) {
      assert(base_ <= dst_);
      assert(dst_ <= limit_);
      size_t avail = limit_ - dst_;
      if (avail == 0) {
        if (!UnmapCurrentRegion() || !MapNewRegion()) {
          return -1;
        }
      }
      size_t n = (left <= avail) ? left : avail;
      memcpy(dst_, src, n);
      dst_ += n;
      src += n;
      left -= n;
    }
    return 0;
  }

  virtual int Close() {
    int ret = 0;
    size_t unused = limit_ - dst_;
    if (!UnmapCurrentRegion()) {
      //s = IOError(filename_, errno);
      ret = -1;
    } else if (unused > 0) {
      // Trim the extra space at the end of the file
      if (ftruncate(fd_, file_offset_ - unused) < 0) {
        //s = IOError(filename_, errno);
        ret = -1;
      }
    }

    if (close(fd_) < 0) {
      if (!ret) {
        //s = IOError(filename_, errno);
        ret = -1;
      }
    }

    fd_ = -1;
    base_ = NULL;
    limit_ = NULL;
    return ret;
  }

  virtual int Flush() {
    //return Status::OK();
    return 0;
  }

  virtual int Sync() {
    int ret = 0;

    if (pending_sync_) {
      // Some unmapped data was not synced
      pending_sync_ = false;
      if (fdatasync(fd_) < 0) {
        ret = -1;
      }
    }

    if (dst_ > last_sync_) {
      // Find the beginnings of the pages that contain the first and last
      // bytes to be synced.
      size_t p1 = TruncateToPageBoundary(last_sync_ - base_);
      size_t p2 = TruncateToPageBoundary(dst_ - base_ - 1);
      last_sync_ = dst_;
      if (msync(base_ + p1, p2 - p1 + page_size_, MS_SYNC) < 0) {
        ret = -1;
      }
    }

    return ret;
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

int NewSequentialFile(const std::string& fname, SequentialFile** result)
{
  FILE *f = fopen(fname.c_str(), "r");
  if (f == NULL) {
    *result = NULL;
    return -1;
  } else {
    *result = new PosixSequentialFile(fname, f);
    return 0;
  }
}

int NewWritableFile(const std::string& fname, WritableFile** result) {
  const int fd = open(fname.c_str(), O_CREAT | O_RDWR | O_TRUNC, 0644);
  if (fd < 0) {
    *result = NULL;
    return -1;
  } else {
    *result = new PosixMmapFile(fname, fd, kPageSize);
  }
  return 0;
}

int NewRWFile(const std::string& fname, RWFile** result) {
  const int fd = open(fname.c_str(), O_CREAT | O_RDWR, 0644);
  if (fd < 0) {
    *result = NULL;
    return -1;
  } else {
    *result = new MmapRWFile(fname, fd, kPageSize);
  }
  return 0;
}

int AppendWritableFile(const std::string& fname, WritableFile** result, uint64_t write_len) {
  const int fd = open(fname.c_str(), O_RDWR, 0644);
  if (fd < 0) {
    *result = NULL;
    return -1;
  } else {
    *result = new PosixMmapFile(fname, fd, kPageSize, write_len);
  }
  return 0;
}
}
