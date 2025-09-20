#include "sound.h"

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include <memory>
#include <stdexcept>
#include <mutex>
#include <filesystem>

#include "utils/logging.h"

namespace platformer {

SoundPlayer::SoundPlayer() {
    auto result = ma_engine_init(NULL, &engine_);
    if (result != MA_SUCCESS) {
      std::cout << "failed to initialize audio" << std::endl;
      exit(1);
    }
}

SoundPlayer::~SoundPlayer() {
ma_engine_uninit(&engine_);
}

bool SoundPlayer::LoadWavFromFilesystem(const std::filesystem::path& path, const std::string& sample_name) {
  //Sample sample{};
  SoundUniquePtr sample{new ma_sound{}};
  auto result = ma_sound_init_from_file(&engine_, path.string().c_str(), 0, nullptr, nullptr, sample.get());

  if (result != MA_SUCCESS) {
    LOG_ERROR("Failed loading sample `" << path << "`.");
    return false;
  }

  std::lock_guard<std::mutex> lock(mutex_);
  samples_.emplace(sample_name, std::move(sample));
  return true;
}

bool SoundPlayer::PlaySample(const std::string& sample_name, bool loops) const {
  std::lock_guard<std::mutex> lock(mutex_);

  if (!samples_.count(sample_name)) {
    LOG_ERROR("No sample found for identifier `" << sample_name << "`.");
    return false;
  }
  auto* sample = samples_.at(sample_name).get();
  ma_sound_stop(sample);
  ma_sound_seek_to_pcm_frame(sample, 0);
  ma_sound_start(sample);                 
  //ma_sound_start(samples_.at(sample_name).get());
  return true;
}

}  // namespace platformer
