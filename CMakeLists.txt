cmake_minimum_required(VERSION 3.20)
project(DarkEngine LANGUAGES C CXX)
if(NOT WIN32)
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()
if(APPLE)
  enable_language(OBJCXX)
endif()

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/LibFFmpeg.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/LibPng.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/OpenALSoft.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/SDL.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/Zlib.cmake")
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
add_subdirectory(unixsrc)
if(WIN32)
  add_subdirectory(winsrc)
endif()
add_subdirectory(assets)

if(NOT ANDROID AND DARK_COMPILE_VULKAN_SHADERS)
  install(FILES ${DARK_VULKAN_SHADER_OUTPUTS}
    DESTINATION bin/generated/vulkan)
endif()
