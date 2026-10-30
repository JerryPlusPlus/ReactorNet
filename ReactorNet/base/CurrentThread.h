
#ifndef REACTORNET_BASE_CURRENTTHREAD_H
#define REACTORNET_BASE_CURRENTTHREAD_H

#include "ReactorNet/base/Types.h"

namespace reactor
{
    namespace CurrentThread
    {
     /*
     thread_local 是 C++11 引入的关键字，用于声明“线程局部变量”：每个线程都有自己独立的一份变量，并且这份变量可以在线程内持续保存状态。
     */
      extern thread_local int t_cachedTid;
      extern thread_local char t_tidString[32];
      extern thread_local int t_tidStringLength;
      /*t_threadName 是一个线程标签，它可以：
        •	标识线程是谁（初始名称）
        •	标记线程当前处于什么阶段（finished/crashed）
        •	暴露给操作系统和日志系统供调试和监控使用
      */
      extern thread_local const char* t_threadName;
      void cacheTid();

      inline int tid()
      {
        // Cache the Linux TID once per thread.
        if (t_cachedTid == 0)
        {
          cacheTid();
        }
        return t_cachedTid;
      }

      inline const char* tidString() // for logging
      {
        return t_tidString;
      }

      inline int tidStringLength() // for logging
      {
        return t_tidStringLength;
      }

      inline const char* name()
      {
        return t_threadName;
      }

      bool isMainThread();

      void sleepUsec(int64_t usec);  // for testing
      // 获取当前线程的函数调用栈，便于定位异常
      string stackTrace(bool demangle = true);


    }  // namespace CurrentThread
}  // namespace reactor

#endif  // REACTORNET_BASE_CURRENTTHREAD_H
