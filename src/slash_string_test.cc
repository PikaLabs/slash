// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "testharness.h"

namespace slash {

class StringTest {
 public:
};

TEST(StringTest, test) {
  ASSERT_EQ(1, 1);
}

}  // namespace slash

int main() {
  return slash::test::RunAllTests();
}
