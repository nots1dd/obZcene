#pragma once

/**
 * @file FileBrowser.hpp
 * @brief Directory-listing file picker used by the editor overlay.
 *
 * A tiny state machine: it owns a current directory, a filtered, sorted
 * listing of its entries (directories first, then files), and a cursor. The
 * viewer drives it with navigation calls and either cancels or confirms a
 * selection with #take_selected. Rendering lives in Viewer/Overlay.hpp
 * (draw_browser), so this type is UI-free and unit-testable.
 *
 * @see Viewer/Overlay.hpp (draw_browser), src/Viewer/Viewer.cpp (driver)
 */

#include <algorithm>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace obzcene
{

/**
 * @brief A navigable directory listing with a selection cursor.
 */
class FileBrowser
{
public:
  /** @brief One row in the listing. */
  struct Entry
  {
    std::filesystem::path path; ///< Full path.
    std::string           name; ///< Display name (filename only).
    bool                  is_dir{false};
  };

  FileBrowser() = default;

  /**
   * @brief Open the browser at `start_dir` (falls back to the current
   *        working directory when the path does not exist).
   * @param extension  Only files with this extension are listed (".obj").
   */
  auto open(const std::filesystem::path& start_dir, std::string extension) -> void
  {
    m_extension = std::move(extension);
    m_dir       = std::filesystem::exists(start_dir) && std::filesystem::is_directory(start_dir)
                    ? start_dir
                    : std::filesystem::current_path();
    m_open      = true;
    refresh();
  }

  /** @brief Close without selecting anything. */
  auto close() -> void
  {
    m_open = false;
    m_selected.reset();
  }

  /** @brief True while the browser is shown. */
  [[nodiscard]] auto is_open() const -> bool { return m_open; }

  /** @brief Current directory. */
  [[nodiscard]] auto current_dir() const -> const std::filesystem::path& { return m_dir; }

  /** @brief Current listing (sorted: directories first, then files). */
  [[nodiscard]] auto entries() const -> const std::vector<Entry>& { return m_entries; }

  /** @brief Cursor row, always < entries().size() when non-empty. */
  [[nodiscard]] auto cursor() const -> std::size_t { return m_cursor; }

  /** @brief Move the cursor by `delta` rows, clamped to the listing. */
  auto move_cursor(int delta) -> void
  {
    if (m_entries.empty())
    {
      m_cursor = 0;
      return;
    }
    const auto next = static_cast<long>(m_cursor) + delta;
    m_cursor =
      static_cast<std::size_t>(std::clamp(next, 0L, static_cast<long>(m_entries.size()) - 1L));
  }

  /**
   * @brief Activate the entry under the cursor: descend into directories, or
   *        pick a file (records it and closes the browser).
   * @note The picked file is read via #take_selected.
   */
  auto activate() -> void
  {
    if (m_cursor >= m_entries.size())
    {
      return;
    }
    const auto& entry = m_entries[m_cursor];
    if (entry.is_dir)
    {
      m_dir = entry.path;
      refresh();
      return;
    }
    m_selected = entry.path;
    m_open     = false;
  }

  /** @brief Go to the parent directory (no-op at the filesystem root). */
  auto go_parent() -> void
  {
    if (m_dir.has_parent_path() && m_dir.parent_path() != m_dir)
    {
      m_dir = m_dir.parent_path();
      refresh();
    }
  }

  /**
   * @brief Consume the picked file, if any.
   * @return The picked path once; std::nullopt afterwards.
   */
  [[nodiscard]] auto take_selected() -> std::optional<std::filesystem::path>
  {
    auto out = std::move(m_selected);
    m_selected.reset();
    return out;
  }

private:
  /** @brief Rebuild the listing for the current directory. */
  auto refresh() -> void
  {
    m_entries.clear();
    auto entries = std::vector<Entry>{};
    auto ec      = std::error_code{};
    for (const auto& item : std::filesystem::directory_iterator(m_dir, ec))
    {
      if (ec)
      {
        break;
      }
      const auto is_dir = item.is_directory(ec);
      if (!is_dir && item.path().extension() != m_extension)
      {
        continue;
      }
      entries.push_back(Entry{item.path(), item.path().filename().string(), is_dir});
    }
    std::sort(entries.begin(), entries.end(),
              [](const Entry& a, const Entry& b)
              {
                if (a.is_dir != b.is_dir)
                {
                  return a.is_dir; // directories first
                }
                return a.name < b.name;
              });
    m_entries = std::move(entries);
    m_cursor  = 0;
  }

  std::filesystem::path                m_dir;
  std::string                          m_extension{".obj"};
  std::vector<Entry>                   m_entries;
  std::size_t                          m_cursor{0};
  std::optional<std::filesystem::path> m_selected;
  bool                                 m_open{false};
};

} // namespace obzcene
