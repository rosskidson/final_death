#include "utils/console_commands.h"

#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "utils/check.h"

namespace platformer {

namespace {

std::vector<std::string> split(const std::string& input) {
  std::istringstream iss(input);
  std::vector<std::string> result;
  std::string word;
  while (iss >> word) {  // extracts words separated by whitespace
    result.push_back(word);
  }
  return result;
}
}  // namespace

bool CommandInterface::ParseInput(const std::string& input) {
  auto split_string = split(input);
  if (split_string.size() < this->GetMinNumberArguments()) {
    std::cout << std::endl << GetHelp() << std::endl;
    // std::cout << "Not enough arguments for command '" << command_name_ << "'. Expected "
    //           << this->GetMinNumberArguments() << ", got " << split_string.size() << std::endl;
    return false;
  }
  return this->ProcessInput(std::move(split_string));
}

std::string CommandList::GetSubCommandsFormatted() const {
  std::stringstream ss;
  for (const auto& sub_command : sub_commands_) {
    ss << sub_command->CommandName() << std::endl;
  }
  return ss.str();
}

std::string CommandList::GetHelp() const {
  std::stringstream ss;
  ss << "Sub Commands:" << std::endl;
  ss << GetSubCommandsFormatted();
  return ss.str();
}

bool CommandList::ProcessInput(std::vector<std::string> arguments) {
  if (arguments.empty()) {
    std::cout << std::endl << GetHelp() << std::endl;
    return true;
  }

  for (const auto& sub_command : sub_commands_) {
    if (sub_command->CommandName() == arguments[0]) {
      return sub_command->ProcessInput(
          std::vector<std::string>(arguments.begin() + 1, arguments.end()));
    }
  }
  std::cout << "Command '" << arguments[0] << "' not found. Available commands:" << std::endl
            << GetSubCommandsFormatted();
  return false;
}

bool Command::ProcessInput(std::vector<std::string> arguments) {
  if (arguments.size() < GetMinNumberArguments()) {
    std::cout << std::endl << GetHelp() << std::endl;
    return false;
  }
  return callback_(std::move(arguments));
}

// Command::Command() {}

}  // namespace platformer