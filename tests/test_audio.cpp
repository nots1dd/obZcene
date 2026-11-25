#include <obzcene/Core/Logger.hpp>
#include <obzcene/Engine/Audio.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <cstdio>
#include <filesystem>

namespace
{

/** @brief Write a minimal 16-bit PCM stereo WAV (a constant tone). */
auto write_test_wav(const std::filesystem::path& path, int frames = 480) -> void
{
  std::FILE* file = std::fopen(path.string().c_str(), "wb");
  REQUIRE(file != nullptr);

  const auto        data_bytes = static_cast<std::uint32_t>(frames * 2 * 2);
  const auto        rate       = std::uint32_t{48000};
  const std::string fmt_tag    = "fmt ";
  const std::string data_tag   = "data";

  std::fwrite("RIFF", 1, 4, file);
  std::fwrite(&data_bytes, 4, 1, file); // placeholder, patched below
  std::fwrite("WAVE", 1, 4, file);
  std::fwrite(fmt_tag.data(), 1, 4, file);
  const auto fmt_size  = std::uint32_t{16};
  const auto pcm_tag   = std::uint16_t{1};
  const auto channels  = std::uint16_t{2};
  const auto bits      = std::uint16_t{16};
  const auto byte_rate = rate * 2 * 2;
  const auto align     = std::uint16_t{4};
  std::fwrite(&fmt_size, 4, 1, file);
  std::fwrite(&pcm_tag, 2, 1, file);
  std::fwrite(&channels, 2, 1, file);
  std::fwrite(&rate, 4, 1, file);
  std::fwrite(&byte_rate, 4, 1, file);
  std::fwrite(&align, 2, 1, file);
  std::fwrite(&bits, 2, 1, file);
  std::fwrite(data_tag.data(), 1, 4, file);
  std::fwrite(&data_bytes, 4, 1, file);
  for (auto i = 0; i < frames; ++i)
  {
    const auto sample = static_cast<std::int16_t>(12000); // constant tone
    std::fwrite(&sample, 2, 1, file);
    std::fwrite(&sample, 2, 1, file);
  }
  const auto total = std::uint32_t{36} + data_bytes;
  std::fseek(file, 4, SEEK_SET);
  std::fwrite(&total, 4, 1, file);
  std::fclose(file);
}

} // namespace

TEST_CASE("load_audio_clip decodes a WAV into float32 stereo 48k", "[audio]")
{
  obzcene::Logger::init();
  const auto path = std::filesystem::temp_directory_path() / "obz_test_tone.wav";
  write_test_wav(path);
  const auto clip = obzcene::load_audio_clip(path.string());
  std::filesystem::remove(path);
  REQUIRE(clip != nullptr);
  REQUIRE(clip->frames() >= 480);
  REQUIRE(clip->samples.size() == clip->frames() * obzcene::kAudioChannels);
}

TEST_CASE("load_audio_clip rejects a missing file", "[audio]")
{
  obzcene::Logger::init();
  REQUIRE(obzcene::load_audio_clip("/definitely/not/here.wav") == nullptr);
}

TEST_CASE("Audio offline mode: play is a safe no-op", "[audio]")
{
  auto audio = obzcene::Audio{};
  // No init() call: offline. Everything must be a safe no-op.
  auto clip         = std::make_shared<obzcene::AudioClip>();
  clip->samples     = std::vector<float>(480, 0.5F);
  const auto handle = audio.play(clip);
  REQUIRE(handle == obzcene::kInvalidSound);
  REQUIRE(audio.active_voices() == 0);
  audio.stop(handle); // no crash
  audio.stop_all();
  audio.shutdown();
}

TEST_CASE("spatial_gain is flat inside the reference and rolls off beyond", "[audio][spatial]")
{
  REQUIRE(obzcene::spatial_gain(0.0F) == Catch::Approx(1.0F));
  REQUIRE(obzcene::spatial_gain(1.0F) == Catch::Approx(1.0F));
  REQUIRE(obzcene::spatial_gain(2.0F) == Catch::Approx(0.5F));
  REQUIRE(obzcene::spatial_gain(11.0F) == Catch::Approx(0.1F).margin(0.01F));
  // Louder rolloff decays faster; zero rolloff keeps full volume.
  REQUIRE(obzcene::spatial_gain(2.0F, 1.0F, 3.0F) < obzcene::spatial_gain(2.0F, 1.0F, 1.0F));
  REQUIRE(obzcene::spatial_gain(100.0F, 1.0F, 0.0F) == Catch::Approx(1.0F));
  // Degenerate reference stays safe.
  REQUIRE(obzcene::spatial_gain(5.0F, 0.0F) == Catch::Approx(1.0F));
}

TEST_CASE("spatial_pan maps left/right of the listener", "[audio][spatial]")
{
  const obzcene::math::Vec3f listener{0, 0, 0};
  const obzcene::math::Vec3f right{1, 0, 0};
  REQUIRE(obzcene::spatial_pan({2, 0, 0}, listener, right) == Catch::Approx(1.0F));
  REQUIRE(obzcene::spatial_pan({-2, 0, 0}, listener, right) == Catch::Approx(-1.0F));
  REQUIRE(obzcene::spatial_pan({0, 0, -2}, listener, right) == Catch::Approx(0.0F).margin(1e-6));
  REQUIRE(obzcene::spatial_pan({0, 0, 0}, listener, right) == Catch::Approx(0.0F));
  // Diagonal source pans partially.
  REQUIRE(obzcene::spatial_pan({1, 0, -1}, listener, right) ==
          Catch::Approx(0.7071F).margin(0.001F));
}

TEST_CASE("spatial_options folds distance into volume and direction into pan", "[audio][spatial]")
{
  const obzcene::math::Vec3f listener{0, 0, 0};
  const obzcene::math::Vec3f right{1, 0, 0};
  const auto options = obzcene::spatial_options({4, 0, 0}, listener, right, 0.8F, 1.0F, 1.0F);
  REQUIRE(options.volume == Catch::Approx(0.8F * 0.25F));
  REQUIRE(options.pan == Catch::Approx(1.0F));
  REQUIRE(!options.loop);
}
