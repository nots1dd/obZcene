/**
 * @file Overlay.cpp
 * @brief Fixed 5×7 bitmap font and the viewer HUD renderer.
 *
 * Contains the printable-ASCII glyph table (space .. ~) and the draw routines
 * declared by Viewer/Overlay.hpp. `draw_stats` formats the per-frame panels:
 * resolution/fps, mesh stats, bounds, rasterizer counters, camera state and a
 * light summary, plus a one-line key-legend and the texture list.
 *
 * @see Viewer/Overlay.hpp (declaration), Viewer/Viewer.cpp (caller)
 */

#include "obzcene/Viewer/Overlay.hpp"
#include "obzcene/Core/Casts.hpp"

#include <array>
#include <cstdio>
#include <string>
#include <utility>
#include <vector>

namespace obzcene::overlay
{

namespace
{

constexpr int kGlyphWidth   = 5;
constexpr int kGlyphHeight  = 7;
constexpr int kGlyphAdvance = 6;

/** @brief One 5×7 glyph bitmap; each `rows[r]` is 5 '#'/'.' chars. */
struct Glyph
{
  const char* rows[7];
};

constexpr std::array<Glyph, 95> kFont = {{
  {".....", ".....", ".....", ".....", ".....", ".....", "....."}, // space
  {"..#..", "..#..", "..#..", "..#..", "..#..", ".....", "..#.."}, // !
  {".#.#.", ".#.#.", ".#.#.", ".....", ".....", ".....", "....."}, // "
  {".#.#.", ".#.#.", "#####", ".#.#.", "#####", ".#.#.", ".#.#."}, // #
  {"..#..", ".####", "#.#..", ".###.", "..#.#", "####.", "..#.."}, // $
  {"##..#", "##.#.", "...#.", "..#..", ".#...", ".##..", "#..##"}, // %
  {".##..", "#..#.", "#.#..", ".##..", "#..#.", "#..#.", "###.#"}, // &
  {"..#..", "..#..", "..#..", ".....", ".....", ".....", "....."}, // '
  {".###.", "#....", "#....", "#....", "#....", "#....", ".###."}, // (
  {".###.", "....#", "....#", "....#", "....#", "....#", ".###."}, // )
  {".....", ".#.#.", "..#..", "#####", "..#..", ".#.#.", "....."}, // *
  {".....", "..#..", "..#..", "#####", "..#..", "..#..", "....."}, // +
  {".....", ".....", ".....", ".....", "..#..", "..#..", ".#..."}, // ,
  {".....", ".....", ".....", "#####", ".....", ".....", "....."}, // -
  {".....", ".....", ".....", ".....", ".....", ".##..", ".##.."}, // .
  {"....#", "....#", "...#.", "..#..", ".#...", "#....", "#...."}, // /
  {".###.", "#...#", "#...#", "#...#", "#...#", "#...#", ".###."}, // 0
  {"..#..", ".##..", "..#..", "..#..", "..#..", "..#..", ".###."}, // 1
  {".###.", "#...#", "....#", "...#.", ".#...", "#....", "#####"}, // 2
  {".###.", "#...#", "....#", "..##.", "....#", "#...#", ".###."}, // 3
  {"...#.", "..##.", ".#.#.", "#..#.", "#####", "...#.", "...#."}, // 4
  {"#####", "#....", "####.", "....#", "....#", "#...#", ".###."}, // 5
  {"..##.", ".#...", "#....", "####.", "#...#", "#...#", ".###."}, // 6
  {"#####", "....#", "...#.", "..#..", ".#...", ".#...", ".#..."}, // 7
  {".###.", "#...#", "#...#", ".###.", "#...#", "#...#", ".###."}, // 8
  {".###.", "#...#", "#...#", ".####", "....#", "...#.", ".##.."}, // 9
  {".....", ".##..", ".##..", ".....", ".##..", ".##..", "....."}, // :
  {".....", "..#..", "..#..", ".....", "..#..", "..#..", ".#..."}, // ;
  {"...#.", "..#..", ".#...", "#....", ".#...", "..#..", "...#."}, // <
  {".....", ".....", "#####", ".....", "#####", ".....", "....."}, // =
  {".#...", "..#..", "...#.", "....#", "...#.", "..#..", ".#..."}, // >
  {".###.", "#...#", "....#", "...#.", "..#..", ".....", "..#.."}, // ?
  {".###.", "#...#", "....#", ".#.#.", "#.#.#", "#.#.#", ".###."}, // @
  {".###.", "#...#", "#...#", "#####", "#...#", "#...#", "#...#"}, // A
  {"####.", "#...#", "#...#", "####.", "#...#", "#...#", "####."}, // B
  {".###.", "#...#", "#....", "#....", "#....", "#...#", ".###."}, // C
  {"####.", "#...#", "#...#", "#...#", "#...#", "#...#", "####."}, // D
  {"#####", "#....", "#....", "####.", "#....", "#....", "#####"}, // E
  {"#####", "#....", "#....", "####.", "#....", "#....", "#...."}, // F
  {".###.", "#...#", "#....", "#.###", "#...#", "#...#", ".####"}, // G
  {"#...#", "#...#", "#...#", "#####", "#...#", "#...#", "#...#"}, // H
  {".###.", "..#..", "..#..", "..#..", "..#..", "..#..", ".###."}, // I
  {"..###", "...#.", "...#.", "...#.", "...#.", "#..#.", ".##.."}, // J
  {"#...#", "#..#.", "#.#..", "##...", "#.#..", "#..#.", "#...#"}, // K
  {"#....", "#....", "#....", "#....", "#....", "#....", "#####"}, // L
  {"#...#", "##.##", "#.#.#", "#.#.#", "#...#", "#...#", "#...#"}, // M
  {"#...#", "##..#", "#.#.#", "#..##", "#...#", "#...#", "#...#"}, // N
  {".###.", "#...#", "#...#", "#...#", "#...#", "#...#", ".###."}, // O
  {"####.", "#...#", "#...#", "####.", "#....", "#....", "#...."}, // P
  {".###.", "#...#", "#...#", "#...#", "#.#.#", "#..#.", ".##.#"}, // Q
  {"####.", "#...#", "#...#", "####.", "#.#..", "#..#.", "#...#"}, // R
  {".####", "#....", "#....", ".###.", "....#", "....#", "####."}, // S
  {"#####", "..#..", "..#..", "..#..", "..#..", "..#..", "..#.."}, // T
  {"#...#", "#...#", "#...#", "#...#", "#...#", "#...#", ".###."}, // U
  {"#...#", "#...#", "#...#", "#...#", "#...#", ".#.#.", "..#.."}, // V
  {"#...#", "#...#", "#...#", "#.#.#", "#.#.#", "##.##", "#...#"}, // W
  {"#...#", "#...#", ".#.#.", "..#..", ".#.#.", "#...#", "#...#"}, // X
  {"#...#", "#...#", ".#.#.", "..#..", "..#..", "..#..", "..#.."}, // Y
  {"#####", "....#", "...#.", "..#..", ".#...", "#....", "#####"}, // Z
  {".###.", "#....", "#....", "#....", "#....", "#....", ".###."}, // [
  {"#....", "#....", ".#...", "..#..", "...#.", "....#", "....#"}, // backslash
  {".###.", "....#", "....#", "....#", "....#", "....#", ".###."}, // ]
  {"..#..", ".#.#.", "#...#", ".....", ".....", ".....", "....."}, // ^
  {".....", ".....", ".....", ".....", ".....", ".....", "#####"}, // _
  {".#...", "..#..", ".....", ".....", ".....", ".....", "....."}, // `
  {".....", ".....", ".###.", "....#", ".####", "#...#", ".####"}, // a
  {"#....", "#....", "####.", "#...#", "#...#", "#...#", "####."}, // b
  {".....", ".....", ".###.", "#...#", "#....", "#...#", ".###."}, // c
  {"....#", "....#", ".####", "#...#", "#...#", "#...#", ".####"}, // d
  {".....", ".....", ".###.", "#...#", "#####", "#....", ".###."}, // e
  {"..##.", ".#..#", ".#...", "####.", ".#...", ".#...", ".#..."}, // f
  {".....", ".####", "#...#", ".###.", "....#", "#...#", ".###."}, // g
  {"#....", "#....", "####.", "#...#", "#...#", "#...#", "#...#"}, // h
  {"..#..", ".....", ".##..", "..#..", "..#..", "..#..", ".###."}, // i
  {"...#.", ".....", "..##.", "...#.", "...#.", "#..#.", ".##.."}, // j
  {"#....", "#....", "#..#.", "#.#..", "##...", "#.#..", "#..#."}, // k
  {".##..", "..#..", "..#..", "..#..", "..#..", "..#..", ".###."}, // l
  {".....", ".....", "##.#.", "#.#.#", "#.#.#", "#...#", "#...#"}, // m
  {".....", ".....", "####.", "#...#", "#...#", "#...#", "#...#"}, // n
  {".....", ".....", ".###.", "#...#", "#...#", "#...#", ".###."}, // o
  {".....", "####.", "#...#", "####.", "#....", "#....", "#...."}, // p
  {".....", ".####", "#...#", ".####", "....#", "....#", "....#"}, // q
  {".....", ".....", "#.##.", "#..#.", "#....", "#....", "#...."}, // r
  {".....", ".....", ".####", "#....", ".###.", "....#", "####."}, // s
  {".#...", ".#...", "####.", ".#...", ".#...", ".#..#", "..##."}, // t
  {".....", ".....", "#...#", "#...#", "#...#", "#...#", ".####"}, // u
  {".....", ".....", "#...#", "#...#", ".#.#.", ".#.#.", "..#.."}, // v
  {".....", ".....", "#...#", "#...#", "#.#.#", "#.#.#", ".#.#."}, // w
  {".....", ".....", "#...#", ".#.#.", "..#..", ".#.#.", "#...#"}, // x
  {".....", "#...#", ".#.#.", "..#..", "..#..", "..#..", "..#.."}, // y
  {".....", ".....", "#####", "...#.", "..#..", ".#...", "#####"}, // z
  {"..##.", ".#...", ".#...", "##...", ".#...", ".#...", "..##."}, // {
  {"..#..", "..#..", "..#..", "..#..", "..#..", "..#..", "..#.."}, // |
  {".##..", "..#..", "..#..", "..##.", "..#..", "..#..", ".##.."}, // }
  {".....", ".....", ".##.#", "#.##.", ".....", ".....", "....."}, // ~
}};

[[nodiscard]] auto is_set(const Glyph& glyph, int row, int col) -> bool
{
  return glyph.rows[row][col] == '#';
}

template <typename... Args> auto linef(const char* format, Args&&... args) -> std::string
{
  auto buffer = std::array<char, 256>{};
  std::snprintf(buffer.data(), buffer.size(), format, std::forward<Args>(args)...);
  return std::string{buffer.data()};
}

} // namespace

auto text_width(const std::string& text) -> int
{
  return narrow_cast<i32>(text.size()) * kGlyphAdvance;
}

auto draw_text(FrameBuffer& fb, int x, int y, const std::string& text, const Color& color) -> void
{
  auto cursor_x = x;
  for (const auto c : text)
  {
    const auto cc = static_cast<unsigned char>(c);
    if (cc >= 0x20 && cc <= 0x7e)
    {
      const auto& glyph = kFont[static_cast<usz>(cc) - 0x20u];
      for (auto row = 0; row < kGlyphHeight; ++row)
      {
        for (auto col = 0; col < kGlyphWidth; ++col)
        {
          if (is_set(glyph, row, col))
          {
            const auto px = cursor_x + col;
            const auto py = y + row;
            if (fb.inside(px, py))
            {
              fb.at(px, py) = color;
            }
          }
        }
      }
    }
    cursor_x += kGlyphAdvance;
  }
}

auto draw_stats(FrameBuffer& fb, const RenderStats& stats, const Mesh& mesh,
                const std::vector<Texture>& textures, const std::vector<LightF>& lights,
                const CameraF& camera, float fps, float frame_ms, bool wireframe, usz active_light,
                bool headlamp) -> void
{
  constexpr Color kTextColor{200, 205, 214};
  constexpr Color kDimColor{120, 124, 132};

  const auto min         = mesh.bounds.min;
  const auto max         = mesh.bounds.max;
  const auto pos         = camera.position();
  const auto draw_width  = fb.width();
  const auto draw_height = fb.height();

  auto directional = usz{0};
  auto point       = usz{0};
  auto spot        = usz{0};
  for (const auto& light : lights)
  {
    if (light.type == LightType::Directional)
    {
      ++directional;
    }
    else if (light.type == LightType::Point)
    {
      ++point;
    }
    else
    {
      ++spot;
    }
  }

  const auto lines = std::vector<std::string>{
    linef("obZcene  %dx%d  %5.1f fps  %6.2f ms", draw_width, draw_height, fps, frame_ms),
    linef("mesh: %zu tris  %zu verts  %zu mats  %zu objects", mesh.triangles.size(),
          mesh.vertices.size(), mesh.materials.size(), mesh.object_count),
    linef("bounds: (%.2f,%.2f,%.2f)-(%.2f,%.2f,%.2f)", min.x, min.y, min.z, max.x, max.y, max.z),
    linef("render: proc %zu  bf %zu  vp %zu  near %zu  deg %zu  drawn %zu",
          stats.triangles_processed, stats.triangles_culled_backface,
          stats.triangles_culled_viewport, stats.triangles_culled_nearfar,
          stats.triangles_degenerate, stats.triangles_rendered),
    linef("pixels: shaded %zu  written %zu", stats.pixels_shaded, stats.pixels_written),
    linef("camera: (%.2f,%.2f,%.2f)  mode %s", pos.x, pos.y, pos.z,
          wireframe ? "wireframe" : "solid"),
    linef("lights: %zu (%zu dir, %zu point, %zu spot)", lights.size(), directional, point, spot),
  };

  auto light_info = std::string{};
  if (headlamp)
  {
    light_info = "headlamp";
    if (active_light < lights.size())
    {
      light_info += " + ";
    }
  }
  if (active_light < lights.size())
  {
    const auto& active = lights[active_light];
    if (light_info.empty())
    {
      light_info = linef("[%zu]", active_light);
    }
    if (active.type == LightType::Directional)
    {
      light_info += linef(" dir (%.2f, %.2f, %.2f)", active.direction.x, active.direction.y,
                          active.direction.z);
    }
    else
    {
      light_info +=
        linef(" pos (%.2f, %.2f, %.2f)", active.position.x, active.position.y, active.position.z);
    }
  }

  const auto draw_line = [&](int line_index, const std::string& text, const Color& color)
  { draw_text(fb, 8, 8 + line_index * (kGlyphHeight + 2), text, color); };

  auto line_index = 0;
  draw_line(line_index++, lines[0], kTextColor);
  for (usz i = 1; i < lines.size(); ++i)
  {
    draw_line(line_index++, lines[i], kDimColor);
  }
  draw_line(line_index++, "light: " + (light_info.empty() ? std::string{"none"} : light_info),
            kTextColor);
  draw_line(line_index++, "textures:", kTextColor);
  if (textures.empty())
  {
    draw_line(line_index++, "  none", kDimColor);
  }
  else
  {
    for (usz i = 0; i < textures.size(); ++i)
    {
      const auto& material = i < mesh.materials.size() ? mesh.materials[i] : Material{};
      const auto  name = material.diffuse_texture.empty() ? "(builtin)" : material.diffuse_texture;
      draw_line(line_index++,
                linef("  [%zu] %s %dx%d", i, name.c_str(), textures[i].width, textures[i].height),
                kDimColor);
    }
  }
  draw_line(line_index++, "F1 wire   F2 stats   F3 headlamp   [ ] light   arrows/PgU/PgD move",
            kDimColor);
}

} // namespace obzcene::overlay