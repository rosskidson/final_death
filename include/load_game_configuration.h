#pragma once

#include <optional>

#include "common_types/game_configuration.h"

namespace platformer {

std::optional<GameConfiguration> LoadGameConfiguration(const std::string& path);

}