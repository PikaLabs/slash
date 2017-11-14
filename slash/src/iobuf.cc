// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.
//
// Author: <Gao Dunqiao> gaodunqiao@360.cn

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/uio.h>

#include "slash/include/likely.h"
#include "slash/include/iobuf.h"

namespace slash {

IOBufBlockPool g_block_pool;

struct IOBuf::Block {
  static const int kDefaultBlockSize = 8192;

  explicit Block(size_t block_size = kDefaultBlockSize)
    : buffer_(nullptr),
      capacity_(0),
      size_(0),
      ref_count_(1) {
    if (block_size != 0) {
      size_t min_alloc_size = (block_size + 7) & ~7;  // align 8 byte

      buffer_ = static_cast<char*>(malloc(min_alloc_size));
      capacity_ = min_alloc_size;
    }
  }

  void inc_ref() { ref_count_.fetch_add(1, std::memory_order_acq_rel); }

  void dec_ref() {
    int new_ref = ref_count_.fetch_sub(1, std::memory_order_acq_rel);
    assert(new_ref >= 1);
    if (new_ref > 1) {
      return;
    }
    if (capacity_ > 0) {
      free(buffer_);
    }
    delete this;
  }

  bool is_full() { return size_ >= capacity_; }
  bool empty() { return size_ == 0; }
  const char* data() { return buffer_; }
  char* writable_pos() { return buffer_ + size_; }
  void append(size_t n) { size_ += n; }

  size_t remain_buf() {
    assert(capacity_ == 0 || capacity_ >= size_);
    return capacity_ > 0 ? capacity_ - size_ : 0;
  }

  char* buffer_;
  size_t capacity_;
  size_t size_;
  std::atomic<int> ref_count_;
};

IOBuf::IOBuf() : view_head_(0, 0, nullptr), block_view_count_(0) {}

IOBuf::~IOBuf() {
  BlockView* bview = view_head_.next;
  for (size_t i = 0; i < block_view_count_; i++) {
    BlockView* next = bview->next;
    RemoveView(bview);
    bview = next;
  }
}

void IOBuf::Append(const char* data, size_t length) {
  size_t sz = 0;
  while (sz < length) {
    Block* b = g_block_pool.AllocNewBlock();

    size_t min_sz = std::min(length, b->remain_buf());
    memcpy(b->writable_pos(), data + sz, min_sz);

    PushBackView(b->size_, min_sz, b);

    b->append(min_sz);
    sz += min_sz;
  }
}

ssize_t IOBuf::AppendFromFd(int fd) {
  Block* b;
  b = g_block_pool.AllocNewBlock();
  ssize_t rn = read(fd, b->writable_pos(), b->remain_buf());
  if (rn > 0) {
    PushBackView(b->size_, rn, b);
    b->append(rn);
  } else {
    g_block_pool.ReleaseBlock(b);
  }
  return rn;
}

ssize_t IOBuf::WriteIntoFd(int fd) {
  BlockView* v = view_head_.next;
  struct iovec vec[block_view_count_];
  for (size_t i = 0; i < block_view_count_; i++) {
    vec[i].iov_base = v->block->buffer_ + v->offset;
    vec[i].iov_len = v->length;
    v = v->next;
  }
  // ssize_t rn = write(fd, v->block->buffer_ + v->offset, v->length);
  ssize_t rn = writev(fd, vec, block_view_count_);
  if (rn > 0) {
    TrimStart(rn);
  }
  return rn;
}

void IOBuf::TrimStart(size_t n) {
  size_t rn = n;
  BlockView* v = view_head_.next;
  while (rn > 0) {
    if (rn >= v->length) {
      rn -= v->length;
      RemoveView(v);
    } else {
      v->offset += rn;
      v->length -= rn;
      // rn = 0;
      return;
    }
    v = v->next;
  }
}

IOBuf IOBuf::CopyN(size_t n) {
  IOBuf result;
  size_t rn = n;
  BlockView* v = view_head_.next;
  while (rn > 0) {
    size_t min_sz = std::min(rn, v->length);
    result.PushBackView(v->offset, min_sz, v->block);
    rn -= min_sz;
    v = v->next;
  }
  return result;
}

void IOBuf::CutInto(void* buf, size_t n) {
  size_t rn = n;
  BlockView* v = view_head_.next;
  while (rn > 0) {
    size_t min_sz = std::min(rn, v->length);
    memcpy(buf, v->block->buffer_ + v->offset, min_sz);
    if (min_sz == v->length) {
      rn -= min_sz;
      RemoveView(v);
    } else {
      v->offset += rn;
      v->length -= rn;
      // rn = 0;
      return;
    }
    v = v->next;
  }
}

size_t IOBuf::length() {
  size_t total_length = 0;
  BlockView* view = view_head_.next;
  for (size_t i = 0; i < block_view_count_; i++) {
    total_length += view->length;
    view = view->next;
  }
  return total_length;
}

std::string IOBuf::ToString() {
  std::string result;
  BlockView* view = view_head_.next;
  for (size_t i = 0; i < block_view_count_; i++) {
    result.append(view->block->buffer_ + view->offset, view->length);
    view = view->next;
  }
  return result;
}

void IOBuf::PushBackView(size_t offset, size_t length, Block* b) {
  b->inc_ref();
  BlockView* v = new BlockView(offset, length, b);
  BlockView* rear = view_head_.prev;
  v->next = rear->next;
  v->prev = rear;
  rear->next->prev = v;
  rear->next = v;
  block_view_count_++;
}

void IOBuf::RemoveView(BlockView* v) {
  block_view_count_--;
  v->block->dec_ref();
  g_block_pool.ReleaseBlock(v->block);
  v->prev->next = v->next;
  v->next->prev = v->prev;
  delete v;
}

IOBufBlockPool::~IOBufBlockPool() {
  for (auto b : block_pool_) {
    b->dec_ref();
  }
}

IOBuf::Block* IOBufBlockPool::AllocNewBlock() {
  for (auto b : block_pool_) {
    if (!b->is_full()) {
      return b;
    }
  }

  IOBuf::Block* b = new IOBuf::Block;
  block_pool_.insert(b);

  return b;
}

void IOBufBlockPool::ReleaseBlock(IOBuf::Block* b) {
  if (block_pool_.size() >= MAX_BLOCK_COUNT) {
    b->dec_ref();
    block_pool_.erase(b);
    return;
  }
  if (b->ref_count_.load(std::memory_order_acquire) == 1) {
    b->size_ = 0;
  }
}

void IOBufBlockPool::Debug() {
  for (auto b : block_pool_) {
    printf("Block: %p, ref: %d\n", b, b->ref_count_.load());
  }
  printf("======\n");
}

IOBufZeroCopyOutputStream::IOBufZeroCopyOutputStream(IOBuf* buf)
    : buf_(buf) {
}

bool IOBufZeroCopyOutputStream::Next(void** data, int* size) {
  IOBuf::Block* b;
  if (buf_->block_view_count_ != 0 &&
      !buf_->view_head_.prev->block->is_full()) {
    b = buf_->view_head_.prev->block;
  } else {
    b = g_block_pool.AllocNewBlock();
    if (UNLIKELY(!b)) {
      return false;
    }
  }
  *size = b->remain_buf();
  *data = b->writable_pos();

  buf_->PushBackView(b->size_, *size, b);
  b->append(*size);
  return true;
}

void IOBufZeroCopyOutputStream::BackUp(int count) {
  size_t rc = count;
  IOBuf::BlockView* cur_view  = buf_->view_head_.prev;
  while (buf_->block_view_count_ > 0 &&
         rc > cur_view->length) {
    rc -= cur_view->length;
    IOBuf::BlockView* prev = cur_view->prev;
    buf_->RemoveView(cur_view);
    cur_view = prev;
  }
  cur_view->length -= rc;
}

int64_t IOBufZeroCopyOutputStream::ByteCount() const {
  return buf_->length();
}

IOBufZeroCopyInputStream::IOBufZeroCopyInputStream(IOBuf* buf)
    : buf_(buf),
      cur_view_(buf_->view_head_.next),
      pos_in_view_(0) {
}

bool IOBufZeroCopyInputStream::Next(const void** data, int* size) {
  // Switch to next view
  if (pos_in_view_ == cur_view_->length) {
    if (cur_view_->next == &buf_->view_head_) {
      return false;  // There is no more block view
    }
    cur_view_ = cur_view_->next;
    pos_in_view_ = 0;
  }

  *data = cur_view_->block->data() + cur_view_->offset + pos_in_view_;
  *size = cur_view_->length - pos_in_view_;
  pos_in_view_ += *size;
  return true;
}

void IOBufZeroCopyInputStream::BackUp(int count) {
  size_t rc = count;
  while (rc > pos_in_view_) {
    cur_view_ = cur_view_->prev;
    rc -= pos_in_view_;
  }
  pos_in_view_ -= rc;
}

bool IOBufZeroCopyInputStream::Skip(int count) {
  size_t rc = count;
  while (pos_in_view_ + rc > cur_view_->length) {
    if (cur_view_->next == &buf_->view_head_) {
      return false;  // There is no more block view
    }
    rc -= cur_view_->length - pos_in_view_;
    cur_view_ = cur_view_->next;
    pos_in_view_ = 0;
  }
  pos_in_view_ += rc;
  return true;
}

google::protobuf::int64 IOBufZeroCopyInputStream::ByteCount() const {
  return buf_->length();
}

}  // namespace slash
