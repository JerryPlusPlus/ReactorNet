
#include "ReactorNet/base/ThreadPool.h"

#include "ReactorNet/base/Exception.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <utility>

namespace reactor
{

ThreadPool::ThreadPool(const string& nameArg)
  : m_name(nameArg)
{
}

ThreadPool::~ThreadPool()
{
  stop();
}

void ThreadPool::start(int numThreads)
{
  if (numThreads < 0)
  {
    throw std::invalid_argument("ThreadPool thread count must not be negative");
  }

  std::unique_lock<std::mutex> lock(m_mutex);
  assert(m_threads.empty());
  assert(!m_running);
  m_running = true;
  try
  {
    m_threads.reserve(static_cast<size_t>(numThreads));
    for (int i = 0; i < numThreads; ++i)
    {
      std::unique_ptr<Thread> thread(new Thread(
          [this] { runInThread(); }, m_name + std::to_string(i + 1)));
      thread->start();
      m_threads.emplace_back(std::move(thread));
    }
    lock.unlock();

    if (numThreads == 0 && m_threadInitCallback)
    {
      m_threadInitCallback();
    }
  }
  catch (...)
  {
    if (lock.owns_lock())
    {
      lock.unlock();
    }
    stop();
    std::lock_guard<std::mutex> cleanupLock(m_mutex);
    m_threads.clear();
    throw;
  }
}

void ThreadPool::stop()
{
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_running)
    {
      return;
    }
    m_running = false;
  }
  m_notEmpty.notify_all();
  m_notFull.notify_all();
  for (const auto& thread : m_threads)
  {
    thread->join();
  }
}

size_t ThreadPool::queueSize() const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_queue.size();
}

void ThreadPool::run(Task task)
{
  if (!task)
  {
    return;
  }

  std::unique_lock<std::mutex> lock(m_mutex);
  if (m_threads.empty())
  {
    lock.unlock();
    task();
    return;
  }

  m_notFull.wait(lock, [this] { return !m_running || !isFull(); });
  if (!m_running)
  {
    return;
  }
  m_queue.emplace_back(std::move(task));
  m_notEmpty.notify_one();
}

ThreadPool::Task ThreadPool::take()
{
  std::unique_lock<std::mutex> lock(m_mutex);
  m_notEmpty.wait(lock, [this] { return !m_running || !m_queue.empty(); });
  if (!m_running)
  {
    return Task{};
  }

  Task task = std::move(m_queue.front());
  m_queue.pop_front();
  if (m_maxQueueSize > 0)
  {
    m_notFull.notify_one();
  }
  return task;
}

bool ThreadPool::isFull() const
{
  return m_maxQueueSize > 0 && m_queue.size() >= m_maxQueueSize;
}

void ThreadPool::runInThread()
{
  try
  {
    if (m_threadInitCallback)
    {
      m_threadInitCallback();
    }
    for (;;)
    {
      Task task = take();
      if (!task)
      {
        break;
      }
      task();
    }
  }
  catch (const Exception& ex)
  {
    std::fprintf(stderr, "exception caught in ThreadPool %s\n", m_name.c_str());
    std::fprintf(stderr, "reason: %s\n", ex.what());
    std::fprintf(stderr, "stack trace: %s\n", ex.stackTrace());
    std::abort();
  }
  catch (const std::exception& ex)
  {
    std::fprintf(stderr, "exception caught in ThreadPool %s\n", m_name.c_str());
    std::fprintf(stderr, "reason: %s\n", ex.what());
    std::abort();
  }
  catch (...)
  {
    std::fprintf(stderr, "unknown exception caught in ThreadPool %s\n", m_name.c_str());
    throw; // rethrow
  }
}

}  // namespace reactor

