/**
 * @file ImageWriter.cpp
 * @brief PPM writer implementation.
 *
 * Serializes the framebuffer as binary P6 PPM (header + 3 bytes/pixel RGB,
 * dropping the alpha channel). Used by snapshot mode and the viewer's offline
 * path to write rendered frames to disk.
 *
 * @see Texture/ImageWriter.hpp (declaration), Renderer/FrameBuffer.hpp
 */

#include "obzcene/Texture/ImageWriter.hpp"

#include "obzcene/Core/Types.hpp"

#include <fstream>

namespace obzcene
{

auto write_ppm(const std::string& path, const FrameBuffer& fb) -> bool
{
  std::ofstream out(path, std::ios::binary);
  if (!out)
  {
    return false;
  }
  out << "P6\n" << fb.width() << ' ' << fb.height() << "\n255\n";
  const auto* pixels       = fb.color_data();
  const auto  pixels_count = static_cast<usz>(fb.width()) * static_cast<usz>(fb.height());
  for (usz i = 0; i < pixels_count; ++i)
  {
    out.put(static_cast<char>(pixels[i * 4]));
    out.put(static_cast<char>(pixels[i * 4 + 1]));
    out.put(static_cast<char>(pixels[i * 4 + 2]));
  }
  return out.good();
}

} // namespace obzcene