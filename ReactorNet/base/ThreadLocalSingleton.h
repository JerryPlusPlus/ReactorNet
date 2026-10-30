// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef REACTORNET_BASE_THREADLOCALSINGLETON_H
#define REACTORNET_BASE_THREADLOCALSINGLETON_H
#include "ReactorNet/base/noncopyable.h"
#include <memory>
namespace reactor {
template<typename T> class ThreadLocalSingleton : noncopyable {
 public:
  ThreadLocalSingleton() = delete;
  ~ThreadLocalSingleton() = delete;
  static T& instance() {
    auto& ptr = storage();
    if (!ptr) ptr.reset(new T());
    return *ptr;
  }
  static T* pointer() { return storage().get(); }
 private:
  static std::unique_ptr<T>& storage() {
    static thread_local std::unique_ptr<T> ptr;
    return ptr;
  }
};
} // namespace reactor
#endif
