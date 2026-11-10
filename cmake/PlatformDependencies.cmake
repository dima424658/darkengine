# SDL3 and media/image dependencies. Desktop builds static SDL3 from its
# submodule; Android retains its SDL AAR. zlib is a static submodule on all
# platforms.
if(NOT ANDROID)
  set(SDL_SHARED OFF CACHE BOOL "Build SDL3 statically" FORCE)
  set(SDL_STATIC ON CACHE BOOL "Build SDL3 statically" FORCE)
  set(SDL_TESTS OFF CACHE BOOL "Do not build SDL3 tests" FORCE)
  set(SDL_EXAMPLES OFF CACHE BOOL "Do not build SDL3 examples" FORCE)
  set(SDL_INSTALL OFF CACHE BOOL "Do not install SDL3 from this build" FORCE)
  add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/SDL"
    "${CMAKE_BINARY_DIR}/third_party/SDL" EXCLUDE_FROM_ALL)
  set(DARK_SDL_TARGET SDL3::SDL3)

else()
  find_package(SDL3 CONFIG REQUIRED)
  set(DARK_SDL_TARGET SDL3::SDL3)
endif()

set(ZLIB_BUILD_SHARED OFF CACHE BOOL "Build zlib statically" FORCE)
set(ZLIB_BUILD_STATIC ON CACHE BOOL "Build zlib statically" FORCE)
set(ZLIB_BUILD_TESTING OFF CACHE BOOL "Do not build zlib tests" FORCE)
set(ZLIB_INSTALL OFF CACHE BOOL "Do not install zlib from this build" FORCE)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/zlib"
  "${CMAKE_BINARY_DIR}/third_party/zlib" EXCLUDE_FROM_ALL)
add_library(ZLIB::ZLIB ALIAS zlibstatic)

if(ANDROID)
  find_library(DARK_ANDROID_LOG_LIBRARY NAMES log REQUIRED)
endif()
include("${CMAKE_CURRENT_LIST_DIR}/LibFFmpeg.cmake")
