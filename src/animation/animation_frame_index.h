#pragma once

#include <cstdint>
#include <ostream>
#include <stdexcept>

namespace platformer {

// Wrapper for an int
// Like an optional, but with two non valid states:
// - Not Initialized
// - Expired (The animation has ended)
enum class AnimationFrameState : uint8_t { Uninitialized, Valid, Expired };

class AnimationFrameIndex {
 public:
  class FrameIndexAccessError : public std::runtime_error {
   public:
    explicit FrameIndexAccessError(const std::string& msg) : std::runtime_error(msg) {}
  };

  AnimationFrameIndex() = default;
  AnimationFrameIndex(AnimationFrameState state) : state_{state} {}
  AnimationFrameIndex(int index) : state_{AnimationFrameState::Valid}, index_{index} {}

  [[nodiscard]] static std::string ToString(AnimationFrameState state) {
    switch (state) {
      case AnimationFrameState::Uninitialized:
        return "uninitialized";
      case AnimationFrameState::Valid:
        return "valid";
      case AnimationFrameState::Expired:
        return "expired";
      default:
        return "unknown state";
    }
  }

  [[nodiscard]] bool Valid() const { return state_ == AnimationFrameState::Valid; }
  [[nodiscard]] bool Uninitialized() const { return state_ == AnimationFrameState::Uninitialized; }
  [[nodiscard]] bool Expired() const { return state_ == AnimationFrameState::Expired; }
  [[nodiscard]] AnimationFrameState GetState() const { return state_; }

  [[nodiscard]] int Index() const {
    if (!this->Valid()) {
      throw FrameIndexAccessError("No index available, current state is " + ToString(this->state_));
    }
    return index_;
  }

  [[nodiscard]] int operator*() const { return index_; }
  [[nodiscard]] AnimationFrameState State() const { return state_; }

  void SetIndex(int index) {
    state_ = AnimationFrameState::Valid;
    index_ = index;
  }

  AnimationFrameIndex& operator=(int index) {
    SetIndex(index);
    return *this;
  }

  void SetExpired() { state_ = AnimationFrameState::Expired; }

  void Reset() {
    state_ = AnimationFrameState::Uninitialized;
    index_ = 0;
  }

  bool operator==(const AnimationFrameIndex& other) const noexcept {
    if (state_ != other.state_) {
      return false;
    }
    if (state_ == AnimationFrameState::Valid) {
      return index_ == other.index_;
    }
    return true;
  }

  bool operator!=(const AnimationFrameIndex& other) const noexcept { return !(*this == other); }

 private:
  AnimationFrameState state_{AnimationFrameState::Uninitialized};
  int index_{0};
};

inline std::ostream& operator<<(std::ostream& os, const AnimationFrameIndex& frame_idx) {
  if (frame_idx.Valid()) {
    os << frame_idx.Index();
    return os;
  }
  os << AnimationFrameIndex::ToString(frame_idx.GetState());
  return os;
}

}  // namespace platformer