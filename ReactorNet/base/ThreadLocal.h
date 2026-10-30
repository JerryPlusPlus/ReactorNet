// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef REACTORNET_BASE_THREADLOCAL_H
#define REACTORNET_BASE_THREADLOCAL_H
#include "ReactorNet/base/noncopyable.h"
#include <atomic>
#include <cstdint>
#include <memory>
#include <unordered_map>
namespace reactor {
// Each instance has a stable key; per-thread values are destroyed at thread exit.
// The instance must outlive calls to value(), as with any shared object.
template<typename T> class ThreadLocal : noncopyable {
 public:
  ThreadLocal() : m_key(nextKey().fetch_add(1, std::memory_order_relaxed)) {}
  ~ThreadLocal() = default;
  T& value() {
    auto& ptr = values()[m_key];
    if (!ptr) ptr.reset(new T());
    return *ptr;
  }
 private:
  using Values = std::unordered_map<uint64_t, std::unique_ptr<T>>;
  static Values& values() { static thread_local Values local; return local; }
  static std::atomic<uint64_t>& nextKey() { static std::atomic<uint64_t> key{0}; return key; }
  const uint64_t m_key;
};
} // namespace reactor
#endif
