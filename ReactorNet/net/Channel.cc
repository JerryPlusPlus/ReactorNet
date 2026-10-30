// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "ReactorNet/log/Logging.h"
#include "ReactorNet/net/Channel.h"
#include "ReactorNet/net/EventLoop.h"

#include <sstream>

#include <poll.h>

using namespace reactor;
using namespace reactor::net;

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = POLLIN | POLLPRI;
const int Channel::kWriteEvent = POLLOUT;

Channel::Channel(EventLoop* loop, int fd__)
  : m_loop(loop),
    m_fd(fd__),
    m_events(0),
    m_revents(0),
    m_index(-1),
    m_logHup(true),
    m_tied(false),
    m_eventHandling(false),
    m_addedToLoop(false)
{
}

Channel::~Channel()
{
  assert(!m_eventHandling);
  assert(!m_addedToLoop);
  if (m_loop->isInLoopThread())
  {
    assert(!m_loop->hasChannel(this));
  }
}

void Channel::tie(const std::shared_ptr<void>& obj)
{
  m_tie = obj;
  m_tied = true;
}

void Channel::update()
{
  m_addedToLoop = true;
  m_loop->updateChannel(this);
}

void Channel::remove()
{
  assert(isNoneEvent());
  m_addedToLoop = false;
  m_loop->removeChannel(this);
}

void Channel::handleEvent(Timestamp receiveTime)
{
  std::shared_ptr<void> guard;
  if (m_tied)
  {
    guard = m_tie.lock();
    if (guard)
    {
      handleEventWithGuard(receiveTime);
    }
  }
  else
  {
    handleEventWithGuard(receiveTime);
  }
}

void Channel::handleEventWithGuard(Timestamp receiveTime)
{
  m_eventHandling = true;
  LOG_TRACE << reventsToString();
  if ((m_revents & POLLHUP) && !(m_revents & POLLIN))
  {
    if (m_logHup)
    {
      LOG_WARN << "fd = " << m_fd << " Channel::handle_event() POLLHUP";
    }
    if (m_closeCallback) m_closeCallback();
  }

  if (m_revents & POLLNVAL)
  {
    LOG_WARN << "fd = " << m_fd << " Channel::handle_event() POLLNVAL";
  }

  if (m_revents & (POLLERR | POLLNVAL))
  {
    if (m_errorCallback) m_errorCallback();
  }
  if (m_revents & (POLLIN | POLLPRI | POLLRDHUP))
  {
    if (m_readCallback) m_readCallback(receiveTime);
  }
  if (m_revents & POLLOUT)
  {
    if (m_writeCallback) m_writeCallback();
  }
  m_eventHandling = false;
}

string Channel::reventsToString() const
{
  return eventsToString(m_fd, m_revents);
}

string Channel::eventsToString() const
{
  return eventsToString(m_fd, m_events);
}

string Channel::eventsToString(int fd, int ev)
{
  std::ostringstream oss;
  oss << fd << ": ";
  if (ev & POLLIN)
    oss << "IN ";
  if (ev & POLLPRI)
    oss << "PRI ";
  if (ev & POLLOUT)
    oss << "OUT ";
  if (ev & POLLHUP)
    oss << "HUP ";
  if (ev & POLLRDHUP)
    oss << "RDHUP ";
  if (ev & POLLERR)
    oss << "ERR ";
  if (ev & POLLNVAL)
    oss << "NVAL ";

  return oss.str();
}
