// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "ReactorNet/net/Poller.h"

#include "ReactorNet/net/Channel.h"

using namespace reactor;
using namespace reactor::net;

Poller::Poller(EventLoop* loop)
  : m_ownerLoop(loop)
{
}

Poller::~Poller() = default;

bool Poller::hasChannel(Channel* channel) const
{
  assertInLoopThread();
  ChannelMap::const_iterator it = m_channels.find(channel->fd());
  return it != m_channels.end() && it->second == channel;
}

