#include <catch2/catch_all.hpp>

#include <obzcene/Math/Math.hpp>

static constexpr float PI = 3.14159265358979323846F;

TEST_CASE("Mat3 default construction", "[mat3]")
{
  obzcene::math::Mat3f m;
  REQUIRE(m(0, 0) == 1.0F);
  REQUIRE(m(1, 1) == 1.0F);
  REQUIRE(m(2, 2) == 1.0F);
  REQUIRE(m(0, 1) == 0.0F);
  REQUIRE(m(1, 0) == 0.0F);
}

TEST_CASE("Mat3 zero", "[mat3]")
{
  auto m = obzcene::math::Mat3f::zero();
  for (int i = 0; i < 9; ++i)
    REQUIRE(m.data[i] == 0.0F);
}

TEST_CASE("Mat3 from elements", "[mat3]")
{
  obzcene::math::Mat3f m(1.0F, 2.0F, 3.0F, 4.0F, 5.0F, 6.0F, 7.0F, 8.0F, 9.0F);
  REQUIRE(m(0, 0) == 1.0F);
  REQUIRE(m(0, 2) == 3.0F);
  REQUIRE(m(1, 1) == 5.0F);
  REQUIRE(m(2, 0) == 7.0F);
  REQUIRE(m(2, 2) == 9.0F);

  auto r = m.row(0);
  REQUIRE(r.x == 1.0F);
  REQUIRE(r.y == 2.0F);
  REQUIRE(r.z == 3.0F);

  auto c = m.col(1);
  REQUIRE(c.x == 2.0F);
  REQUIRE(c.y == 5.0F);
  REQUIRE(c.z == 8.0F);
}

TEST_CASE("Mat3 from array", "[mat3]")
{
  float       arr[9] = {9, 8, 7, 6, 5, 4, 3, 2, 1};
  obzcene::math::Mat3f m(arr);
  REQUIRE(m(0, 0) == 9.0F);
  REQUIRE(m(2, 2) == 1.0F);
}

TEST_CASE("Mat3 diagonal ctor", "[mat3]")
{
  obzcene::math::Mat3f m(5.0F);
  REQUIRE(m(0, 0) == 5.0F);
  REQUIRE(m(1, 1) == 5.0F);
  REQUIRE(m(2, 2) == 5.0F);
  REQUIRE(m(0, 1) == 0.0F);
}

TEST_CASE("Mat3 equality", "[mat3]")
{
  obzcene::math::Mat3f a(1, 2, 3, 4, 5, 6, 7, 8, 9);
  obzcene::math::Mat3f b(1, 2, 3, 4, 5, 6, 7, 8, 9);
  obzcene::math::Mat3f c(9, 8, 7, 6, 5, 4, 3, 2, 1);

  REQUIRE(a == b);
  REQUIRE_FALSE(a == c);
  REQUIRE(a != c);
  REQUIRE_FALSE(a != b);
}

TEST_CASE("Mat3 multiplication identity", "[mat3]")
{
  constexpr obzcene::math::Mat3f id;
  constexpr obzcene::math::Mat3f m(1, 2, 3, 4, 5, 6, 7, 8, 9);
  REQUIRE((id * m) == m);
  REQUIRE((m * id) == m);
}

TEST_CASE("Mat3 mat-vec multiply", "[mat3]")
{
  obzcene::math::Mat3f m(1, 0, 0, 0, 2, 0, 0, 0, 3);
  obzcene::math::Vec3f v{4.0F, 5.0F, 6.0F};
  auto        r = m * v;
  REQUIRE(r == obzcene::math::Vec3f{4.0F, 10.0F, 18.0F});
}

TEST_CASE("Mat3 mat-mat multiply", "[mat3]")
{
  obzcene::math::Mat3f a(1, 2, 3, 4, 5, 6, 7, 8, 9);
  obzcene::math::Mat3f b(9, 8, 7, 6, 5, 4, 3, 2, 1);
  auto        r = a * b;
  REQUIRE(r(0, 0) == 30.0F);
  REQUIRE(r(0, 1) == 24.0F);
  REQUIRE(r(0, 2) == 18.0F);
  REQUIRE(r(1, 0) == 84.0F);
  REQUIRE(r(1, 1) == 69.0F);
  REQUIRE(r(1, 2) == 54.0F);
  REQUIRE(r(2, 0) == 138.0F);
  REQUIRE(r(2, 1) == 114.0F);
  REQUIRE(r(2, 2) == 90.0F);
}

TEST_CASE("Mat3 arithmetic operators", "[mat3]")
{
  obzcene::math::Mat3f a(1, 2, 3, 4, 5, 6, 7, 8, 9);
  obzcene::math::Mat3f b(9, 8, 7, 6, 5, 4, 3, 2, 1);

  auto sum = a + b;
  REQUIRE(sum(0, 0) == 10.0F);
  REQUIRE(sum(2, 2) == 10.0F);

  auto diff = b - a;
  REQUIRE(diff(0, 0) == 8.0F);
  REQUIRE(diff(0, 2) == 4.0F);

  auto scaled = a * 2.0F;
  REQUIRE(scaled(0, 0) == 2.0F);
  REQUIRE(scaled(2, 2) == 18.0F);

  auto div = a / 2.0F;
  REQUIRE(div(0, 0) == Catch::Approx(0.5F));
  REQUIRE(div(2, 2) == Catch::Approx(4.5F));

  obzcene::math::Mat3f c = a;
  c *= b;
  REQUIRE(c == a * b);
}

TEST_CASE("Mat3 transpose", "[mat3]")
{
  obzcene::math::Mat3f m(1, 2, 3, 4, 5, 6, 7, 8, 9);
  auto        t = m.transpose();
  REQUIRE(t(0, 0) == 1.0F);
  REQUIRE(t(0, 1) == 4.0F);
  REQUIRE(t(0, 2) == 7.0F);
  REQUIRE(t(1, 0) == 2.0F);
  REQUIRE(t(1, 2) == 8.0F);
  REQUIRE(t(2, 0) == 3.0F);
  REQUIRE(t(2, 2) == 9.0F);

  REQUIRE((m.transpose().transpose()) == m);
}

TEST_CASE("Mat3 determinant", "[mat3]")
{
  obzcene::math::Mat3f id;
  REQUIRE(id.determinant() == Catch::Approx(1.0F));

  obzcene::math::Mat3f m(2, 0, 0, 0, 3, 0, 0, 0, 4);
  REQUIRE(m.determinant() == Catch::Approx(24.0F));

  obzcene::math::Mat3f singular(1, 2, 3, 4, 5, 6, 7, 8, 9);
  REQUIRE(singular.determinant() == Catch::Approx(0.0F));
}

TEST_CASE("Mat3 inverse", "[mat3]")
{
  obzcene::math::Mat3f m(2, 1, 0, 1, 3, 1, 0, 1, 4);
  auto        inv     = m.inverse();
  auto        product = m * inv;

  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j)
      REQUIRE(product(i, j) == Catch::Approx(i == j ? 1.0F : 0.0F).margin(1e-5F));
}

TEST_CASE("Mat3 scaling", "[mat3]")
{
  auto m = obzcene::math::Mat3f::scaling(2.0F, 3.0F, 4.0F);
  auto r = m * obzcene::math::Vec3f{1.0F, 1.0F, 1.0F};
  REQUIRE(r == obzcene::math::Vec3f{2.0F, 3.0F, 4.0F});
}

TEST_CASE("Mat3 rotation_x", "[mat3]")
{
  auto rx = obzcene::math::Mat3f::rotation_x(PI / 2.0F);
  auto r  = rx * obzcene::math::Vec3f{0.0F, 1.0F, 0.0F};
  REQUIRE(r.x == Catch::Approx(0.0F).margin(1e-5F));
  REQUIRE(r.y == Catch::Approx(0.0F).margin(1e-5F));
  REQUIRE(r.z == Catch::Approx(1.0F).margin(1e-5F));
}

TEST_CASE("Mat3 rotation_y", "[mat3]")
{
  auto ry = obzcene::math::Mat3f::rotation_y(PI / 2.0F);
  auto r  = ry * obzcene::math::Vec3f{0.0F, 0.0F, 1.0F};
  REQUIRE(r.x == Catch::Approx(1.0F).margin(1e-5F));
  REQUIRE(r.y == Catch::Approx(0.0F).margin(1e-5F));
  REQUIRE(r.z == Catch::Approx(0.0F).margin(1e-5F));
}

TEST_CASE("Mat3 rotation_z", "[mat3]")
{
  auto rz = obzcene::math::Mat3f::rotation_z(PI / 2.0F);
  auto r  = rz * obzcene::math::Vec3f{1.0F, 0.0F, 0.0F};
  REQUIRE(r.x == Catch::Approx(0.0F).margin(1e-5F));
  REQUIRE(r.y == Catch::Approx(1.0F).margin(1e-5F));
  REQUIRE(r.z == Catch::Approx(0.0F).margin(1e-5F));
}

TEST_CASE("Mat3 rotation_axis", "[mat3]")
{
  auto m = obzcene::math::Mat3f::rotation_axis(obzcene::math::Vec3f{0, 0, 1}, PI / 2.0F);
  auto r = m * obzcene::math::Vec3f{1.0F, 0.0F, 0.0F};
  REQUIRE(r.x == Catch::Approx(0.0F).margin(1e-5F));
  REQUIRE(r.y == Catch::Approx(1.0F).margin(1e-5F));
  REQUIRE(r.z == Catch::Approx(0.0F).margin(1e-5F));
}

TEST_CASE("Mat3 inverse rotation restores original", "[mat3]")
{
  auto rx      = obzcene::math::Mat3f::rotation_x(0.3F);
  auto ry      = obzcene::math::Mat3f::rotation_y(0.7F);
  auto rz      = obzcene::math::Mat3f::rotation_z(1.2F);
  auto m       = rx * ry * rz;
  auto inv     = m.inverse();
  auto product = inv * m;

  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j)
      REQUIRE(product(i, j) == Catch::Approx(i == j ? 1.0F : 0.0F).margin(1e-5F));
}

TEST_CASE("Mat3 is_almost_equal", "[mat3]")
{
  obzcene::math::Mat3f a(1, 2, 3, 4, 5, 6, 7, 8, 9);
  obzcene::math::Mat3f b(1, 2, 3, 4, 5, 6, 7, 8, 9.0000001F);
  obzcene::math::Mat3f c(1, 2, 3, 4, 5, 6, 7, 8, 100);
  REQUIRE(a.is_almost_equal(b));
  REQUIRE_FALSE(a.is_almost_equal(c));
}

TEST_CASE("Mat3 row and col accessor", "[mat3]")
{
  obzcene::math::Mat3f m(1, 2, 3, 4, 5, 6, 7, 8, 9);

  REQUIRE(m.row(0) == obzcene::math::Vec3f{1, 2, 3});
  REQUIRE(m.row(1) == obzcene::math::Vec3f{4, 5, 6});
  REQUIRE(m.row(2) == obzcene::math::Vec3f{7, 8, 9});

  REQUIRE(m.col(0) == obzcene::math::Vec3f{1, 4, 7});
  REQUIRE(m.col(1) == obzcene::math::Vec3f{2, 5, 8});
  REQUIRE(m.col(2) == obzcene::math::Vec3f{3, 6, 9});
}
