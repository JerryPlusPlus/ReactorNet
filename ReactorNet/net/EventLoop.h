// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is a public header file, it must only include public header files.

#ifndef REACTORNET_NET_EVENTLOOP_H
#define REACTORNET_NET_EVENTLOOP_H

#include <atomic>
#include <functional>
#include <vector>

#include "ReactorNet/base/Any.h"

#include <mutex>
#include "ReactorNet/base/noncopyable.h"
#include "ReactorNet/base/CurrentThread.h"
#include "ReactorNet/base/Timestamp.h"
#include "ReactorNet/net/Callbacks.h"
#include "ReactorNet/net/TimerId.h"

namespace reactor
{
namespace net
{

class Channel;
class Poller;
class TimerQueue;

///
/// Reactor, at most one per thread.
///
/// This is an interface class, so don't expose too much details.
class EventLoop : noncopyable
{
 public:
  using Functor = std::function<void()>;

  EventLoop();
  ~EventLoop();  // force out-line dtor, for std::unique_ptr members.

  ///
  /// Loops forever.
  ///
  /// Must be called in the same thread as creation of the object.
  ///
  void loop();

  /// Quits loop.
  ///
  /// This is not 100% thread safe, if you call through a raw pointer,
  /// better to call through shared_ptr<EventLoop> for 100% safety.
  void quit();

  ///
  /// Time when poll returns, usually means data arrival.
  ///
  Timestamp pollReturnTime() const { return m_pollReturnTime; }

  int64_t iteration() const { return m_iteration; }

  /// Runs callback immediately in the loop thread.
  /// It wakes up the loop, and run the cb.
  /// If in the same loop thread, cb is run within the function.
  /// Safe to call from other threads.
  void runInLoop(Functor cb);
  /// Queues callback in the loop thread.
  /// Runs after finish pooling.
  /// Safe to call from other threads.
  void queueInLoop(Functor cb);

  size_t queueSize() const;

  // timers

  ///
  /// Runs callback at 'time'.
  /// Safe to call from other threads.
  ///
  TimerId runAt(Timestamp time, TimerCallback cb);
  ///
  /// Runs callback after @c delay seconds.
  /// Safe to call from other threads.
  ///
  TimerId runAfter(double delay, TimerCallback cb);
  ///
  /// Runs callback every @c interval seconds.
  /// Safe to call from other threads.
  ///
  TimerId runEvery(double interval, TimerCallback cb);
  ///
  /// Cancels the timer.
  /// Safe to call from other threads.
  ///
  void cancel(TimerId timerId);

  // internal usage
  void wakeup();
  void updateChannel(Channel* channel);
  void removeChannel(Channel* channel);
  bool hasChannel(Channel* channel);

  // pid_t threadId() const { return m_threadId; }
  void assertInLoopThread()
  {
    if (!isInLoopThread())
    {
      abortNotInLoopThread();
    }
  }
  bool isInLoopThread() const { return m_threadId == CurrentThread::tid(); }
  // bool callingPendingFunctors() const { return m_callingPendingFunctors; }
  bool eventHandling() const { return m_eventHandling; }

  void setContext(const Any& context)
  { m_context = context; }

  const Any& getContext() const
  { return m_context; }

  Any* getMutableContext()
  { return &m_context; }

  static EventLoop* getEventLoopOfCurrentThread();

 private:
  void abortNotInLoopThread();
  void handleRead();  // waked up
  void doPendingFunctors();

  void printActiveChannels() const; // DEBUG

  using ChannelList = std::vector<Channel*>;

  bool m_looping; /* atomic */
  std::atomic<bool> m_quit;
  bool m_eventHandling; /* atomic */
  bool m_callingPendingFunctors; /* atomic */
  int64_t m_iteration;
  const pid_t m_threadId;
  Timestamp m_pollReturnTime;
  std::unique_ptr<Poller> m_poller;
  std::unique_ptr<TimerQueue> m_timerQueue;
  int m_wakeupFd;
  // unlike in TimerQueue, which is an internal class,
  // we don't expose Channel to client.
  std::unique_ptr<Channel> m_wakeupChannel;
  Any m_context;

  // scratch variables
  ChannelList m_activeChannels;
  Channel* m_currentActiveChannel;

  mutable std::mutex m_mutex;
  std::vector<Functor> m_pendingFunctors;
};

}  // namespace net
}  // namespace reactor

#endif  // REACTORNET_NET_EVENTLOOP_H
