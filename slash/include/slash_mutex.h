#ifndef SLASH_MUTEXLOCK_H_
#define SLASH_MUTEXLOCK_H_

#include <pthread.h>
#include <string>
#include <unordered_map>

namespace slash {

class CondVar;

class Mutex {
public:
  Mutex();
  ~Mutex();

  void Lock();
  void Unlock();
  void AssertHeld() { }

private:
  friend class CondVar;
  pthread_mutex_t mu_;

  // No copying
  Mutex(const Mutex&);
  void operator=(const Mutex&);
};

class CondVar {
public:
  explicit CondVar(Mutex* mu);
  ~CondVar();
  void Wait();
  /*
   * timeout is millisecond
   * so if you want to wait for 1 s, you should call
   * TimeWait(1000);
   * return false if timeout
   */
  bool TimedWait(uint32_t timeout);
  void Signal();
  void SignalAll();
  
private:
  pthread_cond_t cv_;
  Mutex* mu_;
};

class MutexLock {
public:
  explicit MutexLock(Mutex *mu)
    : mu_(mu)  {
      this->mu_->Lock();
    }
  ~MutexLock() { this->mu_->Unlock(); }

private:
  Mutex *const mu_;
  // No copying allowed
  MutexLock(const MutexLock&);
  void operator=(const MutexLock&);
};

typedef pthread_once_t OnceType;
extern void InitOnce(OnceType* once, void (*initializer)());

class RWLock {
  public:
    RWLock(pthread_rwlock_t *mu, bool is_rwlock) :
		mu_(mu) {
      if (is_rwlock) {
        pthread_rwlock_wrlock(this->mu_);
      } else {
        pthread_rwlock_rdlock(this->mu_);
      }
    }
    ~RWLock() { pthread_rwlock_unlock(this->mu_); }

  private:
    pthread_rwlock_t *const mu_;
    // No copying allowed
    RWLock(const RWLock&);
    void operator=(const RWLock&);
};

class RefMutex {
 public:
  RefMutex();
  ~RefMutex();

  // Lock and Unlock will increase and decrease refs_,
  // should check refs before Unlock
  void Lock();
  void Unlock();

  void Ref();
  void Unref();
  bool IsLastRef() {
    return refs_ == 1;
  }

 private:
  pthread_mutex_t mu_;
  int refs_;

  // No copying
  RefMutex(const RefMutex&);
  void operator=(const RefMutex&);
};

class RecordMutex {
public:
  RecordMutex() {};
  ~RecordMutex();

  void Lock(const std::string &key);
  void Unlock(const std::string &key);

private:

  Mutex mutex_;

  std::unordered_map<std::string, RefMutex *> records_;

  // No copying
  RecordMutex(const RecordMutex&);
  void operator=(const RecordMutex&);
};

class RecordLock {
 public:
  RecordLock(RecordMutex *mu, const std::string &key)
      : mu_(mu), key_(key) {
        mu_->Lock(key_);
      }
  ~RecordLock() { mu_->Unlock(key_); }

 private:
  RecordMutex *const mu_;
  std::string key_;

  // No copying allowed
  RecordLock(const RecordLock&);
  void operator=(const RecordLock&);
};

}

#endif
