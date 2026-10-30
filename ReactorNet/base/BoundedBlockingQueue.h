// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef REACTORNET_BASE_BOUNDEDBLOCKINGQUEUE_H
#define REACTORNET_BASE_BOUNDEDBLOCKINGQUEUE_H
#include "ReactorNet/base/noncopyable.h"
#include <condition_variable>
#include <deque>
#include <mutex>
#include <stdexcept>
#include <utility>
namespace reactor {
template<typename T>
class BoundedBlockingQueue : noncopyable {
 public:
  explicit BoundedBlockingQueue(int capacity) : m_capacity(capacity) {
    if (capacity <= 0) throw std::invalid_argument("queue capacity must be positive");
  }
  void put(const T& value) { putImpl(value); }
  void put(T&& value) { putImpl(std::move(value)); }
  T take() {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_notEmpty.wait(lock, [this] { return !m_queue.empty(); });
    T value = std::move(m_queue.front());
    m_queue.pop_front();
    m_notFull.notify_one();
    return value;
  }
  bool empty() const { return size() == 0; }
  bool full() const { return size() == m_capacity; }
  size_t size() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_queue.size();
  }
  size_t capacity() const { return m_capacity; }
 private:
  template<typename U> void putImpl(U&& value) {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_notFull.wait(lock, [this] { return m_queue.size() < m_capacity; });
    m_queue.emplace_back(std::forward<U>(value));
    m_notEmpty.notify_one();
  }
  const size_t m_capacity;
  mutable std::mutex m_mutex;
  std::condition_variable m_notEmpty;
  std::condition_variable m_notFull;
  std::deque<T> m_queue;
};
} // namespace reactor
#endif
