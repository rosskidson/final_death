#pragma once

#include <vector>
#include <string>
#include <memory>

namespace platformer {

  class CommandInterface {
    public:
    CommandInterface();
    virtual ~CommandInterface();

    virtual int GetMinNumberArgument() = 0;
    virtual std::string GetHelp() = 0;

  };

  class CommandList : public CommandInterface {
    public:
    CommandList();

    private:
    std::vector<std::unique_ptr<CommandInterface>> sub_commands_;
  };

  class Command : public CommandInterface {
    public:
    Command();

    private:

  };
}