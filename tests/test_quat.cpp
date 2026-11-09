#include <catch2/catch_all.hpp>

#include <obzcene/Math/Math.hpp>

static constexpr float PI = 3.14159265358979323846F;

TEST_CASE("Quat default construction", "[quat]")
{
  obzcene::math::Quatf q;
  REQUIRE(q.w == 1.0F);
  REQUIRE(q.x == 0.0F);
  REQUIRE(q.y == 0.0F);
  REQUIRE(q.z == 0.0F);
}

TEST_CASE("Quat from components", "[quat]")
{
  obzcene::math::Quatf q{0.5F, 0.5F, 0.5F, 0.5F};
  REQUIRE(q.w == 0.5F);
  REQUIRE(q.x == 0.5F);
  REQUIRE(q.y == 0.5F);
  REQUIRE(q.z == 0.5F);
}

TEST_CASE("Quat from axis-angle", "[quat]")
{
  obzcene::math::Quatf q(PI, obzcene::math::Vec3f{0, 0, 1});
  REQUIRE(q.w == Catch::Approx(0.0F).margin(1e-5F));
  REQUIRE(q.z == Catch::Approx(1.0F).margin(1e-5F));
}

TEST_CASE("Quat equality", "[quat]")
{
  obzcene::math::Quatf a{1.0F, 2.0F, 3.0F, 4.0F};
  obzcene::math::Quatf b{1.0F, 2.0F, 3.0F, 4.0F};
  obzcene::math::Quatf c{4.0F, 3.0F, 2.0F, 1.0F};
  REQUIRE(a == b);
  REQUIRE_FALSE(a == c);
  REQUIRE(a != c);
}

TEST_CASE("Quat length", "[quat]")
{
  obzcene::math::Quatf q{1.0F, 0.0F, 0.0F, 0.0F};
  REQUIRE(q.length() == Catch::Approx(1.0F));
  REQUIRE(q.length_sq() == Catch::Approx(1.0F));

  obzcene::math::Quatf q2{1.0F, 2.0F, 3.0F, 4.0F};
  REQUIRE(q2.length() == Catch::Approx(std::sqrt(30.0F)));
}

TEST_CASE("Quat normalized", "[quat]")
{
  obzcene::math::Quatf q{1.0F, 2.0F, 3.0F, 4.0F};
  auto        n = q.normalized();
  REQUIRE(n.length() == Catch::Approx(1.0F));
  REQUIRE(n.w == Catch::Approx(1.0F / std::sqrt(30.0F)));
}

TEST_CASE("Quat conjugate", "[quat]")
{
  obzcene::math::Quatf q{1.0F, 2.0F, 3.0F, 4.0F};
  auto        c = q.conjugate();
  REQUIRE(c.w == 1.0F);
  REQUIRE(c.x == -2.0F);
  REQUIRE(c.y == -3.0F);
  REQUIRE(c.z == -4.0F);
}

TEST_CASE("Quat inverse", "[quat]")
{
  obzcene::math::Quatf q(PI / 4.0F, obzcene::math::Vec3f{0, 1, 0});
  auto        inv     = q.inverse();
  auto        product = q * inv;
  REQUIRE(product.w == Catch::Approx(1.0F).margin(1e-5F));
  REQUIRE(product.x == Catch::Approx(0.0F).margin(1e-5F));
  REQUIRE(product.y == Catch::Approx(0.0F).margin(1e-5F));
  REQUIRE(product.z == Catch::Approx(0.0F).margin(1e-5F));
}

TEST_CASE("Quat product identity", "[quat]")
{
  obzcene::math::Quatf id;
  obzcene::math::Quatf q{0.5F, 0.5F, 0.5F, 0.5F};
  REQUIRE((id * q) == q);
  REQUIRE((q * id) == q);
}

TEST_CASE("Quat rotation via multiplication", "[quat]")
{
  obzcene::math::Quatf q(PI / 2.0F, obzcene::math::Vec3f{0, 0, 1});
  obzcene::math::Vec3f v{1.0F, 0.0F, 0.0F};
  auto        r = q * v;
  REQUIRE(r.x == Catch::Approx(0.0F).margin(1e-5F));
  REQUIRE(r.y == Catch::Approx(1.0F).margin(1e-5F));
  REQUIRE(r.z == Catch::Approx(0.0F).margin(1e-5F));
}

TEST_CASE("Quat identity rotation preserves vector", "[quat]")
{
  obzcene::math::Quatf id;
  obzcene::math::Vec3f v{1.0F, 2.0F, 3.0F};
  auto        r = id * v;
  REQUIRE(r.x == Catch::Approx(1.0F));
  REQUIRE(r.y == Catch::Approx(2.0F));
  REQUIRE(r.z == Catch::Approx(3.0F));
}

TEST_CASE("Quat to_mat3 identity", "[quat]")
{
  obzcene::math::Quatf id;
  auto        m = id.to_mat3();
  REQUIRE(m(0, 0) == Catch::Approx(1.0F).margin(1e-5F));
  REQUIRE(m(1, 1) == Catch::Approx(1.0F).margin(1e-5F));
  REQUIRE(m(2, 2) == Catch::Approx(1.0F).margin(1e-5F));
  REQUIRE(m(0, 1) == Catch::Approx(0.0F).margin(1e-5F));
}

TEST_CASE("Quat to_mat3 90deg Z rotation", "[quat]")
{
  obzcene::math::Quatf q(PI / 2.0F, obzcene::math::Vec3f{0, 0, 1});
  auto        m = q.to_mat3();
  auto        r = m * obzcene::math::Vec3f{1, 0, 0};
  REQUIRE(r.x == Catch::Approx(0.0F).margin(1e-5F));
  REQUIRE(r.y == Catch::Approx(1.0F).margin(1e-5F));
  REQUIRE(r.z == Catch::Approx(0.0F).margin(1e-5F));
}

TEST_CASE("Quat to_mat4 identity", "[quat]")
{
  obzcene::math::Quatf id;
  auto        m = id.to_mat4();
  REQUIRE(m(0, 0) == Catch::Approx(1.0F).margin(1e-5F));
  REQUIRE(m(3, 3) == Catch::Approx(1.0F).margin(1e-5F));
  REQUIRE(m(0, 3) == Catch::Approx(0.0F).margin(1e-5F));
}

TEST_CASE("Quat to_mat4 matches to_mat3 upper-left", "[quat]")
{
  obzcene::math::Quatf q(0.7F, obzcene::math::Vec3f{1, 1, 0}.normalized());
  auto        m3 = q.to_mat3();
  auto        m4 = q.to_mat4();
  for (int r = 0; r < 3; ++r)
    for (int c = 0; c < 3; ++c)
      REQUIRE(m4(r, c) == Catch::Approx(m3(r, c)).margin(1e-5F));
}

TEST_CASE("Quat from_euler roundtrip", "[quat]")
{
  obzcene::math::Vec3f euler{0.3F, 0.5F, 0.7F};
  auto        q      = obzcene::math::Quatf::from_euler(euler.x, euler.y, euler.z);
  auto        euler2 = q.to_euler();
  REQUIRE(euler2.x == Catch::Approx(euler.x).margin(1e-5F));
  REQUIRE(euler2.y == Catch::Approx(euler.y).margin(1e-5F));
  REQUIRE(euler2.z == Catch::Approx(euler.z).margin(1e-5F));
}

TEST_CASE("Quat to_axis_angle roundtrip", "[quat]")
{
  obzcene::math::Vec3f axis{0.0F, 1.0F, 0.0F};
  float       angle = PI / 3.0F;
  obzcene::math::Quatf q(angle, axis);
  auto [out_axis, out_angle] = q.to_axis_angle();
  REQUIRE(out_angle == Catch::Approx(angle).margin(1e-5F));
  REQUIRE(out_axis.x == Catch::Approx(0.0F).margin(1e-5F));
  REQUIRE(out_axis.y == Catch::Approx(1.0F).margin(1e-5F));
  REQUIRE(out_axis.z == Catch::Approx(0.0F).margin(1e-5F));
}

TEST_CASE("Quat slerp identity", "[quat]")
{
  obzcene::math::Quatf a;
  auto        mid = obzcene::math::Quatf::slerp(a, a, 0.5F);
  REQUIRE(mid.is_almost_equal(a));
}

TEST_CASE("Quat slerp t=0 and t=1", "[quat]")
{
  obzcene::math::Quatf a(PI / 4.0F, obzcene::math::Vec3f{0, 1, 0});
  obzcene::math::Quatf b(PI / 2.0F, obzcene::math::Vec3f{0, 1, 0});
  auto        s0 = obzcene::math::Quatf::slerp(a, b, 0.0F);
  auto        s1 = obzcene::math::Quatf::slerp(a, b, 1.0F);
  REQUIRE(s0.is_almost_equal(a));
  REQUIRE(s1.is_almost_equal(b));
}

TEST_CASE("Quat nlerp t=0 and t=1", "[quat]")
{
  obzcene::math::Quatf a;
  obzcene::math::Quatf b(PI / 4.0F, obzcene::math::Vec3f{0, 1, 0});
  auto        n0 = obzcene::math::Quatf::nlerp(a, b, 0.0F);
  auto        n1 = obzcene::math::Quatf::nlerp(a, b, 1.0F);
  REQUIRE(n0.is_almost_equal(a));
  REQUIRE(n1.is_almost_equal(b));
}

TEST_CASE("Quat is_almost_equal", "[quat]")
{
  obzcene::math::Quatf a{1.0F, 2.0F, 3.0F, 4.0F};
  obzcene::math::Quatf b{1.0F, 2.0F, 3.0F, 4.0000001F};
  obzcene::math::Quatf c{1.0F, 2.0F, 3.0F, 100.0F};
  REQUIRE(a.is_almost_equal(b));
  REQUIRE_FALSE(a.is_almost_equal(c));
}

TEST_CASE("Quat double rotation", "[quat]")
{
  obzcene::math::Quatf q(PI / 4.0F, obzcene::math::Vec3f{0, 0, 1});
  auto        q2 = q * q;
  obzcene::math::Vec3f v{1, 0, 0};

  auto r1 = q2 * v;
  auto r2 = (q * q) * v;
  REQUIRE(r1.x == Catch::Approx(r2.x).margin(1e-5F));
  REQUIRE(r1.y == Catch::Approx(r2.y).margin(1e-5F));
  REQUIRE(r1.z == Catch::Approx(r2.z).margin(1e-5F));
}
