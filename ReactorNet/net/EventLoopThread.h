// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is a public header file, it must only include public header files.

#ifndef REACTORNET_NET_EVENTLOOPTHREAD_H
#define REACTORNET_NET_EVENTLOOPTHREAD_H

#include <condition_variable>
#include <mutex>
#include "ReactorNet/base/noncopyable.h"
#include "ReactorNet/base/Thread.h"

namespace reactor
{
namespace net
{

class EventLoop;

class EventLoopThread : noncopyable
{
 public:
  using ThreadInitCallback = std::function<void(EventLoop*)>;

  EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback(),
                  const string& name = string());
  ~EventLoopThread();
  EventLoop* startLoop();

 private:
  void threadFunc();

  EventLoop* m_loop;
  bool m_exiting{false};
  bool m_ready{false};
  Thread m_thread;
  std::mutex m_mutex;
  std::condition_variable m_cond;
  ThreadInitCallback m_callback;
};

}  // namespace net
}  // namespace reactor

#endif  // REACTORNET_NET_EVENTLOOPTHREAD_H

