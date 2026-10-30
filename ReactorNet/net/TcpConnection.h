// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is a public header file, it must only include public header files.

#ifndef REACTORNET_NET_TCPCONNECTION_H
#define REACTORNET_NET_TCPCONNECTION_H

#include "ReactorNet/base/noncopyable.h"
#include "ReactorNet/base/StringPiece.h"
#include "ReactorNet/base/Types.h"
#include "ReactorNet/net/Callbacks.h"
#include "ReactorNet/net/Buffer.h"
#include "ReactorNet/net/InetAddress.h"

#include <memory>
#include <atomic>

#include "ReactorNet/base/Any.h"

// struct tcp_info is in <netinet/tcp.h>
struct tcp_info;

namespace reactor
{
namespace net
{

class Channel;
class EventLoop;
class Socket;

///
/// TCP connection, for both client and server usage.
///
/// This is an interface class, so don't expose too much details.
class TcpConnection : noncopyable,
                      public std::enable_shared_from_this<TcpConnection>
{
 public:
  /// Constructs a TcpConnection with a connected sockfd
  ///
  /// User should not create this object.
  TcpConnection(EventLoop* loop,
                const string& name,
                int sockfd,
                const InetAddress& localAddr,
                const InetAddress& peerAddr);
  ~TcpConnection();

  EventLoop* getLoop() const { return m_loop; }
  const string& name() const { return m_name; }
  const InetAddress& localAddress() const { return m_localAddr; }
  const InetAddress& peerAddress() const { return m_peerAddr; }
  bool connected() const { return m_state == kConnected; }
  bool disconnected() const { return m_state == kDisconnected; }
  // return true if success.
  bool getTcpInfo(struct tcp_info*) const;
  string getTcpInfoString() const;

  // void send(string&& message); // C++11
  void send(const void* message, int len);
  void send(const StringPiece& message);
  // void send(Buffer&& message); // C++11
  void send(Buffer* message);  // this one will swap data
  void shutdown(); // NOT thread safe, no simultaneous calling
  // void shutdownAndForceCloseAfter(double seconds); // NOT thread safe, no simultaneous calling
  void forceClose();
  void forceCloseWithDelay(double seconds);
  void setTcpNoDelay(bool on);
  // reading or not
  void startRead();
  void stopRead();
  bool isReading() const { return m_reading; }; // NOT thread safe, may race with start/stopReadInLoop

  void setContext(const Any& context)
  { m_context = context; }

  const Any& getContext() const
  { return m_context; }

  Any* getMutableContext()
  { return &m_context; }

  void setConnectionCallback(const ConnectionCallback& cb)
  { m_connectionCallback = cb; }

  void setMessageCallback(const MessageCallback& cb)
  { m_messageCallback = cb; }

  void setWriteCompleteCallback(const WriteCompleteCallback& cb)
  { m_writeCompleteCallback = cb; }

  void setHighWaterMarkCallback(const HighWaterMarkCallback& cb, size_t highWaterMark)
  { m_highWaterMarkCallback = cb; m_highWaterMark = highWaterMark; }

  /// Advanced interface
  Buffer* inputBuffer()
  { return &m_inputBuffer; }

  Buffer* outputBuffer()
  { return &m_outputBuffer; }

  /// Internal use only.
  void setCloseCallback(const CloseCallback& cb)
  { m_closeCallback = cb; }

  // called when TcpServer accepts a new connection
  void connectEstablished();   // should be called only once
  // called when TcpServer has removed me from its map
  void connectDestroyed();  // should be called only once

 private:
  enum StateE { kDisconnected, kConnecting, kConnected, kDisconnecting };
  void handleRead(Timestamp receiveTime);
  void handleWrite();
  void handleClose();
  void handleError();
  // void sendInLoop(string&& message);
  void sendInLoop(const StringPiece& message);
  void sendInLoop(const void* message, size_t len);
  void shutdownInLoop();
  // void shutdownAndForceCloseInLoop(double seconds);
  void forceCloseInLoop();
  void setState(StateE s) { m_state = s; }
  const char* stateToString() const;
  void startReadInLoop();
  void stopReadInLoop();

  EventLoop* m_loop;
  const string m_name;
  std::atomic<StateE> m_state;
  std::atomic<bool> m_reading;
  bool m_peerShutdown{false}; // Only accessed by the owning EventLoop.
  // we don't expose those classes to client.
  std::unique_ptr<Socket> m_socket;
  std::unique_ptr<Channel> m_channel;
  const InetAddress m_localAddr;
  const InetAddress m_peerAddr;
  ConnectionCallback m_connectionCallback;
  MessageCallback m_messageCallback;
  WriteCompleteCallback m_writeCompleteCallback;
  HighWaterMarkCallback m_highWaterMarkCallback;
  CloseCallback m_closeCallback;
  size_t m_highWaterMark;
  Buffer m_inputBuffer;
  Buffer m_outputBuffer; // FIXME: use list<Buffer> as output buffer.
  Any m_context;
  // FIXME: m_creationTime, m_lastReceiveTime
  //        m_bytesReceived, m_bytesSent
};

using TcpConnectionPtr = std::shared_ptr<TcpConnection>;

}  // namespace net
}  // namespace reactor

#endif  // REACTORNET_NET_TCPCONNECTION_H
