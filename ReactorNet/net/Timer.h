// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is an internal header file, you should not include this.

#ifndef REACTORNET_NET_TIMER_H
#define REACTORNET_NET_TIMER_H

#include <atomic>
#include "ReactorNet/base/noncopyable.h"
#include "ReactorNet/base/Timestamp.h"
#include "ReactorNet/net/Callbacks.h"

namespace reactor
{
namespace net
{

///
/// Internal class for timer event.
///
class Timer : noncopyable
{
 public:
  Timer(TimerCallback cb, Timestamp when, double interval)
    : m_callback(std::move(cb)),
      m_expiration(when),
      m_interval(interval),
      m_repeat(interval > 0.0),
      m_sequence(m_numCreated.fetch_add(1, std::memory_order_relaxed) + 1)
  { }

  void run() const
  {
    m_callback();
  }

  Timestamp expiration() const  { return m_expiration; }
  bool repeat() const { return m_repeat; }
  int64_t sequence() const { return m_sequence; }

  void restart(Timestamp now);

  static int64_t numCreated() { return m_numCreated.load(std::memory_order_relaxed); }

 private:
  const TimerCallback m_callback;
  Timestamp m_expiration;
  const double m_interval;
  const bool m_repeat;
  const int64_t m_sequence;

  static std::atomic<int64_t> m_numCreated;
};

}  // namespace net
}  // namespace reactor

#endif  // REACTORNET_NET_TIMER_H
