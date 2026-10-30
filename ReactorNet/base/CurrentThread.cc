
#include "ReactorNet/base/CurrentThread.h"

#include <cxxabi.h>
#include <execinfo.h>
#include <cstdlib>
#include <memory>
#include <type_traits>
#include <sys/types.h>

namespace reactor
{
    namespace CurrentThread
    {
    thread_local int t_cachedTid = 0;
    thread_local char t_tidString[32];
    thread_local int t_tidStringLength = 6;
    thread_local const char* t_threadName = "unknown";

    static_assert(std::is_same<int, pid_t>::value, "pid_t should be int");

    string stackTrace(bool demangle)
    {
      void* frames[200];
      const int count = ::backtrace(frames, 200);
      std::unique_ptr<char*, decltype(&std::free)> symbols(
          ::backtrace_symbols(frames, count), &std::free);
      string result;
      if (!symbols) return result;
      for (int i = 1; i < count; ++i) {
        string line(symbols.get()[i]);
        const auto left = line.find('(');
        const auto right = line.find('+', left);
        if (demangle && left != string::npos && right != string::npos) {
          int status = 0;
          const string mangled = line.substr(left + 1, right - left - 1);
          std::unique_ptr<char, decltype(&std::free)> name(
              abi::__cxa_demangle(mangled.c_str(), nullptr, nullptr, &status), &std::free);
          if (status == 0 && name) line.replace(left + 1, right - left - 1, name.get());
        }
        result += line + '\n';
      }
      return result;
    }

    }  // namespace CurrentThread
}  // namespace reactor
