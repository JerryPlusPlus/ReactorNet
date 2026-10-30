// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "ReactorNet/base/Exception.h"
#include "ReactorNet/base/CurrentThread.h"

namespace reactor
{

Exception::Exception(string msg)
  : m_message(std::move(msg)),
    m_stack(CurrentThread::stackTrace(/*demangle=*/false))
{
}

}  // namespace reactor
