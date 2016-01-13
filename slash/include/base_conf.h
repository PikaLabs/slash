#ifndef BASE_CONF_H__
#define BASE_CONF_H__

#include "stdlib.h"
#include "stdio.h"
#include "slash_define.h"

#include <string>
#include <vector>

#define CONF_ITEM_LEN 1024

namespace slash {

class BaseConf
{
public:
  explicit BaseConf(const std::string &path);
  virtual ~BaseConf();

  int LoadConf();

  bool GetConfInt(const std::string &name, int* value) const;
  bool GetConfStr(const std::string &name, std::string *value) const;
  bool GetConfBool(const std::string &name, bool* value) const;

  void DumpConf() const;
  bool WriteBack();

private:
  std::string path_;

  enum ConfType {
    kConf = 0,
    kComment = 1,
  };

  struct ConfItem {
    ConfType type; // 0 means conf, 1 means comment
    std::string name;
    std::string value;
    ConfItem(ConfType t, const std::string &v) :
      type(t),
      name(""),
      value(v)
    { }
    ConfItem(ConfType t, const std::string &n, const std::string &v) :
      type(t),
      name(n),
      value(v)
    {}
  };


  std::vector<ConfItem> item_;
  std::vector<ConfItem>::iterator iter;

  /*
   * No copy && no assign operator
   */
  BaseConf(const BaseConf&);
  void operator=(const BaseConf&);
};

}

#endif
