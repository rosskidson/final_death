#include "animated_sprite.h"

#include <utils/check.h>
#include <utils/game_clock.h>

#include <filesystem>
#include <memory>
#include <optional>

#include "nlohmann/json.hpp"
#include "utils/chrono_helpers.h"
#include "utils/logging.h"

namespace platformer {

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace {

// The metadata format uses an index instead of just a list of frames, ¯\_(ツ)_/¯
std::vector<std::string> GenerateIndexLookup(const std::string& sprite_base_name,
                                             const int size,
                                             const int start_frame_idx,
                                             const int end_frame_idx) {
  std::vector<std::string> index;
  auto actual_end_frame_idx = end_frame_idx == -1 ? size : end_frame_idx + 1;
  index.reserve(size);
  for (int i = start_frame_idx; i < actual_end_frame_idx; ++i) {
    index.push_back(sprite_base_name + " " + std::to_string(i) + ".aseprite");
  }
  return index;
}

}  // namespace

std::optional<AnimatedSprite> AnimatedSprite::CreateAnimatedSprite(
    const std::filesystem::path& sprite_sheet_path,
    bool loops,
    int start_frame_idx,
    int end_frame_idx,
    int intro_frames,
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
  animated_sprite.intro_frames_ = intro_frames;
  int frame_count = static_cast<int>(sprite_meta["frames"].size());
  if (frame_count < 1) {
    LOG_ERROR("Sprite has zero frames.");
    return std::nullopt;
  }
  if (start_frame_idx < 0 || start_frame_idx >= frame_count) {
    LOG_ERROR("Invalid start frame idx " << start_frame_idx << " for sprite with " << frame_count
                                         << " frames.");
    return std::nullopt;
  }
  if (end_frame_idx > 0 && (end_frame_idx < start_frame_idx || end_frame_idx >= frame_count)) {
    LOG_ERROR("Invalid end frame idx " << end_frame_idx << " for sprite with " << frame_count
                                       << " frames.");
    return std::nullopt;
  }
  if (end_frame_idx != -1) {
    frame_count = end_frame_idx - start_frame_idx + 1;
  }
  std::optional<int> width;
  std::optional<int> height;
  for (const std::string& index : GenerateIndexLookup(metadata_path.stem().string(), frame_count,
                                                      start_frame_idx, end_frame_idx)) {
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
  animated_sprite.signals_to_emit_.resize(animated_sprite.frames_.size());
  animated_sprite.signals_to_emit_on_expiration_.emplace_back("AnimationEnded");

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

Sprite AnimatedSprite::GetFrame(const TimePoint start_time) const {
  const auto current_frame_idx = GetCurrentFrameIdx(start_time);
  if (current_frame_idx == -1) {
    return {frames_.back().get(), draw_offset_x, draw_offset_y};
  }
  return {frames_.at(current_frame_idx).get(), draw_offset_x, draw_offset_y};
}

int AnimatedSprite::GetTotalAnimationTimeMs() const { return frame_timing_lookup_.back(); }

int AnimatedSprite::GetCurrentFrameIdx(const TimePoint start_time) const {
  auto time_elapsed = ToMs(GameClock::NowGlobal() - start_time);

  // Check if it has expired first.
  if (!loops_ && time_elapsed >= frame_timing_lookup_.back()) {
    // TODO(BT-13): Use std::optional instead of -1
    return -1;
  }

  // Intro frames are frames that only play the first time through
  // After one iteration of animation, skip them.
  const auto& total_time = frame_timing_lookup_.back();
  if (intro_frames_ > -1 && time_elapsed > total_time) {
    time_elapsed -= total_time;
    const auto intro_time = frame_timing_lookup_.at(intro_frames_);
    const auto looping_duration = total_time - intro_time;
    time_elapsed = intro_time + (time_elapsed % looping_duration);
  } else {
    time_elapsed = time_elapsed % frame_timing_lookup_.back();
  }

  const auto itr =
      std::upper_bound(frame_timing_lookup_.begin(), frame_timing_lookup_.end(), time_elapsed);

  if (itr == frame_timing_lookup_.end()) {
    return static_cast<int>(frames_.size()) - 1;
  }
  return static_cast<int>(std::distance(frame_timing_lookup_.begin(), itr));
}

std::vector<std::string> AnimatedSprite::GetAnimationEvents(const TimePoint start_time,
                                                            int& last_animation_frame) const {
  // TODO(BT-04):: This doesn't check skipped frames
  const int frame_idx = GetCurrentFrameIdx(start_time);
  if (frame_idx == last_animation_frame) {
    return {};
  }
  last_animation_frame = frame_idx;
  if (frame_idx == -1) {
    return signals_to_emit_on_expiration_;
  }
  return signals_to_emit_.at(frame_idx);
}

void AnimatedSprite::AddEventSignal(const int frame_idx, const std::string& event_name) {
  RB_CHECK(frame_idx >= 0 && frame_idx < signals_to_emit_.size());
  signals_to_emit_[frame_idx].push_back(event_name);
}

void AnimatedSprite::AddExpiredEventSignal(const std::string& event_name) {
  signals_to_emit_on_expiration_.push_back(event_name);
}

}  // namespace platformer
