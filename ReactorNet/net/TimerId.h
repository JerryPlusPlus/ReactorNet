// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is a public header file, it must only include public header files.

#ifndef REACTORNET_NET_TIMERID_H
#define REACTORNET_NET_TIMERID_H

#include "ReactorNet/base/copyable.h"
#include <cstdint>

namespace reactor
{
namespace net
{

class Timer;

///
/// An opaque identifier, for canceling Timer.
///
class TimerId : public reactor::copyable
{
 public:
  TimerId()
    : m_timer(nullptr),
      m_sequence(0)
  {
  }

  TimerId(Timer* timer, int64_t seq)
    : m_timer(timer),
      m_sequence(seq)
  {
  }

  // default copy-ctor, dtor and assignment are okay

  friend class TimerQueue;

 private:
  Timer* m_timer;
  int64_t m_sequence;
};

}  // namespace net
}  // namespace reactor

#endif  // REACTORNET_NET_TIMERID_H
