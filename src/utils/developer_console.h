#pragma once

#include <utils/parameter_server.h>

#include <memory>
#include <string>

namespace platformer {

void PrintConsoleWelcome();

void DeveloperConsole(const std::string& sCommand,
                      std::shared_ptr<ParameterServer>& parameter_server);

}  // namespace platformer