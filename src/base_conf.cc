#include "base_conf.h"
#include "sys/stat.h"
#include <glog/logging.h>
#include <algorithm>
#include "env.h"
#include "xdebug.h"

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
  char name[CONF_ITEM_LEN], value[CONF_ITEM_LEN];
  int line_len = 0;
  int name_len = 0, value_len = 0;
  int sep_sign = 0;
  ConfType type = kConf;

  while (sequential_file->ReadLine(line, CONF_ITEM_LEN) != NULL) {
    sep_sign = 0;
    name_len = 0;
    value_len = 0;
    type = kComment;
    line_len = strlen(line);
    for (int i = 0; i < line_len; i++) {
      if (line[i] == NUMBER) {
        type = kComment;
        break;
      }
      switch (line[i]) {
      case COLON: 
        type = kConf;
        sep_sign = 1;
      case SPACE:
        continue;
      case '\r':
        continue;
      case '\n':
        continue;
      default:
        if (sep_sign == 0) {
          name[name_len++] = line[i];
        } else {
          value[value_len++] = line[i];
        }
      }
    }

    if (type == kConf) {
      item_.push_back(ConfItem(kConf, std::string(name, name_len), std::string(value, value_len)));
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

bool BaseConf::SetConfInt(const std::string &name, const int value)
{
  for (int i = 0; i < item_.size(); i++) {
    if (item_[i].type == kComment) {
      continue;
    }
    if (name == item_[i].name) {
      item_[i].value = std::to_string(value);
      return true;
    }
  }
  return false;
}

bool BaseConf::SetConfStr(const std::string &name, const std::string &value)
{
  for (int i = 0; i < item_.size(); i++) {
    if (item_[i].type == kComment) {
      continue;
    }
    if (name == item_[i].name) {
      item_[i].value = value;
      return true;
    }
  }
  return false;
}

bool BaseConf::SetConfBool(const std::string &name, const bool value)
{
  for (int i = 0; i < item_.size(); i++) {
    if (item_[i].type == kComment) {
      continue;
    }
    if (name == item_[i].name) {
      if (value == true) {
        item_[i].value = "true";
      } else {
        item_[i].value = "false";
      }
      return true;
    }
  }
  return false;
}

void BaseConf::DumpConf() const
{
  for (int i = 0; i < item_.size(); i++) {
    if (item_[i].type == kConf) {
      printf("%s %s\n", item_[i].name.c_str(), item_[i].value.c_str());
    }
  }
}

bool BaseConf::WriteBack()
{
  WritableFile *write_file;
  int ret = NewWritableFile(path_ + ".tmp", &write_file);
  log_info("ret %d", ret);
  std::string tmp;
  for (int i = 0; i < item_.size(); i++) {
    if (item_[i].type == kComment) {
      tmp = item_[i].value;
    } else {
      tmp = item_[i].name + " : " + item_[i].value + "\n";
    }
    log_info("Write tmp %s", tmp.c_str());
    write_file->Append(tmp.c_str(), tmp.length());
  }
  write_file->Close();
  RenameFile(path_ + ".tmp", path_);

}

}
