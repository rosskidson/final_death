#pragma once

#include <deque>
#include <iostream>
#include <map>
#include <numeric>
#include <string>
#include "chrono_helpers.h"

namespace platformer {

class SimpleProfiler {
 public:
  SimpleProfiler() = default;

  void Reset() { last_measurement_ = Clock::now(); }

  void LogEvent(const std::string& key) {
    const auto delta = Clock::now() - last_measurement_;
    auto& timing_buffer = time_measurements_[key];
    timing_buffer.push_back(ToUs(delta));
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
  static uint64_t GetAverage(const std::deque<int64_t>& values) {
    if (values.empty()) {
      return 0;
    }
    return std::accumulate(values.begin(), values.end(), uint64_t{0}) / values.size();
  }

  static constexpr int kBufferSize = 100;
  std::map<std::string, std::deque<int64_t>> time_measurements_;
  TimePoint last_measurement_;
};

}  // namespace platformer