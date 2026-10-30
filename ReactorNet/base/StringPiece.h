// Taken from PCRE pcre_stringpiece.h
//
// Copyright (c) 2005, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Author: Sanjay Ghemawat
//
// A string like object that points into another piece of memory.
// Useful for providing an interface that allows clients to easily
// pass in either a "const char*" or a "string".
//
// Arghh!  I wish C++ literals were automatically of type "string".

#ifndef REACTORNET_BASE_STRINGPIECE_H
#define REACTORNET_BASE_STRINGPIECE_H

#include <string.h>
#include <ostream>    // for ostream forward-declaration

#include "ReactorNet/base/Types.h"

namespace reactor
{

// For passing C-style string argument to a function.
class StringArg // copyable
{
 public:
  StringArg(const char* str)
    : m_str(str)
  { }

  StringArg(const string& str)
    : m_str(str.c_str())
  { }

  const char* c_str() const { return m_str; }

 private:
  const char* m_str;
};

class StringPiece {
 private:
  const char*   m_ptr;
  int           m_length;

 public:
  // We provide non-explicit singleton constructors so users can pass
  // in a "const char*" or a "string" wherever a "StringPiece" is
  // expected.
  StringPiece()
    : m_ptr(""), m_length(0) { }
  StringPiece(const char* str)
    : m_ptr(str), m_length(static_cast<int>(strlen(m_ptr))) { }
  StringPiece(const unsigned char* str)
    : m_ptr(reinterpret_cast<const char*>(str)),
      m_length(static_cast<int>(strlen(m_ptr))) { }
  StringPiece(const string& str)
    : m_ptr(str.data()), m_length(static_cast<int>(str.size())) { }
  StringPiece(const char* offset, int len)
    : m_ptr(offset), m_length(len) { }

  // data() may return a pointer to a buffer with embedded NULs, and the
  // returned buffer may or may not be null terminated.  Therefore it is
  // typically a mistake to pass data() to a routine that expects a NUL
  // terminated string.  Use "as_string().c_str()" if you really need to do
  // this.  Or better yet, change your routine so it does not rely on NUL
  // termination.
  const char* data() const { return m_ptr; }
  int size() const { return m_length; }
  bool empty() const { return m_length == 0; }
  const char* begin() const { return m_ptr; }
  const char* end() const { return m_ptr + m_length; }

  void clear() { m_ptr = ""; m_length = 0; }
  void set(const char* buffer, int len) { m_ptr = buffer; m_length = len; }
  void set(const char* str) {
    m_ptr = str;
    m_length = static_cast<int>(strlen(str));
  }
  void set(const void* buffer, int len) {
    m_ptr = reinterpret_cast<const char*>(buffer);
    m_length = len;
  }

  char operator[](int i) const { return m_ptr[i]; }

  void remove_prefix(int n) {
    m_ptr += n;
    m_length -= n;
  }

  void remove_suffix(int n) {
    m_length -= n;
  }

  bool operator==(const StringPiece& x) const {
    return ((m_length == x.m_length) &&
            (memcmp(m_ptr, x.m_ptr, m_length) == 0));
  }
  bool operator!=(const StringPiece& x) const {
    return !(*this == x);
  }

#define STRINGPIECE_BINARY_PREDICATE(cmp,auxcmp)                             \
  bool operator cmp (const StringPiece& x) const {                           \
    int r = memcmp(m_ptr, x.m_ptr, m_length < x.m_length ? m_length : x.m_length); \
    return ((r auxcmp 0) || ((r == 0) && (m_length cmp x.m_length)));          \
  }
  STRINGPIECE_BINARY_PREDICATE(<,  <);
  STRINGPIECE_BINARY_PREDICATE(<=, <);
  STRINGPIECE_BINARY_PREDICATE(>=, >);
  STRINGPIECE_BINARY_PREDICATE(>,  >);
#undef STRINGPIECE_BINARY_PREDICATE

  int compare(const StringPiece& x) const {
    int r = memcmp(m_ptr, x.m_ptr, m_length < x.m_length ? m_length : x.m_length);
    if (r == 0) {
      if (m_length < x.m_length) r = -1;
      else if (m_length > x.m_length) r = +1;
    }
    return r;
  }

  string as_string() const {
    return string(data(), size());
  }

  void CopyToString(string* target) const {
    target->assign(m_ptr, m_length);
  }

  // Does "this" start with "x"
  bool starts_with(const StringPiece& x) const {
    return ((m_length >= x.m_length) && (memcmp(m_ptr, x.m_ptr, x.m_length) == 0));
  }
};

}  // namespace reactor

// ------------------------------------------------------------------
// Functions used to create STL containers that use StringPiece
//  Remember that a StringPiece's lifetime had better be less than
//  that of the underlying string or char*.  If it is not, then you
//  cannot safely store a StringPiece into an STL container
// ------------------------------------------------------------------



// allow StringPiece to be logged
inline std::ostream& operator<<(std::ostream& o, const reactor::StringPiece& piece) {
  return piece.empty() ? o : o.write(piece.data(), piece.size());
}

#endif  // REACTORNET_BASE_STRINGPIECE_H
