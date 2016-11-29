// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.
#include "include/cond_lock.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "include/xdebug.h"

namespace slash {

static void PthreadCall(const char* label, int result) {
  if (result != 0) {
    fprintf(stderr, "Pthread CondLock error function %s: %s\n", label, strerror(result));
    abort();
  }
}

CondLock::CondLock() {
  PthreadCall("init condlock", pthread_mutex_init(&mutex_, NULL));
}

CondLock::~CondLock() {
  PthreadCall("destroy condlock", pthread_mutex_unlock(&mutex_));
}

void CondLock::Lock() {
  PthreadCall("mutex lock", pthread_mutex_lock(&mutex_));
}

void CondLock::Unlock() {
  PthreadCall("mutex unlock", pthread_mutex_unlock(&mutex_));
}

void CondLock::Wait() {
  PthreadCall("condlock wait", pthread_cond_wait(&cond_, &mutex_));
}

void CondLock::TimedWait(uint32_t timeout) {
  /*
   * pthread_cond_timedwait api use absolute API
   * so we need gettimeofday + timeout
   */
  struct timeval now;
  gettimeofday(&now, NULL);
  struct timespec tsp;

  tsp.tv_sec = now.tv_sec;
  tsp.tv_sec += timeout / 1000;
  tsp.tv_nsec = now.tv_usec * 1000;
  tsp.tv_nsec += static_cast<long>(timeout % 1000) * 1000000;
  pthread_cond_timedwait(&cond_, &mutex_, &tsp);
}

void CondLock::Signal() {
  PthreadCall("condlock signal", pthread_cond_signal(&cond_));
}

void CondLock::Broadcast() {
  PthreadCall("condlock broadcast", pthread_cond_broadcast(&cond_));
}

}  // namespace slash
