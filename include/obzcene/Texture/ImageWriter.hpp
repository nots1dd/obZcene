#pragma once

/**
 * @file ImageWriter.hpp
 * @brief Snapshot the framebuffer to a file.
 *
 * Used by snapshot mode (obzcene::app::render_snapshot in src/App/Scene.cpp)
 * to dump the rendered frame to a PPM. The importer (Texture/ImageLoader.hpp)
 * formats things on the input side; this is the output side.
 *
 * @see src/Texture/ImageWriter.cpp (implementation),
 *      Renderer/FrameBuffer.hpp (data source)
 */

#include "obzcene/Renderer/FrameBuffer.hpp"

#include <string>

namespace obzcene
{

/**
 * @brief Write a framebuffer as a binary P6 PPM.
 * @param path Destination file (parent directory must exist).
 * @param fb   The rendered target to serialize.
 * @return True on success; false if the file could not be opened/written.
 */
[[nodiscard]] auto write_ppm(const std::string& path, const FrameBuffer& fb) -> bool;

} // namespace obzcene