/**
 * @file Window.cpp
 * @brief SDL2 window/renderer implementation for the interactive viewer.
 *
 * Creates a resizable window with an accelerated renderer (falling back to
 * software), owns the streaming texture that mirrors the CPU framebuffer, and
 * pumps quit/escape/mouse events into the state the viewer reads. Events that
 * are handled here: SDL_QUIT, Escape, and relative mouse motion.
 *
 * @see Renderer/Window.hpp (declaration), Viewer/Viewer.cpp (consumer)
 */

#include "obzcene/Renderer/Window.hpp"

namespace obzcene
{

Window::Window(const std::string& title, int width, int height)
{
  if (SDL_Init(SDL_INIT_VIDEO) != 0)
  {
    return;
  }
  m_window = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width,
                              height, SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN);
  if (m_window == nullptr)
  {
    return;
  }
  m_renderer =
    SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (m_renderer == nullptr)
  {
    m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_SOFTWARE);
  }
  if (m_renderer == nullptr)
  {
    SDL_DestroyWindow(m_window);
    m_window = nullptr;
    return;
  }
  m_open = true;
}

Window::~Window()
{
  if (m_framebuffer != nullptr)
  {
    SDL_DestroyTexture(m_framebuffer);
  }
  if (m_renderer != nullptr)
  {
    SDL_DestroyRenderer(m_renderer);
  }
  if (m_window != nullptr)
  {
    SDL_DestroyWindow(m_window);
  }
  SDL_Quit();
}

auto Window::handle_events() -> void
{
  SDL_Event event;
  while (SDL_PollEvent(&event) != 0)
  {
    if (event.type == SDL_QUIT)
    {
      m_open = false;
    }
    else if (event.type == SDL_MOUSEMOTION)
    {
      m_mouse_dx += event.motion.xrel;
      m_mouse_dy += event.motion.yrel;
    }
  }
}

auto Window::present(const FrameBuffer& fb) -> void
{
  if (m_framebuffer == nullptr || m_framebuffer_width != fb.width() ||
      m_framebuffer_height != fb.height())
  {
    if (m_framebuffer != nullptr)
    {
      SDL_DestroyTexture(m_framebuffer);
      m_framebuffer = nullptr;
    }
    m_framebuffer = ensure_framebuffer(fb.width(), fb.height());
  }
  if (m_framebuffer == nullptr)
  {
    return;
  }
  SDL_UpdateTexture(m_framebuffer, nullptr, fb.color_data(), fb.width() * 4);
  SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 255);
  SDL_RenderClear(m_renderer);
  SDL_RenderCopy(m_renderer, m_framebuffer, nullptr, nullptr);
  SDL_RenderPresent(m_renderer);
}

auto Window::is_open() const -> bool { return m_open; }

auto Window::close() -> void { m_open = false; }

auto Window::key(SDL_Scancode scancode) const -> bool
{
  const auto* state = SDL_GetKeyboardState(nullptr);
  return state != nullptr && state[scancode] != 0;
}

auto Window::take_mouse_delta() -> std::pair<long, long>
{
  auto delta = std::make_pair(m_mouse_dx, m_mouse_dy);
  m_mouse_dx = 0;
  m_mouse_dy = 0;
  return delta;
}

auto Window::width() const -> int
{
  int w = 0;
  int h = 0;
  SDL_GetWindowSize(m_window, &w, &h);
  return w;
}

auto Window::height() const -> int
{
  int w = 0;
  int h = 0;
  SDL_GetWindowSize(m_window, &w, &h);
  return h;
}

auto Window::set_relative_mouse(bool enabled) -> void
{
  SDL_SetRelativeMouseMode(enabled ? SDL_TRUE : SDL_FALSE);
}

auto Window::ensure_framebuffer(int width, int height) -> SDL_Texture*
{
  if (m_renderer == nullptr)
  {
    return nullptr;
  }
  m_framebuffer_width  = width;
  m_framebuffer_height = height;
  auto* texture = SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING,
                                    width, height);
  return texture;
}

} // namespace obzcene