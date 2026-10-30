// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#pragma once

#include "ReactorNet/base/StringPiece.h"
#include "ReactorNet/base/noncopyable.h"
#include <zlib.h>

namespace reactor
{

class GzipFile : noncopyable
{
 public:
  GzipFile(GzipFile&& rhs) noexcept
    : m_file(rhs.m_file)
  {
    rhs.m_file = nullptr;
  }

  ~GzipFile()
  {
    if (m_file)
    {
      ::gzclose(m_file);
    }
  }

  GzipFile& operator=(GzipFile&& rhs) noexcept
  {
    swap(rhs);
    return *this;
  }

  bool valid() const { return m_file != nullptr; }
  void swap(GzipFile& rhs) { std::swap(m_file, rhs.m_file); }
#if ZLIB_VERNUM >= 0x1240
  bool setBuffer(int size) { return ::gzbuffer(m_file, size) == 0; }
#endif

  // return the number of uncompressed bytes actually read, 0 for eof, -1 for error
  int read(void* buf, int len) { return ::gzread(m_file, buf, len); }

  // return the number of uncompressed bytes actually written
  int write(StringPiece buf) { return ::gzwrite(m_file, buf.data(), buf.size()); }

  // number of uncompressed bytes
  off_t tell() const { return ::gztell(m_file); }

#if ZLIB_VERNUM >= 0x1240
  // number of compressed bytes
  off_t offset() const { return ::gzoffset(m_file); }
#endif

  // int flush(int f) { return ::gzflush(m_file, f); }

  static GzipFile openForRead(StringArg filename)
  {
    return GzipFile(::gzopen(filename.c_str(), "rbe"));
  }

  static GzipFile openForAppend(StringArg filename)
  {
    return GzipFile(::gzopen(filename.c_str(), "abe"));
  }

  static GzipFile openForWriteExclusive(StringArg filename)
  {
    return GzipFile(::gzopen(filename.c_str(), "wbxe"));
  }

  static GzipFile openForWriteTruncate(StringArg filename)
  {
    return GzipFile(::gzopen(filename.c_str(), "wbe"));
  }

 private:
  explicit GzipFile(gzFile file)
    : m_file(file)
  {
  }

  gzFile m_file;
};

}  // namespace reactor
