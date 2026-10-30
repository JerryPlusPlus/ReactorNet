// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef REACTORNET_BASE_EXCEPTION_H
#define REACTORNET_BASE_EXCEPTION_H

#include "ReactorNet/base/Types.h"
#include <exception>

namespace reactor
{

class Exception : public std::exception
{
 public:
  Exception(string what);
  ~Exception() noexcept override = default;

  // default copy-ctor and operator= are okay.

  const char* what() const noexcept override
  {
    return m_message.c_str();
  }

  const char* stackTrace() const noexcept
  {
    return m_stack.c_str();
  }

 private:
  string m_message;
  string m_stack;
};

}  // namespace reactor

#endif  // REACTORNET_BASE_EXCEPTION_H
