# Shared SDL3/Vulkan renderer foundation and shader compilation.
set(dark_rtx_sdk_default OFF)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT ANDROID)
  set(dark_rtx_sdk_default ON)
endif()
option(DARK_ENABLE_RTX_SDK "Build NVIDIA RTXDI/SHaRC/NRD lighting dependencies" ${dark_rtx_sdk_default})

option(DARK_COMPILE_VULKAN_SHADERS
  "Compile Vulkan renderer GLSL shaders to SPIR-V" ON)
if(NOT ANDROID AND NOT DARK_COMPILE_VULKAN_SHADERS)
  message(FATAL_ERROR
    "Desktop SDL3 builds require Vulkan shader compilation")
endif()
find_package(Vulkan MODULE REQUIRED)

if(DARK_COMPILE_VULKAN_SHADERS)
  if(NOT Vulkan_GLSLC_EXECUTABLE)
    message(FATAL_ERROR
      "Vulkan was found, but FindVulkan could not find glslc. Install the Vulkan SDK or set Vulkan_GLSLC_EXECUTABLE.")
  endif()
  set(DARK_VULKAN_SHADER_OUTPUT_DIR
    "${CMAKE_CURRENT_BINARY_DIR}/generated/vulkan")
  file(MAKE_DIRECTORY "${DARK_VULKAN_SHADER_OUTPUT_DIR}")
  set(DARK_VULKAN_SHADER_OUTPUTS)
  # Native renderer shaders remain independent of RTX SDK sources and tools.
  foreach(shader scene.vert scene.frag)
    set(shader_source "${PROJECT_SOURCE_DIR}/libsrc/lgd3d/shaders/${shader}")
    set(shader_output "${DARK_VULKAN_SHADER_OUTPUT_DIR}/${shader}.spv")
    add_custom_command(OUTPUT "${shader_output}"
      COMMAND "${Vulkan_GLSLC_EXECUTABLE}" "$<IF:$<CONFIG:Debug>,-O0,-O>" --target-env=vulkan1.1
        "${shader_source}" -o "${shader_output}" DEPENDS "${shader_source}" VERBATIM)
    list(APPEND DARK_VULKAN_SHADER_OUTPUTS "${shader_output}")
  endforeach()
  if(DARK_ENABLE_RTX_SDK)
    include("${CMAKE_CURRENT_LIST_DIR}/NrdShaderCompatibility.cmake")
    dark_prepare_nrd_glsl("${DARK_VULKAN_SHADER_OUTPUT_DIR}/include")
    include("${CMAKE_CURRENT_LIST_DIR}/RtxdiShaderCompatibility.cmake")
    dark_prepare_rtxdi_glsl("${DARK_VULKAN_SHADER_OUTPUT_DIR}/include")
    set(dark_world_material_headers
      "${PROJECT_SOURCE_DIR}/libsrc/lgd3d/shaders/surface_geometry.glsl"
      "${PROJECT_SOURCE_DIR}/libsrc/lgd3d/shaders/world_material.glsl"
      "${PROJECT_SOURCE_DIR}/libsrc/lgd3d/shaders/material_sampling.glsl")
    foreach(shader world.vert world_surface.vert world_surface.frag world_lighting.frag world_lighting_native.frag
        world_composite.vert world_composite.frag world_transparent.frag world_transparent_native.frag dlss_prepare.comp)
      set(shader_source "${PROJECT_SOURCE_DIR}/libsrc/lgd3d/shaders/${shader}")
      set(shader_output "${DARK_VULKAN_SHADER_OUTPUT_DIR}/${shader}.spv")
      add_custom_command(OUTPUT "${shader_output}"
        COMMAND "${Vulkan_GLSLC_EXECUTABLE}" "$<IF:$<CONFIG:Debug>,-O0,-O>" --target-env=vulkan1.2 --target-spv=spv1.4
          -I "${DARK_VULKAN_SHADER_OUTPUT_DIR}/include" "${shader_source}" -o "${shader_output}"
        DEPENDS "${shader_source}" ${dark_world_material_headers} "${DARK_VULKAN_SHADER_OUTPUT_DIR}/include/NRD_glsl.hlsli"
          "${PROJECT_SOURCE_DIR}/libsrc/lgd3d/shaders/world_lighting_body.glsl"
          "${PROJECT_SOURCE_DIR}/libsrc/lgd3d/shaders/world_transparent_body.glsl"
          "${PROJECT_SOURCE_DIR}/libsrc/lgd3d/shaders/ray_scene.glsl" VERBATIM)
      list(APPEND DARK_VULKAN_SHADER_OUTPUTS "${shader_output}")
    endforeach()
  foreach(pass RANGE 0 2)
    set(shader_output "${DARK_VULKAN_SHADER_OUTPUT_DIR}/rtxdi_${pass}.comp.spv")
    add_custom_command(OUTPUT "${shader_output}"
      COMMAND "${Vulkan_GLSLC_EXECUTABLE}" "$<IF:$<CONFIG:Debug>,-O0,-O>" --target-env=vulkan1.2 --target-spv=spv1.4
        -DDI_PASS=${pass} -I "${DARK_VULKAN_SHADER_OUTPUT_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/libsrc/lgd3d/shaders/rtxdi.comp" -o "${shader_output}"
      DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/libsrc/lgd3d/shaders/rtxdi.comp" ${dark_world_material_headers}
        "${CMAKE_CURRENT_SOURCE_DIR}/libsrc/lgd3d/shaders/rtxdi_bridge.glsl"
        "${CMAKE_CURRENT_SOURCE_DIR}/libsrc/lgd3d/shaders/ray_scene.glsl"
        "${DARK_VULKAN_SHADER_OUTPUT_DIR}/include/Rtxdi/DI/TemporalResampling.hlsli"
        "${DARK_VULKAN_SHADER_OUTPUT_DIR}/include/Rtxdi/DI/SpatialResampling.hlsli"
        "${DARK_VULKAN_SHADER_OUTPUT_DIR}/include/Rtxdi/Utils/Math.hlsli" VERBATIM)
    list(APPEND DARK_VULKAN_SHADER_OUTPUTS "${shader_output}")
  endforeach()
    file(GLOB sharc_sdk_headers CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/3rdparty/RTXGI/Libraries/Sharc/include/*.h")
    foreach(shader sharc_update sharc_resolve sharc_query)
      set(shader_output "${DARK_VULKAN_SHADER_OUTPUT_DIR}/${shader}.comp.spv")
      add_custom_command(OUTPUT "${shader_output}"
        COMMAND "${Vulkan_GLSLC_EXECUTABLE}" "$<IF:$<CONFIG:Debug>,-O0,-O>" --target-env=vulkan1.2 --target-spv=spv1.4
          -I "${DARK_VULKAN_SHADER_OUTPUT_DIR}/include" -I "${PROJECT_SOURCE_DIR}/3rdparty/RTXGI/Libraries/Sharc/include"
          "${PROJECT_SOURCE_DIR}/libsrc/lgd3d/shaders/${shader}.comp" -o "${shader_output}"
        DEPENDS "${PROJECT_SOURCE_DIR}/libsrc/lgd3d/shaders/${shader}.comp" ${dark_world_material_headers}
          "${PROJECT_SOURCE_DIR}/libsrc/lgd3d/shaders/sharc_bridge.glsl"
          "${PROJECT_SOURCE_DIR}/libsrc/lgd3d/shaders/rtxdi_bridge.glsl"
          "${PROJECT_SOURCE_DIR}/libsrc/lgd3d/shaders/ray_scene.glsl" ${sharc_sdk_headers} VERBATIM)
      list(APPEND DARK_VULKAN_SHADER_OUTPUTS "${shader_output}")
    endforeach()
  endif()
  add_custom_target(dark_vulkan_shaders DEPENDS ${DARK_VULKAN_SHADER_OUTPUTS})
endif()
if(ANDROID)
  # Android packages shader bytecode as app assets; SDL_LoadFile reads these from the APK.
  set(DARK_VULKAN_SHADER_DIR "vulkan")
elseif(DARK_COMPILE_VULKAN_SHADERS)
  # Runtime resolves installed SPIR-V files relative to SDL_GetBasePath().
  set(DARK_VULKAN_SHADER_DIR "generated/vulkan")
endif()
if(NOT LINUX AND NOT APPLE AND NOT ANDROID AND NOT WIN32)
message(FATAL_ERROR "lgd3d requires SDL3 surface support")
endif()
if(ANDROID AND (NOT DARK_VULKAN_HEADERS_SOURCE OR
   NOT EXISTS "${DARK_VULKAN_HEADERS_SOURCE}/include/vulkan/vulkan.hpp"))
  message(FATAL_ERROR
    "Android requires DARK_VULKAN_HEADERS_SOURCE to name the extracted Khronos Vulkan-Headers tree")
endif()
