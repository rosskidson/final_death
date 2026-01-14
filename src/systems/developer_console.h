#pragma once

#include <registry.h>

#include <memory>
#include <optional>
#include <string>

#include "utils/console_commands.h"
#include "utils/parameter_server.h"

namespace platformer {

// Developer console may act directly on the registry, so in terms of ECS it is considered a
// 'system'

class DeveloperConsole {
 public:
  DeveloperConsole(std::shared_ptr<ParameterServer> parameter_server,
                   std::shared_ptr<Registry> registry);

  bool ProcessCommandLine(const std::string& command);

  void PrintConsoleWelcome();

 private:
  std::shared_ptr<ParameterServer> parameter_server_;
  std::shared_ptr<Registry> registry_;
  bool console_opened_before_;

  std::unique_ptr<CommandList> top_level_command_list_;
};

}  // namespace platformer