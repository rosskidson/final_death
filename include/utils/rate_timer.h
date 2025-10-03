#pragma once

#include <chrono>
#include <iostream>
#include <thread>

#include "chrono_helpers.h"

namespace platformer {

class RateTimer {
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
        std::cout << "Now (us):          " << ToUs(now) << std::endl;
        std::cout << "End of frame (us): " << ToUs(frame_end_) << std::endl;
        std::cout << "Time overrun (us): " << ToUs(now - frame_end_) << std::endl;
      }

      // If overrun occurs, reset so the loop doesn't have to play catch up.
      Reset();
    }

    std::this_thread::sleep_until(frame_end_);
    frame_end_ += single_frame_;
  }

 private:
  TimePoint frame_end_;
  Duration single_frame_;
};

}  // namespace platformer