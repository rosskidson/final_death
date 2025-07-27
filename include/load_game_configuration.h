#pragma once

#include <optional>

#include "game_configuration.h"

namespace platformer {

std::optional<GameConfiguration> LoadGameConfiguration(const std::string& path);

}