// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef REACTORNET_BASE_BLOCKINGQUEUE_H
#define REACTORNET_BASE_BLOCKINGQUEUE_H
#include "ReactorNet/base/noncopyable.h"
#include <condition_variable>
#include <deque>
#include <mutex>
#include <utility>
namespace reactor {
template<typename T>
class BlockingQueue : noncopyable {
 public:
  void put(const T& value) { putImpl(value); }
  void put(T&& value) { putImpl(std::move(value)); }
  T take() {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_notEmpty.wait(lock, [this] { return !m_queue.empty(); });
    T value = std::move(m_queue.front());
    m_queue.pop_front();
    return value;
  }
  size_t size() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_queue.size();
  }
 private:
  template<typename U> void putImpl(U&& value) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_queue.emplace_back(std::forward<U>(value));
    m_notEmpty.notify_one();
  }
  mutable std::mutex m_mutex;
  std::condition_variable m_notEmpty;
  std::deque<T> m_queue;
};
} // namespace reactor
#endif
