#pragma once

#include "ReactorNet/base/noncopyable.h"
#include "ReactorNet/net/Buffer.h"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#include <zlib.h>

namespace reactor
{
namespace net
{

// input is zlib compressed data, output uncompressed data
// FIXME: finish this
class ZlibInputStream : noncopyable
{
 public:
  explicit ZlibInputStream(Buffer* output)
    : m_output(output),
      m_zerror(Z_OK)
  {
    memZero(&m_zstream, sizeof m_zstream);
    m_zerror = inflateInit(&m_zstream);
  }

  ~ZlibInputStream()
  {
    finish();
  }

  bool write(StringPiece buf);
  bool write(Buffer* input);
  bool finish();
    // inflateEnd(&m_zstream);

 private:
  int decompress(int flush);

  Buffer* m_output;
  z_stream m_zstream;
  int m_zerror;
};

// input is uncompressed data, output zlib compressed data
class ZlibOutputStream : noncopyable
{
 public:
  explicit ZlibOutputStream(Buffer* output)
    : m_output(output),
      m_zerror(Z_OK),
      m_bufferSize(1024)
  {
    memZero(&m_zstream, sizeof m_zstream);
    m_zerror = deflateInit(&m_zstream, Z_DEFAULT_COMPRESSION);
  }

  ~ZlibOutputStream()
  {
    finish();
  }

  // Return last error message or nullptr if no error.
  const char* zlibErrorMessage() const { return m_zstream.msg; }

  int zlibErrorCode() const { return m_zerror; }
  int64_t inputBytes() const { return m_zstream.total_in; }
  int64_t outputBytes() const { return m_zstream.total_out; }
  int internalOutputBufferSize() const { return m_bufferSize; }

  bool write(StringPiece buf)
  {
    if (m_zerror != Z_OK)
      return false;

    assert(m_zstream.next_in == nullptr && m_zstream.avail_in == 0);
    void* in = const_cast<char*>(buf.data());
    m_zstream.next_in = static_cast<Bytef*>(in);
    m_zstream.avail_in = buf.size();
    while (m_zstream.avail_in > 0 && m_zerror == Z_OK)
    {
      m_zerror = compress(Z_NO_FLUSH);
    }
    if (m_zstream.avail_in == 0)
    {
      assert(static_cast<const void*>(m_zstream.next_in) == buf.end());
      m_zstream.next_in = nullptr;
    }
    return m_zerror == Z_OK;
  }

  // compress input as much as possible, not guarantee consuming all data.
  bool write(Buffer* input)
  {
    if (m_zerror != Z_OK)
      return false;

    void* in = const_cast<char*>(input->peek());
    m_zstream.next_in = static_cast<Bytef*>(in);
    m_zstream.avail_in = static_cast<int>(input->readableBytes());
    if (m_zstream.avail_in > 0 && m_zerror == Z_OK)
    {
      m_zerror = compress(Z_NO_FLUSH);
    }
    input->retrieve(input->readableBytes() - m_zstream.avail_in);
    return m_zerror == Z_OK;
  }

  bool finish()
  {
    if (m_zerror != Z_OK)
      return false;

    while (m_zerror == Z_OK)
    {
      m_zerror = compress(Z_FINISH);
    }
    m_zerror = deflateEnd(&m_zstream);
    bool ok = m_zerror == Z_OK;
    m_zerror = Z_STREAM_END;
    return ok;
  }

 private:
  int compress(int flush)
  {
    m_output->ensureWritableBytes(m_bufferSize);
    m_zstream.next_out = reinterpret_cast<Bytef*>(m_output->beginWrite());
    m_zstream.avail_out = static_cast<int>(m_output->writableBytes());
    int error = ::deflate(&m_zstream, flush);
    m_output->hasWritten(m_output->writableBytes() - m_zstream.avail_out);
    if (m_output->writableBytes() == 0 && m_bufferSize < 65536)
    {
      m_bufferSize *= 2;
    }
    return error;
  }

  Buffer* m_output;
  z_stream m_zstream;
  int m_zerror;
  int m_bufferSize;
};

}  // namespace net
}  // namespace reactor
