cmake_minimum_required(VERSION 3.24)
project(DarkEngine LANGUAGES C CXX)

option(DARK_TARGET_VITA "Configure the PS Vita port" OFF)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/Vita.cmake")

if(DARK_TARGET_VITA)
  return()
endif()

if(NOT WIN32)
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

if(APPLE)
  enable_language(OBJC)
endif()

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/Bison.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/LibFFmpeg.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/Zlib.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/LibPng.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/OpenALSoft.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/SDL.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/Vulkan.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/Squirrel.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/OggVorbis.cmake")

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/Sanitizers.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/Tracy.cmake")

# Keep executable paths stable when their targets are defined in src/.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/EngineConfig.cmake")

set(dark_win32_native_osm_default OFF)
if(WIN32 AND CMAKE_SIZEOF_VOID_P EQUAL 4)
  set(dark_win32_native_osm_default ON)
endif()
option(DARK_WIN32_NATIVE_OSM
  "Load compatible Thief II OSM modules natively in 32-bit Windows builds"
  ${dark_win32_native_osm_default})
if(DARK_WIN32_NATIVE_OSM AND
   (NOT WIN32 OR NOT CMAKE_SIZEOF_VOID_P EQUAL 4))
  message(FATAL_ERROR
    "DARK_WIN32_NATIVE_OSM requires a 32-bit Windows build")
endif()

add_subdirectory(libsrc)
add_subdirectory(src)
add_subdirectory(assets)

if(NOT ANDROID)
  enable_testing()
  add_subdirectory(tools/material_test EXCLUDE_FROM_ALL)
  add_subdirectory(tools/movie_test EXCLUDE_FROM_ALL)
  add_subdirectory(tools/md_hit_test EXCLUDE_FROM_ALL)
endif()

if(NOT ANDROID AND DARK_COMPILE_VULKAN_SHADERS)
  install(FILES ${DARK_VULKAN_SHADER_OUTPUTS}
    DESTINATION bin/generated/vulkan)
endif()
