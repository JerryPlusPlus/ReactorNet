// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "ReactorNet/net/EventLoopThreadPool.h"

#include "ReactorNet/net/EventLoop.h"
#include "ReactorNet/net/EventLoopThread.h"

#include <stdio.h>

using namespace reactor;
using namespace reactor::net;

EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseLoop, const string& nameArg)
  : m_baseLoop(baseLoop),
    m_name(nameArg),
    m_started(false),
    m_numThreads(0),
    m_next(0)
{
}

EventLoopThreadPool::~EventLoopThreadPool()
{
  // Don't delete loop, it's stack variable
}

void EventLoopThreadPool::start(const ThreadInitCallback& cb)
{
  assert(!m_started);
  m_baseLoop->assertInLoopThread();

  m_started = true;

  for (int i = 0; i < m_numThreads; ++i)
  {
    EventLoopThread* t = new EventLoopThread(cb, m_name + std::to_string(i));
    m_threads.push_back(std::unique_ptr<EventLoopThread>(t));
    m_loops.push_back(t->startLoop());
  }
  if (m_numThreads == 0 && cb)
  {
    cb(m_baseLoop);
  }
}

EventLoop* EventLoopThreadPool::getNextLoop()
{
  m_baseLoop->assertInLoopThread();
  assert(m_started);
  EventLoop* loop = m_baseLoop;

  if (!m_loops.empty())
  {
    // round-robin
    loop = m_loops[m_next];
    ++m_next;
    if (implicit_cast<size_t>(m_next) >= m_loops.size())
    {
      m_next = 0;
    }
  }
  return loop;
}

EventLoop* EventLoopThreadPool::getLoopForHash(size_t hashCode)
{
  m_baseLoop->assertInLoopThread();
  EventLoop* loop = m_baseLoop;

  if (!m_loops.empty())
  {
    loop = m_loops[hashCode % m_loops.size()];
  }
  return loop;
}

std::vector<EventLoop*> EventLoopThreadPool::getAllLoops()
{
  m_baseLoop->assertInLoopThread();
  assert(m_started);
  if (m_loops.empty())
  {
    return std::vector<EventLoop*>(1, m_baseLoop);
  }
  else
  {
    return m_loops;
  }
}
