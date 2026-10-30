// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is a public header file, it must only include public header files.

#ifndef REACTORNET_NET_TCPSERVER_H
#define REACTORNET_NET_TCPSERVER_H

#include <atomic>
#include "ReactorNet/base/Types.h"
#include "ReactorNet/net/TcpConnection.h"

#include <map>

namespace reactor
{
namespace net
{

class Acceptor;
class EventLoop;
class EventLoopThreadPool;

///
/// TCP server, supports single-threaded and thread-pool models.
///
/// This is an interface class, so don't expose too much details.
class TcpServer : noncopyable
{
 public:
  using ThreadInitCallback = std::function<void(EventLoop*)>;
  enum Option
  {
    kNoReusePort,
    kReusePort,
  };

  //TcpServer(EventLoop* loop, const InetAddress& listenAddr);
  TcpServer(EventLoop* loop,
            const InetAddress& listenAddr,
            const string& nameArg,
            Option option = kNoReusePort);
  ~TcpServer();  // force out-line dtor, for std::unique_ptr members.

  const string& ipPort() const { return m_ipPort; }
  const string& name() const { return m_name; }
  EventLoop* getLoop() const { return m_loop; }

  /// Set the number of threads for handling input.
  ///
  /// Always accepts new connection in loop's thread.
  /// Must be called before @c start
  /// @param numThreads
  /// - 0 means all I/O in loop's thread, no thread will created.
  ///   this is the default value.
  /// - 1 means all I/O in another thread.
  /// - N means a thread pool with N threads, new connections
  ///   are assigned on a round-robin basis.
  void setThreadNum(int numThreads);
  void setThreadInitCallback(const ThreadInitCallback& cb)
  { m_threadInitCallback = cb; }
  /// valid after calling start()
  std::shared_ptr<EventLoopThreadPool> threadPool()
  { return m_threadPool; }

  /// Starts the server if it's not listenning.
  ///
  /// It's harmless to call it multiple times.
  /// Thread safe.
  void start();

  /// Set connection callback.
  /// Not thread safe.
  void setConnectionCallback(const ConnectionCallback& cb)
  { m_connectionCallback = cb; }

  /// Set message callback.
  /// Not thread safe.
  void setMessageCallback(const MessageCallback& cb)
  { m_messageCallback = cb; }

  /// Set write complete callback.
  /// Not thread safe.
  void setWriteCompleteCallback(const WriteCompleteCallback& cb)
  { m_writeCompleteCallback = cb; }

 private:
  /// Not thread safe, but in loop
  void newConnection(int sockfd, const InetAddress& peerAddr);
  /// Thread safe.
  void removeConnection(const TcpConnectionPtr& conn);
  /// Not thread safe, but in loop
  void removeConnectionInLoop(const TcpConnectionPtr& conn);

  using ConnectionMap = std::map<string, TcpConnectionPtr>;

  EventLoop* m_loop;  // the acceptor loop
  const string m_ipPort;
  const string m_name;
  std::unique_ptr<Acceptor> m_acceptor; // avoid revealing Acceptor
  std::shared_ptr<EventLoopThreadPool> m_threadPool;
  ConnectionCallback m_connectionCallback;
  MessageCallback m_messageCallback;
  WriteCompleteCallback m_writeCompleteCallback;
  ThreadInitCallback m_threadInitCallback;
  std::atomic<bool> m_started{false};
  // always in loop thread
  int m_nextConnId;
  ConnectionMap m_connections;
};

}  // namespace net
}  // namespace reactor

#endif  // REACTORNET_NET_TCPSERVER_H
