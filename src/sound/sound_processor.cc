#include "sound/sound_processor.h"

namespace platformer {
  
void SoundProcessor::ProcessAnimationEvents(const std::vector<AnimationEvent>& events) const {
  for (const auto& event : events) {
    if (event.event_name == "ShootShotgun") {
      sound_player_->PlaySample("shotgun_fire", false);
    } else if (event.event_name == "ReloadShotgun") {
      sound_player_->PlaySample("shotgun_reload", false);
    }
  }
}


}