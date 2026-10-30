// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef REACTORNET_BASE_LOGGING_H
#define REACTORNET_BASE_LOGGING_H

#include "ReactorNet/log/LogStream.h"
#include "ReactorNet/base/Timestamp.h"
#include <atomic>

namespace reactor
{

class TimeZone;

class Logger
{
 public:
  enum LogLevel
  {
    TRACE,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL,
    NUM_LOG_LEVELS,
  };

  // compile time calculation of basename of source file
  class SourceFile
  {
   public:
    template<int N>
    SourceFile(const char (&arr)[N])
      : m_data(arr),
        m_size(N-1)
    {
      const char* slash = strrchr(m_data, '/'); // builtin function
      if (slash)
      {
        m_data = slash + 1;
        m_size -= static_cast<int>(m_data - arr);
      }
    }

    explicit SourceFile(const char* filename)
      : m_data(filename)
    {
      const char* slash = strrchr(filename, '/');
      if (slash)
      {
        m_data = slash + 1;
      }
      m_size = static_cast<int>(strlen(m_data));
    }

    const char* m_data;
    int m_size;
  };

  Logger(SourceFile file, int line);
  Logger(SourceFile file, int line, LogLevel level);
  Logger(SourceFile file, int line, LogLevel level, const char* func);
  Logger(SourceFile file, int line, bool toAbort);
  ~Logger();

  LogStream& stream() { return m_impl.m_stream; }

  static LogLevel logLevel();
  static void setLogLevel(LogLevel level);

  using OutputFunc = void (*)(const char* msg, int len);
  using FlushFunc = void (*)();
  static void setOutput(OutputFunc);
  static void setFlush(FlushFunc);
  static void setTimeZone(const TimeZone& tz);

 private:

class Impl
{
 public:
  using LogLevel = Logger::LogLevel;
  Impl(LogLevel level, int old_errno, const SourceFile& file, int line);
  void formatTime();
  void finish();

  Timestamp m_time;
  LogStream m_stream;
  LogLevel m_level;
  int m_line;
  SourceFile m_basename;
};

  Impl m_impl;

};

extern std::atomic<Logger::LogLevel> g_logLevel;

inline Logger::LogLevel Logger::logLevel()
{
  return g_logLevel.load(std::memory_order_relaxed);
}

//
// CAUTION: do not write:
//
// if (good)
//   LOG_INFO << "Good news";
// else
//   LOG_WARN << "Bad news";
//
// this expends to
//
// if (good)
//   if (logging_INFO)
//     logInfoStream << "Good news";
//   else
//     logWarnStream << "Bad news";
//
#define LOG_TRACE if (reactor::Logger::logLevel() <= reactor::Logger::TRACE) \
  reactor::Logger(__FILE__, __LINE__, reactor::Logger::TRACE, __func__).stream()
#define LOG_DEBUG if (reactor::Logger::logLevel() <= reactor::Logger::DEBUG) \
  reactor::Logger(__FILE__, __LINE__, reactor::Logger::DEBUG, __func__).stream()
#define LOG_INFO if (reactor::Logger::logLevel() <= reactor::Logger::INFO) \
  reactor::Logger(__FILE__, __LINE__).stream()
#define LOG_WARN reactor::Logger(__FILE__, __LINE__, reactor::Logger::WARN).stream()
#define LOG_ERROR reactor::Logger(__FILE__, __LINE__, reactor::Logger::ERROR).stream()
#define LOG_FATAL reactor::Logger(__FILE__, __LINE__, reactor::Logger::FATAL).stream()
#define LOG_SYSERR reactor::Logger(__FILE__, __LINE__, false).stream()
#define LOG_SYSFATAL reactor::Logger(__FILE__, __LINE__, true).stream()

const char* strerror_tl(int savedErrno);

// Taken from glog/logging.h
//
// Check that the input is non nullptr.  This very useful in constructor
// initializer lists.

#define CHECK_NOTNULL(val) \
  ::reactor::CheckNotNull(__FILE__, __LINE__, "'" #val "' Must be non nullptr", (val))

// A small helper for CHECK_NOTNULL().
template <typename T>
T* CheckNotNull(Logger::SourceFile file, int line, const char *names, T* ptr)
{
  if (ptr == nullptr)
  {
   Logger(file, line, Logger::FATAL).stream() << names;
  }
  return ptr;
}

}  // namespace reactor

#endif  // REACTORNET_BASE_LOGGING_H
