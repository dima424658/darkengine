cmake_minimum_required(VERSION 3.24)
project(DarkEngine LANGUAGES C CXX)

if(NOT WIN32)
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

if(APPLE)
  enable_language(OBJC)
endif()

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/Bison.cmake")
set(DARK_WINDOWS_MOVIE_BACKEND "ffmpeg" CACHE STRING
  "Windows movie backend: ffmpeg or mfplay")
set_property(CACHE DARK_WINDOWS_MOVIE_BACKEND PROPERTY STRINGS
  ffmpeg mfplay)
if(WIN32 AND NOT DARK_WINDOWS_MOVIE_BACKEND MATCHES "^(ffmpeg|mfplay)$")
  message(FATAL_ERROR "DARK_WINDOWS_MOVIE_BACKEND must be 'ffmpeg' or 'mfplay'")
endif()
if(NOT WIN32 OR DARK_WINDOWS_MOVIE_BACKEND STREQUAL "ffmpeg")
  include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/LibFFmpeg.cmake")
endif()
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

add_subdirectory(libsrc)
add_subdirectory(src)
add_subdirectory(assets)

if(NOT ANDROID)
  enable_testing()
  add_subdirectory(tools/material_test EXCLUDE_FROM_ALL)
  add_subdirectory(tools/movie_test EXCLUDE_FROM_ALL)
endif()

if(NOT ANDROID AND DARK_COMPILE_VULKAN_SHADERS)
  install(FILES ${DARK_VULKAN_SHADER_OUTPUTS}
    DESTINATION bin/generated/vulkan)
endif()
