cmake_minimum_required(VERSION 3.20)
project(DarkEnginePortability LANGUAGES C CXX)
if(APPLE)
  enable_language(OBJCXX)
endif()

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/PlatformDependencies.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/OpenALSoft.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/LibPng.cmake")

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)

if(WIN32)
  set(DARK_PLATFORM_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/winsrc/platform")
  set(DARK_PLATFORM_SOURCE "${DARK_PLATFORM_ROOT}/platform_services.cpp")
else()
  set(DARK_PLATFORM_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/unixsrc/platform")
  set(DARK_PLATFORM_SOURCE "${DARK_PLATFORM_ROOT}/platform_services.cpp")
endif()

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/Vulkan.cmake")

add_library(dark_platform STATIC
  ${DARK_PLATFORM_SOURCE}
  libsrc/cpptools/dynfunc.cpp
)
target_include_directories(dark_platform PUBLIC
  h
  libsrc/cpptools
  ${DARK_PLATFORM_ROOT}
)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  # unixsrc/platform uses POSIX interfaces such as setenv; strict C++17 mode
  # hides those declarations on glibc unless the default feature set is enabled.
  target_compile_definitions(dark_platform PRIVATE _DEFAULT_SOURCE)
endif()
if(WIN32)
  target_link_libraries(dark_platform PUBLIC
    advapi32 user32 ws2_32)
endif()
target_link_libraries(dark_platform PUBLIC ${DARK_SDL_TARGET})
if(NOT WIN32)
  target_link_libraries(dark_platform PUBLIC ${CMAKE_DL_LIBS})
  if(ANDROID)
    target_link_libraries(dark_platform PUBLIC "${DARK_ANDROID_LOG_LIBRARY}")
  endif()
endif()

add_library(dark_compression STATIC 3rdparty/blast/blast.c)
target_include_directories(dark_compression PUBLIC 3rdparty/blast)

option(DARK_BUILD_ENGINE "Compile the platform-neutral engine libraries" ON)

if(DARK_BUILD_ENGINE)
  include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/EngineLibraries.cmake")
  include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/EnginePrograms.cmake")
endif()

if(NOT ANDROID)
  if(TARGET thief2)
    install(TARGETS thief2 RUNTIME DESTINATION bin)
  endif()
  if(TARGET dromed)
    install(TARGETS dromed RUNTIME DESTINATION bin)
  endif()
  if(DARK_COMPILE_VULKAN_SHADERS)
    install(FILES ${DARK_VULKAN_SHADER_OUTPUTS}
      DESTINATION bin/generated/vulkan)
  endif()
endif()
