#include "base_conf.h"
#include "sys/stat.h"
#include <glog/logging.h>
#include <algorithm>
#include "env.h"

namespace slash {

BaseConf::BaseConf(const std::string &path) :
  path_(path)
{
}

BaseConf::~BaseConf()
{
}

int BaseConf::LoadConf()
{
  if (!FileExists(path_)) {
    return -1;
  }
  SequentialFile *sequential_file;
  NewSequentialFile(path_, &sequential_file);

  // read conf items
  
  char line[CONF_ITEM_LEN];
  int line_len = 0;
  std::string name, value;
  char *ptr;
  int sep_sign = 0;

  while (sequential_file->ReadLine(line, CONF_ITEM_LEN) != NULL) {
    sep_sign = 0;
    line_len = strlen(line);
    for (int i = 0; i < line_len; i++) {
      switch (line[i]) {
      case NUMBER:
        break;
      case COLON: 
        sep_sign = 1;
      case SPACE:
        continue;
      case '\r':
        continue;
      case '\n':
        continue;
      default:
        if (sep_sign == 0) {
          name += line[i];
        } else {
          value += line[i];
        }
      }
    }

    if (sep_sign == 1) {
      item_.push_back(ConfItem(kConf, name, value));
    } else {
      item_.push_back(ConfItem(kComment, std::string(line, line_len)));
    }
  }

  sequential_file->Close();
  return 0;
}


bool BaseConf::GetConfInt(const std::string &name, int* value) const
{
  for (int i = 0; i < item_.size(); i++) {
    if (item_[i].type == 1) {
      continue;
    }
    if (name == item_[i].name) {
      (*value) = atoi(item_[i].value.c_str());
      return true;
    }
  }
  return false;
}

bool BaseConf::GetConfStr(const std::string &name, std::string *val) const
{
  for (int i = 0; i < item_.size(); i++) {
    if (item_[i].type == 1) {
      continue;
    }
    if (name == item_[i].name) {
      (*val) = item_[i].value;
      return true;
    }
  }
  return false;
}

bool BaseConf::GetConfBool(const std::string &name, bool* value) const
{
  for (int i = 0; i < item_.size(); i++) {
    if (item_[i].type == 1) {
      continue;
    }
    if (name == item_[i].name) {
      if (item_[i].value == "true" || item_[i].value == "1" || item_[i].value == "yes") {
        (*value) = true;
      } else if (item_[i].value == "false" || item_[i].value == "0" || item_[i].value == "no") {
        (*value) = false;
      }
      return true;
    }
  }
  return false;
}

void BaseConf::DumpConf() const
{
  for (int i = 0; i < item_.size(); i++) {
    printf("%2d %s %s\n", i + 1, item_[i].name.c_str(), item_[i].value.c_str());
  }
}

bool BaseConf::WriteBack()
{
  WritableFile *write_file;
  NewWritableFile(path_ + ".tmp", &write_file);
  std::string tmp;
  for (int i = 0; i < item_.size(); i++) {
    if (item_[i].type == kComment) {
      tmp = item_[i].value + "\n";
    } else {
      tmp = item_[i].name + " : " + item_[i].value + "\n";
    }
    write_file->Append(tmp.c_str(), tmp.length());
  }
  write_file->Close();

}

}
