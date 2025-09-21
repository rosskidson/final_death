
#include "animated_sprite.h"

#include <utils/game_clock.h>

#include <cassert>
#include <filesystem>
#include <memory>
#include <optional>

#include "nlohmann/json.hpp"
#include "utils/logging.h"

namespace platformer {

namespace fs = std::filesystem;
using json = nlohmann::json;

// The metadata format uses an index instead of just a list of frames, ¯\_(ツ)_/¯
std::vector<std::string> GenerateIndexLookup(const std::string& sprite_base_name, int size) {
  std::vector<std::string> index;
  index.reserve(size);
  for (int i = 0; i < size; ++i) {
    index.push_back(sprite_base_name + " " + std::to_string(i) + ".aseprite");
  }
  return index;
}

std::optional<AnimatedSprite> AnimatedSprite::CreateAnimatedSprite(
    const std::filesystem::path& sprite_sheet_path,
    bool loops,
    bool forwards_backwards) try {
  // Check both spritesheet and metadata files are preset.
  if (!fs::exists(sprite_sheet_path)) {
    LOG_ERROR("Path '" << sprite_sheet_path << "' does not exist");
    return std::nullopt;
  }
  const auto metadata_path = fs::path(sprite_sheet_path).replace_extension(".json");
  if (!fs::exists(metadata_path)) {
    LOG_ERROR("Path '" << metadata_path << "' does not exist");
    return std::nullopt;
  }

  std::ifstream file_in(metadata_path);
  if (!file_in.is_open()) {
    LOG_ERROR("Failed to open file.");
    return std::nullopt;
  }
  const json sprite_meta = json::parse(file_in);

  olc::Sprite spritesheet_img{};
  if (spritesheet_img.LoadFromFile(sprite_sheet_path.string()) != olc::rcode::OK) {
    LOG_ERROR("Failed loading sprite ");
    return std::nullopt;
  }

  AnimatedSprite animated_sprite{};
  animated_sprite.loops_ = loops;
  animated_sprite.forwards_backwards_ = forwards_backwards;
  int frame_count = static_cast<int>(sprite_meta["frames"].size());
  if (frame_count < 1) {
    LOG_ERROR("Sprite has zero frames.");
    return std::nullopt;
  }
  std::optional<int> width;
  std::optional<int> height;
  for (const std::string& index : GenerateIndexLookup(metadata_path.stem(), frame_count)) {
    const auto& frame = sprite_meta["frames"][index];
    if (!width.has_value() || !height.has_value()) {
      width = frame["frame"]["w"];
      height = frame["frame"]["h"];
    }
    if (width != frame["frame"]["w"] || height != frame["frame"]["h"]) {
      LOG_ERROR("Detected frames of different size(s) with the sprite sheet. Starting size: "
                << *width << " x " << *height << ". New size: " << frame["frame"]["w"] << " x "
                << frame["frame"]["h"]);
      return std::nullopt;
    }
    animated_sprite.frame_timing_.emplace_back(frame["duration"]);
    auto& sprite =
        animated_sprite.frames_.emplace_back(std::make_unique<olc::Sprite>(*width, *height));
    const int x_start = frame["frame"]["x"];
    const int y_start = frame["frame"]["y"];
    if (x_start + *width > spritesheet_img.width || y_start + *height > spritesheet_img.height) {
      LOG_ERROR("Sprite dimensions out of bounds. Max x: "
                << x_start + *width << " Max y: " << y_start + *height
                << " Spritesheet dimensions: " << spritesheet_img.width << " x "
                << spritesheet_img.height);
      return std::nullopt;
    }
    for (int j = 0; j < *height; ++j) {
      for (int i = 0; i < *width; ++i) {
        sprite->SetPixel(i, j, spritesheet_img.GetPixel(x_start + i, y_start + j));
      }
    }
  }

  // Forwards backwards means we cycle from the last frame back to the first frame.
  // This isn't 1984, so instead of doing index gymnastics, just copy those frames after the last
  // frame.
  if (forwards_backwards) {
    for (int frame_idx = static_cast<int>(animated_sprite.frames_.size()) - 2; frame_idx > 0;
         frame_idx--) {
      animated_sprite.frames_.emplace_back(animated_sprite.frames_[frame_idx]->Duplicate());
      animated_sprite.frame_timing_.push_back(animated_sprite.frame_timing_[frame_idx]);
    }
  }
  animated_sprite.frame_timing_lookup_.push_back(animated_sprite.frame_timing_[0]);

  for (int i = 1; i < animated_sprite.frame_timing_.size(); ++i) {
    animated_sprite.frame_timing_lookup_.push_back(animated_sprite.frame_timing_[i] +
                                                   animated_sprite.frame_timing_lookup_[i - 1]);
  }
  return animated_sprite;
} catch (const json::parse_error& e) {
  LOG_ERROR("Failed parsing json metadata for sprite sheet '" << sprite_sheet_path
                                                              << "'. Error: " << e.what());
  return std::nullopt;
} catch (const json::type_error& e) {
  LOG_ERROR("Failed loading sprite sheet '"
            << sprite_sheet_path << "'. Metadata file has unexpected format. Error: " << e.what());
  return std::nullopt;
} catch (const std::exception& e) {
  LOG_ERROR("Failed loading sprite sheet '" << sprite_sheet_path
                                            << "'. Unknown error: " << e.what());
  return std::nullopt;
}

void AnimatedSprite::StartAnimation() { start_time_ = GameClock::NowGlobal(); }

bool AnimatedSprite::Expired() const {
  if (loops_) {
    return false;
  }
  const auto time_elapsed = GameClock::NowGlobal() - start_time_;
  const auto frame_count = frames_.size();
  return (time_elapsed.count() / 1000000) > frame_timing_lookup_.back();
}

const olc::Sprite* AnimatedSprite::GetFrame() const {
  if (Expired()) {
    return nullptr;
  }
  int time_elapsed = static_cast<int>((GameClock::NowGlobal() - start_time_).count() / 1e6);
  time_elapsed = time_elapsed % frame_timing_lookup_.back();

  const auto itr =
      std::upper_bound(frame_timing_lookup_.begin(), frame_timing_lookup_.end(), time_elapsed);

  if (itr == frame_timing_lookup_.end()) {
    return frames_.back().get();
  }
  const int frame_idx = std::distance(frame_timing_lookup_.begin(), itr);
  return frames_.at(frame_idx).get();
}

}  // namespace platformer