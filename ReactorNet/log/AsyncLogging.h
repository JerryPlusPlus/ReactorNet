// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef REACTORNET_BASE_ASYNCLOGGING_H
#define REACTORNET_BASE_ASYNCLOGGING_H

#include <mutex>
#include "ReactorNet/base/noncopyable.h"
#include "ReactorNet/base/Thread.h"
#include "ReactorNet/log/LogStream.h"

#include <atomic>
#include <condition_variable>
#include <vector>

namespace reactor
{

class AsyncLogging : noncopyable
{
 public:

  AsyncLogging(const string& basename,
               off_t rollSize,
               int flushInterval = 3);

  ~AsyncLogging() { stop(); }
  void append(const char* logline, int len);
  void start();
  void stop();

 private:

  void threadFunc();

  using Buffer = reactor::detail::FixedBuffer<reactor::detail::kLargeBuffer>;
  using BufferVector = std::vector<std::unique_ptr<Buffer>>;
  using BufferPtr = BufferVector::value_type;

  const int m_flushInterval;
  std::atomic<bool> m_running;
  const string m_basename;
  const off_t m_rollSize;
  reactor::Thread m_thread;
  std::condition_variable m_readyCondition;
  bool m_ready{false};
  std::mutex m_mutex;
  std::condition_variable m_cond;
  BufferPtr m_currentBuffer;
  BufferPtr m_nextBuffer;
  BufferVector m_buffers;
};

}  // namespace reactor

#endif  // REACTORNET_BASE_ASYNCLOGGING_H
