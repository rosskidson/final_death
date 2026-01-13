#include "developer_console.h"

#include <array>
#include <iostream>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "parameter_server.h"
#include "utils/console_commands.h"

namespace platformer {

constexpr std::array<std::string_view, 2> kCommands = {"param", "respawn"};

std::vector<std::string> split(const std::string& input) {
  std::istringstream iss(input);
  std::vector<std::string> result;
  std::string word;
  while (iss >> word) {  // extracts words separated by whitespace
    result.push_back(word);
  }
  return result;
}

void PrintConsoleWelcome() {
  std::cout << "#######################################" << std::endl;
  std::cout << "   D E V E L O P E R    C O N S O L E   " << std::endl;
  std::cout << "#######################################" << std::endl << std::endl;
  std::cout << " Available commands: " << std::endl;
  // for (const auto& command : kCommands) {
  //   std::cout << " " << command << std::endl;
  // }
  std::cout << std::endl;
}

std::optional<ConsoleEvent> DeveloperConsole(const std::string& sCommand,
                                             std::shared_ptr<ParameterServer>& parameter_server) {
  std::vector<std::unique_ptr<CommandInterface>> param_commands;
  {
    std::stringstream ss;
    ss << "Usage: " << std::endl;
    ss << "param get <parameter>" << std::endl;
    ss << "e.g. > param get physics/gravity" << std::endl;
    CallbackFn callback = [parameter_server](std::vector<std::string> arguments) -> bool {
      const auto& param = arguments[0];
      if (!parameter_server->ParameterExists(param)) {
        std::cout << "Parameter `" << param << "` doesn't exist" << std::endl << std::endl;
        return false;
      }
      std::cout << parameter_server->GetParameter<double>(param) << std::endl << std::endl;
      return true;
    };
    param_commands.emplace_back(std::make_unique<Command>("get", 1, ss.str(), callback));
  }
  {
    std::stringstream ss;
    ss << "Usage: " << std::endl << std::endl;
    ss << "param set <parameter> <value>" << std::endl;
    ss << "e.g. > param set physics/gravity 10" << std::endl;
    CallbackFn callback = [parameter_server](std::vector<std::string> arguments) -> bool {
      const auto& param = arguments[0];
      // TODO(BT-09):: stod throws std::invalid_argument
      const auto val = std::stod(arguments[1]);
      if (!parameter_server->ParameterExists(param)) {
        std::cout << "Parameter `" << param << "` doesn't exist" << std::endl << std::endl;
        return false;
      }
      // TODO(BT-01):: We either need a type erased version of set parameter,
      // Or we need to detect the type and call it correctly.
      parameter_server->SetParameter(param, val);
      std::cout << "Parameter set to " << val << "." << std::endl << std::endl;
      return true;
    };
    param_commands.emplace_back(std::make_unique<Command>("set", 2, ss.str(), callback));
  }
  {
    CallbackFn callback = [parameter_server](std::vector<std::string> /*arguments*/) -> bool {
      int max_param_key_length = 0;
      for (const auto& key : parameter_server->ListParameterKeys()) {
        max_param_key_length = std::max(max_param_key_length, static_cast<int>(key.size()));
      }
      for (const auto& key : parameter_server->ListParameterKeys()) {
        std::cout << key;
        for (int i = 0; i < max_param_key_length + 3 - key.length(); ++i) {
          std::cout << " ";
        }
        // TODO(BT-01):: type erasure.
        std::cout << parameter_server->GetParameter<double>(key) << std::endl;
      }
      std::cout << std::endl;
      return true;
    };
    param_commands.emplace_back(std::make_unique<Command>("list", 0, "", callback));
  }
  {
    std::stringstream ss;
    ss << "Usage: " << std::endl;
    ss << "param info <parameter>" << std::endl;
    ss << "e.g. > param info physics/gravity" << std::endl;
    CallbackFn callback = [parameter_server](std::vector<std::string> arguments) -> bool {
      const auto& param = arguments[0];
      if (!parameter_server->ParameterExists(param)) {
        std::cout << "Parameter `" << param << "` doesn't exist" << std::endl << std::endl;
        return {};
      }
      std::cout << std::endl << parameter_server->GetParameterInfo(param) << std::endl;
      return true;
    };
    param_commands.emplace_back(std::make_unique<Command>("info", 1, ss.str(), callback));
  }
  auto param = std::make_unique<CommandList>("param", std::move(param_commands));

  std::vector<std::unique_ptr<CommandInterface>> top_level_commands;
  top_level_commands.emplace_back(std::move(param));

  std::optional<ConsoleEvent> console_event;
  {
    CallbackFn callback = [&console_event](std::vector<std::string> /*arguments*/) -> bool {
      console_event = ConsoleEvent{"respawn"};
      return true;
    };
    top_level_commands.emplace_back(std::make_unique<Command>("respawn", 0, "", callback));
  }

  auto top_level = std::make_unique<CommandList>("top_level", std::move(top_level_commands));
  top_level->ParseInput(sCommand);
  return console_event;
}

}  // namespace platformer