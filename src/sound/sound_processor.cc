#include "sound_processor.h"

namespace platformer {

void SoundProcessor::ProcessAnimationEvents(const std::vector<AnimationEvent>& events) const {
  // TODO:: We need to be able to check what weapon the player has to play the right sound.
  for (const auto& event : events) {
    if (event.event_name == "PlayerShoot" ) {
      sound_player_->PlaySample("shotgun_fire", false);
    } else if (event.event_name == "ReloadShotgun") {
      sound_player_->PlaySample("shotgun_reload", false);
    }
  }
}

}  // namespace platformer