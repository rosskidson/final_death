#pragma once

#include "animation/animation_event.h"
#include "registry.h"
#include "sound_player.h"

namespace platformer {
  public:
  class SoundSystem {
    SoundSystem(std::shared_ptr<Registry> registry, std::shared_ptr<SoundPlayer> sound_player):
    registry_{registry},
    sound_player_{sound_player} {}
  private:
    std::shared_ptr<Registry> registry_;
    std::shared_ptr<SoundPlayer> sound_player_;
  };
}