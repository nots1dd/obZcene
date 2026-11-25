#pragma once

/**
 * @file Audio.hpp
 * @brief Audio playback for games: clips, voices and a software mixer
 *        (namespace obzcene).
 *
 * obzcene::AudioClip is a decoded, engine-native PCM buffer (float32,
 * stereo, 48 kHz); obzcene::load_audio_clip decodes WAV files through
 * SDL_LoadWAV and converts channels/format/rate itself (nearest-neighbour
 * resample, mono/1st-2nd channel mapping). obzcene::Audio
 * owns an SDL audio device and mixes active voices in its callback; games
 * trigger playback through `Engine::audio()` (see Engine/Engine.hpp).
 *
 * @see src/Engine/Audio.cpp (implementation), Engine/Assets.hpp (asset cache)
 */

#include "obzcene/Core/Types.hpp"
#include "obzcene/Math/Vec3.hpp"

#include <memory>
#include <string>
#include <vector>

#include <SDL.h>

namespace obzcene
{

/** @brief Engine-native PCM: interleaved float32 stereo at 48 kHz. */
inline constexpr int kAudioRate     = 48000;
inline constexpr int kAudioChannels = 2;

/**
 * @brief A decoded sound effect, ready for mixing.
 */
struct AudioClip
{
  /** @brief Interleaved stereo samples (frames = samples / kAudioChannels). */
  std::vector<float> samples;
  /** @brief Frame count (samples.size() / kAudioChannels). */
  [[nodiscard]] auto frames() const -> usz { return samples.size() / kAudioChannels; }
  /** @brief Duration in seconds. */
  [[nodiscard]] auto duration() const -> float
  {
    return static_cast<float>(frames()) / static_cast<float>(kAudioRate);
  }
};

/**
 * @brief Decode a WAV file into an AudioClip.
 * @param path  File path (WAV, any format; converted to float32 stereo 48k).
 * @return The clip, or nullptr on failure (unsupported/corrupt file).
 * @note WAV only for now — decode happens through SDL, so no extra codec
 *       dependency is dragged in.
 */
[[nodiscard]] auto load_audio_clip(const std::string& path) -> std::shared_ptr<AudioClip>;

/** @brief Handle to a playing voice (stable until it finishes or is stopped). */
using SoundHandle = ui64;
/** @brief Invalid voice handle. */
inline constexpr SoundHandle kInvalidSound = 0;

/** @brief How a voice plays. */
struct PlayOptions
{
  /** @brief Linear gain, 1 = as authored. */
  float volume{1.0F};
  /** @brief Stereo position: -1 = hard left, 0 = centre, +1 = hard right. */
  float pan{0.0F};
  /** @brief Restart from the beginning forever. */
  bool loop{false};
};

/**
 * @brief Inverse-distance gain rolloff for a sound `distance` away.
 *
 * 1.0 inside `reference` units, then falls off as
 * `reference / (reference + rolloff·(distance - reference))` — the classic
 * OpenAL linear rolloff. Always positive, so it composes with `volume`.
 */
[[nodiscard]] inline auto spatial_gain(float distance, float reference = 1.0F, float rolloff = 1.0F)
  -> float
{
  if (distance <= reference || reference <= 0.0F || rolloff <= 0.0F)
  {
    return 1.0F;
  }
  return reference / (reference + rolloff * (distance - reference));
}

/**
 * @brief Stereo pan in [-1, 1] of `source` for a listener at `listener`
 *        facing along `listener_right`'s complement (pass the camera's right
 *        vector). 0 for coincident positions.
 */
[[nodiscard]] inline auto spatial_pan(const math::Vec3f& source, const math::Vec3f& listener,
                                      const math::Vec3f& listener_right) -> float
{
  const auto to_source = source - listener;
  const auto distance  = math::length(to_source);
  if (distance < 1.0e-6F)
  {
    return 0.0F;
  }
  return math::clamp(math::dot(to_source / distance, listener_right), -1.0F, 1.0F);
}

/**
 * @brief PlayOptions for a world-space sound: distance rolloff into `volume`
 *        and direction into `pan`, ready for Audio::play.
 */
[[nodiscard]] inline auto spatial_options(const math::Vec3f& source, const math::Vec3f& listener,
                                          const math::Vec3f& listener_right, float base_volume,
                                          float reference = 1.0F, float rolloff = 1.0F)
  -> PlayOptions
{
  const auto distance = math::length(source - listener);
  return PlayOptions{base_volume * spatial_gain(distance, reference, rolloff),
                     spatial_pan(source, listener, listener_right), false};
}

/**
 * @brief The audio device and voice mixer.
 *
 * Construct once per game (obzcene::Engine does this). The SDL audio callback
 * runs on a separate thread; play/stop/set_volume are safe to call from the
 * game thread (the voice list is guarded by a mutex held only briefly).
 */
class Audio
{
public:
  Audio() = default;
  ~Audio();

  Audio(const Audio&)                    = delete;
  auto operator=(const Audio&) -> Audio& = delete;

  /**
   * @brief Open the audio device. Safe to call when audio is unavailable
   *        (headless CI): the mixer reports offline and play() is a no-op.
   * @return True when the device is live.
   */
  auto init() -> bool;
  /** @brief Close the device and stop all voices. */
  auto shutdown() -> void;
  /** @brief True when the device is live (false in offline mode). */
  [[nodiscard]] auto is_available() const -> bool;

  /**
   * @brief Play a clip; returns a handle for later control.
   * @return kInvalidSound when offline or the clip is empty.
   */
  auto play(const std::shared_ptr<const AudioClip>& clip, const PlayOptions& options = {})
    -> SoundHandle;
  /** @brief Stop one voice (no-op for stale handles). */
  auto stop(SoundHandle handle) -> void;
  /** @brief Change a playing voice's gain (no-op for stale handles). */
  auto set_volume(SoundHandle handle, float volume) -> void;
  /** @brief Stop every voice. */
  auto stop_all() -> void;
  /** @brief Number of currently playing voices. */
  [[nodiscard]] auto active_voices() const -> usz;
  /** @brief Master gain applied at the final mix. */
  auto set_master_volume(float volume) -> void;

private:
  /** @brief One playing instance of a clip. */
  struct Voice
  {
    std::shared_ptr<const AudioClip> clip;
    usz                              position{0}; ///< frame cursor into clip->samples
    float                            volume{1.0F};
    float                            pan{0.0F}; ///< -1 left .. +1 right
    bool                             loop{false};
    SoundHandle                      handle{kInvalidSound};
  };

  /** @brief SDL callback: mix all voices into the output buffer. */
  static auto audio_callback(void* userdata, unsigned char* stream, int len) -> void;
  /** @brief Mix `frames` frames of all voices into `out` (caller-locked). */
  auto mix(float* out, usz frames) -> void;

  SDL_AudioDeviceID  m_device{0};
  std::vector<Voice> m_voices;
  SoundHandle        m_next_handle{1};
  float              m_master_volume{1.0F};
  bool               m_available{false};
  /** @brief Guards m_voices against the audio thread. */
  mutable SDL_mutex* m_mutex{nullptr};
};

} // namespace obzcene
