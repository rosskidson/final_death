#pragma once

#include <memory>
#include <string>
#include <vector>

namespace platformer {

class CommandInterface {
 public:
  CommandInterface(std::string command_name) : command_name_(std::move(command_name)) {}
  virtual ~CommandInterface() = default;

  virtual int GetMinNumberArguments() = 0;
  virtual std::string GetHelp() = 0;
  virtual bool ProcessInput(std::vector<std::string> arguments) = 0;

  bool ParseInput(const std::string& input);
  [[nodiscard]] const std::string& CommandName() const { return command_name_; }

 private:
  std::string command_name_;
};

class CommandList : public CommandInterface {
 public:
  CommandList(std::string command_name, std::vector<std::unique_ptr<CommandInterface>> sub_commands)
      : CommandInterface(std::move(command_name)), sub_commands_(std::move(sub_commands)) {}

  int GetMinNumberArguments() override { return 1; }
  std::string GetHelp() override;
  bool ProcessInput(std::vector<std::string> arguments) override;

  std::string GetSubCommandsFormatted();

 private:
  std::vector<std::unique_ptr<CommandInterface>> sub_commands_;
};

class Command : public CommandInterface {
 public:
  Command(std::string command_name) : CommandInterface(std::move(command_name)) {}

  int GetMinNumberArguments() override { return 0; }
  std::string GetHelp() override { return "not implemented"; };
  bool ProcessInput(std::vector<std::string> arguments) override {return true;}

 private:
};
}  // namespace platformer