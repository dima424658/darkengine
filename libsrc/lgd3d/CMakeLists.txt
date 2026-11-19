add_library(dark_lgd3d STATIC
  legacy_driver.cpp
  legacy_textures.cpp
  legacy_primitives.c
  legacy_texture_manager.c
  vulkan.cpp
  world_scene.cpp
  vulkan_world_geometry.cpp
  vulkan_world_features.cpp
  vulkan_context.cpp
  vulkan_render_backend.cpp
  vulkan_scene_renderer.cpp
)
dark_configure_library(dark_lgd3d)

target_link_libraries(dark_lgd3d PRIVATE Vulkan::Vulkan)
if(ANDROID)
  target_include_directories(dark_lgd3d SYSTEM PRIVATE
    "${DARK_VULKAN_HEADERS_SOURCE}/include")
endif()
if(DEFINED DARK_VULKAN_SHADER_DIR)
  target_compile_definitions(dark_lgd3d PRIVATE
    DARK_VULKAN_SHADER_DIR="${DARK_VULKAN_SHADER_DIR}")
endif()
if(TARGET dark_vulkan_shaders)
  add_dependencies(dark_lgd3d dark_vulkan_shaders)
endif()

if(DARK_ENABLE_RTX_SDK)
  target_sources(dark_lgd3d PRIVATE vulkan_ray_scene.cpp vulkan_world_renderer.cpp)
  target_link_libraries(dark_lgd3d PRIVATE dark_rtxdi_vulkan)
  target_compile_definitions(dark_lgd3d PRIVATE DARK_RTX_SDK_AVAILABLE=1)
else()
  target_sources(dark_lgd3d PRIVATE vulkan_world_renderer_stub.cpp)
endif()
