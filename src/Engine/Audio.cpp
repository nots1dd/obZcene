/**
 * @file Audio.cpp
 * @brief Audio device, clip decoding and the voice mixer.
 *
 * Implements obzcene::Audio on top of SDL's audio device: clips are decoded
 * once into engine-native PCM (float32 stereo, 48 kHz) by load_audio_clip,
 * and the callback sums every active voice into the output. The game thread
 * mutates the voice list under a mutex held only for list operations; the
 * audio thread mixes under the same lock.
 *
 * Offline mode: when no audio device is available (headless CI, missing
 * hardware) init() reports unavailable and playback calls become no-ops, so
 * games run unchanged.
 *
 * @see Engine/Audio.hpp (declarations)
 */

#include "obzcene/Engine/Audio.hpp"

#include "obzcene/Core/Casts.hpp"
#include "obzcene/Core/Logger.hpp"

#include <algorithm>
#include <cstring>

namespace obzcene
{

namespace
{

/** @brief Locked section for the voice list. */
struct VoiceLock
{
  SDL_mutex* mutex;
  explicit VoiceLock(SDL_mutex* m) : mutex(m) { SDL_LockMutex(mutex); }
  ~VoiceLock() { SDL_UnlockMutex(mutex); }
};

/** @brief One channel of one source frame as a float in [-1, 1]. */
[[nodiscard]] auto sample_as_float(const Uint8* raw, SDL_AudioFormat format, usz offset) -> float
{
  switch (format)
  {
    case AUDIO_U8:
    {
      return (static_cast<float>(raw[offset]) - 128.0F) / 128.0F;
    }
    case AUDIO_S8:
    {
      return static_cast<float>(reinterpret_cast<const Sint8*>(raw)[offset]) / 128.0F;
    }
    case AUDIO_S16SYS:
    {
      return static_cast<float>(reinterpret_cast<const Sint16*>(raw)[offset]) / 32768.0F;
    }
    case AUDIO_S32SYS:
    {
      return static_cast<float>(reinterpret_cast<const Sint32*>(raw)[offset]) / 2147483648.0F;
    }
    case AUDIO_F32SYS:
    {
      return reinterpret_cast<const float*>(raw)[offset];
    }
    default:
      return 0.0F;
  }
}

} // namespace

auto load_audio_clip(const std::string& path) -> std::shared_ptr<AudioClip>
{
  SDL_AudioSpec wav_spec;
  Uint8*        wav_buffer = nullptr;
  Uint32        wav_length = 0;
  if (SDL_LoadWAV(path.c_str(), &wav_spec, &wav_buffer, &wav_length) == nullptr)
  {
    LOG_WARN("Failed to load audio '{}': {}", path, SDL_GetError());
    return nullptr;
  }

  const auto bytes_per_sample = static_cast<usz>(SDL_AUDIO_BITSIZE(wav_spec.format)) / 8;
  const auto bytes_per_frame  = bytes_per_sample * static_cast<usz>(wav_spec.channels);
  if (bytes_per_frame == 0 || (wav_length % bytes_per_frame) != 0)
  {
    SDL_FreeWAV(wav_buffer);
    LOG_WARN("Unsupported audio layout in '{}'", path);
    return nullptr;
  }

  const auto in_frames  = static_cast<usz>(wav_length) / bytes_per_frame;
  const auto out_frames = in_frames * kAudioRate / static_cast<usz>(wav_spec.freq);

  auto clip = std::make_shared<AudioClip>();
  clip->samples.resize(out_frames * kAudioChannels);
  for (usz out_frame = 0; out_frame < out_frames; ++out_frame)
  {
    // Nearest-neighbour resample to the engine rate.
    const auto in_frame =
      std::min(out_frame * static_cast<usz>(wav_spec.freq) / kAudioRate, in_frames - 1);
    for (usz channel = 0; channel < kAudioChannels; ++channel)
    {
      const auto source_channel = std::min(channel, static_cast<usz>(wav_spec.channels) - 1);
      clip->samples[out_frame * kAudioChannels + channel] =
        sample_as_float(wav_buffer, wav_spec.format, in_frame * wav_spec.channels + source_channel);
    }
  }
  SDL_FreeWAV(wav_buffer);
  LOG_INFO("Loaded audio '{}': {:.2f}s", path, clip->duration());
  return clip;
}

Audio::~Audio() { shutdown(); }

auto Audio::init() -> bool
{
  if (m_available)
  {
    return true;
  }
  if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0)
  {
    LOG_WARN("Audio init failed ({}); running silent", SDL_GetError());
    return false;
  }
  m_mutex = SDL_CreateMutex();
  SDL_AudioSpec want{};
  want.freq     = kAudioRate;
  want.format   = AUDIO_F32;
  want.channels = static_cast<Uint8>(kAudioChannels);
  want.samples  = 1024;
  want.callback = &Audio::audio_callback;
  want.userdata = this;
  m_device      = SDL_OpenAudioDevice(nullptr, 0, &want, nullptr, 0);
  if (m_device == 0)
  {
    LOG_WARN("No audio device ({}); running silent", SDL_GetError());
    SDL_DestroyMutex(m_mutex);
    m_mutex = nullptr;
    return false;
  }
  SDL_PauseAudioDevice(m_device, 0);
  m_available = true;
  return true;
}

auto Audio::shutdown() -> void
{
  if (m_device != 0)
  {
    SDL_CloseAudioDevice(m_device);
    m_device = 0;
  }
  if (m_mutex != nullptr)
  {
    SDL_DestroyMutex(m_mutex);
    m_mutex = nullptr;
  }
  m_voices.clear();
  m_available = false;
}

auto Audio::is_available() const -> bool { return m_available; }

auto Audio::play(const std::shared_ptr<const AudioClip>& clip, const PlayOptions& options)
  -> SoundHandle
{
  if (!m_available || clip == nullptr || clip->samples.empty())
  {
    return kInvalidSound;
  }
  const VoiceLock lock(m_mutex);
  Voice           voice;
  voice.clip   = clip;
  voice.volume = options.volume;
  voice.pan    = std::clamp(options.pan, -1.0F, 1.0F);
  voice.loop   = options.loop;
  voice.handle = m_next_handle++;
  m_voices.push_back(std::move(voice));
  return m_voices.back().handle;
}

auto Audio::stop(SoundHandle handle) -> void
{
  if (!m_available)
  {
    return;
  }
  const VoiceLock lock(m_mutex);
  std::erase_if(m_voices, [handle](const Voice& voice) { return voice.handle == handle; });
}

auto Audio::set_volume(SoundHandle handle, float volume) -> void
{
  if (!m_available)
  {
    return;
  }
  const VoiceLock lock(m_mutex);
  for (auto& voice : m_voices)
  {
    if (voice.handle == handle)
    {
      voice.volume = volume;
      return;
    }
  }
}

auto Audio::stop_all() -> void
{
  if (!m_available)
  {
    return;
  }
  const VoiceLock lock(m_mutex);
  m_voices.clear();
}

auto Audio::active_voices() const -> usz
{
  if (!m_available)
  {
    return 0;
  }
  const VoiceLock lock(m_mutex);
  return m_voices.size();
}

auto Audio::set_master_volume(float volume) -> void { m_master_volume = volume; }

auto Audio::audio_callback(void* userdata, unsigned char* stream, int len) -> void
{
  auto*      self   = static_cast<Audio*>(userdata);
  auto*      out    = reinterpret_cast<float*>(stream);
  const auto frames = static_cast<usz>(len) / (sizeof(float) * kAudioChannels);
  std::memset(stream, 0, static_cast<usz>(len));
  self->mix(out, frames);
}

auto Audio::mix(float* out, usz frames) -> void
{
  const VoiceLock lock(m_mutex);
  for (auto it = m_voices.begin(); it != m_voices.end();)
  {
    auto&       voice       = *it;
    const auto* samples     = voice.clip->samples.data();
    const auto  clip_frames = voice.clip->frames();
    const auto  gain        = voice.volume * m_master_volume;
    const auto  gain_left   = gain * (voice.pan <= 0.0F ? 1.0F : 1.0F - voice.pan);
    const auto  gain_right  = gain * (voice.pan >= 0.0F ? 1.0F : 1.0F + voice.pan);
    for (usz frame = 0; frame < frames; ++frame)
    {
      if (voice.position >= clip_frames)
      {
        break;
      }
      const auto offset = voice.position * kAudioChannels;
      out[frame * kAudioChannels] += samples[offset] * gain_left;
      out[frame * kAudioChannels + 1] += samples[offset + 1] * gain_right;
      ++voice.position;
    }
    if (voice.position >= clip_frames)
    {
      if (voice.loop)
      {
        voice.position = 0;
        ++it;
      }
      else
      {
        it = m_voices.erase(it);
      }
    }
    else
    {
      ++it;
    }
  }
}

} // namespace obzcene
