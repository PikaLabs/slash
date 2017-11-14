// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.
//
// Author: <Gao Dunqiao> gaodunqiao@360.cn

#include <iostream>
#include <memory>

#include "slash/include/iobuf.h"
#include "slash/include/slash_testharness.h"
#include "slash/tests/iobuf_test.pb.h"

namespace slash {

class IOBufTest {
};

TEST(IOBufTest, AppendTest) {
  IOBuf buf;
  std::string fuck = "fuck";
  const char* hello = "hello1111111111111111111111111111111111111111";
  buf.Append(fuck);
  buf.Append(hello, strlen(hello));
  printf("block count: %lu\n", g_block_pool.BlockCount());
  printf("content: %s\nlength: %lu\n",
         buf.ToString().c_str(), buf.length());
}

TEST(IOBufTest, ZeroCopyTest) {
  ZeroCopyNode::Node node;
  node.set_ip("192.168.1.1");
  node.set_port(1001);
  IOBuf buf;
  IOBufZeroCopyOutputStream output(&buf);
  node.SerializeToZeroCopyStream(&output);

  printf("block count: %lu\n", g_block_pool.BlockCount());
  printf("length: %lu\n", buf.length());

  ZeroCopyNode::Node node1;
  IOBufZeroCopyInputStream input(&buf);
  // node1.ParseFromString(str);
  node1.ParseFromZeroCopyStream(&input);
  printf("ip: %s, port: %d\n", node1.ip().c_str(), node1.port());
}

TEST(IOBufTest, ChangeSize) {
  IOBuf buf;
  std::string test_str("hello 12345678901234567890");
  buf.Append(test_str);
  buf.TrimStart(6);
  ASSERT_EQ(test_str.substr(6), buf.ToString());
  printf("str:%s\n", buf.ToString().c_str());

  IOBuf buf1 = buf.CopyN(10);
  ASSERT_EQ(buf.ToString(), test_str.substr(10));
  ASSERT_EQ(buf1.ToString(), test_str.substr(0, 10));
}

}  // namespace slash
