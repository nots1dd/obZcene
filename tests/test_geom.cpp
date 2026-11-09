#include <catch2/catch_all.hpp>

#include <obzcene/Math/Math.hpp>

static constexpr float PI = 3.14159265358979323846F;

TEST_CASE("Ray point_at", "[ray]")
{
  obzcene::math::Rayf r{obzcene::math::Vec3f{0, 0, 0}, obzcene::math::Vec3f{1, 0, 0}};
  auto       p = r.point_at(5.0F);
  REQUIRE(p == obzcene::math::Vec3f{5.0F, 0.0F, 0.0F});
}

TEST_CASE("Ray intersect_sphere hit", "[ray]")
{
  obzcene::math::Rayf r{obzcene::math::Vec3f{0, 0, -10}, obzcene::math::Vec3f{0, 0, 1}};
  float      t = 0;
  REQUIRE(r.intersect_sphere(obzcene::math::Vec3f{0, 0, 0}, 1.0F, &t));
  REQUIRE(t == Catch::Approx(9.0F));
}

TEST_CASE("Ray intersect_sphere miss", "[ray]")
{
  obzcene::math::Rayf r{obzcene::math::Vec3f{0, 10, 0}, obzcene::math::Vec3f{1, 0, 0}};
  REQUIRE_FALSE(r.intersect_sphere(obzcene::math::Vec3f{0, 0, 0}, 1.0F));
}

TEST_CASE("Ray intersect_sphere from inside", "[ray]")
{
  obzcene::math::Rayf r{obzcene::math::Vec3f{0, 0, 0}, obzcene::math::Vec3f{1, 0, 0}};
  float      t = 0;
  REQUIRE(r.intersect_sphere(obzcene::math::Vec3f{0, 0, 0}, 5.0F, &t));
  REQUIRE(t == Catch::Approx(5.0F));
}

TEST_CASE("Ray intersect_plane hit", "[ray]")
{
  obzcene::math::Rayf r{obzcene::math::Vec3f{0, 0, 0}, obzcene::math::Vec3f{0, 0, 1}};
  float      t = 0;
  REQUIRE(r.intersect_plane(obzcene::math::Vec3f{0, 0, 1}, 5.0F, &t));
  REQUIRE(t == Catch::Approx(5.0F));
}

TEST_CASE("Ray intersect_plane parallel miss", "[ray]")
{
  obzcene::math::Rayf r{obzcene::math::Vec3f{0, 0, 0}, obzcene::math::Vec3f{1, 0, 0}};
  REQUIRE_FALSE(r.intersect_plane(obzcene::math::Vec3f{0, 0, 1}, 5.0F));
}

TEST_CASE("Ray intersect_plane behind miss", "[ray]")
{
  obzcene::math::Rayf r{obzcene::math::Vec3f{0, 0, 10}, obzcene::math::Vec3f{0, 0, 1}};
  REQUIRE_FALSE(r.intersect_plane(obzcene::math::Vec3f{0, 0, 1}, 5.0F));
}

TEST_CASE("Ray intersect_aabb hit", "[ray]")
{
  obzcene::math::Rayf  r{obzcene::math::Vec3f{-10, 0, 0}, obzcene::math::Vec3f{1, 0, 0}};
  obzcene::math::AABBf box{obzcene::math::Vec3f{-1, -1, -1}, obzcene::math::Vec3f{1, 1, 1}};
  float       t_min = 0, t_max = 0;
  REQUIRE(r.intersect_aabb(box, &t_min, &t_max));
  REQUIRE(t_min == Catch::Approx(9.0F));
  REQUIRE(t_max == Catch::Approx(11.0F));
}

TEST_CASE("Ray intersect_aabb miss", "[ray]")
{
  obzcene::math::Rayf  r{obzcene::math::Vec3f{-10, 5, 0}, obzcene::math::Vec3f{1, 0, 0}};
  obzcene::math::AABBf box{obzcene::math::Vec3f{-1, -1, -1}, obzcene::math::Vec3f{1, 1, 1}};
  REQUIRE_FALSE(r.intersect_aabb(box));
}

TEST_CASE("Plane construction", "[plane]")
{
  obzcene::math::Planef p;
  REQUIRE(p.normal == obzcene::math::Vec3f{0, 1, 0});
  REQUIRE(p.distance == 0.0F);

  obzcene::math::Planef p2(obzcene::math::Vec3f{0, 0, 1}, 5.0F);
  REQUIRE(p2.normal == obzcene::math::Vec3f{0, 0, 1});
  REQUIRE(p2.distance == 5.0F);
}

TEST_CASE("Plane from 3 points", "[plane]")
{
  obzcene::math::Planef p(obzcene::math::Vec3f{0, 0, 0}, obzcene::math::Vec3f{1, 0, 0}, obzcene::math::Vec3f{0, 1, 0});
  REQUIRE(p.normal.is_almost_equal(obzcene::math::Vec3f{0, 0, 1}));
}

TEST_CASE("Plane signed_distance", "[plane]")
{
  obzcene::math::Planef p(obzcene::math::Vec3f{0, 1, 0}, 3.0F);
  REQUIRE(p.signed_distance(obzcene::math::Vec3f{0, 5, 0}) == Catch::Approx(2.0F));
  REQUIRE(p.signed_distance(obzcene::math::Vec3f{0, 3, 0}) == Catch::Approx(0.0F));
  REQUIRE(p.signed_distance(obzcene::math::Vec3f{0, 1, 0}) == Catch::Approx(-2.0F));
}

TEST_CASE("Plane normalized", "[plane]")
{
  obzcene::math::Planef p(obzcene::math::Vec3f{0, 2, 0}, 6.0F);
  auto         n = p.normalized();
  REQUIRE(n.normal.is_almost_equal(obzcene::math::Vec3f{0, 1, 0}));
  REQUIRE(n.distance == Catch::Approx(3.0F));
}

TEST_CASE("AABB construction", "[aabb]")
{
  obzcene::math::AABBf box{obzcene::math::Vec3f{-1, -1, -1}, obzcene::math::Vec3f{1, 1, 1}};
  REQUIRE(box.min == obzcene::math::Vec3f{-1, -1, -1});
  REQUIRE(box.max == obzcene::math::Vec3f{1, 1, 1});
}

TEST_CASE("AABB empty", "[aabb]")
{
  auto e = obzcene::math::AABBf::empty();
  REQUIRE(e.min.x == Catch::Approx(std::numeric_limits<float>::max()));
  REQUIRE(e.max.x == Catch::Approx(std::numeric_limits<float>::lowest()));
}

TEST_CASE("AABB center and extents", "[aabb]")
{
  obzcene::math::AABBf box{obzcene::math::Vec3f{0, 0, 0}, obzcene::math::Vec3f{4, 6, 8}};
  REQUIRE(box.center() == obzcene::math::Vec3f{2, 3, 4});
  REQUIRE(box.extents() == obzcene::math::Vec3f{2, 3, 4});
  REQUIRE(box.size() == obzcene::math::Vec3f{4, 6, 8});
}

TEST_CASE("AABB from_center_extent", "[aabb]")
{
  auto box = obzcene::math::AABBf::from_center_extent(obzcene::math::Vec3f{5, 5, 5}, obzcene::math::Vec3f{1, 2, 3});
  REQUIRE(box.min == obzcene::math::Vec3f{4, 3, 2});
  REQUIRE(box.max == obzcene::math::Vec3f{6, 7, 8});
}

TEST_CASE("AABB surface_area and volume", "[aabb]")
{
  obzcene::math::AABBf box{obzcene::math::Vec3f{0, 0, 0}, obzcene::math::Vec3f{1, 2, 3}};
  REQUIRE(box.volume() == Catch::Approx(6.0F));
  REQUIRE(box.surface_area() == Catch::Approx(22.0F));
}

TEST_CASE("AABB contains_point", "[aabb]")
{
  obzcene::math::AABBf box{obzcene::math::Vec3f{-1, -1, -1}, obzcene::math::Vec3f{1, 1, 1}};
  REQUIRE(box.contains_point(obzcene::math::Vec3f{0, 0, 0}));
  REQUIRE(box.contains_point(obzcene::math::Vec3f{-1, -1, -1}));
  REQUIRE_FALSE(box.contains_point(obzcene::math::Vec3f{2, 0, 0}));
}

TEST_CASE("AABB contains_aabb", "[aabb]")
{
  obzcene::math::AABBf outer{obzcene::math::Vec3f{-2, -2, -2}, obzcene::math::Vec3f{2, 2, 2}};
  obzcene::math::AABBf inner{obzcene::math::Vec3f{-1, -1, -1}, obzcene::math::Vec3f{1, 1, 1}};
  obzcene::math::AABBf partial{obzcene::math::Vec3f{0, 0, 0}, obzcene::math::Vec3f{3, 3, 3}};

  REQUIRE(outer.contains_aabb(inner));
  REQUIRE_FALSE(inner.contains_aabb(outer));
  REQUIRE_FALSE(outer.contains_aabb(partial));
}

TEST_CASE("AABB intersects", "[aabb]")
{
  obzcene::math::AABBf a{obzcene::math::Vec3f{0, 0, 0}, obzcene::math::Vec3f{2, 2, 2}};
  obzcene::math::AABBf b{obzcene::math::Vec3f{1, 1, 1}, obzcene::math::Vec3f{3, 3, 3}};
  obzcene::math::AABBf c{obzcene::math::Vec3f{5, 5, 5}, obzcene::math::Vec3f{6, 6, 6}};

  REQUIRE(a.intersects(b));
  REQUIRE(b.intersects(a));
  REQUIRE_FALSE(a.intersects(c));
  REQUIRE_FALSE(c.intersects(a));
}

TEST_CASE("AABB expand point", "[aabb]")
{
  obzcene::math::AABBf box{obzcene::math::Vec3f{0, 0, 0}, obzcene::math::Vec3f{1, 1, 1}};
  box.expand(obzcene::math::Vec3f{2, -1, 3});
  REQUIRE(box.min == obzcene::math::Vec3f{0, -1, 0});
  REQUIRE(box.max == obzcene::math::Vec3f{2, 1, 3});
}

TEST_CASE("AABB union_with", "[aabb]")
{
  obzcene::math::AABBf a{obzcene::math::Vec3f{0, 0, 0}, obzcene::math::Vec3f{1, 1, 1}};
  obzcene::math::AABBf b{obzcene::math::Vec3f{-2, -2, -2}, obzcene::math::Vec3f{-1, -1, -1}};
  auto        u = a.union_with(b);
  REQUIRE(u.min == obzcene::math::Vec3f{-2, -2, -2});
  REQUIRE(u.max == obzcene::math::Vec3f{1, 1, 1});
}

TEST_CASE("Frustum contains_point", "[frustum]")
{
  obzcene::math::Planef planes[6] = {
    obzcene::math::Planef{obzcene::math::Vec3f{1, 0, 0}, -1}, obzcene::math::Planef{obzcene::math::Vec3f{-1, 0, 0}, -1},
    obzcene::math::Planef{obzcene::math::Vec3f{0, 1, 0}, -1}, obzcene::math::Planef{obzcene::math::Vec3f{0, -1, 0}, -1},
    obzcene::math::Planef{obzcene::math::Vec3f{0, 0, 1}, -1}, obzcene::math::Planef{obzcene::math::Vec3f{0, 0, -1}, -1}};
  obzcene::math::Frustumf f(planes[0], planes[1], planes[2], planes[3], planes[4], planes[5]);

  REQUIRE(f.contains_point(obzcene::math::Vec3f{0, 0, 0}));
  REQUIRE(f.contains_point(obzcene::math::Vec3f{1, 1, 1}));
  REQUIRE_FALSE(f.contains_point(obzcene::math::Vec3f{2, 0, 0}));
}

TEST_CASE("Frustum contains_sphere", "[frustum]")
{
  obzcene::math::Planef planes[6] = {
    obzcene::math::Planef{obzcene::math::Vec3f{1, 0, 0}, -1}, obzcene::math::Planef{obzcene::math::Vec3f{-1, 0, 0}, -1},
    obzcene::math::Planef{obzcene::math::Vec3f{0, 1, 0}, -1}, obzcene::math::Planef{obzcene::math::Vec3f{0, -1, 0}, -1},
    obzcene::math::Planef{obzcene::math::Vec3f{0, 0, 1}, -1}, obzcene::math::Planef{obzcene::math::Vec3f{0, 0, -1}, -1}};
  obzcene::math::Frustumf f(planes[0], planes[1], planes[2], planes[3], planes[4], planes[5]);

  REQUIRE(f.contains_sphere(obzcene::math::Vec3f{0, 0, 0}, 0.5F));
  REQUIRE_FALSE(f.contains_sphere(obzcene::math::Vec3f{5, 0, 0}, 0.5F));
}

TEST_CASE("Frustum contains_aabb", "[frustum]")
{
  obzcene::math::Planef planes[6] = {
    obzcene::math::Planef{obzcene::math::Vec3f{1, 0, 0}, -1}, obzcene::math::Planef{obzcene::math::Vec3f{-1, 0, 0}, -1},
    obzcene::math::Planef{obzcene::math::Vec3f{0, 1, 0}, -1}, obzcene::math::Planef{obzcene::math::Vec3f{0, -1, 0}, -1},
    obzcene::math::Planef{obzcene::math::Vec3f{0, 0, 1}, -1}, obzcene::math::Planef{obzcene::math::Vec3f{0, 0, -1}, -1}};
  obzcene::math::Frustumf f(planes[0], planes[1], planes[2], planes[3], planes[4], planes[5]);

  obzcene::math::AABBf inside{obzcene::math::Vec3f{-0.5F, -0.5F, -0.5F}, obzcene::math::Vec3f{0.5F, 0.5F, 0.5F}};
  obzcene::math::AABBf outside{obzcene::math::Vec3f{-2, -2, -2}, obzcene::math::Vec3f{-1.5F, -1.5F, -1.5F}};

  REQUIRE(f.contains_aabb(inside));
  REQUIRE_FALSE(f.contains_aabb(outside));
}

TEST_CASE("barycentric", "[geometry]")
{
  obzcene::math::Vec2f a{0, 0};
  obzcene::math::Vec2f b{4, 0};
  obzcene::math::Vec2f c{0, 4};

  auto center = obzcene::math::barycentric(obzcene::math::Vec2f{1, 1}, a, b, c);
  REQUIRE(center.x == Catch::Approx(0.5F).margin(1e-5F));
  REQUIRE(center.y == Catch::Approx(0.25F).margin(1e-5F));
  REQUIRE(center.z == Catch::Approx(0.25F).margin(1e-5F));

  auto at_a = obzcene::math::barycentric(a, a, b, c);
  REQUIRE(at_a.x == Catch::Approx(1.0F).margin(1e-5F));
  REQUIRE(at_a.y == Catch::Approx(0.0F).margin(1e-5F));
  REQUIRE(at_a.z == Catch::Approx(0.0F).margin(1e-5F));
}

TEST_CASE("point_in_triangle", "[geometry]")
{
  obzcene::math::Vec2f a{0, 0};
  obzcene::math::Vec2f b{4, 0};
  obzcene::math::Vec2f c{0, 4};

  REQUIRE(obzcene::math::point_in_triangle(obzcene::math::Vec2f{1, 1}, a, b, c));
  REQUIRE(obzcene::math::point_in_triangle(a, a, b, c));
  REQUIRE_FALSE(obzcene::math::point_in_triangle(obzcene::math::Vec2f{5, 5}, a, b, c));
}

TEST_CASE("triangle_area", "[geometry]")
{
  obzcene::math::Vec2f a{0, 0};
  obzcene::math::Vec2f b{4, 0};
  obzcene::math::Vec2f c{0, 4};
  REQUIRE(obzcene::math::triangle_area(a, b, c) == Catch::Approx(8.0F));
}

TEST_CASE("reflect", "[geometry]")
{
  obzcene::math::Vec3f incident{1.0F, -1.0F, 0.0F};
  obzcene::math::Vec3f normal{0.0F, 1.0F, 0.0F};
  auto        r = obzcene::math::reflect(incident, normal);
  REQUIRE(r.x == Catch::Approx(1.0F));
  REQUIRE(r.y == Catch::Approx(1.0F));
  REQUIRE(r.z == Catch::Approx(0.0F));
}

TEST_CASE("refract", "[geometry]")
{
  obzcene::math::Vec3f incident{0.0F, -1.0F, 0.0F};
  obzcene::math::Vec3f normal{0.0F, 1.0F, 0.0F};
  auto        r = obzcene::math::refract(incident, normal, 1.0F);
  REQUIRE(r.x == Catch::Approx(0.0F).margin(1e-5F));
  REQUIRE(r.y == Catch::Approx(-1.0F).margin(1e-5F));
  REQUIRE(r.z == Catch::Approx(0.0F).margin(1e-5F));
}

TEST_CASE("refract total internal reflection", "[geometry]")
{
  obzcene::math::Vec3f incident = obzcene::math::Vec3f{1.0F, -1.0F, 0.0F}.normalized();
  obzcene::math::Vec3f normal{0.0F, 1.0F, 0.0F};
  auto        r = obzcene::math::refract(incident, normal, 0.5F);
  REQUIRE(r == obzcene::math::Vec3f::zero());
}

TEST_CASE("smoothstep", "[geometry]")
{
  REQUIRE(obzcene::math::smoothstep(0.0F, 1.0F, -1.0F) == Catch::Approx(0.0F));
  REQUIRE(obzcene::math::smoothstep(0.0F, 1.0F, 0.0F) == Catch::Approx(0.0F));
  REQUIRE(obzcene::math::smoothstep(0.0F, 1.0F, 0.5F) == Catch::Approx(0.5F));
  REQUIRE(obzcene::math::smoothstep(0.0F, 1.0F, 1.0F) == Catch::Approx(1.0F));
  REQUIRE(obzcene::math::smoothstep(0.0F, 1.0F, 2.0F) == Catch::Approx(1.0F));
}

TEST_CASE("smootherstep", "[geometry]")
{
  REQUIRE(obzcene::math::smootherstep(0.0F, 1.0F, -1.0F) == Catch::Approx(0.0F));
  REQUIRE(obzcene::math::smootherstep(0.0F, 1.0F, 0.0F) == Catch::Approx(0.0F));
  REQUIRE(obzcene::math::smootherstep(0.0F, 1.0F, 0.5F) == Catch::Approx(0.5F));
  REQUIRE(obzcene::math::smootherstep(0.0F, 1.0F, 1.0F) == Catch::Approx(1.0F));
  REQUIRE(obzcene::math::smootherstep(0.0F, 1.0F, 2.0F) == Catch::Approx(1.0F));
}

TEST_CASE("catmull_rom scalar", "[geometry]")
{
  float r = obzcene::math::catmull_rom(0.0F, 1.0F, 2.0F, 3.0F, 0.0F);
  REQUIRE(r == Catch::Approx(1.0F));

  r = obzcene::math::catmull_rom(0.0F, 1.0F, 2.0F, 3.0F, 1.0F);
  REQUIRE(r == Catch::Approx(2.0F));
}

TEST_CASE("catmull_rom Vec2", "[geometry]")
{
  obzcene::math::Vec2f p0{0, 0}, p1{1, 0}, p2{2, 0}, p3{3, 0};
  auto        r = obzcene::math::catmull_rom(p0, p1, p2, p3, 0.5F);
  REQUIRE(r.y == Catch::Approx(0.0F));
  REQUIRE(r.x == Catch::Approx(1.5F).margin(0.1F));
}

TEST_CASE("catmull_rom Vec3", "[geometry]")
{
  obzcene::math::Vec3f p0{0, 0, 0}, p1{1, 1, 1}, p2{2, 2, 2}, p3{3, 3, 3};
  auto        r = obzcene::math::catmull_rom(p0, p1, p2, p3, 0.0F);
  REQUIRE(r.x == Catch::Approx(1.0F));
  REQUIRE(r.y == Catch::Approx(1.0F));
  REQUIRE(r.z == Catch::Approx(1.0F));
}

TEST_CASE("face_forward", "[geometry]")
{
  obzcene::math::Vec3f n{0, 0, 1};
  obzcene::math::Vec3f view{0, 0, -1};
  auto        ff = obzcene::math::face_forward(n, view);
  REQUIRE(ff == -n);

  auto ff2 = obzcene::math::face_forward(n, -view);
  REQUIRE(ff2 == n);
}
