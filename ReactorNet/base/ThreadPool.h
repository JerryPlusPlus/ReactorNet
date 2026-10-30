
#ifndef REACTORNET_BASE_THREADPOOL_H
#define REACTORNET_BASE_THREADPOOL_H

#include "ReactorNet/base/Thread.h"
#include "ReactorNet/base/Types.h"

#include <condition_variable>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

namespace reactor
{

class ThreadPool : noncopyable
{
 public:
  using Task = std::function<void()>;

  explicit ThreadPool(const string& nameArg = "ThreadPool");
  ~ThreadPool();

  // Must be called before start(). A zero queue size means no limit.
  void setMaxQueueSize(int maxSize) { m_maxQueueSize = maxSize; }
  void setThreadInitCallback(const Task& cb) { m_threadInitCallback = cb; }

  // Call start()/stop() outside worker tasks and serialize lifecycle calls.
  void start(int numThreads);
  // Waits for active tasks; queued tasks are not guaranteed to execute.
  void stop();

  const string& name() const { return m_name; }
  size_t queueSize() const;

  // With no worker threads, tasks execute in the calling thread.
  void run(Task task);

 private:
  // The caller must hold m_mutex.
  bool isFull() const;
  void runInThread();
  Task take();

  mutable std::mutex m_mutex;
  std::condition_variable m_notEmpty;
  std::condition_variable m_notFull;
  string m_name;
  Task m_threadInitCallback;
  std::vector<std::unique_ptr<Thread>> m_threads;
  std::deque<Task> m_queue;
  size_t m_maxQueueSize{0};
  bool m_running{false};
};

}  // namespace reactor

#endif  // REACTORNET_BASE_THREADPOOL_H
