// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is a public header file, it must only include public header files.

#ifndef REACTORNET_NET_TCPCLIENT_H
#define REACTORNET_NET_TCPCLIENT_H

#include <mutex>
#include <atomic>
#include "ReactorNet/base/noncopyable.h"
#include "ReactorNet/net/TcpConnection.h"

namespace reactor
{
namespace net
{

class Connector;
using ConnectorPtr = std::shared_ptr<Connector>;

class TcpClient : noncopyable
{
 public:
  // TcpClient(EventLoop* loop);
  // TcpClient(EventLoop* loop, const string& host, uint16_t port);
  TcpClient(EventLoop* loop,
            const InetAddress& serverAddr,
            const string& nameArg);
  ~TcpClient();  // force out-line dtor, for std::unique_ptr members.

  void connect();
  void disconnect();
  void stop();

  TcpConnectionPtr connection() const
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_connection;
  }

  EventLoop* getLoop() const { return m_loop; }
  bool retry() const { return m_retry; }
  void enableRetry() { m_retry = true; }

  const string& name() const
  { return m_name; }

  /// Set connection callback.
  /// Not thread safe.
  void setConnectionCallback(ConnectionCallback cb)
  { m_connectionCallback = std::move(cb); }

  /// Set message callback.
  /// Not thread safe.
  void setMessageCallback(MessageCallback cb)
  { m_messageCallback = std::move(cb); }

  /// Set write complete callback.
  /// Not thread safe.
  void setWriteCompleteCallback(WriteCompleteCallback cb)
  { m_writeCompleteCallback = std::move(cb); }

 private:
  /// Not thread safe, but in loop
  void newConnection(int sockfd);
  /// Not thread safe, but in loop
  void removeConnection(const TcpConnectionPtr& conn);

  EventLoop* m_loop;
  ConnectorPtr m_connector; // avoid revealing Connector
  const string m_name;
  ConnectionCallback m_connectionCallback;
  MessageCallback m_messageCallback;
  WriteCompleteCallback m_writeCompleteCallback;
  std::atomic<bool> m_retry;
  std::atomic<bool> m_connect;
  // always in loop thread
  int m_nextConnId;
  mutable std::mutex m_mutex;
  TcpConnectionPtr m_connection;
};

}  // namespace net
}  // namespace reactor

#endif  // REACTORNET_NET_TCPCLIENT_H
