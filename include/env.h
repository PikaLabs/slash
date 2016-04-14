#ifndef SLASH_ENV_H_
#define SLASH_ENV_H_

#include <string>
#include <vector>
#include <unistd.h>

#include "slash_status.h"

namespace slash {

class WritableFile;
class SequentialFile;
class RWFile;

/*
 * Set size of initial mmap size
 */
void SetMmapBoundSize(size_t size);

static const size_t kPageSize = getpagesize();


/*
 * File Operations
 */
int IsDir(const std::string& path);
int DeleteDir(const std::string& path);
bool DeleteDirIfExist(const std::string& path);
int CreateDir(const std::string& path);
int CreatePath(const std::string& path, mode_t mode = 0755);
uint64_t Du(const std::string& path);

/*
 * Rsync
 *
 */
const std::string kRsyncConfFile = "slash_rsync.conf";
const std::string kRsyncLogFile = "slash_rsync.log";
const std::string kRsyncPidFile = "slash_rsync.pid";
const std::string kRsyncLockFile = "slash_rsync.lock";
const std::string kRsyncSubDir = "rsync";
struct RsyncRemote {
  std::string host;
  int port;
  std::string module;
  int kbps; //speed limit
  RsyncRemote(const std::string& _host, const int _port,
      const std::string& _module, const int _kbps)
  : host(_host), port(_port), module(_module), kbps(_kbps) {}
};
int StartRsync(const std::string& rsync_path, const std::string& module, const int port);
int StopRsync(const std::string& path);
int RsyncSendFile(const std::string& local_file_path, const std::string& remote_file_path,
    const RsyncRemote& remote);

/*
 * Whether the file is exist
 * If exist return true, else return false
 */
int FileExists(const std::string& path);

Status DeleteFile(const std::string& fname);

int RenameFile(const std::string& oldname, const std::string& newname);

int GetChildren(const std::string& dir, std::vector<std::string>& result);
bool GetDescendant(const std::string& dir, std::vector<std::string>& result);


uint64_t NowMicros();
void SleepForMicroseconds(int micros);

Status NewSequentialFile(const std::string& fname, SequentialFile** result);

Status NewWritableFile(const std::string& fname, WritableFile** result);

Status NewRWFile(const std::string& fname, RWFile** result);

Status AppendSequentialFile(const std::string& fname, SequentialFile** result);

Status AppendWritableFile(const std::string& fname, WritableFile** result, uint64_t write_len = 0);


// A file abstraction for sequential writing.  The implementation
// must provide buffering since callers may append small fragments
// at a time to the file.
class WritableFile {
 public:
  WritableFile() { }
  virtual ~WritableFile();

  virtual Status Append(const Slice& data) = 0;
  virtual Status Close() = 0;
  virtual Status Flush() = 0;
  virtual Status Sync() = 0;
  virtual uint64_t Filesize() = 0;

 private:
  // No copying allowed
  WritableFile(const WritableFile&);
  void operator=(const WritableFile&);
};

// A abstract for the sequential readable file
class SequentialFile {
 public:
  SequentialFile() {};
  virtual ~SequentialFile();
  //virtual Status Read(size_t n, char *&result, char *scratch) = 0;
  virtual Status Read(size_t n, Slice* result, char* scratch) = 0;
  virtual Status Skip(uint64_t n) = 0;
  //virtual Status Close() = 0;
  virtual char *ReadLine(char *buf, int n) = 0;
};

class RWFile {
public:
  RWFile() { }
  virtual ~RWFile();
  virtual char* GetData() = 0;

private:
  // No copying allowed
  RWFile(const RWFile&);
  void operator=(const RWFile&);
};


}   // namespace slash
#endif  // SLASH_ENV_H_
