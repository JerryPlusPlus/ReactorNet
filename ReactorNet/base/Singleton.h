// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef REACTORNET_BASE_SINGLETON_H
#define REACTORNET_BASE_SINGLETON_H

#include "ReactorNet/base/noncopyable.h"

#include <assert.h>
#include <mutex>
#include <cstdint>
#include <stdlib.h> // atexit

namespace reactor
{

namespace detail
{
// This doesn't detect inherited member functions!
// http://stackoverflow.com/questions/1966362/sfinae-to-check-for-inherited-member-functions
template<typename T>
struct has_no_destroy
{
  template <typename C> static char test(decltype(&C::no_destroy));
  template <typename C> static int32_t test(...);
  const static bool value = sizeof(test<T>(0)) == 1;
};
}  // namespace detail

template<typename T>
class Singleton : noncopyable
{
 public:
  Singleton() = delete;
  ~Singleton() = delete;

  static T& instance()
  {
    std::call_once(m_once, &Singleton::init);
    assert(m_value != nullptr);
    return *m_value;
  }

 private:
  static void init()
  {
    m_value = new T();
    if (!detail::has_no_destroy<T>::value)
    {
      ::atexit(destroy);
    }
  }

  static void destroy()
  {
    static_assert(sizeof(T) > 0, "T must be complete");

    delete m_value;
    m_value = nullptr;
  }

 private:
  static std::once_flag m_once;
  static T*             m_value;
};

template<typename T>
std::once_flag Singleton<T>::m_once;

template<typename T>
T* Singleton<T>::m_value = nullptr;

}  // namespace reactor

#endif  // REACTORNET_BASE_SINGLETON_H
