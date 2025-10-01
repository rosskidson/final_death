#pragma once

#include <chrono>
#include <iostream>
#include <thread>

class RateTimer {
  using Clock = std::chrono::steady_clock;
  using TimePoint = Clock::time_point;
  using Duration = Clock::duration;

 public:
  explicit RateTimer(double rate) {
    std::chrono::duration<double> period(1. / rate);
    single_frame_ = std::chrono::duration_cast<Duration>(period);
    frame_end_ = Clock::now();
  }

  void Reset() { frame_end_ = Clock::now(); }

  void Sleep(const bool debug) {
    const auto now = Clock::now();
    if (now > frame_end_) {
      if (debug) {
        std::cerr << "Frame timer has overrun. " << std::endl;
      }
      if (debug) {
        std::cout << "Now (us):          " << GetUs(now) << std::endl;
        std::cout << "End of frame (us): " << GetUs(frame_end_) << std::endl;
        std::cout << "Time overrun (us): " << GetUs(now - frame_end_) << std::endl;
      }

      // If overrun occurs, reset so the loop doesn't have to play catch up.
      Reset();
      frame_end_ += single_frame_;
      return;
    }

    #ifdef _WIN32
    // Windows has terrible clock precision. 
    // Sleep until 2ms before the deadline and then busy wait the rest
    std::this_thread::sleep_until(frame_end_ - std::chrono::milliseconds(2));
    while(Clock::now() < frame_end_);
    #else
    std::this_thread::sleep_until(frame_end_ );
    #endif

    frame_end_ += single_frame_;
  }

 private:
  static uint64_t GetUs(const TimePoint& time_point) {
    return std::chrono::duration_cast<std::chrono::microseconds>(time_point.time_since_epoch())
        .count() % 100000;
  }
  static uint64_t GetUs(const Duration& duration) {
    return std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
  }

  TimePoint frame_end_;
  Duration single_frame_;
};
