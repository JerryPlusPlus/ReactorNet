// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is an internal header file, you should not include this.

#ifndef REACTORNET_NET_ACCEPTOR_H
#define REACTORNET_NET_ACCEPTOR_H

#include <functional>

#include "ReactorNet/net/Channel.h"
#include "ReactorNet/net/Socket.h"

namespace reactor
{
namespace net
{

class EventLoop;
class InetAddress;

///
/// Acceptor of incoming TCP connections.
///
class Acceptor : noncopyable
{
 public:
  using NewConnectionCallback = std::function<void (int sockfd, const InetAddress&)>;

  Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reuseport);
  ~Acceptor();

  void setNewConnectionCallback(const NewConnectionCallback& cb)
  { m_newConnectionCallback = cb; }

  bool listenning() const { return m_listenning; }
  void listen();

 private:
  void handleRead();

  EventLoop* m_loop;
  Socket m_acceptSocket;
  Channel m_acceptChannel;
  NewConnectionCallback m_newConnectionCallback;
  bool m_listenning;
  int m_idleFd;
};

}  // namespace net
}  // namespace reactor

#endif  // REACTORNET_NET_ACCEPTOR_H
