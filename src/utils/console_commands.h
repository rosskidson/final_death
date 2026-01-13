#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace platformer {

class CommandInterface {
 public:
  CommandInterface(std::string command_name) : command_name_(std::move(command_name)) {}
  virtual ~CommandInterface() = default;

  virtual std::string GetHelp() const = 0;
  virtual bool ProcessInput(std::vector<std::string> arguments) = 0;
  virtual int GetMinNumberArguments() const = 0;

  bool ParseInput(const std::string& input);
  [[nodiscard]] const std::string& CommandName() const { return command_name_; }

 private:
  std::string command_name_;
};

class CommandList : public CommandInterface {
 public:
  CommandList(std::string command_name, std::vector<std::unique_ptr<CommandInterface>> sub_commands)
      : CommandInterface(std::move(command_name)), sub_commands_(std::move(sub_commands)) {}

  std::string GetHelp() const override;
  bool ProcessInput(std::vector<std::string> arguments) override;
  int GetMinNumberArguments() const override { return 1; }

  std::string GetSubCommandsFormatted() const;

 private:
  std::vector<std::unique_ptr<CommandInterface>> sub_commands_;
};

using CallbackFn = std::function<bool(std::vector<std::string>)>;

class Command : public CommandInterface {
 public:
  Command(std::string command_name, int min_num_arguments, std::string help, CallbackFn callback)
      : CommandInterface(std::move(command_name)),
        min_num_arguments_{min_num_arguments},
        help_{std::move(help)},
        callback_{std::move(callback)} {}

  std::string GetHelp() const override { return help_; };
  bool ProcessInput(std::vector<std::string> arguments) override;
  int GetMinNumberArguments() const override { return min_num_arguments_; }

 private:
  int min_num_arguments_;
  std::string help_;
  CallbackFn callback_;
};
}  // namespace platformer