#include "utils/developer_console.h"

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "utils/parameter_server.h"

namespace platformer {

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
  std::cout << " param " << std::endl << std::endl;
}

// TODO:: clean up this mess!!! Do a class structure to handle nested commands, usage etc.
void DeveloperConsole(const std::string& sCommand,
                      std::shared_ptr<ParameterServer>& parameter_server) {
  std::cout << std::endl;
  const auto split_string = split(sCommand);
  if (split_string.empty()) {
    std::cout << "No command entered." << std::endl;
  }
  if (split_string[0] == "param") {
    if (split_string.size() == 1) {
      std::cout << "Sub commands:" << std::endl << std::endl;
      std::cout << "  list" << std::endl;
      std::cout << "  set" << std::endl;
      std::cout << "  get" << std::endl;
      std::cout << "  info" << std::endl << std::endl;
    }
    if (split_string[1] == "list") {
      int max_param_key_length = 0;
      for (const auto& key : parameter_server->ListParameterKeys()) {
        max_param_key_length = std::max(max_param_key_length, static_cast<int>(key.size()));
      }
      for (const auto& key : parameter_server->ListParameterKeys()) {
        std::cout << key;
        for (int i = 0; i < max_param_key_length + 3 - key.length(); ++i) {
          std::cout << " ";
        }
        // TODO:: type erasure.
        std::cout << parameter_server->GetParameter<double>(key) << std::endl;
      }
      std::cout << std::endl;
    }
    if (split_string[1] == "set") {
      if (split_string.size() < 4) {
        std::cout << "Usage: " << std::endl << std::endl;
        std::cout << "param set <parameter> <value>" << std::endl;
        std::cout << "e.g. > param set physics/gravity 10" << std::endl << std::endl;
      }
      const auto& param = split_string[2];
      // TODO:: stod throws std::invalid_argument
      const auto val = std::stod(split_string[3]);
      if (!parameter_server->ParameterExists(param)) {
        std::cout << "Parameter `" << param << "` doesn't exist" << std::endl << std::endl;
      }
      // TODO:: We either need a type erased version of set parameter,
      // Or we need to detect the type and call it correctly.
      parameter_server->SetParameter(param, val);
      std::cout << "Parameter set to " << val << "." << std::endl << std::endl;
    }
    if (split_string[1] == "get") {
      if (split_string.size() < 3) {
        std::cout << "Usage: " << std::endl;
        std::cout << "param get <parameter>" << std::endl;
        std::cout << "e.g. > param get physics/gravity" << std::endl << std::endl;
      }
      const auto& param = split_string[2];
      if (!parameter_server->ParameterExists(param)) {
        std::cout << "Parameter `" << param << "` doesn't exist" << std::endl << std::endl;
      }
      std::cout << parameter_server->GetParameter<double>(param) << std::endl << std::endl;
    }
    if (split_string[1] == "info") {
      if (split_string.size() < 3) {
        std::cout << "Usage: " << std::endl;
        std::cout << "param info <parameter>" << std::endl;
        std::cout << "e.g. > param info physics/gravity" << std::endl << std::endl;
      }
      const auto& param = split_string[2];
      if (!parameter_server->ParameterExists(param)) {
        std::cout << "Parameter `" << param << "` doesn't exist" << std::endl << std::endl;
      }
      std::cout << parameter_server->GetParameterInfo(param) << std::endl << std::endl;
    }
  }
}

}  // namespace platformer