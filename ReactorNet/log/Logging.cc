// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "ReactorNet/log/Logging.h"

#include "ReactorNet/base/CurrentThread.h"
#include "ReactorNet/base/Timestamp.h"
#include "ReactorNet/base/TimeZone.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <sstream>

namespace reactor
{

/*
class LoggerImpl
{
 public:
  using LogLevel = Logger::LogLevel;
  LoggerImpl(LogLevel level, int old_errno, const char* file, int line);
  void finish();

  Timestamp m_time;
  LogStream m_stream;
  LogLevel m_level;
  int m_line;
  const char* m_fullname;
  const char* m_basename;
};
*/

thread_local char t_errnobuf[512];
thread_local char t_time[64];
thread_local time_t t_lastSecond;

const char* strerror_tl(int savedErrno)
{
  return strerror_r(savedErrno, t_errnobuf, sizeof t_errnobuf);
}

Logger::LogLevel initLogLevel()
{
  if (::getenv("MUDUO_LOG_TRACE"))
    return Logger::TRACE;
  else if (::getenv("MUDUO_LOG_DEBUG"))
    return Logger::DEBUG;
  else
    return Logger::INFO;
}

std::atomic<Logger::LogLevel> g_logLevel{initLogLevel()};

const char* LogLevelName[Logger::NUM_LOG_LEVELS] =
{
  "TRACE ",
  "DEBUG ",
  "INFO  ",
  "WARN  ",
  "ERROR ",
  "FATAL ",
};

// helper class for known string length at compile time
class T
{
 public:
  T(const char* str, unsigned len)
    :m_str(str),
     m_len(len)
  {
    assert(strlen(str) == m_len);
  }

  const char* m_str;
  const unsigned m_len;
};

inline LogStream& operator<<(LogStream& s, T v)
{
  s.append(v.m_str, v.m_len);
  return s;
}

inline LogStream& operator<<(LogStream& s, const Logger::SourceFile& v)
{
  s.append(v.m_data, v.m_size);
  return s;
}

void defaultOutput(const char* msg, int len)
{
  size_t n = fwrite(msg, 1, len, stdout);
  //FIXME check n
  (void)n;
}

void defaultFlush()
{
  fflush(stdout);
}

std::atomic<Logger::OutputFunc> g_output{defaultOutput};
std::atomic<Logger::FlushFunc> g_flush{defaultFlush};
TimeZone g_logTimeZone;

}  // namespace reactor

using namespace reactor;

Logger::Impl::Impl(LogLevel level, int savedErrno, const SourceFile& file, int line)
  : m_time(Timestamp::now()),
    m_stream(),
    m_level(level),
    m_line(line),
    m_basename(file)
{
  formatTime();
  CurrentThread::tid();
  m_stream << T(CurrentThread::tidString(), CurrentThread::tidStringLength());
  m_stream << T(LogLevelName[level], 6);
  if (savedErrno != 0)
  {
    m_stream << strerror_tl(savedErrno) << " (errno=" << savedErrno << ") ";
  }
}

void Logger::Impl::formatTime()
{
  int64_t microSecondsSinceEpoch = m_time.microSecondsSinceEpoch();
  time_t seconds = static_cast<time_t>(microSecondsSinceEpoch / Timestamp::kMicroSecondsPerSecond);
  int microseconds = static_cast<int>(microSecondsSinceEpoch % Timestamp::kMicroSecondsPerSecond);
  if (seconds != t_lastSecond)
  {
    t_lastSecond = seconds;
    struct tm tm_time;
    if (g_logTimeZone.valid())
    {
      tm_time = g_logTimeZone.toLocalTime(seconds);
    }
    else
    {
      ::gmtime_r(&seconds, &tm_time); // FIXME TimeZone::fromUtcTime
    }

    int len = snprintf(t_time, sizeof(t_time), "%4d%02d%02d %02d:%02d:%02d",
        tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
        tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
    assert(len == 17); (void)len;
  }

  if (g_logTimeZone.valid())
  {
    Fmt us(".%06d ", microseconds);
    assert(us.length() == 8);
    m_stream << T(t_time, 17) << T(us.data(), 8);
  }
  else
  {
    Fmt us(".%06dZ ", microseconds);
    assert(us.length() == 9);
    m_stream << T(t_time, 17) << T(us.data(), 9);
  }
}

void Logger::Impl::finish()
{
  m_stream << " - " << m_basename << ':' << m_line << '\n';
}

Logger::Logger(SourceFile file, int line)
  : m_impl(INFO, 0, file, line)
{
}

Logger::Logger(SourceFile file, int line, LogLevel level, const char* func)
  : m_impl(level, 0, file, line)
{
  m_impl.m_stream << func << ' ';
}

Logger::Logger(SourceFile file, int line, LogLevel level)
  : m_impl(level, 0, file, line)
{
}

Logger::Logger(SourceFile file, int line, bool toAbort)
  : m_impl(toAbort?FATAL:ERROR, errno, file, line)
{
}

Logger::~Logger()
{
  m_impl.finish();
  const LogStream::Buffer& buf(stream().buffer());
  g_output.load()(buf.data(), buf.length());
  if (m_impl.m_level == FATAL)
  {
    g_flush.load()();
    abort();
  }
}

void Logger::setLogLevel(Logger::LogLevel level)
{
  g_logLevel = level;
}

void Logger::setOutput(OutputFunc out)
{
  g_output = out;
}

void Logger::setFlush(FlushFunc flush)
{
  g_flush = flush;
}

void Logger::setTimeZone(const TimeZone& tz)
{
  g_logTimeZone = tz;
}
