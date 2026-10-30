// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is an internal header file, you should not include this.

#ifndef REACTORNET_NET_POLLER_EPOLLPOLLER_H
#define REACTORNET_NET_POLLER_EPOLLPOLLER_H

#include "ReactorNet/net/Poller.h"

#include <vector>

struct epoll_event;

namespace reactor
{
namespace net
{

///
/// IO Multiplexing with epoll(4).
///
class EPollPoller : public Poller
{
 public:
  EPollPoller(EventLoop* loop);
  ~EPollPoller() override;

  Timestamp poll(int timeoutMs, ChannelList* activeChannels) override;
  void updateChannel(Channel* channel) override;
  void removeChannel(Channel* channel) override;

 private:
  static const int kInitEventListSize = 16;

  static const char* operationToString(int op);

  void fillActiveChannels(int numEvents,
                          ChannelList* activeChannels) const;
  void update(int operation, Channel* channel);

  using EventList = std::vector<struct epoll_event>;

  int m_epollfd;
  EventList m_events;
};

}  // namespace net
}  // namespace reactor
#endif  // REACTORNET_NET_POLLER_EPOLLPOLLER_H
