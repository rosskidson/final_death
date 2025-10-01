#pragma once

#include <chrono>
#include <deque>
#include <iostream>
#include <map>
#include <numeric>
#include <string>

class SimpleProfiler {
  using Clock = std::chrono::steady_clock;
  using TimePoint = Clock::time_point;
  using Duration = Clock::duration;

 public:
  SimpleProfiler() = default;

  void Reset() { last_measurement_ = Clock::now(); }

  void LogEvent(const std::string& key) {
    const auto delta = Clock::now() - last_measurement_;
    auto& timing_buffer = time_measurements_[key];
    timing_buffer.push_back(GetUs(delta));
    while (timing_buffer.size() > kBufferSize) {
      timing_buffer.pop_front();
    }
    last_measurement_ = Clock::now();
  }

  void PrintTimings() {
    for (const auto& [key, timings] : time_measurements_) {
      std::cout << key << ": " << GetAverage(timings) << "us" << std::endl;
    }
  }

 private:
  static uint64_t GetUs(const Duration& duration) {
    return std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
  }

  static uint64_t GetAverage(const std::deque<uint64_t>& values) {
    if (values.empty()) {
      return 0;
    }
    uint64_t total{};
    // TODO:: std::accumulate
    for (const auto& val : values) {
      total += val;
    }
    return total / values.size();
  }

  static constexpr int kBufferSize = 100;
  std::map<std::string, std::deque<uint64_t>> time_measurements_;
  TimePoint last_measurement_;
};