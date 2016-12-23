// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#ifndef INCLUDE_BASE_CONF_H_
#define INCLUDE_BASE_CONF_H_

#include <string>
#include <vector>

#include "stdlib.h"
#include "stdio.h"

#include "slash_define.h"

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
  bool GetConfStrVec(const std::string &name, std::vector<std::string> *value) const;

  bool SetConfInt(const std::string &name, const int value);
  bool SetConfStr(const std::string &name, const std::string &value);
  bool SetConfBool(const std::string &name, const bool value);

  void DumpConf() const;
  bool WriteBack();
  void WriteSampleConf() const;

private:

  struct Rep;
  Rep* rep_;

  /*
   * No copy && no assign operator
   */
  BaseConf(const BaseConf&);
  void operator=(const BaseConf&);
};

}  // namespace slash

#endif  // INCLUDE_BASE_CONF_H_
