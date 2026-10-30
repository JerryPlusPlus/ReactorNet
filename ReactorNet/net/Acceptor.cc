// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "ReactorNet/net/Acceptor.h"

#include "ReactorNet/log/Logging.h"
#include "ReactorNet/net/EventLoop.h"
#include "ReactorNet/net/InetAddress.h"
#include "ReactorNet/net/SocketsOps.h"

#include <errno.h>
#include <fcntl.h>
//#include <sys/types.h>
//#include <sys/stat.h>
#include <unistd.h>

using namespace reactor;
using namespace reactor::net;

Acceptor::Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reuseport)
  : m_loop(loop),
    m_acceptSocket(sockets::createNonblockingOrDie(listenAddr.family())),
    m_acceptChannel(loop, m_acceptSocket.fd()),
    m_listenning(false),
    m_idleFd(::open("/dev/null", O_RDONLY | O_CLOEXEC))
{
  assert(m_idleFd >= 0);
  m_acceptSocket.setReuseAddr(true);
  m_acceptSocket.setReusePort(reuseport);
  m_acceptSocket.bindAddress(listenAddr);
  m_acceptChannel.setReadCallback(
      std::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor()
{
  m_acceptChannel.disableAll();
  m_acceptChannel.remove();
  ::close(m_idleFd);
}

void Acceptor::listen()
{
  m_loop->assertInLoopThread();
  m_listenning = true;
  m_acceptSocket.listen();
  m_acceptChannel.enableReading();
}

void Acceptor::handleRead()
{
  m_loop->assertInLoopThread();
  InetAddress peerAddr;
  //FIXME loop until no more
  int connfd = m_acceptSocket.accept(&peerAddr);
  if (connfd >= 0)
  {
    // string hostport = peerAddr.toIpPort();
    // LOG_TRACE << "Accepts of " << hostport;
    if (m_newConnectionCallback)
    {
      m_newConnectionCallback(connfd, peerAddr);
    }
    else
    {
      sockets::close(connfd);
    }
  }
  else
  {
    LOG_SYSERR << "in Acceptor::handleRead";
    // Read the section named "The special problem of
    // accept()ing when you can't" in libev's doc.
    // By Marc Lehmann, author of libev.
    if (errno == EMFILE)
    {
      ::close(m_idleFd);
      m_idleFd = ::accept(m_acceptSocket.fd(), nullptr, nullptr);
      ::close(m_idleFd);
      m_idleFd = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
    }
  }
}

