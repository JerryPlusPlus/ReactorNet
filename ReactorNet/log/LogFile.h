// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef REACTORNET_BASE_LOGFILE_H
#define REACTORNET_BASE_LOGFILE_H

#include <mutex>
#include "ReactorNet/base/noncopyable.h"
#include "ReactorNet/base/Types.h"

#include <memory>

namespace reactor
{

namespace FileUtil
{
class AppendFile;
}

class LogFile : noncopyable
{
 public:
  LogFile(const string& basename,
          off_t rollSize,
          bool threadSafe = true,
          int flushInterval = 3,
          int checkEveryN = 1024);
  ~LogFile();

  void append(const char* logline, int len);
  void flush();
  bool rollFile();

 private:
  void append_unlocked(const char* logline, int len);

  static string getLogFileName(const string& basename, time_t* now);

  const string m_basename;
  const off_t m_rollSize;
  const int m_flushInterval;
  const int m_checkEveryN;

  int m_count;

  std::unique_ptr<std::mutex> m_mutex;
  time_t m_startOfPeriod;
  time_t m_lastRoll;
  time_t m_lastFlush;
  std::unique_ptr<FileUtil::AppendFile> m_file;

  const static int kRollPerSeconds_ = 60*60*24;
};

}  // namespace reactor
#endif  // REACTORNET_BASE_LOGFILE_H
