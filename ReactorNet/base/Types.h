#ifndef REACTORNET_BASE_TYPES_H
#define REACTORNET_BASE_TYPES_H

#include <stdint.h>
#include <string.h>  // memset
#include <string>

#include <cassert>
#include <utility>
#include <type_traits>


namespace reactor
{

    using std::string;

    inline void memZero(void* p, size_t n)
    {
      memset(p, 0, n);
    }


    template<typename To, typename From>
    inline To implicit_cast(From const &f)
    {
      return f;
    }

    // use like this: down_cast<T*>(foo); so we only accept pointers
    template<typename To, typename From>     
    inline To down_cast(From* f)                     
    {

      if (false)
      {
        implicit_cast<From*, To>(0);
      }

    #if !defined(NDEBUG) && !defined(GOOGLE_PROTOBUF_NO_RTTI)
      assert(f == nullptr || dynamic_cast<To>(f) != nullptr);  // RTTI: debug mode only!
    #endif
      return static_cast<To>(f);
    }

}  // namespace reactor

#endif  // REACTORNET_BASE_TYPES_H
