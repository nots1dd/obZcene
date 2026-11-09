#include <catch2/catch_all.hpp>

#include <obzcene/Math/Math.hpp>

static constexpr float PI = 3.14159265358979323846F;

TEST_CASE("Mat4 default construction", "[mat4]")
{
  obzcene::math::Mat4f m;
  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 4; ++j)
      REQUIRE(m(i, j) == Catch::Approx(i == j ? 1.0F : 0.0F));
}

TEST_CASE("Mat4 zero", "[mat4]")
{
  auto m = obzcene::math::Mat4f::zero();
  for (int i = 0; i < 16; ++i)
    REQUIRE(m.data[i] == 0.0F);
}

TEST_CASE("Mat4 from elements", "[mat4]")
{
  obzcene::math::Mat4f m(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
  REQUIRE(m(0, 0) == 1.0F);
  REQUIRE(m(0, 3) == 4.0F);
  REQUIRE(m(1, 0) == 5.0F);
  REQUIRE(m(3, 3) == 16.0F);

  REQUIRE(m.row(0) == obzcene::math::Vec4f{1, 2, 3, 4});
  REQUIRE(m.col(3) == obzcene::math::Vec4f{4, 8, 12, 16});
}

TEST_CASE("Mat4 equality", "[mat4]")
{
  obzcene::math::Mat4f a(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
  obzcene::math::Mat4f b(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
  obzcene::math::Mat4f c;
  REQUIRE(a == b);
  REQUIRE_FALSE(a == c);
  REQUIRE(a != c);
}

TEST_CASE("Mat4 mat-vec multiply", "[mat4]")
{
  constexpr obzcene::math::Mat4f id;
  constexpr obzcene::math::Vec4f v{1.0F, 2.0F, 3.0F, 1.0F};
  auto                  r = id * v;
  REQUIRE(r == v);
}

TEST_CASE("Mat4 translation", "[mat4]")
{
  auto        t = obzcene::math::Mat4f::translation(10.0F, 20.0F, 30.0F);
  obzcene::math::Vec4f p{1.0F, 2.0F, 3.0F, 1.0F};
  auto        r = t * p;
  REQUIRE(r.x == Catch::Approx(11.0F));
  REQUIRE(r.y == Catch::Approx(22.0F));
  REQUIRE(r.z == Catch::Approx(33.0F));
  REQUIRE(r.w == Catch::Approx(1.0F));
}

TEST_CASE("Mat4 translation does not affect direction", "[mat4]")
{
  auto        t = obzcene::math::Mat4f::translation(100.0F, 200.0F, 300.0F);
  obzcene::math::Vec4f dir{1.0F, 0.0F, 0.0F, 0.0F};
  auto        r = t * dir;
  REQUIRE(r == dir);
}

TEST_CASE("Mat4 scaling", "[mat4]")
{
  auto        s = obzcene::math::Mat4f::scaling(2.0F, 3.0F, 4.0F);
  obzcene::math::Vec4f v{1.0F, 1.0F, 1.0F, 1.0F};
  auto        r = s * v;
  REQUIRE(r == obzcene::math::Vec4f{2.0F, 3.0F, 4.0F, 1.0F});
}

TEST_CASE("Mat4 rotation_x", "[mat4]")
{
  auto        rx = obzcene::math::Mat4f::rotation_x(PI / 2.0F);
  obzcene::math::Vec4f v{0.0F, 1.0F, 0.0F, 1.0F};
  auto        r = rx * v;
  REQUIRE(r.x == Catch::Approx(0.0F).margin(1e-5F));
  REQUIRE(r.y == Catch::Approx(0.0F).margin(1e-5F));
  REQUIRE(r.z == Catch::Approx(1.0F).margin(1e-5F));
  REQUIRE(r.w == Catch::Approx(1.0F));
}

TEST_CASE("Mat4 rotation_y", "[mat4]")
{
  auto        ry = obzcene::math::Mat4f::rotation_y(PI / 2.0F);
  obzcene::math::Vec4f v{0.0F, 0.0F, 1.0F, 1.0F};
  auto        r = ry * v;
  REQUIRE(r.x == Catch::Approx(1.0F).margin(1e-5F));
  REQUIRE(r.y == Catch::Approx(0.0F).margin(1e-5F));
  REQUIRE(r.z == Catch::Approx(0.0F).margin(1e-5F));
  REQUIRE(r.w == Catch::Approx(1.0F));
}

TEST_CASE("Mat4 rotation_z", "[mat4]")
{
  auto        rz = obzcene::math::Mat4f::rotation_z(PI / 2.0F);
  obzcene::math::Vec4f v{1.0F, 0.0F, 0.0F, 1.0F};
  auto        r = rz * v;
  REQUIRE(r.x == Catch::Approx(0.0F).margin(1e-5F));
  REQUIRE(r.y == Catch::Approx(1.0F).margin(1e-5F));
  REQUIRE(r.z == Catch::Approx(0.0F).margin(1e-5F));
  REQUIRE(r.w == Catch::Approx(1.0F));
}

TEST_CASE("Mat4 rotation_axis", "[mat4]")
{
  auto        m = obzcene::math::Mat4f::rotation_axis(obzcene::math::Vec3f{0, 0, 1}, PI / 2.0F);
  obzcene::math::Vec4f v{1.0F, 0.0F, 0.0F, 1.0F};
  auto        r = m * v;
  REQUIRE(r.x == Catch::Approx(0.0F).margin(1e-5F));
  REQUIRE(r.y == Catch::Approx(1.0F).margin(1e-5F));
  REQUIRE(r.z == Catch::Approx(0.0F).margin(1e-5F));
}

TEST_CASE("Mat4 determinant", "[mat4]")
{
  REQUIRE(obzcene::math::Mat4f::identity().determinant() == Catch::Approx(1.0F));

  auto s = obzcene::math::Mat4f::scaling(2.0F, 3.0F, 4.0F);
  REQUIRE(s.determinant() == Catch::Approx(24.0F));
}

TEST_CASE("Mat4 inverse", "[mat4]")
{
  auto m       = obzcene::math::Mat4f::translation(3.0F, 4.0F, 5.0F);
  auto inv     = m.inverse();
  auto product = m * inv;

  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 4; ++j)
      REQUIRE(product(i, j) == Catch::Approx(i == j ? 1.0F : 0.0F).margin(1e-5F));
}

TEST_CASE("Mat4 inverse scaling", "[mat4]")
{
  auto s       = obzcene::math::Mat4f::scaling(2.0F, 3.0F, 4.0F);
  auto inv     = s.inverse();
  auto product = s * inv;

  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 4; ++j)
      REQUIRE(product(i, j) == Catch::Approx(i == j ? 1.0F : 0.0F).margin(1e-5F));
}

TEST_CASE("Mat4 inverse rotation", "[mat4]")
{
  auto r =
    obzcene::math::Mat4f::rotation_x(0.7F) * obzcene::math::Mat4f::rotation_y(1.2F) * obzcene::math::Mat4f::rotation_z(0.4F);
  auto inv     = r.inverse();
  auto product = inv * r;

  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 4; ++j)
      REQUIRE(product(i, j) == Catch::Approx(i == j ? 1.0F : 0.0F).margin(1e-5F));
}

TEST_CASE("Mat4 transpose", "[mat4]")
{
  obzcene::math::Mat4f m(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
  auto        t = m.transpose();
  REQUIRE(t(0, 0) == 1.0F);
  REQUIRE(t(0, 1) == 5.0F);
  REQUIRE(t(0, 3) == 13.0F);
  REQUIRE(t(1, 0) == 2.0F);
  REQUIRE(t(3, 3) == 16.0F);
  REQUIRE((m.transpose().transpose()) == m);
}

TEST_CASE("Mat4 mat-mat multiply", "[mat4]")
{
  obzcene::math::Mat4f a(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
  obzcene::math::Mat4f id;
  REQUIRE((a * id) == a);
  REQUIRE((id * a) == a);

  obzcene::math::Mat4f b(16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1);
  auto        r = a * b;
  REQUIRE(r(0, 0) == 80.0F);
  REQUIRE(r(3, 3) == 386.0F);
}

TEST_CASE("Mat4 arithmetic", "[mat4]")
{
  obzcene::math::Mat4f a(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
  obzcene::math::Mat4f b(16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1);

  auto sum = a + b;
  for (int i = 0; i < 16; ++i)
    REQUIRE(sum.data[i] == 17.0F);

  auto diff = a - b;
  for (int i = 0; i < 16; ++i)
    REQUIRE(std::abs(diff.data[i]) == Catch::Approx(std::abs(a.data[i] - b.data[i])));

  auto scaled = a * 2.0F;
  REQUIRE(scaled(0, 0) == 2.0F);
  REQUIRE(scaled(3, 3) == 32.0F);

  auto div = a / 2.0F;
  REQUIRE(div(0, 0) == Catch::Approx(0.5F));
  REQUIRE(div(3, 3) == Catch::Approx(8.0F));
}

TEST_CASE("Mat4 upper3", "[mat4]")
{
  obzcene::math::Mat4f m(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
  auto        upper = m.upper3();
  REQUIRE(upper(0, 0) == 1.0F);
  REQUIRE(upper(0, 2) == 3.0F);
  REQUIRE(upper(1, 0) == 5.0F);
  REQUIRE(upper(1, 2) == 7.0F);
  REQUIRE(upper(2, 0) == 9.0F);
  REQUIRE(upper(2, 2) == 11.0F);
}

TEST_CASE("Mat4 look_at", "[mat4]")
{
  obzcene::math::Vec3f eye{0.0F, 0.0F, 5.0F};
  obzcene::math::Vec3f center{0.0F, 0.0F, 0.0F};
  obzcene::math::Vec3f up{0.0F, 1.0F, 0.0F};
  auto        m = obzcene::math::Mat4f::look_at(eye, center, up);

  obzcene::math::Vec4f origin{0.0F, 0.0F, 0.0F, 1.0F};
  auto        r = m * origin;
  REQUIRE(r.z == Catch::Approx(-5.0F).margin(1e-5F));
}

TEST_CASE("Mat4 perspective", "[mat4]")
{
  auto p = obzcene::math::Mat4f::perspective(PI / 4.0F, 16.0F / 9.0F, 0.1F, 100.0F);
  REQUIRE(p.determinant() != Catch::Approx(0.0F));

  obzcene::math::Vec4f z_near{0.0F, 0.0F, -0.1F, 1.0F};
  auto        r = p * z_near;
  REQUIRE(r.w == Catch::Approx(0.1F).margin(1e-4F));
}

TEST_CASE("Mat4 ortho", "[mat4]")
{
  auto        o = obzcene::math::Mat4f::ortho(-1.0F, 1.0F, -1.0F, 1.0F, 0.1F, 100.0F);
  obzcene::math::Vec4f v{0.0F, 0.0F, -0.1F, 1.0F};
  auto        r = o * v;
  REQUIRE(r.x == Catch::Approx(0.0F));
  REQUIRE(r.y == Catch::Approx(0.0F));
}

TEST_CASE("Mat4 translation-rotation compose", "[mat4]")
{
  auto t  = obzcene::math::Mat4f::translation(10.0F, 0.0F, 0.0F);
  auto rz = obzcene::math::Mat4f::rotation_z(PI / 2.0F);
  auto m  = t * rz;

  obzcene::math::Vec4f v{1.0F, 0.0F, 0.0F, 1.0F};
  auto        r = m * v;
  REQUIRE(r.x == Catch::Approx(10.0F).margin(1e-5F));
  REQUIRE(r.y == Catch::Approx(1.0F).margin(1e-5F));
  REQUIRE(r.z == Catch::Approx(0.0F).margin(1e-5F));
}

TEST_CASE("Mat4 is_almost_equal", "[mat4]")
{
  obzcene::math::Mat4f a(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
  obzcene::math::Mat4f b(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16.0000001F);
  obzcene::math::Mat4f c(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 100);
  REQUIRE(a.is_almost_equal(b));
  REQUIRE_FALSE(a.is_almost_equal(c));
}
