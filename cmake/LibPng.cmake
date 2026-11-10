# Build libpng from the pinned source submodule on every platform.
set(ZLIB_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/zlib"
  CACHE PATH "Bundled zlib include directory" FORCE)
set(ZLIB_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/zlib"
  CACHE PATH "Bundled zlib root" FORCE)
set(DARK_PNG_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/libpng" CACHE PATH
  "Pinned libpng source submodule" FORCE)
if(NOT EXISTS "${DARK_PNG_SOURCE}/CMakeLists.txt")
  message(FATAL_ERROR
    "libpng source is missing. Initialize the submodules with "
    "git submodule update --init --recursive.")
endif()
set(PNG_SHARED OFF CACHE BOOL "Do not package a separate libpng shared library" FORCE)
set(PNG_STATIC ON CACHE BOOL "Build libpng for the engine" FORCE)
set(PNG_TESTS OFF CACHE BOOL "Do not build libpng tests" FORCE)
set(PNG_TOOLS OFF CACHE BOOL "Do not build libpng tools" FORCE)
set(PNG_FRAMEWORK OFF CACHE BOOL "Build libpng as a static library" FORCE)
set(SKIP_INSTALL_ALL ON CACHE BOOL "Do not install bundled libpng" FORCE)
add_subdirectory("${DARK_PNG_SOURCE}"
  "${CMAKE_BINARY_DIR}/third_party/libpng" EXCLUDE_FROM_ALL)
