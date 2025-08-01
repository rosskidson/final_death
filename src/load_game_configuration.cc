#include <olcPixelGameEngine.h>

#include <memory>
#include <optional>

#include "config.h"
#include "game_configuration.h"
#include "grid.h"
#include "nlohmann/json.hpp"
#include "tileset.h"
#include "utils/logging.h"

using json = nlohmann::json;

namespace platformer {

std::shared_ptr<TileSet> LoadTileSet(const json& tileset_json) {
  const int width = tileset_json["__cWid"];
  const int height = tileset_json["__cHei"];
  const int tile_size = tileset_json["tileGridSize"];
  const std::string identifier = tileset_json["identifier"];
  std::cout << "loading tileset " << identifier << std::endl;
  const int uid = tileset_json["uid"];
  auto tileset_ptr = std::make_shared<TileSet>(identifier, uid, width, height, tile_size);

  const std::string path_from_config = tileset_json["relPath"];
  const auto tile_path = std::filesystem::path(SOURCE_DIR) / path_from_config;
  std::cout << "path " << tile_path << std::endl;
  olc::Sprite tileset_img(tile_path.string());
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      auto tile = std::make_unique<olc::Sprite>(tile_size, tile_size);
      for (int j = 0; j < tile_size; ++j) {
        for (int i = 0; i < tile_size; ++i) {
          tile->SetPixel(i, j, tileset_img.GetPixel(x * tile_size + i, y * tile_size + j));
        }
      }
      tileset_ptr->SetTile(x, y, std::move(tile));
    }
  }
  return tileset_ptr;
}

Grid<Tile> LoadTileMap(const json& tilemap_json) {
  const int grid_size = tilemap_json["__gridSize"];
  const int width = tilemap_json["__cWid"];
  const int height = tilemap_json["__cHei"];
  Grid<Tile> tiles{width, height};
  //   for (int i = 0; i < width; ++i) {
  //     for (int j = 0; j < height; ++j) {
  //       tiles.SetTile(i, j, Tile{0, 100});
  //     }
  //   }
  for (const auto& tile : tilemap_json["gridTiles"]) {
    const int x = int(tile["px"][0]) / grid_size;
    const int y = int(tile["px"][1]) / grid_size;

    // Map is stored in a y+ down coordinate system. Invert y axis.
    tiles.SetTile(x, height - 1 - y, Tile{tile["f"], tile["t"]});

    int src_x = tile["src"][0];
    int src_y = tile["src"][1];
  }
  std::cout << "  Loaded " << tiles.GetHeight() * tiles.GetWidth() << " tiles " << std::endl;
  return tiles;
}

Grid<int> LoadIntGrid(const json& intgrid_json) {
  const int width = intgrid_json["__cWid"];
  const int height = intgrid_json["__cHei"];
  Grid<int> tiles(width, height);
  int counter = 0;
  for (const auto& tile : intgrid_json["intGridCsv"]) {
    const int y = counter / width;
    const int x = counter - (y * width);
    // Invert y.
    const int y_inv = height - 1 - y;
    tiles.SetTile(x, y_inv, tile);
    counter++;
  }
  std::cout << "  Loaded " << tiles.GetHeight() * tiles.GetWidth() << " tiles " << std::endl;
  return tiles;
}

std::optional<GameConfiguration> LoadGameConfiguration(const std::string& path) {
  std::ifstream file_in(path);
  if (!file_in.is_open()) {
    std::cerr << "Failed to open file.\n";
    return std::nullopt;
  }

  GameConfiguration config;
  const json ldtk = json::parse(file_in);

  for (const auto& tileset_json : ldtk["defs"]["tilesets"]) {
    const int uid = tileset_json["uid"];
    config.tilesets[uid] = LoadTileSet(tileset_json);
  }

  for (const auto& level_json : ldtk["levels"]) {
    Level level{};

    const std::string levelName = level_json["identifier"];
    std::cout << "Level: " << levelName << "\n";

    // Each level contains layerInstances (drawn in reverse order)
    if (!level_json.contains("layerInstances")) {
      continue;
    }
    for (const auto& layer_json : level_json["layerInstances"]) {
      const std::string layer_name = layer_json["__identifier"];
      const std::string layer_type = layer_json["__type"];
      std::cout << "  Layer: " << layer_name << " (" << layer_type << ")\n";

      if (layer_type == "Tiles") {
        level.tile_grid = LoadTileMap(layer_json);
        const int tileset_uid = layer_json["__tilesetDefUid"];
        if (!config.tilesets.count(tileset_uid)) {
          LOG_ERROR("Tileset uid " << tileset_uid << " not found in config.");
          exit(1);
        }
        level.level_tileset = config.tilesets[tileset_uid];
      }
      if (layer_type == "IntGrid") {
        level.property_grid = LoadIntGrid(layer_json);
      }
    }
    config.levels.push_back(std::move(level));
  }
  return config;
}

}  // namespace platformer
