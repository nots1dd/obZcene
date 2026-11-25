cmake_minimum_required(VERSION 3.16)

project(obZcene LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_definitions(-DOBZCENE_DEBUG_BUILD)
endif()

find_package(Threads REQUIRED)
find_package(SDL2 CONFIG REQUIRED)
find_package(SDL2_image CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

set(BACKTRACE_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/external/libbacktrace/build/install")

add_library(obzcene_backtrace STATIC IMPORTED GLOBAL)
set_target_properties(obzcene_backtrace PROPERTIES
  IMPORTED_LOCATION "${BACKTRACE_PREFIX}/lib/libbacktrace.a"
)

add_subdirectory(external/spdlog)
add_subdirectory(external/CLI11)

# ============================================================
# Engine library — everything a game or tool links against.
# ============================================================

add_library(obzcene STATIC
  src/Core/Logger.cpp
  src/Renderer/GpuRenderer.cpp
  src/Renderer/Window.cpp
  src/Texture/ImageWriter.cpp
  src/Texture/TextureLoader.cpp
  src/Viewer/Overlay.cpp
  src/Viewer/Scene.cpp
  src/Viewer/Viewer.cpp
  src/Engine/Assets.cpp
  src/Engine/Audio.cpp
  src/Engine/Engine.cpp
)

target_include_directories(obzcene PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_include_directories(obzcene PRIVATE
  "${BACKTRACE_PREFIX}/include"
)

target_link_libraries(obzcene PUBLIC
  obzcene_backtrace
  spdlog::spdlog
  Threads::Threads
  SDL2::SDL2
  SDL2_image::SDL2_image
  OpenGL::GL
)

# ============================================================
# Editor / viewer executable (CLI front-end over the library).
# ============================================================

add_executable(obZcene
  src/App/App.cpp
  src/App/Scene.cpp
  src/obZcene.cpp
)

target_link_libraries(obZcene PRIVATE
  obzcene
  CLI11::CLI11
)

# ============================================================
# Tests
# ============================================================

enable_testing()

add_subdirectory(external/catch2)

add_executable(math-tests
  tests/test_vec.cpp
  tests/test_mat3.cpp
  tests/test_mat4.cpp
  tests/test_quat.cpp
  tests/test_geom.cpp
)

target_include_directories(math-tests PRIVATE
  include
)

target_link_libraries(math-tests PRIVATE Catch2::Catch2WithMain)

add_test(NAME math-tests COMMAND math-tests)

add_executable(parser-tests
  tests/test_logger.cpp
  tests/test_mtl_parser.cpp
  tests/test_obj_parser.cpp
)

target_link_libraries(parser-tests PRIVATE
  Catch2::Catch2WithMain
  obzcene
)

target_compile_definitions(parser-tests PRIVATE
  ASSETS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets"
)

add_test(NAME parser-tests COMMAND parser-tests)
set_tests_properties(parser-tests PROPERTIES
  ENVIRONMENT "ASSETS_DIR=${CMAKE_CURRENT_SOURCE_DIR}/assets"
)

add_executable(camera-tests
  tests/test_camera.cpp
)

target_include_directories(camera-tests PRIVATE
  include
)

target_link_libraries(camera-tests PRIVATE Catch2::Catch2WithMain)

add_test(NAME camera-tests COMMAND camera-tests)

add_executable(light-tests
  tests/test_light.cpp
)

target_include_directories(light-tests PRIVATE
  include
)

target_link_libraries(light-tests PRIVATE Catch2::Catch2WithMain)

add_test(NAME light-tests COMMAND light-tests)

add_executable(renderer-tests
  tests/test_renderer.cpp
)

target_link_libraries(renderer-tests PRIVATE
  Catch2::Catch2WithMain
  obzcene
)

add_test(NAME renderer-tests COMMAND renderer-tests)
set_tests_properties(renderer-tests PROPERTIES
  ENVIRONMENT "ASSETS_DIR=${CMAKE_CURRENT_SOURCE_DIR}/assets"
)

add_executable(culling-tests
  tests/test_culling.cpp
)

target_link_libraries(culling-tests PRIVATE
  Catch2::Catch2WithMain
  obzcene
)

add_test(NAME culling-tests COMMAND culling-tests)

add_executable(overlay-tests
  tests/test_overlay.cpp
)

target_link_libraries(overlay-tests PRIVATE
  Catch2::Catch2WithMain
  obzcene
)

add_test(NAME overlay-tests COMMAND overlay-tests)

add_executable(scene-tests
  tests/test_scene.cpp
)

target_link_libraries(scene-tests PRIVATE
  Catch2::Catch2WithMain
  obzcene
)

add_test(NAME scene-tests COMMAND scene-tests)
set_tests_properties(scene-tests PROPERTIES
  ENVIRONMENT "ASSETS_DIR=${CMAKE_CURRENT_SOURCE_DIR}/assets"
)

add_executable(engine-tests
  tests/test_engine.cpp
  tests/test_input.cpp
  tests/test_audio.cpp
)

target_link_libraries(engine-tests PRIVATE
  Catch2::Catch2WithMain
  obzcene
)

add_test(NAME engine-tests COMMAND engine-tests)
set_tests_properties(engine-tests PROPERTIES
  ENVIRONMENT "ASSETS_DIR=${CMAKE_CURRENT_SOURCE_DIR}/assets"
)

# ============================================================
# Examples — small games written against the public API.
# ============================================================

add_executable(sandbox-example
  examples/sandbox.cpp
)

target_link_libraries(sandbox-example PRIVATE
  obzcene
)
