cmake_minimum_required(VERSION 3.24)
project(vita_content_test LANGUAGES CXX)

enable_testing()

add_executable(vita_content_test
  main.cpp
  ../../libsrc/platform/vita_paths.cpp
  ../../libsrc/platform/vita_subtitles.cpp
  ../../libsrc/platform/vita_controls.cpp
  ../../libsrc/platform/vita_video_settings.cpp
  ../../libsrc/platform/vita_texture_limits.cpp
  ../../libsrc/platform/vita_acoustics.cpp
  ../../libsrc/platform/vita_power.cpp
  ../../libsrc/platform/vita_ghost_check.cpp
  ../../libsrc/platform/vita_readable_zoom.cpp
)
target_include_directories(vita_content_test PRIVATE
  ../../libsrc/platform
)
target_compile_features(vita_content_test PRIVATE cxx_std_17)
add_test(NAME vita_content COMMAND vita_content_test)
