#ifndef SLASH_ENV_H_
#define SLASH_ENV_H_

#include <string>
#include <vector>

namespace slash {

class WritableFile;
class SequentialFile;
class RWFile;


/*
 * Whether the file is exist
 * If exist return true, else return false
 */
int CreateDir(const std::string& path);

int FileExists(const std::string& path);

int RenameFile(const std::string& oldname, const std::string& newname);

int GetChildren(const std::string& dir, std::vector<std::string>& result);

int NewSequentialFile(const std::string& fname, SequentialFile** result);

int NewWritableFile(const std::string& fname, WritableFile** result);

int NewRWFile(const std::string& fname, RWFile** result);

int AppendWritableFile(const std::string& fname, WritableFile** result, uint64_t write_len = 0);


class WritableFile 
{
public:
  WritableFile();
  virtual ~WritableFile();
  virtual int Append(const char* data, int len) = 0;
  virtual int Close() = 0;
  virtual int Flush() = 0;
  virtual int Sync() = 0;
  virtual uint64_t Filesize() = 0;

private:
  WritableFile(const WritableFile&);
  void operator =(const WritableFile&);

};

// A abstract for the sequential readable file
class SequentialFile
{
public:
  SequentialFile() {};
  virtual ~SequentialFile();
  virtual int Read(size_t n, char *&result, char *scratch) = 0;
  virtual int Skip(uint64_t n) = 0;
  virtual int Close() = 0;
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

}

#endif

