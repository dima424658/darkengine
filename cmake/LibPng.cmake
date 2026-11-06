# Android builds a private static libpng and recreates its installed include layout.
if(ANDROID)
  if(NOT DARK_PNG_SOURCE OR
     NOT EXISTS "${DARK_PNG_SOURCE}/CMakeLists.txt")
    message(FATAL_ERROR
      "Android requires DARK_PNG_SOURCE to name an extracted libpng source tree")
  endif()
  set(PNG_SHARED OFF CACHE BOOL "Do not package a separate libpng shared library" FORCE)
  set(PNG_STATIC ON CACHE BOOL "Build libpng for the engine" FORCE)
  set(PNG_TESTS OFF CACHE BOOL "Do not build libpng tests" FORCE)
  set(PNG_TOOLS OFF CACHE BOOL "Do not build libpng tools" FORCE)
  add_subdirectory("${DARK_PNG_SOURCE}"
    "${CMAKE_BINARY_DIR}/third_party/libpng" EXCLUDE_FROM_ALL)

  # namedres owns a public header named png.h and includes libpng as <libpng16/png.h>.
  set(DARK_PNG_COMPAT_INCLUDE_DIR
    "${CMAKE_BINARY_DIR}/third_party/libpng-include")
  file(MAKE_DIRECTORY "${DARK_PNG_COMPAT_INCLUDE_DIR}/libpng16")
  configure_file("${DARK_PNG_SOURCE}/png.h"
    "${DARK_PNG_COMPAT_INCLUDE_DIR}/libpng16/png.h" COPYONLY)
  configure_file("${DARK_PNG_SOURCE}/pngconf.h"
    "${DARK_PNG_COMPAT_INCLUDE_DIR}/libpng16/pngconf.h" COPYONLY)
  configure_file("${CMAKE_BINARY_DIR}/third_party/libpng/pnglibconf.h"
    "${DARK_PNG_COMPAT_INCLUDE_DIR}/libpng16/pnglibconf.h" COPYONLY)
endif()
