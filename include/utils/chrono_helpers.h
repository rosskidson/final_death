#pragma once

#include <chrono>

namespace platformer {

using Clock = std::chrono::steady_clock;
using TimePoint = Clock::time_point;
using Duration = Clock::duration;

constexpr int64_t ToUs(const TimePoint& time_point) {
  return std::chrono::duration_cast<std::chrono::microseconds>(time_point.time_since_epoch())
      .count();
}

constexpr int64_t ToUs(const Duration& duration) {
  return std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
}

constexpr int64_t ToMs(const TimePoint& time_point) {
  return std::chrono::duration_cast<std::chrono::milliseconds>(time_point.time_since_epoch())
      .count();
}

constexpr int64_t ToMs(const Duration& duration) {
  return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

}  // namespace platformer