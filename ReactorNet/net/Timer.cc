// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "ReactorNet/net/Timer.h"

using namespace reactor;
using namespace reactor::net;

std::atomic<int64_t> Timer::m_numCreated{0};

void Timer::restart(Timestamp now)
{
  if (m_repeat)
  {
    m_expiration = addTime(now, m_interval);
  }
  else
  {
    m_expiration = Timestamp::invalid();
  }
}
