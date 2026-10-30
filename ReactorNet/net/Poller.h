// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is an internal header file, you should not include this.

#ifndef REACTORNET_NET_POLLER_H
#define REACTORNET_NET_POLLER_H

#include <map>
#include <vector>

#include "ReactorNet/base/Timestamp.h"
#include "ReactorNet/net/EventLoop.h"

namespace reactor
{
namespace net
{

class Channel;

///
/// Base class for IO Multiplexing
///
/// This class doesn't own the Channel objects.
class Poller : noncopyable
{
 public:
  using ChannelList = std::vector<Channel*>;

  Poller(EventLoop* loop);
  virtual ~Poller();

  /// Polls the I/O events.
  /// Must be called in the loop thread.
  virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels) = 0;

  /// Changes the interested I/O events.
  /// Must be called in the loop thread.
  virtual void updateChannel(Channel* channel) = 0;

  /// Remove the channel, when it destructs.
  /// Must be called in the loop thread.
  virtual void removeChannel(Channel* channel) = 0;

  virtual bool hasChannel(Channel* channel) const;

  static Poller* newDefaultPoller(EventLoop* loop);

  void assertInLoopThread() const
  {
    m_ownerLoop->assertInLoopThread();
  }

 protected:
  using ChannelMap = std::map<int, Channel*>;
  ChannelMap m_channels;

 private:
  EventLoop* m_ownerLoop;
};

}  // namespace net
}  // namespace reactor

#endif  // REACTORNET_NET_POLLER_H
