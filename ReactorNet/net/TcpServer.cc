// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "ReactorNet/net/TcpServer.h"

#include "ReactorNet/log/Logging.h"
#include "ReactorNet/net/Acceptor.h"
#include "ReactorNet/net/EventLoop.h"
#include "ReactorNet/net/EventLoopThreadPool.h"
#include "ReactorNet/net/SocketsOps.h"

#include <stdio.h>  // snprintf

using namespace reactor;
using namespace reactor::net;

TcpServer::TcpServer(EventLoop* loop,
                     const InetAddress& listenAddr,
                     const string& nameArg,
                     Option option)
  : m_loop(CHECK_NOTNULL(loop)),
    m_ipPort(listenAddr.toIpPort()),
    m_name(nameArg),
    m_acceptor(new Acceptor(loop, listenAddr, option == kReusePort)),
    m_threadPool(new EventLoopThreadPool(loop, m_name)),
    m_connectionCallback(defaultConnectionCallback),
    m_messageCallback(defaultMessageCallback),
    m_nextConnId(1)
{
  m_acceptor->setNewConnectionCallback(
      std::bind(&TcpServer::newConnection, this, _1, _2));
}

TcpServer::~TcpServer()
{
  m_loop->assertInLoopThread();
  LOG_TRACE << "TcpServer::~TcpServer [" << m_name << "] destructing";

  for (auto& item : m_connections)
  {
    TcpConnectionPtr conn(item.second);
    item.second.reset();
    conn->getLoop()->runInLoop(
      std::bind(&TcpConnection::connectDestroyed, conn));
  }
}

void TcpServer::setThreadNum(int numThreads)
{
  assert(0 <= numThreads);
  m_threadPool->setThreadNum(numThreads);
}

void TcpServer::start()
{
  if (!m_started.exchange(true))
  {
    m_threadPool->start(m_threadInitCallback);

    assert(!m_acceptor->listenning());
    m_loop->runInLoop(
        std::bind(&Acceptor::listen, get_pointer(m_acceptor)));
  }
}

void TcpServer::newConnection(int sockfd, const InetAddress& peerAddr)
{
  m_loop->assertInLoopThread();
  EventLoop* ioLoop = m_threadPool->getNextLoop();
  char buf[64];
  snprintf(buf, sizeof buf, "-%s#%d", m_ipPort.c_str(), m_nextConnId);
  ++m_nextConnId;
  string connName = m_name + buf;

  LOG_INFO << "TcpServer::newConnection [" << m_name
           << "] - new connection [" << connName
           << "] from " << peerAddr.toIpPort();
  InetAddress localAddr(sockets::getLocalAddr(sockfd));
  // FIXME poll with zero timeout to double confirm the new connection
  // FIXME use make_shared if necessary
  TcpConnectionPtr conn(new TcpConnection(ioLoop,
                                          connName,
                                          sockfd,
                                          localAddr,
                                          peerAddr));
  m_connections[connName] = conn;
  conn->setConnectionCallback(m_connectionCallback);
  conn->setMessageCallback(m_messageCallback);
  conn->setWriteCompleteCallback(m_writeCompleteCallback);
  conn->setCloseCallback(
      std::bind(&TcpServer::removeConnection, this, _1)); // FIXME: unsafe
  ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));
}

void TcpServer::removeConnection(const TcpConnectionPtr& conn)
{
  // FIXME: unsafe
  m_loop->runInLoop(std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn)
{
  m_loop->assertInLoopThread();
  LOG_INFO << "TcpServer::removeConnectionInLoop [" << m_name
           << "] - connection " << conn->name();
  size_t n = m_connections.erase(conn->name());
  (void)n;
  assert(n == 1);
  EventLoop* ioLoop = conn->getLoop();
  ioLoop->queueInLoop(
      std::bind(&TcpConnection::connectDestroyed, conn));
}

