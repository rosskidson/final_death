#include "sound_player.h"

#define MINIAUDIO_IMPLEMENTATION
#include <filesystem>
#include <mutex>

#include "miniaudio.h"
#include "utils/logging.h"

namespace platformer {

SoundPlayer::SoundPlayer() {
  auto result = ma_engine_init(NULL, &engine_);
  if (result != MA_SUCCESS) {
    // TODO:: Static factory creator pattern.
    std::cout << "failed to initialize audio" << std::endl;
    exit(1);
  }
}

SoundPlayer::~SoundPlayer() { ma_engine_uninit(&engine_); }

bool SoundPlayer::LoadWavFromFilesystem(const std::filesystem::path& path,
                                        const std::string& sample_name) {
  SoundUniquePtr sample{new ma_sound{}};
  auto result =
      ma_sound_init_from_file(&engine_, path.string().c_str(), 0, nullptr, nullptr, sample.get());

  if (result != MA_SUCCESS) {
    LOG_ERROR("Failed loading sample `" << path << "`.");
    return false;
  }

  std::lock_guard<std::mutex> lock(mutex_);
  samples_.emplace(sample_name, std::move(sample));
  return true;
}

bool SoundPlayer::PlaySample(const std::string& sample_name, bool loops, double volume) const {
  std::lock_guard<std::mutex> lock(mutex_);

  if (!samples_.count(sample_name)) {
    LOG_ERROR("No sample found for identifier `" << sample_name << "`.");
    return false;
  }
  auto* sample_ptr = samples_.at(sample_name).get();
  if (ma_sound_is_playing(sample_ptr)) {
    // Stop and replay.  If you want to play both then it needs to be cloned:
    // ma_sound newInstance;
    // ma_sound_init_copy(&engine, &originalSound, 0, nullptr, &newInstance);
    // ma_sound_start(&newInstance);
    // (Needs to be cleaned up after as well)
    ma_sound_stop(sample_ptr);
    ma_sound_seek_to_pcm_frame(sample_ptr, 0);
  }
  if (loops) {
    ma_sound_set_looping(sample_ptr, MA_TRUE);
  }
  ma_sound_set_volume(sample_ptr, volume);  // start quieter
  ma_sound_start(sample_ptr);
  return true;
}

}  // namespace platformer
