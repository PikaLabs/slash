#include "slash_mutex.h"

#include <cstdlib>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

namespace slash {

static void PthreadCall(const char* label, int result) {
  if (result != 0) {
    fprintf(stderr, "pthread %s: %s\n", label, strerror(result));
    abort();
  }
}

Mutex::Mutex() { 
  PthreadCall("init mutex", pthread_mutex_init(&mu_, NULL)); 
}

Mutex::~Mutex() { 
  PthreadCall("destroy mutex", pthread_mutex_destroy(&mu_)); 
}

void Mutex::Lock() { 
  PthreadCall("lock", pthread_mutex_lock(&mu_)); 
}

void Mutex::Unlock() { 
  PthreadCall("unlock", pthread_mutex_unlock(&mu_)); 
}

CondVar::CondVar(Mutex* mu)
  : mu_(mu) {
    PthreadCall("init cv", pthread_cond_init(&cv_, NULL));
  }

CondVar::~CondVar() { 
  PthreadCall("destroy cv", pthread_cond_destroy(&cv_)); 
}

void CondVar::Wait() {
  PthreadCall("wait", pthread_cond_wait(&cv_, &mu_->mu_));
}

void CondVar::TimedWait(uint32_t timeout) {
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
  pthread_cond_timedwait(&cv_, &mu_->mu_, &tsp);
}

void CondVar::Signal() {
  PthreadCall("signal", pthread_cond_signal(&cv_));
}

void CondVar::SignalAll() {
  PthreadCall("broadcast", pthread_cond_broadcast(&cv_));
}

void InitOnce(OnceType* once, void (*initializer)()) {
  PthreadCall("once", pthread_once(once, initializer));
}

RefMutex::RefMutex() {
  refs_ = 0;
  PthreadCall("init mutex", pthread_mutex_init(&mu_, nullptr));
}

RefMutex::~RefMutex() {
  PthreadCall("destroy mutex", pthread_mutex_destroy(&mu_));
}

void RefMutex::Ref() {
  refs_++;
}
void RefMutex::Unref() {
  --refs_;
  if (refs_ == 0) {
    delete this;
  }
}

void RefMutex::Lock() {
  PthreadCall("lock", pthread_mutex_lock(&mu_));
}

void RefMutex::Unlock() {
  PthreadCall("unlock", pthread_mutex_unlock(&mu_));
}

RecordMutex::~RecordMutex() {
  mutex_.Lock();
  
  std::unordered_map<std::string, RefMutex *>::const_iterator it = records_.begin();
  for (; it != records_.end(); it++) {
    delete it->second;
  }
  mutex_.Unlock();
}


void RecordMutex::Lock(const std::string &key) {
  mutex_.Lock();
  std::unordered_map<std::string, RefMutex *>::const_iterator it = records_.find(key);

  if (it != records_.end()) {
    //log_info ("tid=(%u) >Lock key=(%s) exist, map_size=%u", pthread_self(), key.c_str(), records_.size());
    RefMutex *ref_mutex = it->second;
    ref_mutex->Ref();
    mutex_.Unlock();

    ref_mutex->Lock();
    //log_info ("tid=(%u) <Lock key=(%s) exist", pthread_self(), key.c_str());
  } else {
    //log_info ("tid=(%u) >Lock key=(%s) new, map_size=%u ++", pthread_self(), key.c_str(), records_.size());
    RefMutex *ref_mutex = new RefMutex();

    records_.insert(std::make_pair(key, ref_mutex));
    ref_mutex->Ref();
    mutex_.Unlock();

    ref_mutex->Lock();
    //log_info ("tid=(%u) <Lock key=(%s) new", pthread_self(), key.c_str());
  }
}

void RecordMutex::Unlock(const std::string &key) {
  mutex_.Lock();
  std::unordered_map<std::string, RefMutex *>::const_iterator it = records_.find(key);
  
  //log_info ("tid=(%u) >Unlock key=(%s) new, map_size=%u --", pthread_self(), key.c_str(), records_.size());
  if (it != records_.end()) {
    RefMutex *ref_mutex = it->second;

    if (ref_mutex->IsLastRef()) {
      records_.erase(it);
    }
    ref_mutex->Unlock();
    ref_mutex->Unref();
  }

  mutex_.Unlock();
  //log_info ("tid=(%u) <Unlock key=(%s) new", pthread_self(), key.c_str());
}

}

