// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is an internal header file, you should not include this.

#ifndef REACTORNET_NET_TIMERQUEUE_H
#define REACTORNET_NET_TIMERQUEUE_H

#include <set>
#include <vector>

#include "ReactorNet/base/noncopyable.h"
#include "ReactorNet/base/Timestamp.h"
#include "ReactorNet/net/Callbacks.h"
#include "ReactorNet/net/Channel.h"

namespace reactor
{
namespace net
{

class EventLoop;
class Timer;
class TimerId;

///
/// A best efforts timer queue.
/// No guarantee that the callback will be on time.
///
class TimerQueue : noncopyable
{
 public:
  explicit TimerQueue(EventLoop* loop);
  ~TimerQueue();

  ///
  /// Schedules the callback to be run at given time,
  /// repeats if @c interval > 0.0.
  ///
  /// Must be thread safe. Usually be called from other threads.
  TimerId addTimer(TimerCallback cb,
                   Timestamp when,
                   double interval);

  void cancel(TimerId timerId);

 private:

  // FIXME: use unique_ptr<Timer> instead of raw pointers.
  // This requires heterogeneous comparison lookup (N3465) from C++14
  // so that we can find an T* in a set<unique_ptr<T>>.
  using Entry = std::pair<Timestamp, Timer*>;
  using TimerList = std::set<Entry>;
  using ActiveTimer = std::pair<Timer*, int64_t>;
  using ActiveTimerSet = std::set<ActiveTimer>;

  void addTimerInLoop(Timer* timer);
  void cancelInLoop(TimerId timerId);
  // called when timerfd alarms
  void handleRead();
  // move out all expired timers
  std::vector<Entry> getExpired(Timestamp now);
  void reset(const std::vector<Entry>& expired, Timestamp now);

  bool insert(Timer* timer);

  EventLoop* m_loop;
  const int m_timerfd;
  Channel m_timerfdChannel;
  // Timer list sorted by expiration
  TimerList m_timers;

  // for cancel()
  ActiveTimerSet m_activeTimers;
  bool m_callingExpiredTimers; /* atomic */
  ActiveTimerSet m_cancelingTimers;
};

}  // namespace net
}  // namespace reactor
#endif  // REACTORNET_NET_TIMERQUEUE_H
