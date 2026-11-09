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
#include "obzcene/Viewer/FileBrowser.hpp"
#include "obzcene/Viewer/Scene.hpp"

#include <algorithm>
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

auto draw_hud(FrameBuffer& fb, const HudInput& input) -> void
{
  constexpr Color kTextColor{200, 205, 214};
  constexpr Color kDimColor{120, 124, 132};

  const auto& scene       = *input.scene;
  const auto& stats       = *input.stats;
  const auto& lights      = *input.lights;
  const auto  pos         = input.camera->position();
  const auto  draw_width  = fb.width();
  const auto  draw_height = fb.height();

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

  auto total_tris  = usz{0};
  auto total_verts = usz{0};
  for (const auto& object : scene.objects)
  {
    if (object.model == nullptr)
    {
      continue;
    }
    total_tris += object.model->mesh.triangles.size();
    total_verts += object.model->mesh.vertices.size();
  }

  const auto lines = std::vector<std::string>{
    linef("obZcene  %dx%d  %5.1f fps  %6.2f ms", draw_width, draw_height, input.fps,
          input.frame_ms),
    linef("scene: %zu objects  %zu tris  %zu verts", scene.objects.size(), total_tris, total_verts),
    linef("render: proc %zu  bf %zu  vp %zu  near %zu  deg %zu  drawn %zu",
          stats.triangles_processed, stats.triangles_culled_backface,
          stats.triangles_culled_viewport, stats.triangles_culled_nearfar,
          stats.triangles_degenerate, stats.triangles_rendered),
    linef("pixels: shaded %zu  written %zu", stats.pixels_shaded, stats.pixels_written),
    linef("camera: (%.2f,%.2f,%.2f)  mode %s", pos.x, pos.y, pos.z,
          input.wireframe ? "wireframe" : "solid"),
    linef("lights: %zu (%zu dir, %zu point, %zu spot)", lights.size(), directional, point, spot),
  };

  auto light_info = std::string{};
  if (input.headlamp)
  {
    light_info = "headlamp";
    if (input.active_light < lights.size())
    {
      light_info += " + ";
    }
  }
  if (input.active_light < lights.size())
  {
    const auto& active = lights[input.active_light];
    if (light_info.empty())
    {
      light_info = linef("[%zu]", input.active_light);
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

  if (input.selected_object >= 0 && static_cast<usz>(input.selected_object) < scene.objects.size())
  {
    const auto& object = scene.objects[static_cast<usz>(input.selected_object)];
    draw_line(line_index++,
              linef("sel [%d] %s  pos(%.2f,%.2f,%.2f)  yaw %.1f  scale %.2f", input.selected_object,
                    object.name.c_str(), object.position.x, object.position.y, object.position.z,
                    math::rad_to_deg(object.yaw), object.scale),
              kTextColor);
  }

  draw_line(line_index++, "F1 wire  F2 stats  F3 headlamp  F4 add  Tab select  G grab  Del remove",
            kDimColor);
  draw_line(line_index++, "F5 save  F9 load  [] light  arrows/Home/End move", kDimColor);

  const auto selection =
    input.selected_object >= 0 ? linef("#%d", input.selected_object) : std::string{};
  draw_status_bar(fb, input.mode, selection, input.message);
}

auto fill_rect(FrameBuffer& fb, int x, int y, int width, int height, const Color& color) -> void
{
  const auto x0 = std::max(x, 0);
  const auto y0 = std::max(y, 0);
  const auto x1 = std::min(x + width, fb.width());
  const auto y1 = std::min(y + height, fb.height());
  for (auto py = y0; py < y1; ++py)
  {
    for (auto px = x0; px < x1; ++px)
    {
      fb.at(px, py) = color;
    }
  }
}

auto draw_status_bar(FrameBuffer& fb, const std::string& mode, const std::string& selection,
                     const std::string& message) -> void
{
  constexpr Color kBarColor{30, 32, 40};
  constexpr Color kModeColor{140, 220, 160};
  constexpr Color kTextColor{200, 205, 214};

  const auto bar_height = kGlyphHeight + 8;
  const auto bar_y      = fb.height() - bar_height;
  fill_rect(fb, 0, bar_y, fb.width(), bar_height, kBarColor);

  auto       text = std::string{"["} + mode + "]";
  const auto sep  = std::string{"  |  "};
  if (!selection.empty())
  {
    text += sep + "sel: " + selection;
  }
  if (!message.empty())
  {
    text += sep + message;
  }
  draw_text(fb, 8, bar_y + 4, text, kModeColor);
}

auto draw_browser(FrameBuffer& fb, const FileBrowser& browser) -> void
{
  constexpr Color kPanelColor{24, 26, 34};
  constexpr Color kTitleColor{140, 200, 255};
  constexpr Color kTextColor{200, 205, 214};
  constexpr Color kDimColor{120, 124, 132};
  constexpr Color kCursorColor{255, 220, 120};

  const auto  row_height  = kGlyphHeight + 4;
  const auto& entries     = browser.entries();
  const auto  max_visible = std::max(1, (fb.height() - 120) / row_height);
  const auto  panel_width =
    std::clamp(text_width(browser.current_dir().string()) + 40, 260, fb.width() - 40);
  const auto panel_height =
    row_height * static_cast<int>(std::min(entries.size(), static_cast<usz>(max_visible))) + 60;
  const auto panel_x = std::max(0, (fb.width() - panel_width) / 2);
  const auto panel_y = std::max(0, (fb.height() - panel_height) / 2);

  fill_rect(fb, panel_x, panel_y, panel_width, panel_height, kPanelColor);
  draw_text(fb, panel_x + 10, panel_y + 8, browser.current_dir().string(), kTitleColor);

  if (entries.empty())
  {
    draw_text(fb, panel_x + 10, panel_y + 8 + row_height, "(no .obj files here)", kDimColor);
  }

  // Scroll window: keep the cursor row inside the visible range.
  const auto visible = std::min(entries.size(), static_cast<usz>(max_visible));
  const auto first   = browser.cursor() >= visible ? browser.cursor() - visible + 1 : usz{0};
  for (usz row = 0; row < visible; ++row)
  {
    const auto& entry     = entries[first + row];
    const auto  y         = panel_y + 8 + row_height * (static_cast<int>(row) + 1);
    const auto  is_cursor = first + row == browser.cursor();
    draw_text(fb, panel_x + 10, y, is_cursor ? ">" : " ", is_cursor ? kCursorColor : kDimColor);
    draw_text(fb, panel_x + 10 + kGlyphAdvance, y, entry.name + (entry.is_dir ? "/" : ""),
              is_cursor ? kCursorColor : kTextColor);
  }

  draw_text(fb, panel_x + 10, panel_y + panel_height - row_height - 6,
            "up/down: move  enter: open/pick  backspace: up  F4: close", kDimColor);
}

} // namespace obzcene::overlay