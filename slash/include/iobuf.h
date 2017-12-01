// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.
// 
// Author: <Gao Dunqiao> gaodunqiao@360.cn

#ifndef SLASH_IOBUF_H_
#define SLASH_IOBUF_H_

#include <memory>
#include <atomic>
#include <string>
#include <set>

#include <google/protobuf/io/zero_copy_stream.h>
#include "slash/include/slash_slice.h"

namespace slash {

class BlockView;

class IOBuf {
 public:
  IOBuf();
  IOBuf(const IOBuf& buf);
  ~IOBuf();

  struct Block;  // Basic unit of IOBuf

  void Append(const void* data, size_t length);
  void Append(const char* data) {
    return Append(data, strlen(data));
  }
  void Append(const std::string& data) {
    return Append(data.data(), data.length());
  }
  void Append(const Slice& data) {
    return Append(data.data(), data.size());
  }

  ssize_t AppendFromFd(int fd);
  ssize_t WriteIntoFd(int fd);

  void TrimStart(size_t n);

  void CopyN(IOBuf* buf, size_t n);
  void CopynFrom(IOBuf* buf, size_t start, size_t n);
  void CutInto(void* buf, size_t n);

  char ByteAt(size_t index) const;

  size_t length() const;
  std::string ToString() const;

 private:
  friend class IOBufZeroCopyOutputStream;
  friend class IOBufZeroCopyInputStream;

  void PushBackView(size_t offset, size_t length, Block* b);
  void RemoveView(BlockView* view);

  BlockView* view_head_;
  size_t block_view_count_;
};

class IOBufBlockPool {
 public:
  static const size_t MAX_BLOCK_COUNT = 8192;  // 8192 * 8KB = 64MB

  IOBufBlockPool() {}
  ~IOBufBlockPool();

  IOBuf::Block* AllocNewBlock();
  void ReleaseBlock(IOBuf::Block* b);

  size_t BlockCount() { return block_pool_.size(); }

  void Debug();

 private:
  std::set<IOBuf::Block*> block_pool_;
};

extern IOBufBlockPool g_block_pool;

class IOBufZeroCopyOutputStream
    : public google::protobuf::io::ZeroCopyOutputStream {
 public:
  explicit IOBufZeroCopyOutputStream(IOBuf* buf);
  ~IOBufZeroCopyOutputStream() {}

  bool Next(void** data, int* size) override;
  void BackUp(int count) override;
  google::protobuf::int64 ByteCount() const override;

 private:
  IOBuf* buf_;
};

class IOBufZeroCopyInputStream
    : public google::protobuf::io::ZeroCopyInputStream {
 public:
  explicit IOBufZeroCopyInputStream(IOBuf* buf);
  ~IOBufZeroCopyInputStream() {}

  bool Next(const void** data, int* size) override;
  void BackUp(int count) override;
  bool Skip(int count) override;
  google::protobuf::int64 ByteCount() const override;

 private:
  IOBuf* buf_;
  BlockView* cur_view_;
  size_t pos_in_view_;
};

struct BlockView {  // For IOBuf user
  BlockView(size_t o, size_t l, IOBuf::Block* b)
    : offset(o),
      length(l),
      block(b),
      next(this),
      prev(this) {
  }
  size_t offset;
  size_t length;
  IOBuf::Block* block;

  BlockView* next;
  BlockView* prev;
};

}  // namespace slash

#endif  // SLASH_IOBUF_H_
