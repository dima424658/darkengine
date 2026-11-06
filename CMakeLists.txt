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

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/Vulkan.cmake")

if(WIN32)
  set(DARK_PLATFORM_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/winsrc/platform")
  set(DARK_PLATFORM_SOURCE "${DARK_PLATFORM_ROOT}/platform_services.cpp")
else()
  set(DARK_PLATFORM_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/unixsrc/platform")
  set(DARK_PLATFORM_SOURCE "${DARK_PLATFORM_ROOT}/platform_services.cpp")
endif()

add_library(dark_platform STATIC
  ${DARK_PLATFORM_SOURCE}
  libsrc/cpptools/dynfunc.cpp
)
target_include_directories(dark_platform PUBLIC
  h
  libsrc/cpptools
  ${DARK_PLATFORM_ROOT}
)
if(WIN32)
  target_link_libraries(dark_platform PUBLIC
    advapi32 comctl32 gdi32 user32 winmm ws2_32)
endif()
if(NOT WIN32)
  target_link_libraries(dark_platform PUBLIC ${CMAKE_DL_LIBS})
  if(CMAKE_SYSTEM_NAME MATCHES "^(Linux|Darwin)$" OR ANDROID OR WIN32)
    target_link_libraries(dark_platform PUBLIC ${DARK_SDL_TARGET})
  endif()
  if(ANDROID)
    target_link_libraries(dark_platform PUBLIC "${DARK_ANDROID_LOG_LIBRARY}")
  endif()
endif()

add_library(dark_compression STATIC 3rdparty/blast/blast.c)
target_include_directories(dark_compression PUBLIC 3rdparty/blast)

if(CMAKE_SYSTEM_NAME MATCHES "^(Linux|Darwin)$" OR ANDROID OR WIN32)
  set(dark_build_engine_default ON)
else()
  set(dark_build_engine_default OFF)
endif()
option(DARK_BUILD_ENGINE "Compile the platform-neutral engine libraries" ${dark_build_engine_default})

if(DARK_BUILD_ENGINE)
  include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/EngineLibraries.cmake")
  include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/EnginePrograms.cmake")
endif()

if((CMAKE_SYSTEM_NAME MATCHES "^(Linux|Darwin)$" OR ANDROID OR WIN32) AND
   NOT ANDROID)
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
