set(ZLIB_BUILD_SHARED OFF CACHE BOOL "Build zlib statically" FORCE)
set(ZLIB_BUILD_STATIC ON CACHE BOOL "Build zlib statically" FORCE)
set(ZLIB_BUILD_TESTING OFF CACHE BOOL "Do not build zlib tests" FORCE)
set(ZLIB_INSTALL OFF CACHE BOOL "Do not install zlib from this build" FORCE)

set(ZLIB_BUILD_BLAST ON)
set(ZLIB_BLAST_BUILD_SHARED OFF CACHE BOOL "Build blast statically" FORCE)
set(ZLIB_BLAST_BUILD_STATIC ON CACHE BOOL "Build blast statically" FORCE)
set(ZLIB_BLAST_BUILD_TESTING OFF  CACHE BOOL "Do not build blast tests" FORCE)
set(ZLIB_BLAST_INSTALL OFF CACHE BOOL "Do not install blast from this build" FORCE)

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/zlib"
  "${CMAKE_BINARY_DIR}/third_party/zlib" EXCLUDE_FROM_ALL)

target_include_directories(zlib_blast_blastStatic PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/zlib/contrib/blast"
)