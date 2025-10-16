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

