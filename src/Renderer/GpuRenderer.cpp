/**
 * @file GpuRenderer.cpp
 * @brief OpenGL 3.3 core backend implementation (see Renderer/GpuRenderer.hpp).
 *
 * GL entry points beyond 1.1 are loaded through SDL_GL_GetProcAddress (libGL
 * only exports legacy symbols). The scene shader mirrors the CPU shading
 * model (obzcene::shade_point / shade_fragment) so both backends light the
 * same scene the same way: material diffuse x optional diffuse map, ambient
 * term, per-light Lambert + Blinn-Phong with attenuation and spot cones.
 *
 * Meshes upload lazily, cached by Model pointer with a weak_ptr liveness
 * check (a freed Model's address can be reused by a new allocation — an
 * expired weak_ptr means the entry is stale and must be re-uploaded).
 * Diffuse textures are cached by content hash so copied 16x16 block
 * textures share one GL texture. When the mesh cache grows past a bound it
 * is flushed wholesale (terrain chunk rebuilds replace models steadily, and
 * stale entries would leak GPU memory).
 */

#include "obzcene/Renderer/GpuRenderer.hpp"

#include "obzcene/Core/Casts.hpp"
#include "obzcene/Core/Logger.hpp"
#include "obzcene/Viewer/Scene.hpp"

#include <SDL.h>

#include <GL/gl.h>
#include <GL/glext.h>

#include <algorithm>
#include <cstring>

namespace obzcene
{

namespace
{

// -- GL entry points (> 1.1), loaded at init --------------------------------
#define OBZ_GL_FUNCS(X)                                          \
  X(PFNGLGENBUFFERSPROC, glGenBuffers)                           \
  X(PFNGLBINDBUFFERPROC, glBindBuffer)                           \
  X(PFNGLBUFFERDATAPROC, glBufferData)                           \
  X(PFNGLDELETEBUFFERSPROC, glDeleteBuffers)                     \
  X(PFNGLGENVERTEXARRAYSPROC, glGenVertexArrays)                 \
  X(PFNGLBINDVERTEXARRAYPROC, glBindVertexArray)                 \
  X(PFNGLDELETEVERTEXARRAYSPROC, glDeleteVertexArrays)           \
  X(PFNGLENABLEVERTEXATTRIBARRAYPROC, glEnableVertexAttribArray) \
  X(PFNGLVERTEXATTRIBPOINTERPROC, glVertexAttribPointer)         \
  X(PFNGLCREATESHADERPROC, glCreateShader)                       \
  X(PFNGLSHADERSOURCEPROC, glShaderSource)                       \
  X(PFNGLCOMPILESHADERPROC, glCompileShader)                     \
  X(PFNGLGETSHADERIVPROC, glGetShaderiv)                         \
  X(PFNGLGETSHADERINFOLOGPROC, glGetShaderInfoLog)               \
  X(PFNGLDELETESHADERPROC, glDeleteShader)                       \
  X(PFNGLCREATEPROGRAMPROC, glCreateProgram)                     \
  X(PFNGLATTACHSHADERPROC, glAttachShader)                       \
  X(PFNGLLINKPROGRAMPROC, glLinkProgram)                         \
  X(PFNGLGETPROGRAMIVPROC, glGetProgramiv)                       \
  X(PFNGLGETPROGRAMINFOLOGPROC, glGetProgramInfoLog)             \
  X(PFNGLUSEPROGRAMPROC, glUseProgram)                           \
  X(PFNGLDELETEPROGRAMPROC, glDeleteProgram)                     \
  X(PFNGLGETUNIFORMLOCATIONPROC, glGetUniformLocation)           \
  X(PFNGLUNIFORM1IPROC, glUniform1i)                             \
  X(PFNGLUNIFORM1FPROC, glUniform1f)                             \
  X(PFNGLUNIFORM3FPROC, glUniform3f)                             \
  X(PFNGLUNIFORMMATRIX4FVPROC, glUniformMatrix4fv)               \
  X(PFNGLUNIFORMMATRIX3FVPROC, glUniformMatrix3fv)               \
  X(PFNGLACTIVETEXTUREPROC, glActiveTexture)

#define OBZ_GL_DECLARE(type, name) type name = nullptr;
OBZ_GL_FUNCS(OBZ_GL_DECLARE)
#undef OBZ_GL_DECLARE

auto load_gl_functions() -> bool
{
  auto ok = true;
#define OBZ_GL_LOAD(type, name)                                \
  name = reinterpret_cast<type>(SDL_GL_GetProcAddress(#name)); \
  ok   = ok && name != nullptr;
  OBZ_GL_FUNCS(OBZ_GL_LOAD)
#undef OBZ_GL_LOAD
  return ok;
}

constexpr int kMaxLights = 8;

const char* const kSceneVertexShader = R"GLSL(
#version 330 core
layout(location = 0) in vec3 a_pos;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_uv;
uniform mat4 u_model;
uniform mat3 u_normal;
uniform mat4 u_view_proj;
out vec3 v_world;
out vec3 v_normal;
out vec2 v_uv;
void main()
{
  vec4 world = u_model * vec4(a_pos, 1.0);
  v_world    = world.xyz;
  v_normal   = u_normal * a_normal;
  v_uv       = a_uv;
  gl_Position = u_view_proj * world;
}
)GLSL";

const char* const kSceneFragmentShader = R"GLSL(
#version 330 core
in vec3 v_world;
in vec3 v_normal;
in vec2 v_uv;
out vec4 out_color;

uniform vec3 u_diffuse;
uniform vec3 u_specular;
uniform vec3 u_emissive;
uniform vec3 u_mat_ambient;
uniform float u_shininess;
uniform float u_ambient;
uniform vec3 u_cam_pos;
uniform sampler2D u_map;
uniform int u_has_map;
uniform int u_light_count;
uniform vec3 u_fog_color;
uniform float u_fog_start;
uniform float u_fog_end;

struct Light
{
  int type; // 0 directional, 1 point, 2 spot
  vec3 color;
  float intensity;
  vec3 position;
  vec3 direction;
  float constant;
  float linear;
  float quadratic;
  float cutoff;
  float smooth_width;
};
uniform Light u_lights[8];

vec3 shade_point(Light light, vec3 point, vec3 normal, vec3 view_dir, vec3 diffuse)
{
  vec3  light_dir;
  float ndl;
  float factor = 1.0;
  if (light.type == 0)
  {
    light_dir = -light.direction;
    ndl       = max(dot(normal, light_dir), 0.0);
  }
  else
  {
    vec3 to_source = light.position - point;
    float dist_sq  = dot(to_source, to_source);
    if (dist_sq < 1e-8)
    {
      return vec3(0.0);
    }
    float dist = sqrt(dist_sq);
    light_dir        = to_source / dist;
    ndl              = max(dot(normal, light_dir), 0.0);
    factor = 1.0 / (light.constant + light.linear * dist + light.quadratic * dist * dist);
    if (light.type == 2)
    {
      float cosine    = dot(light.direction, -light_dir);
      float cos_inner = cos(light.cutoff);
      float cos_outer = cos(light.cutoff + light.smooth_width);
      factor *= cosine >= cos_inner  ? 1.0
                : cosine <= cos_outer ? 0.0
                : (cosine - cos_outer) / max(cos_inner - cos_outer, 1e-6);
    }
  }
  if (ndl <= 0.0)
  {
    return vec3(0.0);
  }
  float spec = 0.0;
  if (u_specular != vec3(0.0))
  {
    spec = pow(max(dot(normal, normalize(light_dir + view_dir)), 0.0), u_shininess);
  }
  return light.color * light.intensity * factor * (diffuse * ndl + u_specular * spec);
}

void main()
{
  vec3 base = u_diffuse;
  if (u_has_map != 0)
  {
    base *= texture(u_map, vec2(v_uv.x, 1.0 - v_uv.y)).rgb;
  }
  vec3 normal   = normalize(v_normal);
  vec3 view_dir = normalize(u_cam_pos - v_world);
  vec3 shaded = u_mat_ambient * u_ambient * base; // ambient is modulated by the diffuse base
  for (int i = 0; i < u_light_count; ++i)
  {
    shaded += shade_point(u_lights[i], v_world, normal, view_dir, base);
  }
  shaded += u_emissive;
  if (u_fog_end > u_fog_start)
  {
    float dist = distance(u_cam_pos, v_world);
    float keep = clamp((u_fog_end - dist) / (u_fog_end - u_fog_start), 0.0, 1.0);
    shaded     = mix(u_fog_color, shaded, keep);
  }
  out_color = vec4(clamp(shaded, 0.0, 1.0), 1.0);
}
)GLSL";

const char* const kGradientVertexShader = R"GLSL(
#version 330 core
layout(location = 0) in vec2 a_pos;
out float v_t;
void main()
{
  v_t        = a_pos.y * 0.5 + 0.5; // -1 bottom .. +1 top
  gl_Position = vec4(a_pos, 0.0, 1.0);
}
)GLSL";

const char* const kGradientFragmentShader = R"GLSL(
#version 330 core
in float v_t;
out vec4 out_color;
uniform vec3 u_top;
uniform vec3 u_bottom;
void main()
{
  out_color = vec4(mix(u_bottom, u_top, v_t), 1.0);
}
)GLSL";

const char* const kOverlayVertexShader = R"GLSL(
#version 330 core
layout(location = 0) in vec2 a_pos;
out vec2 v_uv;
void main()
{
  v_uv        = a_pos * 0.5 + 0.5;
  gl_Position = vec4(a_pos, 0.0, 1.0);
}
)GLSL";

const char* const kOverlayFragmentShader = R"GLSL(
#version 330 core
in vec2 v_uv;
out vec4 out_color;
uniform sampler2D u_overlay;
void main()
{
  // The CPU overlay framebuffer is top-row-first; GL textures are bottom-first.
  out_color = texture(u_overlay, vec2(v_uv.x, 1.0 - v_uv.y));
}
)GLSL";

auto compile_shader(GLenum type, const char* source, std::string& error) -> GLuint
{
  const auto shader = glCreateShader(type);
  glShaderSource(shader, 1, &source, nullptr);
  glCompileShader(shader);
  auto status = GLint{0};
  glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
  if (status == 0)
  {
    char log[1024];
    glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
    error = log;
    glDeleteShader(shader);
    return 0;
  }
  return shader;
}

auto link_program(const char* vertex_src, const char* fragment_src, std::string& error) -> GLuint
{
  const auto vertex   = compile_shader(GL_VERTEX_SHADER, vertex_src, error);
  const auto fragment = vertex != 0 ? compile_shader(GL_FRAGMENT_SHADER, fragment_src, error) : 0u;
  if (vertex == 0 || fragment == 0)
  {
    return 0;
  }
  const auto program = glCreateProgram();
  glAttachShader(program, vertex);
  glAttachShader(program, fragment);
  glLinkProgram(program);
  glDeleteShader(vertex);
  glDeleteShader(fragment);
  auto status = GLint{0};
  glGetProgramiv(program, GL_LINK_STATUS, &status);
  if (status == 0)
  {
    char log[1024];
    glGetProgramInfoLog(program, sizeof(log), nullptr, log);
    error = log;
    glDeleteProgram(program);
    return 0;
  }
  return program;
}

/** @brief Content key for a texture (cache key for the GL texture table). */
auto texture_key(const Texture& texture) -> usz
{
  auto hash =
    std::hash<usz>{}((static_cast<usz>(texture.width) << 20) ^
                     (static_cast<usz>(texture.height) << 4) ^ static_cast<usz>(texture.channels));
  // Sample bytes spread across the image — cheap and collision-safe enough
  // for the same-size pixel-art textures this engine ships.
  const auto step = std::max<usz>(1, texture.data.size() / 256);
  for (usz i = 0; i < texture.data.size(); i += step)
  {
    hash ^= std::hash<ui8>{}(texture.data[i]) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
  }
  return hash;
}

} // namespace

struct GpuRenderer::Impl
{
  /** @brief One uploaded mesh: interleaved VBO + per-material index buffers. */
  struct GpuMesh
  {
    std::weak_ptr<const Model> owner; ///< liveness guard against address reuse
    GLuint                     vao{0};
    GLuint                     vbo{0};
    struct Segment
    {
      GLuint ebo{0};
      GLint  count{0};
      ui32   material{0};
    };
    std::vector<Segment> segments;
  };

  /** @brief Cached uniform locations of the scene program. */
  struct SceneUniforms
  {
    GLint model{-1};
    GLint normal{-1};
    GLint view_proj{-1};
    GLint diffuse{-1};
    GLint specular{-1};
    GLint emissive{-1};
    GLint mat_ambient{-1};
    GLint shininess{-1};
    GLint ambient{-1};
    GLint cam_pos{-1};
    GLint has_map{-1};
    GLint light_count{-1};
    GLint fog_color{-1};
    GLint fog_start{-1};
    GLint fog_end{-1};
  };

  GLuint        scene_program{0};
  SceneUniforms u{};
  GLuint        overlay_program{0};
  GLuint        gradient_program{0};
  GLint         gradient_top{-1};
  GLint         gradient_bottom{-1};
  GLuint        overlay_vao{0};
  GLuint        overlay_vbo{0};
  GLuint        overlay_texture{0};
  int           overlay_width{0};
  int           overlay_height{0};

  std::unordered_map<const Model*, GpuMesh> meshes;
  std::unordered_map<usz, GLuint>           textures;
  std::string                               last_error;

  void destroy_mesh(GpuMesh& mesh)
  {
    glDeleteVertexArrays(1, &mesh.vao);
    glDeleteBuffers(1, &mesh.vbo);
    for (auto& seg : mesh.segments)
    {
      glDeleteBuffers(1, &seg.ebo);
    }
  }

  ~Impl()
  {
    for (auto& [key, mesh] : meshes)
    {
      destroy_mesh(mesh);
    }
    for (auto& [key, tex] : textures)
    {
      glDeleteTextures(1, &tex);
    }
    if (overlay_texture != 0)
    {
      glDeleteTextures(1, &overlay_texture);
    }
    if (overlay_vbo != 0)
    {
      glDeleteBuffers(1, &overlay_vbo);
    }
    if (overlay_vao != 0)
    {
      glDeleteVertexArrays(1, &overlay_vao);
    }
    if (scene_program != 0)
    {
      glDeleteProgram(scene_program);
    }
    if (overlay_program != 0)
    {
      glDeleteProgram(overlay_program);
    }
    if (gradient_program != 0)
    {
      glDeleteProgram(gradient_program);
    }
  }

  auto init() -> bool
  {
    if (!load_gl_functions())
    {
      last_error = "OpenGL 3.3 entry points unavailable";
      return false;
    }
    scene_program = link_program(kSceneVertexShader, kSceneFragmentShader, last_error);
    if (scene_program == 0)
    {
      return false;
    }
    overlay_program = link_program(kOverlayVertexShader, kOverlayFragmentShader, last_error);
    if (overlay_program == 0)
    {
      return false;
    }
    gradient_program = link_program(kGradientVertexShader, kGradientFragmentShader, last_error);
    if (gradient_program == 0)
    {
      return false;
    }
    gradient_top    = glGetUniformLocation(gradient_program, "u_top");
    gradient_bottom = glGetUniformLocation(gradient_program, "u_bottom");

    glUseProgram(scene_program);
    u.model       = glGetUniformLocation(scene_program, "u_model");
    u.normal      = glGetUniformLocation(scene_program, "u_normal");
    u.view_proj   = glGetUniformLocation(scene_program, "u_view_proj");
    u.diffuse     = glGetUniformLocation(scene_program, "u_diffuse");
    u.specular    = glGetUniformLocation(scene_program, "u_specular");
    u.emissive    = glGetUniformLocation(scene_program, "u_emissive");
    u.mat_ambient = glGetUniformLocation(scene_program, "u_mat_ambient");
    u.shininess   = glGetUniformLocation(scene_program, "u_shininess");
    u.ambient     = glGetUniformLocation(scene_program, "u_ambient");
    u.cam_pos     = glGetUniformLocation(scene_program, "u_cam_pos");
    u.has_map     = glGetUniformLocation(scene_program, "u_has_map");
    u.light_count = glGetUniformLocation(scene_program, "u_light_count");
    u.fog_color   = glGetUniformLocation(scene_program, "u_fog_color");
    u.fog_start   = glGetUniformLocation(scene_program, "u_fog_start");
    u.fog_end     = glGetUniformLocation(scene_program, "u_fog_end");
    glUniform1i(glGetUniformLocation(scene_program, "u_map"), 0);

    // Fullscreen quad for the overlay layer (two triangles, NDC).
    static constexpr float kQuad[] = {-1, -1, 1, -1, 1, 1, -1, -1, 1, 1, -1, 1};
    glGenVertexArrays(1, &overlay_vao);
    glGenBuffers(1, &overlay_vbo);
    glBindVertexArray(overlay_vao);
    glBindBuffer(GL_ARRAY_BUFFER, overlay_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kQuad), kQuad, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
    glBindVertexArray(0);
    glGenTextures(1, &overlay_texture);
    glUseProgram(overlay_program);
    glUniform1i(glGetUniformLocation(overlay_program, "u_overlay"), 0);
    return true;
  }

  /** @brief Upload (or fetch the cached) GPU copy of a model's mesh. */
  auto mesh_for(const std::shared_ptr<const Model>& model) -> const GpuMesh&
  {
    const auto key = model.get();
    if (const auto it = meshes.find(key); it != meshes.end() && !it->second.owner.expired())
    {
      return it->second;
    }

    if (meshes.size() > 256)
    {
      for (auto& [old_key, mesh] : meshes)
      {
        destroy_mesh(mesh);
      }
      meshes.clear();
    }

    GpuMesh gpu_mesh;
    gpu_mesh.owner   = model;
    const auto& mesh = model->mesh;

    std::vector<float> vertices;
    vertices.reserve(mesh.vertices.size() * 8);
    for (const auto& vertex : mesh.vertices)
    {
      vertices.push_back(vertex.position.x);
      vertices.push_back(vertex.position.y);
      vertices.push_back(vertex.position.z);
      vertices.push_back(vertex.normal.x);
      vertices.push_back(vertex.normal.y);
      vertices.push_back(vertex.normal.z);
      vertices.push_back(vertex.texcoord.x);
      vertices.push_back(vertex.texcoord.y);
    }

    // One index buffer per material, in material-table order.
    std::vector<std::vector<ui32>> per_material(mesh.materials.size());
    for (const auto& tri : mesh.triangles)
    {
      const auto material = tri.material_index < mesh.materials.size() ? tri.material_index : 0u;
      auto&      indices  = per_material[material];
      indices.push_back(tri.a);
      indices.push_back(tri.b);
      indices.push_back(tri.c);
    }

    glGenVertexArrays(1, &gpu_mesh.vao);
    glGenBuffers(1, &gpu_mesh.vbo);
    glBindVertexArray(gpu_mesh.vao);
    glBindBuffer(GL_ARRAY_BUFFER, gpu_mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, narrow_cast<GLsizeiptr>(vertices.size() * sizeof(float)),
                 vertices.data(), GL_STATIC_DRAW);
    const auto stride = static_cast<GLsizei>(8 * sizeof(float));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<void*>(6 * sizeof(float)));

    for (usz material = 0; material < per_material.size(); ++material)
    {
      if (per_material[material].empty())
      {
        continue;
      }
      GpuMesh::Segment segment;
      segment.material = narrow_cast<ui32>(material);
      segment.count    = narrow_cast<GLint>(per_material[material].size());
      glGenBuffers(1, &segment.ebo);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, segment.ebo);
      glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                   narrow_cast<GLsizeiptr>(per_material[material].size() * sizeof(ui32)),
                   per_material[material].data(), GL_STATIC_DRAW);
      gpu_mesh.segments.push_back(segment);
    }
    glBindVertexArray(0);

    const auto [it, inserted] = meshes.emplace(key, std::move(gpu_mesh));
    if (!inserted)
    {
      // Address was reused by a live model: replace the stale entry.
      destroy_mesh(it->second);
      it->second = std::move(gpu_mesh);
    }
    return it->second;
  }

  /** @brief Upload (or fetch the cached) GL texture for a CPU texture. */
  auto texture_for(const Texture& texture) -> GLuint
  {
    const auto key = texture_key(texture);
    if (const auto it = textures.find(key); it != textures.end())
    {
      return it->second;
    }
    GLuint id = 0;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    const auto format = texture.channels == 3 ? GL_RGB : GL_RGBA;
    glTexImage2D(GL_TEXTURE_2D, 0, format, texture.width, texture.height, 0, format,
                 GL_UNSIGNED_BYTE, texture.data.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    textures.emplace(key, id);
    return id;
  }
};

GpuRenderer::GpuRenderer()  = default;
GpuRenderer::~GpuRenderer() = default;

auto GpuRenderer::init() -> bool
{
  m_impl = std::make_unique<Impl>();
  return m_impl->init();
}

auto GpuRenderer::ok() const -> bool { return m_impl != nullptr && m_impl->scene_program != 0; }

auto GpuRenderer::last_error() const -> const std::string&
{
  static const std::string kEmpty;
  return m_impl != nullptr ? m_impl->last_error : kEmpty;
}

auto GpuRenderer::render(const Scene& scene, const CameraF& camera,
                         const std::vector<LightF>& lights, float ambient, bool wireframe,
                         const Color& clear, TextureFilter filter, const FogSettings& fog,
                         int width, int height, const Color* sky_top, const Color* sky_bottom)
  -> RenderStats
{
  auto& impl = *m_impl;
  glViewport(0, 0, width, height);
  glClearColor(static_cast<float>(clear.r) / 255.0F, static_cast<float>(clear.g) / 255.0F,
               static_cast<float>(clear.b) / 255.0F, 1.0F);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Optional sky gradient: a fullscreen quad painted behind the scene.
  if (sky_top != nullptr && sky_bottom != nullptr)
  {
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glUseProgram(impl.gradient_program);
    glUniform3f(impl.gradient_top, static_cast<float>(sky_top->r) / 255.0F,
                static_cast<float>(sky_top->g) / 255.0F, static_cast<float>(sky_top->b) / 255.0F);
    glUniform3f(impl.gradient_bottom, static_cast<float>(sky_bottom->r) / 255.0F,
                static_cast<float>(sky_bottom->g) / 255.0F,
                static_cast<float>(sky_bottom->b) / 255.0F);
    glBindVertexArray(impl.overlay_vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    glDepthMask(GL_TRUE);
  }

  glEnable(GL_DEPTH_TEST);
  glDepthMask(GL_TRUE);
  glDisable(GL_BLEND);
  if (wireframe)
  {
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glDisable(GL_CULL_FACE);
  }
  else
  {
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glEnable(GL_CULL_FACE); // CCW front matches the CPU backface convention
  }

  glUseProgram(impl.scene_program);

  const auto view_proj = camera.view_projection().transpose(); // row -> column major
  glUniformMatrix4fv(impl.u.view_proj, 1, GL_FALSE, view_proj.data);
  const auto cam_pos = camera.position();
  glUniform3f(impl.u.cam_pos, cam_pos.x, cam_pos.y, cam_pos.z);
  glUniform1f(impl.u.ambient, ambient);
  glUniform3f(impl.u.fog_color, fog.color.x, fog.color.y, fog.color.z);
  glUniform1f(impl.u.fog_start, fog.start);
  glUniform1f(impl.u.fog_end, fog.end);

  const auto light_count = std::min<usz>(lights.size(), kMaxLights);
  glUniform1i(impl.u.light_count, narrow_cast<GLint>(light_count));
  for (usz i = 0; i < light_count; ++i)
  {
    const auto& light = lights[i];
    const auto  base  = "u_lights[" + std::to_string(i) + "].";
    const auto  loc   = [&](const char* field)
    { return glGetUniformLocation(impl.scene_program, (base + field).c_str()); };
    glUniform1i(loc("type"), static_cast<GLint>(light.type));
    glUniform3f(loc("color"), light.color.x, light.color.y, light.color.z);
    glUniform1f(loc("intensity"), light.intensity);
    glUniform3f(loc("position"), light.position.x, light.position.y, light.position.z);
    glUniform3f(loc("direction"), light.direction.x, light.direction.y, light.direction.z);
    glUniform1f(loc("constant"), light.constant);
    glUniform1f(loc("linear"), light.linear);
    glUniform1f(loc("quadratic"), light.quadratic);
    glUniform1f(loc("cutoff"), light.spot_cutoff);
    glUniform1f(loc("smooth_width"), light.spot_smooth);
  }

  const auto min_filter = filter == TextureFilter::Nearest ? GL_NEAREST : GL_LINEAR;
  for (const auto& object : scene.objects)
  {
    if (object == nullptr || object->model == nullptr)
    {
      continue;
    }
    const auto& model  = object->model;
    const auto  bounds = object->transformed_bounds(model->mesh.bounds);
    if (bounds.min.x <= bounds.max.x && !camera.frustum().contains_aabb(bounds))
    {
      continue; // same whole-object frustum culling as the CPU path
    }

    const auto& gpu_mesh = impl.mesh_for(model);
    if (gpu_mesh.segments.empty())
    {
      continue;
    }
    const auto model_matrix  = object->model_matrix().transpose();
    const auto normal_matrix = object->normal_matrix().transpose();
    glUniformMatrix4fv(impl.u.model, 1, GL_FALSE, model_matrix.data);
    glUniformMatrix3fv(impl.u.normal, 1, GL_FALSE, normal_matrix.data);

    glBindVertexArray(gpu_mesh.vao);
    for (const auto& segment : gpu_mesh.segments)
    {
      const auto& material =
        model->mesh.materials[std::min<usz>(segment.material, model->mesh.materials.size() - 1)];
      glUniform3f(impl.u.diffuse, material.diffuse.x, material.diffuse.y, material.diffuse.z);
      glUniform3f(impl.u.specular, material.specular.x, material.specular.y, material.specular.z);
      glUniform3f(impl.u.emissive, material.emissive.x, material.emissive.y, material.emissive.z);
      glUniform3f(impl.u.mat_ambient, material.ambient.x, material.ambient.y, material.ambient.z);
      glUniform1f(impl.u.shininess, material.shininess);

      auto has_map = false;
      if (segment.material < model->textures.size())
      {
        const auto& texture = model->textures[segment.material];
        if (!texture.empty())
        {
          const auto id = impl.texture_for(texture);
          glActiveTexture(GL_TEXTURE0);
          glBindTexture(GL_TEXTURE_2D, id);
          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, min_filter);
          has_map = true;
        }
      }
      glUniform1i(impl.u.has_map, has_map ? 1 : 0);

      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, segment.ebo);
      glDrawElements(GL_TRIANGLES, segment.count, GL_UNSIGNED_INT, nullptr);
    }
    glBindVertexArray(0);
  }
  return {};
}

auto GpuRenderer::draw_overlay(const FrameBuffer& overlay) -> void
{
  auto& impl = *m_impl;
  glDisable(GL_DEPTH_TEST);
  glDepthMask(GL_FALSE);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glBindTexture(GL_TEXTURE_2D, impl.overlay_texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  if (impl.overlay_width != overlay.width() || impl.overlay_height != overlay.height())
  {
    impl.overlay_width  = overlay.width();
    impl.overlay_height = overlay.height();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, overlay.width(), overlay.height(), 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, overlay.color_data());
  }
  else
  {
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, overlay.width(), overlay.height(), GL_RGBA,
                    GL_UNSIGNED_BYTE, overlay.color_data());
  }

  glUseProgram(impl.overlay_program);
  glBindVertexArray(impl.overlay_vao);
  glDrawArrays(GL_TRIANGLES, 0, 6);
  glBindVertexArray(0);

  glDepthMask(GL_TRUE);
  glEnable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);
}

auto GpuRenderer::readback(FrameBuffer& out) -> bool
{
  auto viewport = std::array<GLint, 4>{};
  glGetIntegerv(GL_VIEWPORT, viewport.data());
  const auto width  = static_cast<int>(viewport[2]);
  const auto height = static_cast<int>(viewport[3]);
  if (width <= 0 || height <= 0)
  {
    return false;
  }
  out.resize(width, height);
  std::vector<ui8> pixels(static_cast<usz>(width) * static_cast<usz>(height) * 4);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
  // GL row 0 is the bottom; the framebuffer is top-row-first.
  const auto row = static_cast<usz>(width) * 4;
  for (int y = 0; y < height; ++y)
  {
    const auto* src = pixels.data() + static_cast<usz>(height - 1 - y) * row;
    for (int x = 0; x < width; ++x)
    {
      out.color_at(static_cast<usz>(y) * static_cast<usz>(width) + static_cast<usz>(x)) =
        Color{src[x * 4 + 0], src[x * 4 + 1], src[x * 4 + 2], src[x * 4 + 3]};
    }
  }
  return true;
}

} // namespace obzcene
