#pragma once

#include <chrono>
namespace platformer {

// NOTE:: If I ever turn this engine into a library, consider make this header internal.
using Clock = std::chrono::steady_clock;
using TimePoint = std::chrono::time_point<Clock>;

}  // namespace platformer