// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is an internal header file, you should not include this.

#ifndef REACTORNET_NET_EVENTLOOPTHREADPOOL_H
#define REACTORNET_NET_EVENTLOOPTHREADPOOL_H

#include "ReactorNet/base/noncopyable.h"
#include "ReactorNet/base/Types.h"

#include <functional>
#include <memory>
#include <vector>

namespace reactor
{

namespace net
{

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool : noncopyable
{
 public:
  using ThreadInitCallback = std::function<void(EventLoop*)>;

  EventLoopThreadPool(EventLoop* baseLoop, const string& nameArg);
  ~EventLoopThreadPool();
  void setThreadNum(int numThreads) { m_numThreads = numThreads; }
  void start(const ThreadInitCallback& cb = ThreadInitCallback());

  // valid after calling start()
  /// round-robin
  EventLoop* getNextLoop();

  /// with the same hash code, it will always return the same EventLoop
  EventLoop* getLoopForHash(size_t hashCode);

  std::vector<EventLoop*> getAllLoops();

  bool started() const
  { return m_started; }

  const string& name() const
  { return m_name; }

 private:

  EventLoop* m_baseLoop;
  string m_name;
  bool m_started;
  int m_numThreads;
  int m_next;
  std::vector<std::unique_ptr<EventLoopThread>> m_threads;
  std::vector<EventLoop*> m_loops;
};

}  // namespace net
}  // namespace reactor

#endif  // REACTORNET_NET_EVENTLOOPTHREADPOOL_H
