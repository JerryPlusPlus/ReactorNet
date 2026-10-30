// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "ReactorNet/log/AsyncLogging.h"
#include "ReactorNet/log/LogFile.h"
#include "ReactorNet/base/Timestamp.h"

#include <stdio.h>
#include <chrono>
#include <stdexcept>

using namespace reactor;

AsyncLogging::AsyncLogging(const string& basename,
                           off_t rollSize,
                           int flushInterval)
  : m_flushInterval(flushInterval),
    m_running(false),
    m_basename(basename),
    m_rollSize(rollSize),
    m_thread([this] { threadFunc(); }, "Logging"),
    m_mutex(),
    m_cond(),
    m_currentBuffer(new Buffer),
    m_nextBuffer(new Buffer),
    m_buffers()
{
  m_currentBuffer->bzero();
  m_nextBuffer->bzero();
  m_buffers.reserve(16);
}

void AsyncLogging::start()
{
  std::unique_lock<std::mutex> lock(m_mutex);
  if (m_thread.started()) throw std::logic_error("AsyncLogging can only start once");
  m_running = true;
  try { m_thread.start(); }
  catch (...) { m_running = false; throw; }
  m_readyCondition.wait(lock, [this] { return m_ready; });
}

void AsyncLogging::stop()
{
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_running.exchange(false)) return;
    m_cond.notify_one();
  }
  m_thread.join();
}

void AsyncLogging::append(const char* logline, int len)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  if (!m_running || len <= 0) return;
  if (m_currentBuffer->avail() > len)
  {
    m_currentBuffer->append(logline, len);
  }
  else
  {
    m_buffers.push_back(std::move(m_currentBuffer));

    if (m_nextBuffer)
    {
      m_currentBuffer = std::move(m_nextBuffer);
    }
    else
    {
      m_currentBuffer.reset(new Buffer); // Rarely happens
    }
    m_currentBuffer->append(logline, len);
    m_cond.notify_one();
  }
}

void AsyncLogging::threadFunc()
{
  assert(m_running == true);
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_ready = true;
    m_readyCondition.notify_one();
  }
  LogFile output(m_basename, m_rollSize, false);
  BufferPtr newBuffer1(new Buffer);
  BufferPtr newBuffer2(new Buffer);
  newBuffer1->bzero();
  newBuffer2->bzero();
  BufferVector buffersToWrite;
  buffersToWrite.reserve(16);
  for (;;)
  {
    assert(newBuffer1 && newBuffer1->length() == 0);
    assert(newBuffer2 && newBuffer2->length() == 0);
    assert(buffersToWrite.empty());

    {
      std::unique_lock<std::mutex> lock(m_mutex);
      m_cond.wait_for(lock, std::chrono::seconds(m_flushInterval),
                      [this] { return !m_running || !m_buffers.empty(); });
      if (!m_running && m_buffers.empty() && m_currentBuffer->length() == 0) break;
      m_buffers.push_back(std::move(m_currentBuffer));
      m_currentBuffer = std::move(newBuffer1);
      buffersToWrite.swap(m_buffers);
      if (!m_nextBuffer)
      {
        m_nextBuffer = std::move(newBuffer2);
      }
    }

    assert(!buffersToWrite.empty());

    if (buffersToWrite.size() > 25)
    {
      char buf[256];
      snprintf(buf, sizeof buf, "Dropped log messages at %s, %zd larger buffers\n",
               Timestamp::now().toFormattedString().c_str(),
               buffersToWrite.size()-2);
      fputs(buf, stderr);
      output.append(buf, static_cast<int>(strlen(buf)));
      buffersToWrite.erase(buffersToWrite.begin()+2, buffersToWrite.end());
    }

    for (const auto& buffer : buffersToWrite)
    {
      // FIXME: use unbuffered stdio FILE ? or use ::writev ?
      output.append(buffer->data(), buffer->length());
    }

    if (buffersToWrite.size() > 2)
    {
      // drop non-bzero-ed buffers, avoid trashing
      buffersToWrite.resize(2);
    }

    if (!newBuffer1)
    {
      assert(!buffersToWrite.empty());
      newBuffer1 = std::move(buffersToWrite.back());
      buffersToWrite.pop_back();
      newBuffer1->reset();
    }

    if (!newBuffer2)
    {
      assert(!buffersToWrite.empty());
      newBuffer2 = std::move(buffersToWrite.back());
      buffersToWrite.pop_back();
      newBuffer2->reset();
    }

    buffersToWrite.clear();
    output.flush();
  }
  output.flush();
}

