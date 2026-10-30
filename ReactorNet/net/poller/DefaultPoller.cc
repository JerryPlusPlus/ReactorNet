// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "ReactorNet/net/Poller.h"
#include "ReactorNet/net/poller/PollPoller.h"
#include "ReactorNet/net/poller/EPollPoller.h"

#include <stdlib.h>

using namespace reactor::net;

Poller* Poller::newDefaultPoller(EventLoop* loop)
{
  if (::getenv("REACTORNET_USE_POLL") || ::getenv("MUDUO_USE_POLL"))
  {
    return new PollPoller(loop);
  }
  else
  {
    return new EPollPoller(loop);
  }
}
