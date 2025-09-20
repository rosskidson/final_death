#pragma once

#include <map>
#include <memory>
#include <string>
#include "miniaudio.h"
#include <unordered_map>
#include <mutex>
#include <filesystem>


namespace platformer {
class SoundPlayer {
 public:
  SoundPlayer();

  ~SoundPlayer();

  [[nodiscard]] bool LoadWavFromFilesystem(const std::filesystem::path& path, const std::string& sample_name);

  bool PlaySample(const std::string& sample_name, bool loops=false, double volume=1.0) const;

  private:
  struct MaSoundDeleter {
    void operator()(ma_sound* s) const noexcept {
        if (s) {
            ma_sound_uninit(s);
            delete s;
        }
    }
};

  using SoundUniquePtr = std::unique_ptr<ma_sound, MaSoundDeleter>;

  
  ma_engine engine_;
  mutable std::unordered_map<std::string, SoundUniquePtr> samples_;
  mutable std::mutex mutex_;
}; 

}  // namespace platformer
