cmake_minimum_required(VERSION 3.21)
project(vulkan_canvas_test LANGUAGES CXX)
find_package(Vulkan REQUIRED)
find_package(SDL3 REQUIRED CONFIG)
add_executable(vulkan_canvas_test main.cpp
  ../../libsrc/lgd3d/vulkan_context.cpp
  ../../libsrc/lgd3d/vulkan_scene_renderer.cpp)
target_compile_features(vulkan_canvas_test PRIVATE cxx_std_17)
target_include_directories(vulkan_canvas_test PRIVATE ../../h ../../libsrc/lgd3d)
target_link_libraries(vulkan_canvas_test PRIVATE Vulkan::Vulkan SDL3::SDL3)
