#include "ReactorNet/base/ThreadPool.h"
#include "ReactorNet/base/CurrentThread.h"

#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <dlfcn.h>
#include <future>
#include <pthread.h>
#include <set>
#include <stdexcept>
#include <system_error>
#include <thread>

namespace
{
std::atomic<int> failCreationAt(0);

void require(bool condition, const char* message)
{
  if (!condition)
  {
    std::fprintf(stderr, "FAIL: %s\n", message);
    std::abort();
  }
}

struct Event
{
  std::promise<void> promise;
  std::shared_future<void> future;
  Event() : future(promise.get_future().share()) {}
  void signal() { promise.set_value(); }
  void wait() const
  {
    require(future.wait_for(std::chrono::seconds(5)) == std::future_status::ready,
            "event completes before timeout");
    future.get();
  }
  bool blocked() const
  {
    return future.wait_for(std::chrono::milliseconds(50)) == std::future_status::timeout;
  }
};
}

extern "C" int pthread_create(pthread_t* thread, const pthread_attr_t* attr,
                             void* (*entry)(void*), void* argument) noexcept
{
  if (failCreationAt.load() > 0 && failCreationAt.fetch_sub(1) == 1)
    return EAGAIN;

  typedef int (*CreateThread)(pthread_t*, const pthread_attr_t*,
                              void* (*)(void*), void*);
  static CreateThread createThread = reinterpret_cast<CreateThread>(
      dlsym(RTLD_NEXT, "pthread_create"));
  require(createThread != nullptr, "resolve real pthread_create");
  return createThread(thread, attr, entry, argument);
}

int main()
{
  {
    reactor::ThreadPool pool;
    const std::thread::id caller = std::this_thread::get_id();
    int tasks = 0;
    int initialized = 0;
    pool.run([&] {
      require(std::this_thread::get_id() == caller, "no-worker task runs in caller");
      ++tasks;
    });
    pool.setThreadInitCallback([&] {
      require(std::this_thread::get_id() == caller, "zero-worker initialization runs in caller");
      ++initialized;
    });
    pool.start(0);
    pool.run([&] { ++tasks; });
    pool.run(reactor::ThreadPool::Task{});
    pool.stop();
    pool.stop();
    require(tasks == 2 && initialized == 1 && pool.queueSize() == 0,
            "zero-worker mode preserves synchronous execution");
  }

  {
    reactor::ThreadPool pool("workers");
    std::mutex namesMutex;
    std::set<std::string> names;
    Event initialized;
    pool.setThreadInitCallback([&] {
      std::lock_guard<std::mutex> lock(namesMutex);
      names.insert(reactor::CurrentThread::name());
      if (names.size() == 4)
        initialized.signal();
    });
    pool.start(4);
    initialized.wait();
    require(names == std::set<std::string>({"workers1", "workers2", "workers3", "workers4"}),
            "worker names and per-thread initialization are retained");

    const int taskCount = 1200;
    std::vector<std::atomic<int>> seen(taskCount);
    for (auto& count : seen)
      count = 0;
    std::atomic<int> completed(0);
    Event allDone;
    pool.run(reactor::ThreadPool::Task{});
    std::vector<std::thread> producers;
    for (int producer = 0; producer < 4; ++producer)
    {
      producers.emplace_back([&, producer] {
        for (int i = producer; i < taskCount; i += 4)
        {
          pool.run([&, i] {
            ++seen[i];
            if (++completed == taskCount)
              allDone.signal();
          });
        }
      });
    }
    for (auto& producer : producers)
      producer.join();
    allDone.wait();
    pool.stop();
    for (const auto& count : seen)
      require(count == 1, "concurrent producers execute each task exactly once");
    require(pool.queueSize() == 0, "completed work leaves an empty queue");
  }

  {
    reactor::ThreadPool pool;
    pool.setMaxQueueSize(1);
    pool.start(1);
    Event active, release, producerEntered, submitted, allDone;
    std::atomic<int> completed(0);
    const auto complete = [&] {
      if (++completed == 3)
        allDone.signal();
    };
    pool.run([&] { active.signal(); release.wait(); complete(); });
    active.wait();
    pool.run(complete);
    require(pool.queueSize() == 1, "bounded queue fills to its configured limit");
    std::thread producer([&] {
      producerEntered.signal();
      pool.run(complete);
      submitted.signal();
    });
    producerEntered.wait();
    require(submitted.blocked(), "producer waits while the queue is full");
    release.signal();
    submitted.wait();
    producer.join();
    allDone.wait();
    pool.stop();
  }

  {
    reactor::ThreadPool pool;
    pool.setMaxQueueSize(1);
    pool.start(1);
    Event active, release, producerEntered, submitted, stopped;
    std::atomic<int> queuedTasks(0);
    pool.run([&] { active.signal(); release.wait(); });
    active.wait();
    pool.run([&] { ++queuedTasks; });
    std::thread producer([&] {
      producerEntered.signal();
      pool.run([&] { ++queuedTasks; });
      submitted.signal();
    });
    producerEntered.wait();
    require(submitted.blocked(), "producer is blocked before stop");
    std::thread stopper([&] { pool.stop(); stopped.signal(); });
    submitted.wait();
    require(stopped.blocked(), "stop waits for the task already in progress");
    release.signal();
    producer.join();
    stopped.wait();
    stopper.join();
    pool.run([&] { ++queuedTasks; });
    pool.stop();
    require(queuedTasks == 0, "stop wakes producers and prevents queued work from starting");
  }

  {
    std::unique_ptr<reactor::ThreadPool> pool(new reactor::ThreadPool);
    pool->start(1);
    Event active, release, destroyed;
    pool->run([&] { active.signal(); release.wait(); });
    active.wait();
    std::thread destroyer([&] { pool.reset(); destroyed.signal(); });
    require(destroyed.blocked(), "destructor joins active workers");
    release.signal();
    destroyed.wait();
    destroyer.join();
  }

  {
    reactor::ThreadPool pool;
    std::atomic<int> initialized(0);
    pool.setThreadInitCallback([&] { ++initialized; });
    failCreationAt = 3;
    bool failed = false;
    try { pool.start(4); }
    catch (const std::system_error& error)
    {
      failed = error.code() == std::errc::resource_unavailable_try_again;
    }
    require(failed && initialized == 2, "partial startup failure joins created workers");
    Event done;
    pool.start(2);
    pool.run([&] { done.signal(); });
    done.wait();
    pool.stop();
    require(initialized == 4, "pool can start again after a failed attempt");
  }

  {
    reactor::ThreadPool pool;
    bool fail = true;
    pool.setThreadInitCallback([&] {
      if (fail)
        throw std::runtime_error("initialization failed");
    });
    bool failed = false;
    try { pool.start(0); }
    catch (const std::runtime_error&) { failed = true; }
    require(failed, "zero-worker initialization errors propagate");
    fail = false;
    pool.start(0);
    pool.stop();
  }

  {
    reactor::ThreadPool pool;
    bool failed = false;
    try { pool.start(-1); }
    catch (const std::invalid_argument&) { failed = true; }
    require(failed, "negative worker counts are rejected before changing pool state");
    pool.start(1);
    pool.stop();
  }

  std::puts("PASS: synchronous mode, initialization/names, 1200 concurrent tasks, bounded queue, stop wakeup, destructor, startup failure/retry");
}
