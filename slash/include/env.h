#ifndef SLASH_ENV_H_
#define SLASH_ENV_H_

#include <string>
#include <vector>

namespace slash {

class WritableFile;
class SequentialFile;

// File operator

int CreateDir(const std::string& path);


/*
 * Whether the file is exist
 * If exist return true, else return false
 */
int FileExists(const std::string& path);

int GetChildren(const std::string& dir, std::vector<std::string>& result);

int NewWritableFile(const std::string& fname, WritableFile** result);

class WritableFile 
{
public:
  WritableFile();
  virtual ~WritableFile();
  virtual int Append(const char* data, int len) = 0;
  virtual int Close() = 0;
  virtual int Flush() = 0;
  virtual int Sync() = 0;

private:
  WritableFile(const WritableFile&);
  void operator =(const WritableFile&);

};


int NewSequentialFile(const std::string& fname, SequentialFile** result);

// A abstract for the sequential readable file
class SequentialFile
{
public:
  SequentialFile() {};
  virtual ~SequentialFile();
  virtual int Read(size_t n, char *&result, char *scratch) = 0;
  virtual int Skip(uint64_t n) = 0;
  virtual int Close() = 0;
  virtual char *ReadLine(char *buf, int n);
};

}

#endif

