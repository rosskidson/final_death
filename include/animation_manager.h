#pragma once

#include <map>
#include <memory>

#include "animated_sprite.h"
#include "player_state.h"
#include "registry.h"
#include "registry_helpers.h"

namespace platformer {

enum class CommonState : uint8_t { Idle, Walk, Jump, Shoot, Other };

enum class ProtagonistState : uint8_t {
  Idle,
  Walk,
  Jump,
  Shoot,
  Crouch,
  CrouchShot,
  AimUp,
  UpShot,
};

class StateInterface {
 public:
  virtual ~StateInterface() = default;
  [[nodiscard]] virtual CommonState GetCommonState() const { return common_state_; }
  void SetState(CommonState state) { common_state_ = state; }

 private:
  CommonState common_state_;
};

class ProtagonistStateAccess : public StateInterface {
 public:
  ProtagonistStateAccess() = default;
  ~ProtagonistStateAccess() override = default;

  [[nodiscard]] ProtagonistState GetState() const { return state_; }
  void SetState(ProtagonistState state) { state_ = state; }

  [[nodiscard]] CommonState GetCommonState() const override {
    switch (state_) {
      case ProtagonistState::Idle:
        return CommonState::Idle;
      case ProtagonistState::Walk:
        return CommonState::Walk;
      case ProtagonistState::Jump:
        return CommonState::Jump;
      case ProtagonistState::Shoot:
        return CommonState::Shoot;
      case ProtagonistState::Crouch:
      case ProtagonistState::CrouchShot:
      case ProtagonistState::AimUp:
      case ProtagonistState::UpShot:
        return CommonState::Other;
    }
  }

 private:
  ProtagonistState state_{ProtagonistState::Idle};
};

struct CommonStateComponent {
  std::shared_ptr<StateInterface> state_interface;
};

struct ProtagonistStateComponent {
  std::shared_ptr<ProtagonistStateAccess> protagonist_state;
};

enum class Actor : uint8_t { Protagonist, Enemy, BadassEnemy, Boss };

class SpriteKey {
 public:
  SpriteKey() = default;
  SpriteKey(Actor actor, PlayerState state) : actor_(actor), state_(state) {}

  bool operator<(const SpriteKey& other) const {
    if (actor_ != other.actor_) {
      return actor_ < other.actor_;
    }
    return state_ < other.state_;
  }

 private:
  Actor actor_;
  PlayerState state_;
};

class AnimationManager {
 public:
  AnimationManager(std::shared_ptr<Registry> registry) : registry_{std::move(registry)} {}

  void AddAnimation(AnimatedSprite sprite, Actor actor, PlayerState state);

  // [[nodiscard]] const AnimatedSprite& GetAnimation(PlayerState action) const;
  // [[nodiscard]] AnimatedSprite& GetAnimation(PlayerState action);

  // [[nodiscard]] const AnimatedSprite& GetActiveAnimation() const;
  // [[nodiscard]] AnimatedSprite& GetActiveAnimation();

  // void SwapAnimation(PlayerState action);

  void Update(EntityId id);

  // void StartAction(Action action);
  // void AddAction(Action action);
  // void EndAction(Action action);

  [[nodiscard]] const olc::Sprite* GetSprite(EntityId id) const;

 private:
  std::map<SpriteKey, AnimatedSprite> animated_sprites_;
  std::shared_ptr<Registry> registry_;
};
}  // namespace platformer