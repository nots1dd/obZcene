#include <catch2/catch_all.hpp>

#include <obzcene/Math/Math.hpp>

TEST_CASE("Vec2 construction and access", "[vec2]")
{
  obzcene::math::Vec2f v;
  REQUIRE(v.x == 0.0F);
  REQUIRE(v.y == 0.0F);

  obzcene::math::Vec2f val{3.0F, 4.0F};
  REQUIRE(val.x == 3.0F);
  REQUIRE(val.y == 4.0F);

  obzcene::math::Vec2f splat{7.0F};
  REQUIRE(splat.x == 7.0F);
  REQUIRE(splat.y == 7.0F);

  REQUIRE(obzcene::math::Vec2f::zero().x == 0.0F);
  REQUIRE(obzcene::math::Vec2f::one().x == 1.0F);

  REQUIRE(val[0] == 3.0F);
  REQUIRE(val[1] == 4.0F);

  obzcene::math::Vec2i ci = val.cast_to<int>();
  REQUIRE(ci.x == 3);
  REQUIRE(ci.y == 4);

  REQUIRE(val.is_almost_equal(obzcene::math::Vec2f{3.0F, 4.0F}));
  REQUIRE_FALSE(val.is_almost_equal(obzcene::math::Vec2f{5.0F, 4.0F}));
}

TEST_CASE("Vec2 arithmetic", "[vec2]")
{
  constexpr obzcene::math::Vec2f a{1.0F, 2.0F};
  constexpr obzcene::math::Vec2f b{3.0F, 4.0F};

  REQUIRE((a + b) == obzcene::math::Vec2f{4.0F, 6.0F});
  REQUIRE((b - a) == obzcene::math::Vec2f{2.0F, 2.0F});
  REQUIRE((-a) == obzcene::math::Vec2f{-1.0F, -2.0F});
  REQUIRE((a * 2.0F) == obzcene::math::Vec2f{2.0F, 4.0F});
  REQUIRE((2.0F * a) == obzcene::math::Vec2f{2.0F, 4.0F});
  REQUIRE((b / 2.0F) == obzcene::math::Vec2f{1.5F, 2.0F});
  REQUIRE((a * b) == obzcene::math::Vec2f{3.0F, 8.0F});
  REQUIRE((b / a) == obzcene::math::Vec2f{3.0F, 2.0F});
  REQUIRE(a == obzcene::math::Vec2f{1.0F, 2.0F});
  REQUIRE(a != b);

  obzcene::math::Vec2f m{1.0F, 1.0F};
  m += obzcene::math::Vec2f{1.0F, 2.0F};
  m -= obzcene::math::Vec2f{0.5F, 0.5F};
  m *= 2.0F;
  m /= 4.0F;
  REQUIRE(m.is_almost_equal(obzcene::math::Vec2f{0.75F, 1.25F}));
}

TEST_CASE("Vec2 geometry", "[vec2]")
{
  constexpr obzcene::math::Vec2f v{3.0F, 4.0F};
  REQUIRE(v.length_sq() == 25.0F);
  REQUIRE(v.length() == Catch::Approx(5.0F));

  obzcene::math::Vec2f n = v.normalized();
  REQUIRE(n.length() == Catch::Approx(1.0F));
  REQUIRE(n.x == Catch::Approx(0.6F));
  REQUIRE(n.y == Catch::Approx(0.8F));

  REQUIRE(obzcene::math::Vec2f::zero().normalized().length() == 0.0F);

  REQUIRE(v.dot(obzcene::math::Vec2f{1.0F, 1.0F}) == 7.0F);
  REQUIRE(obzcene::math::Vec2f{1.0F, 0.0F}.cross(obzcene::math::Vec2f{0.0F, 1.0F}) == 1.0F);
  REQUIRE(obzcene::math::distance(obzcene::math::Vec2f{3.0F, 4.0F}, obzcene::math::Vec2f{0.0F, 0.0F}) == Catch::Approx(5.0F));
}

TEST_CASE("Vec2 free functions", "[vec2]")
{
  constexpr obzcene::math::Vec2f a{1.0F, 2.0F};
  constexpr obzcene::math::Vec2f b{3.0F, 4.0F};

  REQUIRE(obzcene::math::dot(a, b) == 11.0F);
  REQUIRE(obzcene::math::length_sq(a) == 5.0F);
  REQUIRE(obzcene::math::distance(a, b) == obzcene::math::Vec2f{2.0F, 2.0F}.length());

  auto lr = obzcene::math::lerp(a, b, 0.5F);
  REQUIRE(lr.x == Catch::Approx(2.0F));
  REQUIRE(lr.y == Catch::Approx(3.0F));

  REQUIRE(obzcene::math::min(a, b) == a);
  REQUIRE(obzcene::math::max(a, b) == b);
  REQUIRE(obzcene::math::clamp(obzcene::math::Vec2f{-1.0F, 5.0F}, 0.0F, 3.0F) == obzcene::math::Vec2f{0.0F, 3.0F});
}

TEST_CASE("Vec3 construction and access", "[vec3]")
{
  obzcene::math::Vec3f v;
  REQUIRE(v.x == 0.0F);
  REQUIRE(v.y == 0.0F);
  REQUIRE(v.z == 0.0F);

  obzcene::math::Vec3f val{1.0F, 2.0F, 3.0F};
  REQUIRE(val[0] == 1.0F);
  REQUIRE(val[1] == 2.0F);
  REQUIRE(val[2] == 3.0F);

  REQUIRE(obzcene::math::Vec3f::one().x == 1.0F);
  REQUIRE(val.xy() == obzcene::math::Vec2f{1.0F, 2.0F});
}

TEST_CASE("Vec3 arithmetic", "[vec3]")
{
  constexpr obzcene::math::Vec3f a{1.0F, 2.0F, 3.0F};
  constexpr obzcene::math::Vec3f b{4.0F, 5.0F, 6.0F};

  REQUIRE((a + b) == obzcene::math::Vec3f{5.0F, 7.0F, 9.0F});
  REQUIRE((b - a) == obzcene::math::Vec3f{3.0F, 3.0F, 3.0F});
  REQUIRE((-a) == obzcene::math::Vec3f{-1.0F, -2.0F, -3.0F});
  REQUIRE((a * 2.0F) == obzcene::math::Vec3f{2.0F, 4.0F, 6.0F});
  REQUIRE((2.0F * a) == obzcene::math::Vec3f{2.0F, 4.0F, 6.0F});
  REQUIRE((b / 2.0F) == obzcene::math::Vec3f{2.0F, 2.5F, 3.0F});
}

TEST_CASE("Vec3 geometry", "[vec3]")
{
  constexpr obzcene::math::Vec3f v{1.0F, 2.0F, 3.0F};
  REQUIRE(v.length_sq() == 14.0F);
  REQUIRE(v.length() == Catch::Approx(std::sqrt(14.0F)));

  obzcene::math::Vec3f n = v.normalized();
  REQUIRE(n.length() == Catch::Approx(1.0F));

  constexpr obzcene::math::Vec3f x{1.0F, 0.0F, 0.0F};
  constexpr obzcene::math::Vec3f y{0.0F, 1.0F, 0.0F};
  constexpr obzcene::math::Vec3f z{0.0F, 0.0F, 1.0F};

  REQUIRE(x.cross(y) == z);
  REQUIRE(y.cross(z) == x);
  REQUIRE(z.cross(x) == y);
  REQUIRE(y.cross(x) == -z);

  REQUIRE(x.dot(y) == 0.0F);
  REQUIRE(obzcene::math::Vec3f{1.0F, 2.0F, 3.0F}.dot(obzcene::math::Vec3f{4.0F, 5.0F, 6.0F}) == 32.0F);
}

TEST_CASE("Vec4 construction and access", "[vec4]")
{
  constexpr obzcene::math::Vec4f v{1.0F, 2.0F, 3.0F, 4.0F};
  REQUIRE(v[0] == 1.0F);
  REQUIRE(v[3] == 4.0F);
  REQUIRE(v.xyz() == obzcene::math::Vec3f{1.0F, 2.0F, 3.0F});
  REQUIRE(v.xy() == obzcene::math::Vec2f{1.0F, 2.0F});
  REQUIRE(v.length_sq() == 30.0F);
}

TEST_CASE("Vec4 arithmetic", "[vec4]")
{
  constexpr obzcene::math::Vec4f a{1.0F, 2.0F, 3.0F, 4.0F};
  constexpr obzcene::math::Vec4f b{5.0F, 6.0F, 7.0F, 8.0F};

  REQUIRE((a + b) == obzcene::math::Vec4f{6.0F, 8.0F, 10.0F, 12.0F});
  REQUIRE((b - a) == obzcene::math::Vec4f{4.0F, 4.0F, 4.0F, 4.0F});
  REQUIRE((a * 2.0F) == obzcene::math::Vec4f{2.0F, 4.0F, 6.0F, 8.0F});
  REQUIRE((-a) == obzcene::math::Vec4f{-1.0F, -2.0F, -3.0F, -4.0F});
}
