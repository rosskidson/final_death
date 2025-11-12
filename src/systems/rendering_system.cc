#include "rendering_system.h"

#include <algorithm>
#include <memory>

#include "animation/animated_sprite.h"
#include "common_types/basic_types.h"
#include "common_types/components.h"
#include "common_types/game_configuration.h"
#include "common_types/tileset.h"
#include "config.h"
#include "global_defs.h"
#include "utils/logging.h"
#include "utils/parameter_server.h"

namespace platformer {

constexpr double kFollowPlayerScreenRatioX = 0.4;
constexpr double kFollowPlayerScreenRatioY = 0.5;

constexpr double kDrawPlayerCollisions = 0.0;  // TODO(BT-01)::Bool

namespace {
olc::Sprite::Flip GetFlip(EntityId id, const Registry& registry) {
  if (!registry.HasComponent<FacingDirection>(id)) {
    return olc::Sprite::Flip::NONE;
  }
  // From olc:
  // 		enum Flip { NONE = 0, HORIZ = 1, VERT = 2 };
  // This is a bitmask.
  const auto facing = registry.GetComponent<FacingDirection>(id).facing;
  if(facing == Direction::LEFT) {
    return olc::Sprite::Flip::HORIZ;
  }
  if(facing == Direction::DOWN) {
    return olc::Sprite::Flip::VERT;
  }
  return olc::Sprite::Flip::NONE;
}

}  // namespace

RenderingSystem::RenderingSystem(olc::PixelGameEngine* engine_ptr,
                                 Level level,
                                 std::shared_ptr<ParameterServer> parameter_server,
                                 std::shared_ptr<AnimationManager> animation_manager,
                                 std::shared_ptr<Registry> registry)
    : engine_ptr_{engine_ptr},
      level_{std::move(level)},
      parameter_server_{std::move(parameter_server)},
      animation_manager_{std::move(animation_manager)},
      registry_{std::move(registry)} {
  parameter_server_->AddParameter(
      "rendering/follow.player.screen.ratio.x", kFollowPlayerScreenRatioX,
      "How far the player can walk towards the side of the screen before the camera follows, as a "
      "percentage of the screen size. The larger the ratio, the more centered the player will be "
      "on the screen.");
  parameter_server_->AddParameter(
      "rendering/follow.player.screen.ratio.y", kFollowPlayerScreenRatioY,
      "How far the player can walk towards the side of the screen before the camera follows, as a "
      "percentage of the screen size. The larger the ratio, the more centered the player will be "
      "on the screen.");

  parameter_server_->AddParameter("viz/draw.player.collisions", kDrawPlayerCollisions,
                                  "Visualize collisions of the player");

  const int grid_width = level_.tile_grid.GetWidth();
  const int grid_height = level_.tile_grid.GetHeight();

  tile_size_ = level_.level_tileset->GetTileSize();
  viewport_width_ = kScreenWidthPx / static_cast<double>(tile_size_);
  viewport_height_ = kScreenHeightPx / static_cast<double>(tile_size_);

  max_cam_postion_px_x_ = (grid_width * tile_size_) - kScreenWidthPx;
  max_cam_postion_px_y_ = (grid_height * tile_size_) - kScreenHeightPx;

  cam_position_px_x_ = 0;
  cam_position_px_y_ = 0;
}

void RenderingSystem::SetCameraPosition(const Vector2d& absolute_vec) {
  cam_position_px_x_ = static_cast<int>(absolute_vec.x * tile_size_);
  cam_position_px_y_ = static_cast<int>(absolute_vec.y * tile_size_);
  KeepCameraInBounds();
}

void RenderingSystem::MoveCamera(const Vector2d& relative_vec) {
  cam_position_px_x_ += static_cast<int>(relative_vec.x * tile_size_);
  cam_position_px_y_ += static_cast<int>(relative_vec.y * tile_size_);
  KeepCameraInBounds();
}

Vector2d RenderingSystem::GetCameraPosition() const {
  return Vector2d{static_cast<double>(cam_position_px_x_) / tile_size_,
                  static_cast<double>(cam_position_px_y_) / tile_size_};
}

void RenderingSystem::KeepPlayerInFrame(const EntityId player_id) {
  auto [position, collision_box] = registry_->GetComponents<Position, CollisionBox>(player_id);
  const auto screen_ratio_x =
      parameter_server_->GetParameter<double>("rendering/follow.player.screen.ratio.x");
  const auto screen_ratio_y =
      parameter_server_->GetParameter<double>("rendering/follow.player.screen.ratio.y");
  // const auto position = GetCameraPosition();
  const auto convert_to_px = [&](double val) -> int { return static_cast<int>(val * tile_size_); };
  const Vector2i player_pos_px{convert_to_px(position.x), convert_to_px(position.y)};
  // Note: The y is kept to the bottom of the sprite.  Using the center causes the camera to jerk in
  // state transtions if moving up or down in the air.
  const Vector2i middle_of_player_px{
      player_pos_px.x + collision_box.x_offset_px + collision_box.collision_width_px / 2,
      player_pos_px.y};
  const int x_max_px = middle_of_player_px.x - convert_to_px(viewport_width_ * screen_ratio_x);
  const int x_min_px =
      middle_of_player_px.x - convert_to_px(viewport_width_ * (1 - screen_ratio_x));
  const int y_max_px = middle_of_player_px.y - convert_to_px(viewport_height_ * screen_ratio_y);
  const int y_min_px =
      middle_of_player_px.y - convert_to_px(viewport_height_ * (1 - screen_ratio_y));

  cam_position_px_x_ = std::max(cam_position_px_x_, x_min_px);
  cam_position_px_x_ = std::min(cam_position_px_x_, x_max_px);
  cam_position_px_y_ = std::max(cam_position_px_y_, y_min_px);
  cam_position_px_y_ = std::min(cam_position_px_y_, y_max_px);
  KeepCameraInBounds();
}

bool RenderingSystem::AddBackgroundLayer(const std::filesystem::path& background_png,
                                         double scroll_slowdown_factor) {
  BackgroundLayer layer;
  layer.scroll_slowdown_factor = scroll_slowdown_factor;
  layer.background_img = std::make_unique<olc::Sprite>();
  if (layer.background_img->LoadFromFile(background_png.string()) != olc::rcode::OK) {
    std::cout << "Failed loading background image '" << background_png << "'" << std::endl;
    return false;
  }
  background_layers_.emplace_back(std::move(layer));
  return true;
}

bool RenderingSystem::AddForegroundLayer(const std::filesystem::path& background_png,
                                         double scroll_slowdown_factor) {
  BackgroundLayer layer;
  layer.scroll_slowdown_factor = scroll_slowdown_factor;
  layer.background_img = std::make_unique<olc::Sprite>();
  if (layer.background_img->LoadFromFile(background_png.string()) != olc::rcode::OK) {
    std::cout << "Failed loading background image '" << background_png << "'" << std::endl;
    return false;
  }
  foreground_layers_.emplace_back(std::move(layer));
  return true;
}

void RenderingSystem::AddFoundationBackgroundLayer(uint8_t r, uint8_t g, uint8_t b) {
  foundation_background_color_ = olc::Pixel{r, g, b, 255};
}

void RenderingSystem::RenderBackground() {
  if (foundation_background_color_.has_value()) {
    for (int y = 0; y < kScreenHeightPx; ++y) {
      for (int x = 0; x < kScreenWidthPx; ++x) {
        engine_ptr_->Draw(x, y, *foundation_background_color_);
      }
    }
  }

  for (const auto& background_layer : background_layers_) {
    RenderBackgroundLayer(background_layer);
  }
}

void RenderingSystem::RenderForeground() {
  for (const auto& background_layer : foreground_layers_) {
    RenderBackgroundLayer(background_layer);
  }
}

void RenderingSystem::RenderBackgroundLayer(const BackgroundLayer& background_layer) {
  const double scroll_factor = background_layer.scroll_slowdown_factor;
  const auto& background = background_layer.background_img;

  const auto total_height_pixels =
      level_.tile_grid.GetHeight() * level_.level_tileset->GetTileSize();

  int num_of_background_draws{};
  // If the background is taller than the screensize, linearly move the camera such that you see the
  // top of the background at the top of the level, and the bottom at the bottom.
  if (background->height > kScreenHeightPx) {
    // C++ 20
    // auto y_pos = std::lerp(-background->height + kScreenHeightPx, 0.0,
    //  cam_position_px_y_ / (total_height_pixels - kScreenHeightPx));

    const double y_multiplier = static_cast<double>(background->height - kScreenHeightPx) /
                                static_cast<double>(total_height_pixels - kScreenHeightPx);
    auto y_pos =
        static_cast<int>(y_multiplier * cam_position_px_y_ - background->height + kScreenHeightPx);
    auto x_pos = -(static_cast<int>(cam_position_px_x_ / scroll_factor) % background->width);
    while (x_pos < kScreenWidthPx) {
      num_of_background_draws++;
      engine_ptr_->DrawSprite(x_pos, y_pos, background.get());
      x_pos += background->width;
    }
  } else {
    // If y is smaller than the screen size, tile both the same way.
    auto y_pos = (static_cast<int>((cam_position_px_y_ / scroll_factor) - kScreenHeightPx) %
                  background->height);
    while (y_pos < kScreenHeightPx) {
      auto x_pos = -(static_cast<int>(cam_position_px_x_ / scroll_factor) % background->width);
      while (x_pos < kScreenWidthPx) {
        num_of_background_draws++;
        engine_ptr_->DrawSprite(x_pos, y_pos, background.get());
        x_pos += background->width;
      }
      y_pos += background->height;
    }
  }
  // LOG_INFO("number of draws " << num_of_background_draws);
}

void RenderingSystem::RenderTiles() {
  KeepCameraInBounds();

  const auto position = GetCameraPosition();
  const auto& tilemap = level_.tile_grid;
  const auto& tileset = *level_.level_tileset;
  for (int y_itr = 0; y_itr <= viewport_height_ + 1; ++y_itr) {
    for (int x_itr = 0; x_itr <= viewport_width_ + 1; ++x_itr) {
      const double lookup_x = position.x + x_itr;
      const double lookup_y = position.y + y_itr;
      const int lookup_x_int = static_cast<int>(std::floor(lookup_x));
      const int lookup_y_int = static_cast<int>(std::floor(lookup_y));
      int tile_idx{};
      if (lookup_x_int >= 0 && lookup_x_int < tilemap.GetWidth() && lookup_y_int >= 0 &&
          lookup_y_int < tilemap.GetHeight()) {
        tile_idx = tilemap.GetTile(lookup_x_int, lookup_y_int).tile_id;
      }
      auto* tile = tileset.GetTile(tile_idx);
      if (tile_idx == 0) {
        continue;
      }

      const double x_fraction = lookup_x - lookup_x_int;
      const double y_fraction = lookup_y - lookup_y_int;

      const int x_px = static_cast<int>(std::round((x_itr - x_fraction) * tile_size_));
      const int y_px =
          static_cast<int>(std::round(kScreenHeightPx - (y_itr + 1 - y_fraction) * tile_size_));
      //   std::cout << " itrs " << x_itr << " " << y_itr << " "                        //
      //             << " global " << lookup_x << " " << lookup_y << " "                //
      //             << " global floor " << lookup_x_int << " " << lookup_y_int << " "  //
      //             << " faction " << x_fraction << " " << y_fraction << " "           //
      //             << " pixel " << x_px << " " << y_px << " "                         //
      //             << " tile " << tile_idx << std::endl;                              //

      engine_ptr_->DrawSprite(x_px, y_px, tile);
    }
  }
}

void RenderingSystem::RenderEntities() {
  for (auto id : registry_->GetView<Position, Animation>()) {
    this->DrawSprite(id);

    const bool draw_bounding_box =
        parameter_server_->GetParameter<double>("viz/draw.player.collisions") == 1.;
    // if (draw_bounding_box) {
    //   const auto [player_top_left_px_x, player_top_left_px_y] = GetPixelLocation(position, sprite);
    //   RenderEntityCollisionBox(player_top_left_px_x, player_top_left_px_y, id);
    // }
  }

  // for (auto id : registry_->GetView<Position, Projectile>()) {
  //   auto [position] = registry_->GetComponents<Position>(id);
  //   const auto position_in_screen = Vector2d{position.x, position.y} - GetCameraPosition();
  //   const int px_x = static_cast<int>(position_in_screen.x * tile_size_);
  //   const int px_y =
  //       kScreenHeightPx - static_cast<int>(position_in_screen.y * tile_size_);
  //   engine_ptr_->Draw(px_x, px_y, olc::WHITE);
  //   engine_ptr_->Draw(px_x+1, px_y, olc::WHITE);
  //   engine_ptr_->Draw(px_x, px_y+1, olc::WHITE);
  //   engine_ptr_->Draw(px_x-1, px_y, olc::WHITE);
  //   engine_ptr_->Draw(px_x, px_y-1, olc::WHITE);
  // }

  for (auto id : registry_->GetView<Position, Particle>()) {
    const auto &position = registry_->GetComponent<Position>(id);
    const auto [px_x, px_y] = GetPixelLocation(position);
    engine_ptr_->Draw(px_x, px_y, olc::WHITE);
  }
}

Vector2i RenderingSystem::GetPixelLocation(const Position& world_pos){
  const auto position_in_screen = Vector2d{world_pos.x, world_pos.y} - GetCameraPosition();
  const int top_left_px_x = static_cast<int>(position_in_screen.x * tile_size_);
  const int top_left_px_y =
      kScreenHeightPx - static_cast<int>(position_in_screen.y * tile_size_);
  return {top_left_px_x, top_left_px_y};
}

Vector2i RenderingSystem::GetPixelLocation(const Position& world_pos, const Sprite& sprite){
  auto [top_left_px_x, top_left_px_y] = GetPixelLocation(world_pos);
  top_left_px_x -= sprite.draw_offset_x;
  top_left_px_y -= (sprite.sprite_ptr->height - sprite.draw_offset_y);
  return {top_left_px_x, top_left_px_y};
}

void RenderingSystem::DrawSprite(const EntityId id) {
  RB_CHECK(registry_->HasComponent<Animation>(id));
  RB_CHECK(registry_->HasComponent<Position>(id));
  const auto &position = registry_->GetComponent<Position>(id);
  const auto sprite = animation_manager_->GetSprite(id);
  // TODO(BT-14): Sprite offset not applied properly for flipped sprites
  const auto [top_left_px_x, top_left_px_y] = GetPixelLocation(position, sprite);
  const auto flip = GetFlip(id, *registry_);
  engine_ptr_->DrawSprite(top_left_px_x, top_left_px_y,
                          const_cast<olc::Sprite*>(sprite.sprite_ptr), 1, flip);
}

void RenderingSystem::RenderEntityCollisionBox(int entity_top_left_px_x,
                                               int entity_top_left_px_y,
                                               int sprite_height_px,
                                               EntityId entity_id) {
  if (!registry_->HasComponent<CollisionBox>(entity_id) ||
      !registry_->HasComponent<Collision>(entity_id)) {  //
    return;
  }
  auto [collision_box, collisions] = registry_->GetComponents<CollisionBox, Collision>(entity_id);
  const auto& bb_width = collision_box.collision_width_px;
  const auto& bb_height = collision_box.collision_height_px;
  const auto bb_bottom_left_x = entity_top_left_px_x + collision_box.x_offset_px;
  const auto bb_bottom_left_y = entity_top_left_px_y + sprite_height_px + collision_box.y_offset_px;
  auto color = collisions.bottom ? olc::WHITE : olc::BLACK;
  engine_ptr_->DrawLine(bb_bottom_left_x, bb_bottom_left_y, bb_bottom_left_x + bb_width,
                        bb_bottom_left_y, color);

  color = collisions.top ? olc::WHITE : olc::BLACK;
  engine_ptr_->DrawLine(bb_bottom_left_x, bb_bottom_left_y - bb_height, bb_bottom_left_x + bb_width,
                        bb_bottom_left_y - bb_height, color);

  color = collisions.left ? olc::WHITE : olc::BLACK;
  engine_ptr_->DrawLine(bb_bottom_left_x, bb_bottom_left_y, bb_bottom_left_x,
                        bb_bottom_left_y - bb_height, color);

  color = collisions.right ? olc::WHITE : olc::BLACK;
  engine_ptr_->DrawLine(bb_bottom_left_x + bb_width, bb_bottom_left_y, bb_bottom_left_x + bb_width,
                        bb_bottom_left_y - bb_height, color);
}

void RenderingSystem::KeepCameraInBounds() {
  cam_position_px_x_ = std::max(cam_position_px_x_, 0);
  cam_position_px_x_ = std::min(cam_position_px_x_, max_cam_postion_px_x_);
  cam_position_px_y_ = std::max(cam_position_px_y_, 0);
  cam_position_px_y_ = std::min(cam_position_px_y_, max_cam_postion_px_y_);
}

}  // namespace platformer