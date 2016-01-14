#include "env.h"
#include "xdebug.h"

#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <vector>

namespace slash {

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

WritableFile::WritableFile()
{

}
WritableFile::~WritableFile()
{

}

class PosixWritableFile: public WritableFile {

public:
  PosixWritableFile(const std::string &fname, FILE* &f)
    : fname_(fname),
    f_(f) {
  }

  ~PosixWritableFile() {
    fclose(f_);
  }

  virtual int Append(const char* data, int len) override {
    size_t r = fwrite(data, 1, len, f_);
    fflush(f_);
    if (r != strlen(data)) {
      return -1;
    } else {
      return r;
    }
  }

  virtual int Close() override {
    int ret = fclose(f_);

    if (ret != 0) {
      return ret;
    }
    f_ = NULL;
    return 0;
  }

  virtual int Flush() override {
    int ret = fflush(f_);
    return ret;
  }
  virtual int Sync() override {
    return 0;
  }

private:
  std::string fname_;
  FILE* f_;

  PosixWritableFile(const PosixWritableFile&);
  void operator =(const PosixWritableFile&);
};

int NewWritableFile(const std::string& fname, WritableFile** result) 
{
  FILE *f = fopen(fname.c_str(), "a");
  if (f == NULL) {
    *result = NULL;
    return -1;
  } else {
    *result = new PosixWritableFile(fname, f);
    return 0;
  }
}

SequentialFile::~SequentialFile()
{
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
    int ret = 0;
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

}
