add_library(dark_lgd3d STATIC
  clip.c
  modern.cpp
  render.c
  tmgr.c
  vulkan.cpp
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
