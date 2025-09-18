#pragma once

#include <map>
#include <memory>
#include <string>

struct Mix_Chunk;

namespace platformer {
class SoundPlayer {
 public:
  SoundPlayer();

  ~SoundPlayer();

  [[nodiscard]] bool LoadWavFromFilesystem(const std::string& path, const std::string& sample_name);

  [[nodiscard]] bool LoadWavFromMemory(std::unique_ptr<Mix_Chunk>&& sample,
                                       const std::string& sample_name);

  bool PlaySample(const std::string& sample_name, bool loops=false) const;

  void test();

 private:
  std::map<std::string, std::unique_ptr<Mix_Chunk>> samples_;
};
}  // namespace platformer
