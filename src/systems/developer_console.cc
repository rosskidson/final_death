#include "developer_console.h"

#include <array>
#include <iostream>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "utils/console_commands.h"
#include "utils/parameter_server.h"

namespace platformer {

constexpr std::array<std::string_view, 2> kCommands = {"param", "respawn"};

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

std::unique_ptr<CommandList> CreateParamCommandList(
    const std::shared_ptr<ParameterServer>& parameter_server) {
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
  return std::make_unique<CommandList>("param", std::move(param_commands));
}

std::unique_ptr<Command> CreateRespawnCommand(const std::shared_ptr<Registry>& registry) {
  CallbackFn callback = [registry](std::vector<std::string> /*arguments*/) -> bool {
    for (EntityId id : registry->GetView<StateComponent, PlayerComponent>()) {
      auto& entity = registry->GetComponent<StateComponent>(id);
      entity.state.SetState(State::Idle);
    }
    return true;
  };
  return std::make_unique<Command>("respawn", 0, "", std::move(callback));
}

std::unique_ptr<Command> CreateWeaponCommand(const std::shared_ptr<Registry>& registry) {
  CallbackFn callback = [registry](std::vector<std::string> arguments) -> bool {
    for (EntityId id : registry->GetView<PlayerComponent>()) {
      auto& state = registry->GetComponent<PlayerComponent>(id);
      if (arguments[0] == "shotgun") {
        state.weapon = Weapon::Shotgun;
      } else if (arguments[0] == "rifle") {
        state.weapon = Weapon::Rifle;
      } else if (arguments[0] == "next") {
        state.weapon = static_cast<Weapon>((static_cast<int>(state.weapon) + 1) %
                                           static_cast<int>(Weapon::SIZE));
      }
    }
    return true;
  };
  std::stringstream ss;
  ss << "Usage: " << std::endl << std::endl;
  ss << "  weapon <weapon_type>" << std::endl;
  ss << "  weapon next" << std::endl << std::endl;
  ss << "  weapons: " << std::endl;
  ss << "   shotgun" << std::endl;
  ss << "   rifle" << std::endl;
  return std::make_unique<Command>("weapon", 1, ss.str(), std::move(callback));
}

}  // namespace

DeveloperConsole::DeveloperConsole(std::shared_ptr<ParameterServer> parameter_server,
                                   std::shared_ptr<Registry> registry)
    : parameter_server_{std::move(parameter_server)},
      registry_{std::move(registry)},
      console_opened_before_{false} {
  std::vector<std::unique_ptr<CommandInterface>> top_level_commands;
  top_level_commands.emplace_back(CreateParamCommandList(parameter_server_));
  top_level_commands.emplace_back(CreateRespawnCommand(registry_));
  top_level_commands.emplace_back(CreateWeaponCommand(registry_));
  top_level_command_list_ =
      std::make_unique<CommandList>("top_level", std::move(top_level_commands));
}

void DeveloperConsole::PrintConsoleWelcome() {
  if (console_opened_before_) {
    return;
  }
  std::cout << "#######################################" << std::endl;
  std::cout << "   D E V E L O P E R    C O N S O L E   " << std::endl;
  std::cout << "#######################################" << std::endl << std::endl;
  std::cout << " Available commands: " << std::endl;
  std::cout << top_level_command_list_->GetSubCommandsFormatted() << std::endl;
  console_opened_before_ = true;
}

bool DeveloperConsole::ProcessCommandLine(const std::string& command) {
  return top_level_command_list_->ParseInput(command);
}

}  // namespace platformer