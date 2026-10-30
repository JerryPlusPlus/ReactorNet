// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is an internal header file, you should not include this.

#ifndef REACTORNET_NET_CONNECTOR_H
#define REACTORNET_NET_CONNECTOR_H

#include "ReactorNet/base/noncopyable.h"
#include "ReactorNet/net/InetAddress.h"

#include <functional>
#include <atomic>
#include <memory>

namespace reactor
{
namespace net
{

class Channel;
class EventLoop;

class Connector : noncopyable,
                  public std::enable_shared_from_this<Connector>
{
 public:
  using NewConnectionCallback = std::function<void (int sockfd)>;

  Connector(EventLoop* loop, const InetAddress& serverAddr);
  ~Connector();

  void setNewConnectionCallback(const NewConnectionCallback& cb)
  { m_newConnectionCallback = cb; }

  void start();  // can be called in any thread
  void restart();  // must be called in loop thread
  void stop();  // can be called in any thread

  const InetAddress& serverAddress() const { return m_serverAddr; }

 private:
  enum States { kDisconnected, kConnecting, kConnected };
  static const int kMaxRetryDelayMs = 30*1000;
  static const int kInitRetryDelayMs = 500;

  void setState(States s) { m_state = s; }
  void startInLoop();
  void stopInLoop();
  void connect();
  void connecting(int sockfd);
  void handleWrite();
  void handleError();
  void retry(int sockfd);
  int removeAndResetChannel();
  void resetChannel();

  EventLoop* m_loop;
  InetAddress m_serverAddr;
  std::atomic<bool> m_connect;
  std::atomic<States> m_state;
  std::unique_ptr<Channel> m_channel;
  NewConnectionCallback m_newConnectionCallback;
  int m_retryDelayMs;
};

}  // namespace net
}  // namespace reactor

#endif  // REACTORNET_NET_CONNECTOR_H
