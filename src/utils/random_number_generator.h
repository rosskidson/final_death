#pragma once

#include <memory>
#include <mutex>
#include <random>

namespace platformer {

class RandomNumberGenerator {
 public:
  enum class Mode { Deterministic, Hardware };

  RandomNumberGenerator(Mode mode = Mode::Deterministic, unsigned int seed = 42) {
    switch (mode) {
      case Mode::Deterministic:
        engine_ = std::make_unique<std::mt19937>(seed);
        break;
      case Mode::Hardware: {
        std::random_device rd;
        engine_ = std::make_unique<std::mt19937>(rd());
        break;
      }
    }
  }

  int RandomInt(int min, int max) const {
    std::lock_guard<std::mutex> lock{mutex_};
    std::uniform_int_distribution<int> dist(min, max);
    return dist(*engine_);
  }

  double RandomFloat(double min, double max) const {
    std::lock_guard<std::mutex> lock{mutex_};
    std::uniform_real_distribution<double> dist(min, max);
    return dist(*engine_);
  }

 private:
  std::unique_ptr<std::mt19937> engine_;
  mutable std::mutex mutex_;
};

}  // namespace platformer