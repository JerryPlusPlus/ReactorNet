// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef REACTORNET_BASE_TIMESTAMP_H
#define REACTORNET_BASE_TIMESTAMP_H

#include "ReactorNet/base/copyable.h"
#include "ReactorNet/base/Types.h"

#include <ctime>

namespace reactor
{

///
/// Time stamp in UTC, in microseconds resolution.
///
/// This class is immutable.
/// It's recommended to pass it by value, since it's passed in register on x64.
///
class Timestamp : public reactor::copyable
{
 public:
  ///
  /// Constucts an invalid Timestamp.
  ///
  Timestamp()
    : m_microSecondsSinceEpoch(0)
  {
  }

  ///
  /// Constucts a Timestamp at specific time
  ///
  /// @param microSecondsSinceEpoch
  explicit Timestamp(int64_t microSecondsSinceEpochArg)
    : m_microSecondsSinceEpoch(microSecondsSinceEpochArg)
  {
  }

  void swap(Timestamp& that)
  {
    std::swap(m_microSecondsSinceEpoch, that.m_microSecondsSinceEpoch);
  }

  // default copy/assignment/dtor are Okay

  string toString() const;
  string toFormattedString(bool showMicroseconds = true) const;

  bool valid() const { return m_microSecondsSinceEpoch > 0; }

  // for internal usage.
  int64_t microSecondsSinceEpoch() const { return m_microSecondsSinceEpoch; }
  time_t secondsSinceEpoch() const
  { return static_cast<time_t>(m_microSecondsSinceEpoch / kMicroSecondsPerSecond); }

  ///
  /// Get time of now.
  ///
  static Timestamp now();
  static Timestamp invalid()
  {
    return Timestamp();
  }

  static Timestamp fromUnixTime(time_t t)
  {
    return fromUnixTime(t, 0);
  }

  static Timestamp fromUnixTime(time_t t, int microseconds)
  {
    return Timestamp(static_cast<int64_t>(t) * kMicroSecondsPerSecond + microseconds);
  }

  static const int kMicroSecondsPerSecond = 1000 * 1000;

 private:
  int64_t m_microSecondsSinceEpoch;
};

inline bool operator<(Timestamp lhs, Timestamp rhs)
{
  return lhs.microSecondsSinceEpoch() < rhs.microSecondsSinceEpoch();
}

inline bool operator==(Timestamp lhs, Timestamp rhs)
{
  return lhs.microSecondsSinceEpoch() == rhs.microSecondsSinceEpoch();
}

///
/// Gets time difference of two timestamps, result in seconds.
///
/// @param high, low
/// @return (high-low) in seconds
/// @c double has 52-bit precision, enough for one-microsecond
/// resolution for next 100 years.
inline bool operator!=(Timestamp a, Timestamp b) { return !(a == b); }
inline bool operator>(Timestamp a, Timestamp b) { return b < a; }
inline bool operator<=(Timestamp a, Timestamp b) { return !(b < a); }
inline bool operator>=(Timestamp a, Timestamp b) { return !(a < b); }

inline double timeDifference(Timestamp high, Timestamp low)
{
  int64_t diff = high.microSecondsSinceEpoch() - low.microSecondsSinceEpoch();
  return static_cast<double>(diff) / Timestamp::kMicroSecondsPerSecond;
}

///
/// Add @c seconds to given timestamp.
///
/// @return timestamp+seconds as Timestamp
///
inline Timestamp addTime(Timestamp timestamp, double seconds)
{
  int64_t delta = static_cast<int64_t>(seconds * Timestamp::kMicroSecondsPerSecond);
  return Timestamp(timestamp.microSecondsSinceEpoch() + delta);
}

}  // namespace reactor

#endif  // REACTORNET_BASE_TIMESTAMP_H
