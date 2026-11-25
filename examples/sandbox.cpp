/**
 * @file sandbox.cpp
 * @brief A small Minecraft-style sandbox against the public engine API:
 *        height-noise block terrain, a viewmodel gun in the camera's hands,
 *        left-click shooting that breaks blocks, right-click block placing,
 *        a crosshair, a muzzle flash, distance fog, and a day/night cycle
 *        with a 2D sun/moon driving the key light.
 *
 * The terrain uses the engine's static-batching API (obzcene::bake_instances)
 * instead of one scene object per block: blocks are merged into 16x16-column
 * chunk meshes, so a frame renders ~16 frustum-culled objects instead of
 * ~6k. Only EXPOSED faces are baked (a face between two solid blocks can
 * never be seen), and shooting a block only re-bakes the chunk(s) it touched.
 *
 * Build with the project (target `sandbox-example`), then run from the repo
 * root so `assets/` resolves:
 *
 *   ./build/sandbox-example
 *
 * Controls: WASD move, Space jump, Q/E or PgUp/PgDn fly up/down, mouse look
 * (sensitivity adjustable with `[`/`]`), Left Click = break the aimed block,
 * Right Click = place a block against the aimed face, Escape quits.
 */

#include <obzcene/obzcene.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <optional>
#include <unordered_set>

namespace
{

constexpr int   kTerrainRadius  = 24;     ///< half-extent of the terrain grid, in columns
constexpr int   kChunkColumns   = 16;     ///< terrain chunk size, in columns (x by z)
constexpr float kBlockSize      = 1.0F;   ///< world size of one block
constexpr float kGunOffsetR     = 0.26F;  ///< viewmodel: right of the camera
constexpr float kGunOffsetD     = 0.12F;  ///< viewmodel: below the camera
constexpr float kGunOffsetF     = 0.55F;  ///< viewmodel: gun centre forward of the camera
constexpr float kGunLength      = 0.9F;   ///< target world length of the viewmodel gun
constexpr float kShootRange     = 12.0F;  ///< max hit distance for shooting
constexpr float kDayLength      = 150.0F; ///< seconds per full day/night cycle
constexpr float kFlashTime      = 0.07F;  ///< muzzle-flash duration, seconds
constexpr float kWalkSpeed      = 6.0F;   ///< walk speed, blocks/second
constexpr float kWalkAccel      = 30.0F;  ///< horizontal acceleration rate (1/s, snappy)
constexpr float kFlySpeed       = 9.0F;   ///< vertical speed while Q/E is held
constexpr float kGravity        = 24.0F;  ///< fall acceleration when airborne
constexpr float kStepEase       = 20.0F;  ///< stair-climb rise rate (1/s)
constexpr float kPlayerRadius   = 0.3F;   ///< body half-width (keeps the near plane out of walls)
constexpr float kEdgeMargin     = 0.5F;   ///< invisible wall inset from the terrain edge
constexpr float kMaxAltitude    = 40.0F;  ///< flying ceiling, in blocks
constexpr float kEyeHeight      = 1.7F;   ///< camera height above the feet, in blocks
constexpr int   kMaxBuildHeight = 32;     ///< tallest placeable block (y < this)
// block_key packs y into 6 bits; the build height must fit.
static_assert(kMaxBuildHeight <= 64);
constexpr float kJumpSpeed = 8.0F; ///< vertical impulse when jumping

/**
 * @brief Deterministic value-noise height at grid (x, z).
 * @return Height in blocks, in [1, 6].
 */
[[nodiscard]] auto terrain_height(int x, int z) -> int
{
  const auto h1 = std::sin(x * 0.45F) * std::cos(z * 0.35F);
  const auto h2 = std::sin((x + z) * 0.2F + 1.7F);
  const auto h  = (h1 * 0.5F + h2 * 0.5F + 1.0F) * 2.0F;
  return static_cast<int>(h) + 1;
}

/**
 * @brief Unique key for a block coordinate.
 *
 * Bit budget: x/z span [-64, 63] (7 bits each, terrain is far smaller) and y
 * spans [0, 63] (6 bits, covers kMaxBuildHeight). Packing y into fewer bits
 * aliases tall towers onto ground-level cells (a y>=16 block shared a key
 * with (x, y-16, z+1), so digging ground corrupted tower blocks).
 */
[[nodiscard]] constexpr auto block_key(int x, int y, int z) -> obzcene::ui32
{
  return (static_cast<obzcene::ui32>(x + 64) * 128u + static_cast<obzcene::ui32>(z + 64)) * 64u +
         static_cast<obzcene::ui32>(y);
}

/** @brief Which chunk column grid coordinate `c` falls into. */
[[nodiscard]] constexpr auto chunk_of(int c) -> int { return (c + kTerrainRadius) / kChunkColumns; }

/** @brief Grid coordinate of one terrain block. */
using BlockCoord = obzcene::math::Vec3<int>;

/** @brief Terrain block types (indexes into the block model/face arrays). */
enum BlockType : size_t
{
  Grass = 0,
  Dirt,
  Stone,
  Cobblestone,
  OakPlanks,
  BlockTypeCount,
};

/** @brief Asset paths per block type, under models/minecraft-block/. */
constexpr std::array<const char*, BlockTypeCount> kBlockAssets{
  "models/minecraft-block/grass_block.obj", "models/minecraft-block/dirt.obj",
  "models/minecraft-block/stone.obj",       "models/minecraft-block/cobblestone.obj",
  "models/minecraft-block/oak_planks.obj",
};

/**
 * @brief Block type at grid (x, y, z): grass on the surface (oak planks on
 *        the spawn pad), dirt just below, stone at depth with cobblestone
 *        accents.
 */
[[nodiscard]] auto block_type(int x, int y, int z) -> size_t
{
  const auto h = terrain_height(x, z);
  if (y >= h - 1)
  {
    return std::max(std::abs(x), std::abs(z)) <= 2 ? OakPlanks : Grass;
  }
  if (y >= h - 3)
  {
    return Dirt;
  }
  const auto hash = (x * 73856093) ^ (z * 19349663) ^ (y * 83492791);
  return (hash & 15) == 0 ? Cobblestone : Stone;
}

/** @brief One axis direction a block face can point in (index matches the face arrays). */
struct FaceDir
{
  int dx, dy, dz;
};

/** @brief Face directions in canonical order: +X, -X, +Y, -Y, +Z, -Z. */
constexpr std::array<FaceDir, 6> kFaceDirs{
  {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}}};

/**
 * @brief Split a block mesh into six single-face meshes, one per direction.
 *
 * Face membership is read off the averaged vertex normals (the cube has flat
 * faces, so every triangle's corners share one axis-aligned normal). Baking
 * faces individually lets the terrain skip faces pressed against another
 * solid block — they can never be visible.
 */
[[nodiscard]] auto split_block_faces(const obzcene::Mesh& block) -> std::array<obzcene::Mesh, 6>
{
  std::array<obzcene::Mesh, 6> faces;
  for (auto& face : faces)
  {
    face.bounds       = obzcene::math::AABBf::empty();
    face.materials    = block.materials;
    face.object_count = 1;
  }
  for (const auto& tri : block.triangles)
  {
    const auto n =
      block.vertices[tri.a].normal + block.vertices[tri.b].normal + block.vertices[tri.c].normal;
    const auto ax         = std::abs(n.x);
    const auto ay         = std::abs(n.y);
    const auto az         = std::abs(n.z);
    size_t     face_index = 0;
    if (ax >= ay && ax >= az)
    {
      face_index = n.x > 0.0F ? 0 : 1;
    }
    else if (ay >= az)
    {
      face_index = n.y > 0.0F ? 2 : 3;
    }
    else
    {
      face_index = n.z > 0.0F ? 4 : 5;
    }

    auto&      face = faces[face_index];
    const auto base = static_cast<obzcene::ui32>(face.vertices.size());
    for (const auto index : {tri.a, tri.b, tri.c})
    {
      face.bounds.expand(block.vertices[index].position);
      face.vertices.push_back(block.vertices[index]);
    }
    face.triangles.push_back({base, base + 1, base + 2, tri.material_index});
  }
  return faces;
}

/**
 * @brief Aim ray for shooting: from the camera, along its forward vector.
 */
[[nodiscard]] auto make_aim_ray(const obzcene::CameraF& camera) -> obzcene::math::Rayf
{
  return obzcene::math::Rayf{camera.position(), camera.forward()};
}

} // namespace

class SandboxGame : public obzcene::Game
{
public:
  void on_init(obzcene::Engine& engine) override
  {
    engine.set_mouse_sensitivity(1.0F);

    // Assets resolve against the assets root ("assets" by default). Any null
    // means the game was launched from the wrong working directory — bail out
    // cleanly instead of crashing on the first deref.
    auto assets_ok = true;
    for (size_t type = 0; type < BlockTypeCount; ++type)
    {
      m_block_models[type] = engine.assets().model(kBlockAssets[type]);
      assets_ok            = assets_ok && m_block_models[type] != nullptr;
    }
    m_gun     = engine.assets().model("models/gun/ak47.obj");
    m_gunshot = engine.assets().audio("audio/gunshot.wav");
    m_break   = engine.assets().audio("audio/beep.wav"); // optional: positional break/place
    assets_ok = assets_ok && m_gun != nullptr && m_gunshot != nullptr;
    if (!assets_ok)
    {
      engine.assets().describe_roots();
      engine.quit();
      return;
    }
    for (size_t type = 0; type < BlockTypeCount; ++type)
    {
      m_block_faces[type] = split_block_faces(m_block_models[type]->mesh);
    }

    build_terrain(engine);
    spawn_gun(engine);
    spawn_lights(engine);
    spawn_sky();

    // Start the camera standing above the terrain centre.
    const auto ground = static_cast<float>(terrain_height(0, 0));
    engine.camera().set_position({0.0F, ground + 6.0F, 10.0F});
    engine.camera().look_at(engine.camera().position(), {0.0F, ground, 0.0F});
  }

  void on_update(obzcene::Engine& engine, float dt) override
  {
    // Mouse sensitivity tuning: `[` slower, `]` faster.
    if (engine.input().pressed(obzcene::Key::LeftBracket))
    {
      engine.set_mouse_sensitivity(std::max(0.1F, engine.mouse_sensitivity() - 0.1F));
    }
    if (engine.input().pressed(obzcene::Key::RightBracket))
    {
      engine.set_mouse_sensitivity(std::min(5.0F, engine.mouse_sensitivity() + 0.1F));
    }

    // Mouse look: the built-in fly camera is off, so the game owns the camera
    // (mouse deltas arrive through the input snapshot either way).
    const auto [mouse_dx, mouse_dy] = engine.input().mouse_delta();
    const auto look_sensitivity     = 0.0025F * engine.mouse_sensitivity();
    engine.camera().yaw_by(-mouse_dx * look_sensitivity);
    engine.camera().pitch_by(-mouse_dy * look_sensitivity);

    update_movement(engine, dt);
    update_daylight(engine, dt);

    // The gun is a viewmodel: it rides in the camera's right/down/forward frame.
    m_recoil = std::max(0.0F, m_recoil - dt * 6.0F);
    m_flash  = std::max(0.0F, m_flash - dt);
    update_gun_transform(engine);
    update_flash_light(engine);

    // Left click breaks the aimed block; right click places against its face.
    if (engine.input().pressed(obzcene::MouseButton::Left))
    {
      break_block(engine);
    }
    if (engine.input().pressed(obzcene::MouseButton::Right))
    {
      place_block(engine);
    }
  }

  void on_render(obzcene::Engine& engine) override
  {
    // 2D overlays drawn straight into the framebuffer after the scene pass.
    auto& fb = engine.framebuffer();
    draw_sky(fb, engine.camera(), engine.time());
    if (m_flash > 0.0F)
    {
      draw_muzzle_flash(fb, engine.camera());
    }
    draw_crosshair(fb);
  }

private:
  /** @brief One terrain chunk: its scene object plus the live blocks inside. */
  struct Chunk
  {
    obzcene::ObjectHandle   object;
    std::vector<BlockCoord> blocks; ///< grid coords of present blocks (picking)
  };

  /** @brief True when block (x, y, z) is present (natural terrain or placed). */
  [[nodiscard]] auto is_solid(int x, int y, int z) const -> bool
  {
    if (y < 0)
    {
      return true; // bedrock: the terrain floor is never exposed
    }
    if (std::abs(x) > kTerrainRadius || std::abs(z) > kTerrainRadius || y >= kMaxBuildHeight)
    {
      return false;
    }
    return m_placed_keys.contains(block_key(x, y, z)) ||
           (y < terrain_height(x, z) && !m_removed.contains(block_key(x, y, z)));
  }

  /**
   * @brief Top of the highest solid block AT OR BELOW `ref` in column
   *        (x, z) — the support under the player's feet.
   *
   * Unlike a heightfield top, this is overhang-safe: a ceiling above `ref`
   * is ignored, so standing under a dug-out overhang no longer teleports
   * the camera onto the overhang.
   */
  [[nodiscard]] auto support_height(int x, int z, float ref) const -> float
  {
    const auto limit = static_cast<int>(std::ceil(ref));
    auto       top   = 0.0F;
    for (auto y = std::min(terrain_height(x, z), limit) - 1; y >= 0; --y)
    {
      if (!m_removed.contains(block_key(x, y, z)))
      {
        top = static_cast<float>(y + 1) * kBlockSize;
        break;
      }
    }
    for (const auto& placed : m_placed)
    {
      const auto placed_top = static_cast<float>(placed.y + 1) * kBlockSize;
      if (placed.x == x && placed.z == z && placed_top <= ref && placed_top > top)
      {
        top = placed_top;
      }
    }
    return top;
  }

  /** @brief Grid cell of one world-space coordinate. */
  static constexpr auto cell_of(float world) -> int { return static_cast<int>(std::floor(world)); }

  /** @brief True when the block containing world point (x, y, z) is solid. */
  [[nodiscard]] auto point_in_solid(float x, float y, float z) const -> bool
  {
    return is_solid(cell_of(x), cell_of(y), cell_of(z));
  }

  /** @brief Build the terrain as kChunkColumns-sized chunks of baked blocks. */
  void build_terrain(obzcene::Engine& engine)
  {
    constexpr auto chunks_per_side = (2 * kTerrainRadius + kChunkColumns) / kChunkColumns;
    for (auto cx = 0; cx < chunks_per_side; ++cx)
    {
      for (auto cz = 0; cz < chunks_per_side; ++cz)
      {
        m_chunks.push_back(Chunk{});
        rebuild_chunk(engine, m_chunks.back(), cx, cz);
      }
    }
  }

  /**
   * @brief (Re)bake the chunk at chunk coords (cx, cz): gather the exposed
   *        faces per direction, merge them into one mesh, and swap the chunk
   *        object's model.
   */
  void rebuild_chunk(obzcene::Engine& engine, Chunk& chunk, int cx, int cz)
  {
    const auto x0 = cx * kChunkColumns - kTerrainRadius;
    const auto z0 = cz * kChunkColumns - kTerrainRadius;
    const auto x1 = std::min(x0 + kChunkColumns, kTerrainRadius + 1);
    const auto z1 = std::min(z0 + kChunkColumns, kTerrainRadius + 1);

    std::array<std::array<std::vector<obzcene::math::Mat4f>, 6>, BlockTypeCount> face_transforms;
    chunk.blocks.clear();
    for (auto x = x0; x < x1; ++x)
    {
      for (auto z = z0; z < z1; ++z)
      {
        for (auto y = 0; y < terrain_height(x, z); ++y)
        {
          if (!is_solid(x, y, z))
          {
            continue;
          }
          const auto type  = block_type(x, y, z);
          auto       baked = false;
          for (size_t f = 0; f < kFaceDirs.size(); ++f)
          {
            const auto& dir = kFaceDirs[f];
            if (!is_solid(x + dir.dx, y + dir.dy, z + dir.dz))
            {
              face_transforms[type][f].push_back(
                obzcene::math::Mat4f::translation(x * kBlockSize, y * kBlockSize, z * kBlockSize));
              baked = true;
            }
          }
          if (baked)
          {
            chunk.blocks.push_back({x, y, z});
          }
        }
      }
    }

    // Player-placed blocks (may sit outside the natural columns, e.g. towers).
    for (const auto& placed : m_placed)
    {
      if (placed.x < x0 || placed.x >= x1 || placed.z < z0 || placed.z >= z1)
      {
        continue;
      }
      for (size_t f = 0; f < kFaceDirs.size(); ++f)
      {
        const auto& dir = kFaceDirs[f];
        if (!is_solid(placed.x + dir.dx, placed.y + dir.dy, placed.z + dir.dz))
        {
          face_transforms[OakPlanks][f].push_back(obzcene::math::Mat4f::translation(
            static_cast<float>(placed.x) * kBlockSize, static_cast<float>(placed.y) * kBlockSize,
            static_cast<float>(placed.z) * kBlockSize));
        }
      }
      chunk.blocks.push_back(placed);
    }

    // One bake per (type, direction) pair keeps each type's material table
    // shared; appending the bakes yields the chunk mesh. Every append copies
    // the type's material table, so extend the texture list in lockstep to
    // keep the material<->texture index alignment.
    auto mesh   = obzcene::Mesh{};
    mesh.bounds = obzcene::math::AABBf::empty();
    std::vector<obzcene::Texture> textures;
    for (size_t type = 0; type < BlockTypeCount; ++type)
    {
      const auto& type_textures = m_block_models[type]->textures;
      for (size_t f = 0; f < kFaceDirs.size(); ++f)
      {
        if (face_transforms[type][f].empty())
        {
          continue;
        }
        const auto before = mesh.materials.size();
        obzcene::append_mesh(
          mesh, obzcene::bake_instances(m_block_faces[type][f], face_transforms[type][f]),
          obzcene::math::Mat4f::identity());
        for (size_t i = before; i < mesh.materials.size(); ++i)
        {
          textures.push_back(type_textures[i - before]);
        }
      }
    }

    auto model      = std::make_shared<obzcene::Model>();
    model->mesh     = std::move(mesh);
    model->textures = std::move(textures);
    if (chunk.object == nullptr)
    {
      chunk.object       = engine.scene().add_object(std::move(model), obzcene::math::Vec3f{0.0F});
      chunk.object->name = "terrain-chunk";
    }
    else
    {
      chunk.object->model = std::move(model);
    }
  }

  /**
   * @brief Smoothed walking: horizontal velocity eases toward the wish
   *        direction, the camera follows the terrain height smoothly in both
   *        directions (no snapping on steps), Q/E fly with gravity on release,
   *        and an invisible wall keeps the player inside the terrain.
   */
  void update_movement(obzcene::Engine& engine, float dt)
  {
    auto& cam = engine.camera();
    auto  pos = cam.position();

    // Horizontal: accelerate toward the wish direction (smooth start/stop).
    auto forward = cam.forward();
    forward.y    = 0.0F;
    if (forward.length_sq() > 1.0e-8F)
    {
      forward = forward.normalized();
    }
    const auto  right = cam.right();
    const auto& in    = engine.input();
    auto        wish  = forward * static_cast<float>(static_cast<int>(in.down(obzcene::Key::W)) -
                                                     static_cast<int>(in.down(obzcene::Key::S))) +
                right * static_cast<float>(static_cast<int>(in.down(obzcene::Key::D)) -
                                           static_cast<int>(in.down(obzcene::Key::A)));
    if (wish.length_sq() > 1.0F)
    {
      wish = wish.normalized();
    }
    wish             = wish * kWalkSpeed;
    const auto blend = 1.0F - std::exp(-kWalkAccel * dt);
    m_velocity       = m_velocity + (wish - m_velocity) * blend;

    // Horizontal collision, one axis at a time (walls stop that component,
    // the other still slides). The body has a radius: the two leading
    // corners of the eye's footprint are tested, so the camera keeps
    // kPlayerRadius from block faces and the near plane can never clip
    // inside them. Any block blocks movement — even 1-high steps (jump or
    // fly to climb, Minecraft-style; no auto-stepping).
    const auto free_at = [&](float x, float y, float z, bool x_move)
    {
      if (x_move)
      {
        return !point_in_solid(x, y, z - kPlayerRadius) && !point_in_solid(x, y, z + kPlayerRadius);
      }
      return !point_in_solid(x - kPlayerRadius, y, z) && !point_in_solid(x + kPlayerRadius, y, z);
    };
    const auto try_axis = [&](float dx, float dz)
    {
      const auto tx   = pos.x + dx;
      const auto tz   = pos.z + dz;
      const auto lead = dx > 0.0F || dz > 0.0F ? kPlayerRadius : -kPlayerRadius;
      const auto cx   = dx != 0.0F ? tx + lead : tx;
      const auto cz   = dz != 0.0F ? tz + lead : tz;
      if (free_at(cx, pos.y, cz, dx != 0.0F))
      {
        pos.x = tx;
        pos.z = tz;
        return;
      }
      if (dx != 0.0F)
      {
        m_velocity.x = 0.0F;
      }
      if (dz != 0.0F)
      {
        m_velocity.z = 0.0F;
      }
    };
    try_axis(m_velocity.x * dt, 0.0F);
    try_axis(0.0F, m_velocity.z * dt);

    // Barriers: the terrain spans [-kTerrainRadius, kTerrainRadius] columns.
    pos.x = std::clamp(pos.x, -kTerrainRadius + kEdgeMargin, kTerrainRadius + kEdgeMargin);
    pos.z = std::clamp(pos.z, -kTerrainRadius + kEdgeMargin, kTerrainRadius + kEdgeMargin);

    // Vertical: grounded ease follows the support under the feet (not the
    // column top, so overhangs are safe); flying up clamps at ceilings;
    // falling lands on the support below — never inside a block.
    const auto gx        = cell_of(pos.x);
    const auto gz        = cell_of(pos.z);
    const auto want_up   = in.down(obzcene::Key::E) || in.down(obzcene::Key::PageUp);
    const auto want_down = in.down(obzcene::Key::Q) || in.down(obzcene::Key::PageDown);
    if (want_up)
    {
      m_vertical = kFlySpeed;
      m_grounded = false;
    }
    else if (want_down)
    {
      m_vertical = -kFlySpeed;
      m_grounded = false;
    }
    else if (m_grounded && in.pressed(obzcene::Key::Space))
    {
      m_vertical = kJumpSpeed;
      m_grounded = false;
    }
    if (m_grounded)
    {
      // Character-like ground contact: planted when the support is under the
      // feet, a quick rise when a block appears under the feet, and a
      // ballistic fall when walking off an edge (no floating descent).
      const auto support = support_height(gx, gz, pos.y - kEyeHeight + 0.01F);
      const auto target  = support + kEyeHeight;
      if (target > pos.y)
      {
        pos.y += (target - pos.y) * std::min(1.0F, kStepEase * dt);
      }
      else if (pos.y > target + 0.02F)
      {
        m_grounded = false;
        m_vertical = 0.0F;
      }
      else
      {
        pos.y = target;
      }
    }
    else
    {
      if (!want_up && !want_down)
      {
        m_vertical -= kGravity * dt;
      }
      auto new_y = pos.y + m_vertical * dt;
      if (m_vertical <= 0.0F)
      {
        // Falling: land on the support at/below the feet (swept, so even a
        // fast fall cannot tunnel through a block).
        auto landing = support_height(gx, gz, pos.y - kEyeHeight) + kEyeHeight;
        while (point_in_solid(pos.x, landing, pos.z))
        {
          landing += kBlockSize; // low ceiling: pop out above it
        }
        if (new_y <= landing)
        {
          new_y      = landing;
          m_vertical = 0.0F;
          m_grounded = true;
        }
      }
      else if (point_in_solid(pos.x, new_y, pos.z))
      {
        new_y      = std::floor(new_y) - 1.0e-3F; // ceiling: stop just below it
        m_vertical = 0.0F;
      }
      pos.y = std::min(new_y, kMaxAltitude);
    }
    cam.set_position(pos);
  }

  /** @brief Attach the gun model as a viewmodel object, normalized from its bounds. */
  void spawn_gun(obzcene::Engine& engine)
  {
    m_gun_obj       = engine.scene().add_object(m_gun, obzcene::math::Vec3f{0.0F});
    m_gun_obj->name = "gun";

    // Normalize: scale the gun to kGunLength along its longest axis so any
    // model size works — the AK is ~355 units long in model space.
    const auto& bounds     = m_gun->mesh.bounds;
    const auto  size       = bounds.max - bounds.min;
    const auto  max_extent = std::max({size.x, size.y, size.z});
    const auto  scale      = max_extent > 0.0F ? kGunLength / max_extent : 1.0F;
    m_gun_obj->scale       = obzcene::math::Vec3f{scale, scale, scale};
  }

  /** @brief Key light (sun/moon) + dim fill lamp + the muzzle-flash light. */
  void spawn_lights(obzcene::Engine& engine)
  {
    // [0] key light, driven per-frame by update_daylight (sun by day, moon by night).
    engine.scene().lights.push_back(obzcene::LightF::directional(
      obzcene::math::Vec3f{0.0F, -1.0F, 0.0F}, obzcene::math::Vec3f{1, 0.98F, 0.9F}, 1.0F));
    // [1] dim cool fill over the spawn area, so night isn't pitch black.
    engine.scene().lights.push_back(obzcene::LightF::point(
      obzcene::math::Vec3f{0, 8, 6}, obzcene::math::Vec3f{0.7F, 0.8F, 1.0F}, 0.35F));
    // [2] muzzle flash: intensity animated by shooting (0 = dark).
    auto flash      = obzcene::LightF::point(obzcene::math::Vec3f{0.0F},
                                             obzcene::math::Vec3f{1.0F, 0.75F, 0.4F}, 0.0F);
    flash.linear    = 1.0F;
    flash.quadratic = 6.0F;
    engine.scene().lights.push_back(flash);
  }

  /**
   * @brief Advance the day/night cycle: swing the key light around the sky
   *        (sun by day, moon by night) and tint the sky to match.
   *
   * `m_day_time` runs 0..1: 0 = sunrise (+X horizon), 0.25 = noon,
   * 0.5 = sunset, 0.75 = midnight.
   */
  void update_daylight(obzcene::Engine& engine, float dt)
  {
    m_day_time       = std::fmod(m_day_time + dt / kDayLength, 1.0F);
    const auto angle = m_day_time * obzcene::math::two_pi<float>;
    m_sun_toward     = obzcene::math::Vec3f{std::cos(angle), std::sin(angle), 0.25F}.normalized();
    m_sun_elevation  = std::sin(angle);

    auto& key = engine.scene().lights[0];
    key.type  = obzcene::LightType::Directional;
    if (m_sun_elevation > -0.05F)
    {
      // Sun: the key light — warm at the horizon, white and bright at noon.
      const auto day    = std::clamp(m_sun_elevation, 0.0F, 1.0F);
      const auto warmth = 1.0F - std::clamp(day * 2.5F, 0.0F, 1.0F);
      key.direction     = -m_sun_toward;
      key.color         = {1.0F, 0.98F - 0.40F * warmth, 0.90F - 0.55F * warmth};
      key.intensity     = 0.25F + 0.95F * day;
    }
    else
    {
      // Moon: opposite the sun, cool and dim.
      key.direction = m_sun_toward;
      key.color     = {0.45F, 0.55F, 0.85F};
      key.intensity = 0.3F;
    }

    // Sky: vertical gradient (pale horizon, deep zenith), warm dusk band near
    // the horizon, dark at night.
    const auto dayf  = std::clamp(m_sun_elevation * 2.5F, 0.0F, 1.0F);
    const auto duskf = std::clamp(1.0F - std::abs(m_sun_elevation) * 4.0F, 0.0F, 1.0F) *
                       std::clamp((m_sun_elevation + 0.1F) * 3.0F, 0.0F, 1.0F);
    const auto mix     = [](float a, float b, float t) { return a + (b - a) * t; };
    const auto to_byte = [](float v)
    { return static_cast<obzcene::ui8>(std::clamp(v, 0.0F, 255.0F)); };
    const auto top_r = mix(5.0F, 38.0F, dayf) + 30.0F * duskf;
    const auto top_g = mix(8.0F, 105.0F, dayf) + 5.0F * duskf;
    const auto top_b = mix(22.0F, 212.0F, dayf) - 15.0F * duskf;
    const auto hor_r = mix(12.0F, 135.0F, dayf) + 110.0F * duskf;
    const auto hor_g = mix(15.0F, 190.0F, dayf) + 25.0F * duskf;
    const auto hor_b = mix(42.0F, 235.0F, dayf) - 40.0F * duskf;
    engine.set_sky_gradient(true, obzcene::Color{to_byte(top_r), to_byte(top_g), to_byte(top_b)},
                            obzcene::Color{to_byte(hor_r), to_byte(hor_g), to_byte(hor_b)});
    engine.set_clear_color(
      obzcene::Color{to_byte(hor_r), to_byte(hor_g), to_byte(hor_b)}); // flat fallback

    // Distance fog converges on the horizon colour (depth cueing, day/night).
    engine.set_fog(obzcene::FogSettings{
      obzcene::math::Vec3f{hor_r / 255.0F, hor_g / 255.0F, hor_b / 255.0F}, 24.0F, 48.0F});

    // Clouds drift with the wind and wrap around the map.
    for (auto& cloud : m_clouds)
    {
      cloud.x += cloud.speed * dt;
      if (cloud.x > kTerrainRadius + 12.0F)
      {
        cloud.x -= 2.0F * (kTerrainRadius + 12.0F);
      }
    }
  }

  /** @brief Drive the muzzle-flash point light from the flash timer. */
  void update_flash_light(obzcene::Engine& engine)
  {
    auto& flash     = engine.scene().lights[2];
    flash.position  = m_muzzle;
    flash.intensity = m_flash > 0.0F ? 4.0F * (m_flash / kFlashTime) : 0.0F;
  }

  /**
   * @brief Move the viewmodel gun into the camera's right/down/forward frame.
   *
   * The AK model's barrel points down +Z in model space, so it needs a 180°
   * yaw on top of the camera's — and the extra half-turn flips the pitch
   * sign: Ry(yaw + π)·Rx(-pitch) maps the barrel exactly onto the camera
   * forward vector. Recoil adds a small extra upward kick.
   */
  void update_gun_transform(obzcene::Engine& engine)
  {
    const auto& cam     = engine.camera();
    m_gun_obj->position = cam.position() + cam.forward() * kGunOffsetF + cam.right() * kGunOffsetR -
                          cam.up() * kGunOffsetD;
    m_gun_obj->rotation.y = cam.yaw() + obzcene::math::pi<float>;
    m_gun_obj->rotation.x = -(cam.pitch() + m_recoil);
    // Muzzle: half the gun length ahead of its centre — flash anchor.
    m_muzzle = m_gun_obj->position + cam.forward() * (kGunLength * 0.5F);
  }

  /** @brief Project a world-space DIRECTION (point at infinity) to pixels. */
  [[nodiscard]] static auto project_direction(const obzcene::CameraF&     camera,
                                              const obzcene::math::Vec3f& dir,
                                              const obzcene::FrameBuffer& fb)
    -> std::optional<obzcene::math::Vec2i>
  {
    const auto clip = camera.view_projection() * obzcene::math::Vec4f{dir.x, dir.y, dir.z, 0.0F};
    if (clip.w <= 1.0e-6F)
    {
      return std::nullopt; // behind the camera
    }
    const auto ndc_x = clip.x / clip.w;
    const auto ndc_y = clip.y / clip.w;
    return obzcene::math::Vec2i{static_cast<int>((ndc_x * 0.5F + 0.5F) * fb.width()),
                                static_cast<int>((0.5F - ndc_y * 0.5F) * fb.height())};
  }

  /** @brief Project a finite world-space point to pixels (null when behind). */
  [[nodiscard]] static auto project_point(const obzcene::CameraF&     camera,
                                          const obzcene::math::Vec3f& point,
                                          const obzcene::FrameBuffer& fb)
    -> std::optional<obzcene::math::Vec2i>
  {
    const auto clip =
      camera.view_projection() * obzcene::math::Vec4f{point.x, point.y, point.z, 1.0F};
    if (clip.w <= 1.0e-6F)
    {
      return std::nullopt;
    }
    return obzcene::math::Vec2i{static_cast<int>((clip.x / clip.w * 0.5F + 0.5F) * fb.width()),
                                static_cast<int>((0.5F - clip.y / clip.w * 0.5F) * fb.height())};
  }

  /** @brief Scatter the clouds and the night star dome (deterministic). */
  void spawn_sky()
  {
    const auto hash01 = [](int v)
    {
      const auto h = static_cast<unsigned>(v) * 2654435761u;
      return static_cast<float>((h >> 8) % 1000) / 1000.0F;
    };
    // Clouds: soft puffs drifting east across the map at y≈26.
    for (auto i = 0; i < 8; ++i)
    {
      m_clouds.push_back({-30.0F + 60.0F * hash01(i * 3), 26.0F + 4.0F * hash01(i * 3 + 1),
                          -30.0F + 60.0F * hash01(i * 3 + 2), 0.4F + 0.6F * hash01(i * 5 + 4),
                          1.5F + 2.0F * hash01(i * 7 + 5)});
    }
    // Stars: a fixed dome above the horizon, twinkling at night.
    for (auto i = 0; i < 90; ++i)
    {
      const auto azimuth   = hash01(i * 2) * obzcene::math::two_pi<float>;
      const auto elevation = 0.12F + 0.83F * hash01(i * 2 + 1);
      m_stars.push_back(obzcene::math::Vec3f{std::cos(azimuth) * std::cos(elevation),
                                             std::sin(elevation),
                                             std::sin(azimuth) * std::cos(elevation)});
    }
  }

  /** @brief Sun/moon squares, drifting clouds, and stars at night. */
  void draw_sky(obzcene::FrameBuffer& fb, const obzcene::CameraF& camera, float time) const
  {
    if (m_sun_elevation > -0.05F)
    {
      if (const auto px = project_direction(camera, m_sun_toward, fb))
      {
        obzcene::draw2d::square(fb, px->x, px->y, 36, obzcene::Color{255, 240, 170}, true);
      }
    }
    if (m_sun_elevation < 0.05F)
    {
      if (const auto px = project_direction(camera, -m_sun_toward, fb))
      {
        obzcene::draw2d::square(fb, px->x, px->y, 28, obzcene::Color{215, 225, 245}, true);
      }
    }

    // Clouds: blended white puffs behind the terrain (sky pixels only).
    const auto dayf = std::clamp(m_sun_elevation * 2.5F, 0.0F, 1.0F);
    const auto mixc = [dayf](float night, float day)
    { return static_cast<obzcene::ui8>(night + (day - night) * dayf); };
    const auto cloud_c =
      obzcene::Color{mixc(70.0F, 245.0F), mixc(74.0F, 247.0F), mixc(96.0F, 252.0F), 120};
    for (const auto& cloud : m_clouds)
    {
      const auto px = project_point(camera, obzcene::math::Vec3f{cloud.x, cloud.y, cloud.z}, fb);
      if (!px.has_value())
      {
        continue;
      }
      const auto dist =
        obzcene::math::length(obzcene::math::Vec3f{cloud.x, cloud.y, cloud.z} - camera.position());
      const auto r     = std::clamp(cloud.size * 90.0F / dist, 5.0F, 42.0F);
      const auto r_int = static_cast<int>(r);
      obzcene::draw2d::disc(fb, px->x, px->y, r_int, cloud_c, true, true);
      obzcene::draw2d::disc(fb, px->x - static_cast<int>(r * 0.9F), px->y + r_int / 6,
                            static_cast<int>(r * 0.6F), cloud_c, true, true);
      obzcene::draw2d::disc(fb, px->x + static_cast<int>(r * 0.9F), px->y + r_int / 6,
                            static_cast<int>(r * 0.6F), cloud_c, true, true);
    }

    // Stars: only when it is dark; gentle twinkle.
    const auto nightf = std::clamp(-m_sun_elevation * 2.5F, 0.0F, 1.0F);
    if (nightf > 0.05F)
    {
      for (size_t i = 0; i < m_stars.size(); ++i)
      {
        const auto px = project_direction(camera, m_stars[i], fb);
        if (!px.has_value())
        {
          continue;
        }
        const auto twinkle = 0.55F + 0.45F * std::sin(time * 2.5F + static_cast<float>(i) * 1.7F);
        const auto alpha   = static_cast<obzcene::ui8>(nightf * twinkle * 255.0F);
        obzcene::draw2d::paint(fb, px->x, px->y, obzcene::Color{215, 225, 255, alpha}, true, true);
        if (i % 9 == 0) // a few brighter 2px stars
        {
          obzcene::draw2d::paint(fb, px->x + 1, px->y, obzcene::Color{215, 225, 255, alpha}, true,
                                 true);
        }
      }
    }
  }

  /**
   * @brief Muzzle flash starburst: a hot radial-gradient core plus 4 long and
   *        4 short fading spikes, rotated per shot and shrinking as the flash
   *        burns out.
   */
  void draw_muzzle_flash(obzcene::FrameBuffer& fb, const obzcene::CameraF& camera) const
  {
    const auto clip =
      camera.view_projection() * obzcene::math::Vec4f{m_muzzle.x, m_muzzle.y, m_muzzle.z, 1.0F};
    if (clip.w <= camera.near_clip())
    {
      return;
    }
    const auto px = static_cast<int>((clip.x / clip.w * 0.5F + 0.5F) * fb.width());
    const auto py = static_cast<int>((0.5F - clip.y / clip.w * 0.5F) * fb.height());
    const auto t  = m_flash / kFlashTime; // 1 -> 0 as the flash burns out

    const auto put = [&fb](int x, int y, const obzcene::Color& color)
    { obzcene::draw2d::put(fb, x, y, color); };
    const auto lerp_color = [](const obzcene::Color& a, const obzcene::Color& b, float f)
    {
      const auto l = [f](obzcene::ui8 u, obzcene::ui8 v)
      { return static_cast<obzcene::ui8>(static_cast<float>(u) * (1.0F - f) + v * f); };
      return obzcene::Color{l(a.r, b.r), l(a.g, b.g), l(a.b, b.b)};
    };

    // Hot core: near-white centre cooling to orange at the rim.
    const auto core_r = 4 + static_cast<int>(4.0F * t);
    for (auto dy = -core_r; dy <= core_r; ++dy)
    {
      for (auto dx = -core_r; dx <= core_r; ++dx)
      {
        const auto d2 = dx * dx + dy * dy;
        if (d2 > core_r * core_r)
        {
          continue;
        }
        const auto fade = std::sqrt(static_cast<float>(d2)) / static_cast<float>(core_r);
        put(px + dx, py + dy,
            lerp_color(obzcene::Color{255, 252, 235}, obzcene::Color{255, 170, 60}, fade));
      }
    }

    // Starburst spikes: 4 long + 4 short (offset 45°), fading outward.
    const auto long_len  = 18.0F + 20.0F * t;
    const auto short_len = long_len * 0.45F;
    for (auto k = 0; k < 8; ++k)
    {
      const auto angle = m_flash_angle + static_cast<float>(k) * (obzcene::math::pi<float> / 4.0F);
      const auto len   = (k % 2 == 0) ? long_len : short_len;
      const auto ca    = std::cos(angle);
      const auto sa    = std::sin(angle);
      for (auto s = 2.0F; s < len; s += 0.5F)
      {
        const auto fade = s / len;
        const auto color =
          lerp_color(obzcene::Color{255, 205, 95}, obzcene::Color{120, 40, 5}, fade * fade);
        const auto x = px + static_cast<int>(ca * s);
        const auto y = py + static_cast<int>(sa * s);
        put(x, y, color);
        if (s < len * 0.5F)
        {
          // Thicken the bright inner half of each spike.
          put(px + static_cast<int>(ca * s - sa), py + static_cast<int>(sa * s + ca), color);
        }
      }
    }
  }

  /** @brief Small centre crosshair: dark outline with a white core. */
  static void draw_crosshair(obzcene::FrameBuffer& fb)
  {
    const auto           cx = fb.width() / 2;
    const auto           cy = fb.height() / 2;
    const obzcene::Color white{245, 245, 245};
    const obzcene::Color shade{20, 20, 20};
    for (auto i = -9; i <= 9; ++i)
    {
      if (std::abs(i) < 3)
      {
        continue; // gap around the exact aim point
      }
      obzcene::draw2d::put(fb, cx + i, cy - 1, shade);
      obzcene::draw2d::put(fb, cx + i, cy + 1, shade);
      obzcene::draw2d::put(fb, cx - 1, cy + i, shade);
      obzcene::draw2d::put(fb, cx + 1, cy + i, shade);
      obzcene::draw2d::put(fb, cx + i, cy, white);
      obzcene::draw2d::put(fb, cx, cy + i, white);
    }
  }

  /** @brief A block the aim ray hit, plus the face it entered through. */
  struct BlockHit
  {
    BlockCoord block;
    BlockCoord face; ///< outward axis of the entry face (unit, ±1 in one component)
    float      t{0.0F};
  };

  /**
   * @brief Nearest terrain block under the crosshair, or nullopt.
   *
   * Broad-phase: obzcene::raycast_all prefilters the chunks the aim ray
   * actually crosses (the gun is excluded by name); the block-level test
   * then only runs inside those chunks.
   */
  [[nodiscard]] auto aim_block(obzcene::Engine& engine, float max_t) const
    -> std::optional<BlockHit>
  {
    const auto ray         = make_aim_ray(engine.camera());
    const auto near_chunks = obzcene::raycast_all(
      engine.scene(), ray, max_t,
      +[](const obzcene::ObjectHandle& object) { return object->name == "terrain-chunk"; });

    auto best       = max_t;
    auto best_block = std::optional<BlockCoord>{};
    for (const auto& chunk_hit : near_chunks)
    {
      const auto chunk_it = std::find_if(m_chunks.begin(), m_chunks.end(), [&](const Chunk& chunk)
                                         { return chunk.object == chunk_hit.object; });
      if (chunk_it == m_chunks.end())
      {
        continue;
      }
      for (const auto& block : chunk_it->blocks)
      {
        const auto min   = obzcene::math::Vec3f{static_cast<float>(block.x) * kBlockSize,
                                              static_cast<float>(block.y) * kBlockSize,
                                              static_cast<float>(block.z) * kBlockSize};
        float      t_min = 0.0F;
        if (ray.intersect_aabb({min, min + obzcene::math::Vec3f{kBlockSize}}, &t_min) &&
            t_min < best)
        {
          best       = t_min;
          best_block = block;
        }
      }
    }
    if (!best_block.has_value())
    {
      return std::nullopt;
    }
    const auto box   = obzcene::math::AABBf{{static_cast<float>(best_block->x) * kBlockSize,
                                             static_cast<float>(best_block->y) * kBlockSize,
                                             static_cast<float>(best_block->z) * kBlockSize},
                                            {static_cast<float>(best_block->x + 1) * kBlockSize,
                                             static_cast<float>(best_block->y + 1) * kBlockSize,
                                             static_cast<float>(best_block->z + 1) * kBlockSize}};
    const auto point = ray.origin + ray.direction * best;
    return BlockHit{*best_block, obzcene::aabb_entry_face(box, point, 1.0e-3F), best};
  }

  /** @brief Re-bake every chunk touching block (x, z) or its chunk-border neighbours. */
  void rebuild_around(obzcene::Engine& engine, int x, int z)
  {
    constexpr auto chunks_per_side = (2 * kTerrainRadius + kChunkColumns) / kChunkColumns;
    for (const auto c : {x - 1, x, x + 1})
    {
      for (const auto d : {z - 1, z, z + 1})
      {
        if (std::abs(c) > kTerrainRadius || std::abs(d) > kTerrainRadius)
        {
          continue;
        }
        const auto cx = chunk_of(c);
        const auto cz = chunk_of(d);
        rebuild_chunk(engine, m_chunks[static_cast<size_t>(cx * chunks_per_side + cz)], cx, cz);
      }
    }
  }

  /** @brief Left click: break the aimed block (recoil + flash + gunshot). */
  void break_block(obzcene::Engine& engine)
  {
    // Recoil + flash + sound happen regardless of whether we hit something.
    m_recoil      = 0.12F;
    m_flash       = kFlashTime;
    m_flash_angle = engine.time() * 37.0F; // varies the starburst per shot
    engine.audio().play(m_gunshot, {.volume = 0.6F});

    const auto hit = aim_block(engine, kShootRange);
    if (!hit.has_value())
    {
      return;
    }

    // Placed blocks come off the placed list; natural blocks go to m_removed.
    const auto key = block_key(hit->block.x, hit->block.y, hit->block.z);
    if (m_placed_keys.contains(key))
    {
      m_placed_keys.erase(key);
      std::erase(m_placed, hit->block);
    }
    else
    {
      m_removed.insert(key);
    }
    play_at_block(engine, hit->block, 0.7F);
    rebuild_around(engine, hit->block.x, hit->block.z);
  }

  /** @brief Right click: place an oak-planks block against the aimed face. */
  void place_block(obzcene::Engine& engine)
  {
    const auto hit = aim_block(engine, kShootRange);
    if (!hit.has_value())
    {
      return;
    }
    const auto target = hit->block + hit->face;
    if (std::abs(target.x) > kTerrainRadius || std::abs(target.z) > kTerrainRadius ||
        target.y < 0 || target.y >= kMaxBuildHeight || is_solid(target.x, target.y, target.z))
    {
      return;
    }
    // No self-trapping: reject a block that would contain the camera.
    const auto eye = engine.camera().position();
    const auto inside =
      eye.x >= static_cast<float>(target.x) && eye.x <= static_cast<float>(target.x) + kBlockSize &&
      eye.y >= static_cast<float>(target.y) && eye.y <= static_cast<float>(target.y) + kBlockSize &&
      eye.z >= static_cast<float>(target.z) && eye.z <= static_cast<float>(target.z) + kBlockSize;
    if (inside)
    {
      return;
    }
    m_placed.push_back(target);
    m_placed_keys.insert(block_key(target.x, target.y, target.z));
    play_at_block(engine, target, 0.4F);
    rebuild_around(engine, target.x, target.z);
  }

  /** @brief Positional break/place sound centred on a block. */
  void play_at_block(obzcene::Engine& engine, const BlockCoord& block, float volume)
  {
    if (m_break == nullptr)
    {
      return;
    }
    const auto& cam = engine.camera();
    const auto  centre =
      obzcene::math::Vec3f{static_cast<float>(block.x) + 0.5F, static_cast<float>(block.y) + 0.5F,
                           static_cast<float>(block.z) + 0.5F};
    engine.audio().play(
      m_break, obzcene::spatial_options(centre, cam.position(), cam.right(), volume, 3.0F, 1.0F));
  }

  /** @brief One drifting cloud puff. */
  struct Cloud
  {
    float x, y, z;
    float speed;
    float size;
  };

  std::array<std::shared_ptr<const obzcene::Model>, BlockTypeCount> m_block_models;
  std::array<std::array<obzcene::Mesh, 6>, BlockTypeCount>
    m_block_faces; ///< single-face meshes per type and direction
  std::shared_ptr<const obzcene::Model>     m_gun;
  std::shared_ptr<const obzcene::AudioClip> m_gunshot;
  std::shared_ptr<const obzcene::AudioClip> m_break; ///< positional break/place sound (optional)
  obzcene::ObjectHandle                     m_gun_obj;
  std::vector<Chunk>                        m_chunks;
  std::vector<Cloud>                        m_clouds;         ///< drifting sky puffs
  std::vector<obzcene::math::Vec3f>         m_stars;          ///< fixed star dome directions
  std::unordered_set<obzcene::ui32>         m_removed;        ///< shot-away natural blocks
  std::vector<BlockCoord>                   m_placed;         ///< player-built blocks (oak planks)
  std::unordered_set<obzcene::ui32>         m_placed_keys;    ///< fast lookup for m_placed
  obzcene::math::Vec3f                      m_velocity{0.0F}; ///< smoothed walk velocity
  obzcene::math::Vec3f                      m_muzzle{0.0F};   ///< muzzle world position
  obzcene::math::Vec3f                      m_sun_toward{0.0F, 1.0F, 0.0F}; ///< toward the sun
  float                                     m_vertical{0.0F}; ///< vertical fly/fall velocity
  float                                     m_recoil{0.0F};
  float                                     m_flash{0.0F};       ///< muzzle-flash time left
  float                                     m_flash_angle{0.0F}; ///< starburst rotation per shot
  float                                     m_day_time{0.12F};   ///< 0..1, morning start
  float                                     m_sun_elevation{1.0F};
  bool                                      m_grounded{true};
};

auto main() -> int
{
  // OBZCENE_GPU=1 runs the GPU (OpenGL) backend; default is the CPU rasterizer.
  const auto  use_gpu = std::getenv("OBZCENE_GPU") != nullptr;
  auto        engine  = obzcene::Engine{obzcene::EngineConfig{
            .title             = "obZcene sandbox — Minecraft-ish",
            .width             = 1280,
            .height            = 720,
            .ambient           = 0.35F, // soft skylight so shadowed faces stay readable
            .fly_camera        = false, // the game drives the camera (smoothed walking)
            .mouse_sensitivity = 1.0F,
            .clear_color       = obzcene::Color{120, 168, 255},   // sky blue
            .texture_filter    = obzcene::TextureFilter::Nearest, // crisp pixel-art
            .backend = use_gpu ? obzcene::RendererBackend::Gpu : obzcene::RendererBackend::Cpu,
  }};
  SandboxGame game;
  return engine.run(game);
}
