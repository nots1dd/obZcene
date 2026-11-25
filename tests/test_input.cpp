#include <obzcene/Engine/Input.hpp>

#include <catch2/catch_test_macros.hpp>

#include <array>

namespace
{

constexpr auto key_count = static_cast<obzcene::usz>(obzcene::Key::Count);

auto press(obzcene::Input& input, std::array<std::uint8_t, key_count>& state, obzcene::Key key,
           bool down) -> void
{
  state[static_cast<obzcene::usz>(key)] = down ? 1 : 0;
  input.begin_frame(state.data());
}

} // namespace

TEST_CASE("input: down/pressed/released edges", "[input]")
{
  auto                                input = obzcene::Input{};
  std::array<std::uint8_t, key_count> state{};

  // Frame 1: key goes down.
  press(input, state, obzcene::Key::Space, true);
  REQUIRE(input.down(obzcene::Key::Space));
  REQUIRE(input.pressed(obzcene::Key::Space));
  REQUIRE_FALSE(input.released(obzcene::Key::Space));

  // Frame 2: held — no edge.
  press(input, state, obzcene::Key::Space, true);
  REQUIRE(input.down(obzcene::Key::Space));
  REQUIRE_FALSE(input.pressed(obzcene::Key::Space));

  // Frame 3: released.
  press(input, state, obzcene::Key::Space, false);
  REQUIRE_FALSE(input.down(obzcene::Key::Space));
  REQUIRE(input.released(obzcene::Key::Space));

  // Frame 4: fully idle.
  press(input, state, obzcene::Key::Space, false);
  REQUIRE_FALSE(input.released(obzcene::Key::Space));
}

TEST_CASE("input: unrelated keys are independent", "[input]")
{
  auto                                input = obzcene::Input{};
  std::array<std::uint8_t, key_count> state{};
  press(input, state, obzcene::Key::W, true);
  REQUIRE(input.down(obzcene::Key::W));
  REQUIRE_FALSE(input.down(obzcene::Key::A));
  REQUIRE_FALSE(input.pressed(obzcene::Key::A));
}

TEST_CASE("input: mouse delta accumulates and resets", "[input]")
{
  auto input = obzcene::Input{};
  input.add_mouse_delta(1.5F, -2.0F);
  input.add_mouse_delta(0.5F, 1.0F);
  auto [dx, dy] = input.mouse_delta();
  REQUIRE(dx == 2.0F);
  REQUIRE(dy == -1.0F);
  input.reset_mouse_delta();
  auto [dx2, dy2] = input.mouse_delta();
  REQUIRE(dx2 == 0.0F);
  REQUIRE(dy2 == 0.0F);
}

TEST_CASE("input: mouse buttons track down/pressed/released", "[input][mouse]")
{
  constexpr auto btn_count = static_cast<obzcene::usz>(obzcene::MouseButton::Count);
  auto           input     = obzcene::Input{};
  std::array<std::uint8_t, btn_count> buttons{};

  buttons[static_cast<obzcene::usz>(obzcene::MouseButton::Left)] = 1;
  input.begin_frame_buttons(buttons.data());
  REQUIRE(input.down(obzcene::MouseButton::Left));
  REQUIRE(input.pressed(obzcene::MouseButton::Left));
  REQUIRE_FALSE(input.released(obzcene::MouseButton::Left));
  REQUIRE_FALSE(input.down(obzcene::MouseButton::Right));

  input.begin_frame_buttons(buttons.data());
  REQUIRE(input.down(obzcene::MouseButton::Left));
  REQUIRE_FALSE(input.pressed(obzcene::MouseButton::Left));

  buttons[static_cast<obzcene::usz>(obzcene::MouseButton::Left)] = 0;
  input.begin_frame_buttons(buttons.data());
  REQUIRE_FALSE(input.down(obzcene::MouseButton::Left));
  REQUIRE(input.released(obzcene::MouseButton::Left));

  input.begin_frame_buttons(buttons.data());
  REQUIRE_FALSE(input.released(obzcene::MouseButton::Left));
}

TEST_CASE("input: mouse buttons are independent per button", "[input][mouse]")
{
  constexpr auto btn_count = static_cast<obzcene::usz>(obzcene::MouseButton::Count);
  auto           input     = obzcene::Input{};
  std::array<std::uint8_t, btn_count> buttons{};
  buttons[static_cast<obzcene::usz>(obzcene::MouseButton::Left)]  = 1;
  buttons[static_cast<obzcene::usz>(obzcene::MouseButton::Right)] = 1;
  input.begin_frame_buttons(buttons.data());
  REQUIRE(input.down(obzcene::MouseButton::Left));
  REQUIRE(input.down(obzcene::MouseButton::Right));
  REQUIRE_FALSE(input.down(obzcene::MouseButton::Middle));
}
