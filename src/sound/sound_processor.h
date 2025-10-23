#pragma once

#include "animation/animation_event.h"
#include "sound_player.h"

#include <vector>

namespace platformer {
  class SoundProcessor {
  public:
    SoundProcessor(std::shared_ptr<const SoundPlayer> sound_player):
    sound_player_{sound_player} {}

    void ProcessAnimationEvents(const std::vector<AnimationEvent>& events)const;

  private:
    std::shared_ptr<const SoundPlayer> sound_player_;
  };
}