# Shared SDL3/Vulkan renderer foundation and shader compilation.
option(DARK_COMPILE_VULKAN_SHADERS
  "Compile Vulkan renderer GLSL shaders to SPIR-V" ON)
if(NOT ANDROID AND NOT DARK_COMPILE_VULKAN_SHADERS)
  message(FATAL_ERROR
    "Desktop SDL3 builds require Vulkan shader compilation")
endif()
find_package(Vulkan REQUIRED)
if(DARK_COMPILE_VULKAN_SHADERS)
  find_program(DARK_GLSLC_EXECUTABLE NAMES glslc
    HINTS "$ENV{VULKAN_SDK}/bin" "$ENV{VULKAN_SDK}/Bin"
    REQUIRED)
  set(DARK_VULKAN_SHADER_OUTPUT_DIR
    "${CMAKE_CURRENT_BINARY_DIR}/generated/vulkan")
  file(MAKE_DIRECTORY "${DARK_VULKAN_SHADER_OUTPUT_DIR}")
  set(DARK_VULKAN_SHADER_OUTPUTS)
  foreach(shader scene.vert scene.frag)
    set(shader_source "${CMAKE_CURRENT_SOURCE_DIR}/libsrc/lgvk/shaders/${shader}")
    set(shader_output "${DARK_VULKAN_SHADER_OUTPUT_DIR}/${shader}.spv")
    add_custom_command(OUTPUT "${shader_output}"
      COMMAND "${DARK_GLSLC_EXECUTABLE}" --target-env=vulkan1.1
        "${shader_source}" -o "${shader_output}"
      DEPENDS "${shader_source}"
      VERBATIM)
    list(APPEND DARK_VULKAN_SHADER_OUTPUTS "${shader_output}")
  endforeach()
  add_custom_target(dark_vulkan_shaders DEPENDS ${DARK_VULKAN_SHADER_OUTPUTS})
endif()
if(ANDROID)
  # Android packages shader bytecode as app assets; SDL_LoadFile reads these from the APK.
  set(DARK_VULKAN_SHADER_DIR "vulkan")
elseif(DARK_COMPILE_VULKAN_SHADERS)
  # Runtime resolves installed SPIR-V files relative to SDL_GetBasePath().
  set(DARK_VULKAN_SHADER_DIR "generated/vulkan")
endif()
if(NOT CMAKE_SYSTEM_NAME MATCHES "^(Linux|Darwin)$" AND NOT ANDROID AND NOT WIN32)
  message(FATAL_ERROR "lgvk requires SDL3 surface support")
endif()
if(ANDROID AND (NOT DARK_VULKAN_HEADERS_SOURCE OR
   NOT EXISTS "${DARK_VULKAN_HEADERS_SOURCE}/include/vulkan/vulkan.hpp"))
  message(FATAL_ERROR
    "Android requires DARK_VULKAN_HEADERS_SOURCE to name the extracted Khronos Vulkan-Headers tree")
endif()
set(dark_lgvk_sources
  libsrc/lgvk/vulkan.cpp
  libsrc/lgvk/vulkan_context.cpp
  libsrc/lgvk/vulkan_render_backend.cpp
  libsrc/lgvk/vulkan_scene_renderer.cpp)
list(APPEND dark_lgvk_sources
  libsrc/lgvk/vulkan_sdl.cpp)
add_library(lgvk STATIC ${dark_lgvk_sources})
target_include_directories(lgvk PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/libsrc/lgvk"
  "${CMAKE_CURRENT_SOURCE_DIR}/h")
if(ANDROID)
  target_include_directories(lgvk SYSTEM PUBLIC
    "${DARK_VULKAN_HEADERS_SOURCE}/include")
endif()
target_link_libraries(lgvk PUBLIC Vulkan::Vulkan)
if(DARK_SDL_TARGET)
  target_link_libraries(lgvk PUBLIC ${DARK_SDL_TARGET})
endif()
if(DARK_COMPILE_VULKAN_SHADERS)
  add_dependencies(lgvk dark_vulkan_shaders)
endif()
if(DEFINED DARK_VULKAN_SHADER_DIR)
  target_compile_definitions(lgvk PRIVATE
    DARK_VULKAN_SHADER_DIR="${DARK_VULKAN_SHADER_DIR}")
endif()
if(WIN32)
  target_compile_definitions(lgvk PUBLIC
    NOMINMAX WIN32_LEAN_AND_MEAN)
endif()
