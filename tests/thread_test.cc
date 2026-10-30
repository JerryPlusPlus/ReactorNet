#include "ReactorNet/base/Thread.h"
#include "ReactorNet/base/CurrentThread.h"

#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <dlfcn.h>
#include <future>
#include <memory>
#include <pthread.h>
#include <set>
#include <stdexcept>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <system_error>
#include <unistd.h>
#include <vector>

namespace
{
std::atomic<bool> failNextThread(false);
std::atomic<bool> finishBeforeCreateReturns(false);
std::atomic<bool> earlyTaskFinished(false);

void require(bool condition, const char* message)
{
  if (!condition)
  {
    std::fprintf(stderr, "FAIL: %s\n", message);
    std::abort();
  }
}
}

// Inject a real std::thread construction failure without exhausting resources.
extern "C" int pthread_create(pthread_t* thread, const pthread_attr_t* attr,
                             void* (*entry)(void*), void* argument) noexcept
{
  if (failNextThread.exchange(false))
    return EAGAIN;

  typedef int (*CreateThread)(pthread_t*, const pthread_attr_t*,
                              void* (*)(void*), void*);
  static CreateThread createThread = reinterpret_cast<CreateThread>(
      dlsym(RTLD_NEXT, "pthread_create"));
  require(createThread != NULL, "find the real pthread_create");
  const int result = createThread(thread, attr, entry, argument);
  if (result == 0 && finishBeforeCreateReturns.exchange(false))
  {
    while (!earlyTaskFinished.load())
      std::this_thread::yield();
  }
  return result;
}

int main()
{
  const int initialCount = reactor::Thread::numCreated();
  const int immediateDetachCount = 500;
  {
    reactor::Thread neverStarted([] {});
    require(!neverStarted.started(), "construction does not start a thread");
    require(neverStarted.tid() == 0, "unstarted TID is zero");
    require(neverStarted.name() == "Thread" + std::to_string(initialCount + 1),
            "default name uses the construction counter");
  }

  {
    std::promise<pid_t> observedTid;
    std::promise<std::string> observedName;
    std::promise<void> release;
    std::shared_future<void> gate = release.get_future().share();
    std::atomic<bool> completed(false);
    reactor::Thread thread([&] {
      observedTid.set_value(static_cast<pid_t>(::syscall(SYS_gettid)));
      char name[16] = {};
      require(::prctl(PR_GET_NAME, name) == 0, "read kernel thread name");
      observedName.set_value(name);
      gate.wait();
      completed = true;
    }, "thread-test");
    thread.start();
    require(thread.started(), "start updates the state");
    require(thread.tid() > 0, "start waits for TID initialization");
    require(thread.tid() != reactor::CurrentThread::tid(), "worker has its own TID");
    require(thread.tid() == observedTid.get_future().get(), "TID matches the kernel");
    require(observedName.get_future().get() == "thread-test", "kernel name is preserved");
    release.set_value();
    thread.join();
    require(completed, "join waits for task completion");
  }

  {
    std::promise<void> release;
    std::shared_future<void> gate = release.get_future().share();
    std::promise<std::string> completed;
    std::promise<void> destroyed;
    const std::string name = "detached-thread-with-an-owned-long-name";
    {
      std::shared_ptr<int> payload(new int(73), [&](int* value) {
        delete value;
        destroyed.set_value();
      });
      reactor::Thread thread([payload, gate, &completed] {
        gate.wait();
        require(*payload == 73, "detached task owns its captured state");
        completed.set_value(reactor::CurrentThread::name());
      }, name);
      thread.start();
    }
    release.set_value();
    std::future<std::string> result = completed.get_future();
    require(result.wait_for(std::chrono::seconds(5)) == std::future_status::ready,
            "detached worker finishes after wrapper destruction");
    require(result.get() == name, "detached thread retains its name");
    require(destroyed.get_future().wait_for(std::chrono::seconds(5)) ==
                std::future_status::ready,
            "detached task releases owned state after completion");
  }

  {
    std::shared_ptr<int> payload = std::make_shared<int>(42);
    std::atomic<int> calls(0);
    reactor::Thread thread([payload, &calls] { ++calls; });
    const long owners = payload.use_count();
    failNextThread = true;
    bool failed = false;
    try
    {
      thread.start();
    }
    catch (const std::system_error& error)
    {
      failed = error.code() == std::errc::resource_unavailable_try_again;
    }
    require(failed, "thread creation failure propagates std::system_error");
    require(!thread.started() && thread.tid() == 0, "failure restores unstarted state");
    require(payload.use_count() == owners, "creation failure releases task copy");
    thread.start();
    thread.join();
    require(calls == 1, "creation can be retried after failure");
  }

  {
    struct Task
    {
      std::shared_ptr<bool> fail;
      std::atomic<int>* calls;
      Task(std::shared_ptr<bool> flag, std::atomic<int>* count)
        : fail(flag), calls(count) {}
      Task(const Task& other) : fail(other.fail), calls(other.calls)
      {
        if (*fail)
          throw std::runtime_error("task copy failed");
      }
      void operator()() const { ++*calls; }
    };
    std::shared_ptr<bool> fail = std::make_shared<bool>(false);
    std::atomic<int> calls(0);
    reactor::Thread thread(Task(fail, &calls));
    *fail = true;
    bool failed = false;
    try { thread.start(); }
    catch (const std::runtime_error&) { failed = true; }
    require(failed && !thread.started(), "task copy failure restores start state");
    *fail = false;
    thread.start();
    thread.join();
    require(calls == 1, "task remains usable after copy failure");
  }

  {
    std::promise<void> release;
    std::shared_future<void> gate = release.get_future().share();
    std::atomic<int> completed(0);
    std::set<pid_t> tids;
    std::vector<std::unique_ptr<reactor::Thread>> threads;
    for (int i = 0; i < 16; ++i)
    {
      threads.emplace_back(new reactor::Thread([gate, &completed] {
        gate.wait();
        ++completed;
      }));
      threads.back()->start();
      require(tids.insert(threads.back()->tid()).second, "live worker TIDs are unique");
    }
    release.set_value();
    for (const auto& thread : threads)
      thread->join();
    require(completed == 16, "all concurrent workers finish");
  }
  {
    finishBeforeCreateReturns = true;
    reactor::Thread thread([] { earlyTaskFinished = true; });
    thread.start();
    require(earlyTaskFinished, "worker finishes before the creating thread starts waiting");
    require(thread.tid() > 0, "notification before wait still publishes the TID");
    thread.join();
  }

  {
    std::promise<void> destroyed;
    std::atomic<int> completed(0);
    std::shared_ptr<int> lifetime(new int(0), [&](int* value) {
      delete value;
      destroyed.set_value();
    });
    for (int i = 0; i < immediateDetachCount; ++i)
    {
      reactor::Thread thread([lifetime, &completed] { ++completed; });
      thread.start();
      require(thread.tid() > 0, "immediate detach follows startup synchronization");
    }
    lifetime.reset();
    require(destroyed.get_future().wait_for(std::chrono::seconds(5)) ==
                std::future_status::ready,
            "all immediately detached workers release their state");
    require(completed == immediateDetachCount, "all immediately detached tasks finish");
  }
  require(reactor::Thread::numCreated() == initialCount + 22 + immediateDetachCount,
          "construction count is shared across all instances");
  std::puts("PASS: lifecycle, failure/retry, concurrent workers, early notification, immediate detach");
}
