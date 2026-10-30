// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "ReactorNet/net/EventLoopThread.h"
#include "ReactorNet/net/EventLoop.h"
#include <stdexcept>
using namespace reactor;
using namespace reactor::net;

EventLoopThread::EventLoopThread(const ThreadInitCallback& cb, const string& name)
  : m_loop(nullptr), m_thread([this] { threadFunc(); }, name), m_callback(cb) {}

EventLoopThread::~EventLoopThread()
{
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_exiting = true;
    if (m_loop) m_loop->quit();
  }
  if (m_thread.started()) m_thread.join();
}

EventLoop* EventLoopThread::startLoop()
{
  assert(!m_thread.started());
  m_thread.start();
  std::unique_lock<std::mutex> lock(m_mutex);
  m_cond.wait(lock, [this] { return m_ready; });
  if (!m_loop) throw std::runtime_error("EventLoop exited during startup");
  return m_loop;
}

void EventLoopThread::threadFunc()
{
  EventLoop loop;
  if (m_callback) m_callback(&loop);
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_loop = &loop;
    m_ready = true;
    if (m_exiting) loop.quit();
    m_cond.notify_one();
  }
  loop.loop();
  std::lock_guard<std::mutex> lock(m_mutex);
  m_loop = nullptr;
}
