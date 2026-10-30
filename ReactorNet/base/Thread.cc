
#include "ReactorNet/base/Thread.h"
#include "ReactorNet/base/CurrentThread.h"
#include "ReactorNet/base/Exception.h"
#include "ReactorNet/base/Timestamp.h"

#include <memory>
#include <utility>

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <chrono>

namespace reactor
{
    namespace detail
    {

        pid_t gettid()
        {
          return static_cast<pid_t>(::syscall(SYS_gettid));
        }

        void afterFork()
        {
          reactor::CurrentThread::t_cachedTid = 0;
          reactor::CurrentThread::t_threadName = "main";
          CurrentThread::tid();
        }

        class ThreadNameInitializer
        {
         public:
          ThreadNameInitializer()
          {
            reactor::CurrentThread::t_threadName = "main";
            CurrentThread::tid();
            pthread_atfork(nullptr, nullptr, &afterFork);
          }
        };

        ThreadNameInitializer init;

        struct ThreadData
        {
          using ThreadFunc = reactor::Thread::ThreadFunc;
          ThreadFunc m_func;
          string m_name;
          pid_t* m_tid;
          std::mutex* m_startMutex;
          std::condition_variable* m_startCondition;

          ThreadData(ThreadFunc func,
                     const string& name,
                     pid_t* tid,
                     std::mutex* startMutex,
                     std::condition_variable* startCondition)
            : m_func(std::move(func)),
              m_name(name),
              m_tid(tid),
              m_startMutex(startMutex),
              m_startCondition(startCondition)
          { }

          void runInThread()
          {
            {
              std::lock_guard<std::mutex> lock(*m_startMutex);
              *m_tid = reactor::CurrentThread::tid();
              // Notify before unlocking so start() cannot destroy the condition early.
              m_startCondition->notify_one();
            }
            m_tid = nullptr;
            m_startMutex = nullptr;
            m_startCondition = nullptr;

            reactor::CurrentThread::t_threadName = m_name.c_str();
            // 这行代码会将 t_threadName 的值设置给操作系统，使得用 ps 或其他系统工具可以看到该线程的真实名称。
            ::prctl(PR_SET_NAME, reactor::CurrentThread::t_threadName);
            try
            {
              m_func();
              reactor::CurrentThread::t_threadName = "finished";
            }
            catch (const Exception& ex)
            {
              reactor::CurrentThread::t_threadName = "crashed";
              fprintf(stderr, "exception caught in Thread %s\n", m_name.c_str());
              fprintf(stderr, "reason: %s\n", ex.what());
              fprintf(stderr, "stack trace: %s\n", ex.stackTrace());
              abort();
            }
            catch (const std::exception& ex)
            {
              reactor::CurrentThread::t_threadName = "crashed";
              fprintf(stderr, "exception caught in Thread %s\n", m_name.c_str());
              fprintf(stderr, "reason: %s\n", ex.what());
              abort();
            }
            catch (...)
            {
              reactor::CurrentThread::t_threadName = "crashed";
              fprintf(stderr, "unknown exception caught in Thread %s\n", m_name.c_str());
              throw; // rethrow
            }
          }
    };

    }  // namespace detail

void CurrentThread::cacheTid()
{
  if (t_cachedTid == 0)
  {
    t_cachedTid = detail::gettid();
    t_tidStringLength = snprintf(t_tidString, sizeof t_tidString, "%5d ", t_cachedTid);
  }
}

bool CurrentThread::isMainThread()
{
  return tid() == ::getpid();
}

void CurrentThread::sleepUsec(int64_t usec)
{
  std::this_thread::sleep_for(std::chrono::microseconds(usec));
}

std::atomic_int32_t Thread::m_numCreated{ 0 };

Thread::Thread(ThreadFunc func, const string& n)
  : m_started(false),
    m_thread(),
    m_tid(0),
    m_func(std::move(func)),
    m_name(n),
    m_startMutex(),
    m_startCondition()
{
  setDefaultName();
}

Thread::~Thread()
{
  if (m_thread.joinable())
  {
    m_thread.detach();
  }
}

void Thread::setDefaultName()
{
  int num = ++m_numCreated;
  if (m_name.empty())
  {
    char buf[32];
    snprintf(buf, sizeof buf, "Thread%d", num);
    m_name = buf;
  }
}

void Thread::start()
{
  assert(!m_started);
  m_started = true;
  try
  {
    std::unique_ptr<detail::ThreadData> data(
        new detail::ThreadData(m_func, m_name, &m_tid,
                               &m_startMutex, &m_startCondition));
    // Let the worker own its data, including after this Thread is destroyed.
    m_thread = std::thread(&detail::ThreadData::runInThread, std::move(data));
  }
  catch (...)
  {
    m_started = false;
    throw;
  }
  std::unique_lock<std::mutex> lock(m_startMutex);
  m_startCondition.wait(lock, [this] { return m_tid > 0; });
  assert(m_tid > 0);
}

void Thread::join()
{
  assert(m_started);
  assert(m_thread.joinable());
  m_thread.join();
}

}  // namespace reactor

