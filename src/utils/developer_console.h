#pragma once

#include <utils/parameter_server.h>

#include <memory>
#include <optional>
#include <string>

namespace platformer {

struct ConsoleEvent {
  std::string event;
};

void PrintConsoleWelcome();

[[nodiscard]] std::optional<ConsoleEvent> DeveloperConsole(
    const std::string& sCommand,
    std::shared_ptr<ParameterServer>& parameter_server);

}  // namespace platformer