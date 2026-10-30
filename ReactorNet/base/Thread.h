
#ifndef REACTORNET_BASE_THREAD_H
#define REACTORNET_BASE_THREAD_H

#include "ReactorNet/base/noncopyable.h"
#include "ReactorNet/base/Types.h"

#include <atomic>
#include <condition_variable>
#include <thread>
#include <functional>
#include <mutex>
#include <sys/types.h>

namespace reactor
{
	// FIXME: make it movable in C++11
	class Thread : noncopyable
	{
	 public:
	  using ThreadFunc = std::function<void ()>;

	  explicit Thread(ThreadFunc, const string& name = string());

	  ~Thread();

	  void start();
	  void join();

	  bool started() const { return m_started; }
	  pid_t tid() const { return m_tid; }
	  const string& name() const { return m_name; }

	  static int numCreated() { return m_numCreated.load(); }

	 private:
	  void setDefaultName();
	  bool       m_started;
	  std::thread m_thread;
	  // Linux 内核线程 ID，由 gettid() 获取；同一 PID 命名空间中存活的线程 ID 唯一。
	  pid_t      m_tid;
	  ThreadFunc m_func;
	  string     m_name;
	  std::mutex m_startMutex;
	  std::condition_variable m_startCondition;

	  static std::atomic_int32_t m_numCreated;
	};

}  // namespace reactor
#endif  // REACTORNET_BASE_THREAD_H
