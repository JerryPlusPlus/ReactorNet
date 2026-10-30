// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is an internal header file, you should not include this.

#ifndef REACTORNET_NET_CHANNEL_H
#define REACTORNET_NET_CHANNEL_H

#include "ReactorNet/base/noncopyable.h"
#include "ReactorNet/base/Timestamp.h"

#include <functional>
#include <memory>

namespace reactor
{
namespace net
{

class EventLoop;

///
/// A selectable I/O channel.
///
/// This class doesn't own the file descriptor.
/// The file descriptor could be a socket,
/// an eventfd, a timerfd, or a signalfd
class Channel : noncopyable
{
 public:
  using EventCallback = std::function<void()>;
  using ReadEventCallback = std::function<void(Timestamp)>;

  Channel(EventLoop* loop, int fd);
  ~Channel();

  void handleEvent(Timestamp receiveTime);
  void setReadCallback(ReadEventCallback cb)
  { m_readCallback = std::move(cb); }
  void setWriteCallback(EventCallback cb)
  { m_writeCallback = std::move(cb); }
  void setCloseCallback(EventCallback cb)
  { m_closeCallback = std::move(cb); }
  void setErrorCallback(EventCallback cb)
  { m_errorCallback = std::move(cb); }

  /// Tie this channel to the owner object managed by shared_ptr,
  /// prevent the owner object being destroyed in handleEvent.
  void tie(const std::shared_ptr<void>&);

  int fd() const { return m_fd; }
  int events() const { return m_events; }
  void set_revents(int revt) { m_revents = revt; } // used by pollers
  // int revents() const { return m_revents; }
  bool isNoneEvent() const { return m_events == kNoneEvent; }

  void enableReading() { m_events |= kReadEvent; update(); }
  void disableReading() { m_events &= ~kReadEvent; update(); }
  void enableWriting() { m_events |= kWriteEvent; update(); }
  void disableWriting() { m_events &= ~kWriteEvent; update(); }
  void disableAll() { m_events = kNoneEvent; update(); }
  bool isWriting() const { return m_events & kWriteEvent; }
  bool isReading() const { return m_events & kReadEvent; }

  // for Poller
  int index() { return m_index; }
  void set_index(int idx) { m_index = idx; }

  // for debug
  string reventsToString() const;
  string eventsToString() const;

  void doNotLogHup() { m_logHup = false; }

  EventLoop* ownerLoop() { return m_loop; }
  void remove();

 private:
  static string eventsToString(int fd, int ev);

  void update();
  void handleEventWithGuard(Timestamp receiveTime);

  static const int kNoneEvent;
  static const int kReadEvent;
  static const int kWriteEvent;

  EventLoop* m_loop;
  const int  m_fd;
  int        m_events;
  int        m_revents; // it's the received event types of epoll or poll
  int        m_index; // used by Poller.
  bool       m_logHup;

  std::weak_ptr<void> m_tie;
  bool m_tied;
  bool m_eventHandling;
  bool m_addedToLoop;
  ReadEventCallback m_readCallback;
  EventCallback m_writeCallback;
  EventCallback m_closeCallback;
  EventCallback m_errorCallback;
};

}  // namespace net
}  // namespace reactor

#endif  // REACTORNET_NET_CHANNEL_H
