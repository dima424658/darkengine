# Build static libogg and libvorbis from pinned submodules on every platform.
set(DARK_OGG_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/ogg" CACHE PATH
  "Pinned libogg source submodule" FORCE)
set(DARK_VORBIS_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/vorbis" CACHE PATH
  "Pinned libvorbis source submodule" FORCE)
if(NOT EXISTS "${DARK_OGG_SOURCE}/include/ogg/ogg.h")
  message(FATAL_ERROR
    "libogg source is missing. Initialize the submodules with "
    "git submodule update --init --recursive.")
endif()
if(NOT EXISTS "${DARK_VORBIS_SOURCE}/include/vorbis/vorbisfile.h")
  message(FATAL_ERROR
    "libvorbis source is missing. Initialize the submodules with "
    "git submodule update --init --recursive.")
endif()

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static dependencies" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "Disable dependency tests" FORCE)
set(INSTALL_DOCS OFF CACHE BOOL "Disable dependency documentation" FORCE)
set(INSTALL_PKG_CONFIG_MODULE OFF CACHE BOOL
    "Disable dependency pkg-config installation" FORCE)
set(INSTALL_CMAKE_PACKAGE_MODULE OFF CACHE BOOL
    "Disable dependency CMake package installation" FORCE)
add_subdirectory("${DARK_OGG_SOURCE}"
  "${CMAKE_CURRENT_BINARY_DIR}/dark_ogg" EXCLUDE_FROM_ALL)

set(dark_vorbis_sources
  mdct.c smallft.c block.c envelope.c window.c lsp.c lpc.c analysis.c
  synthesis.c psy.c info.c floor1.c floor0.c res0.c mapping0.c registry.c
  codebook.c sharedbook.c lookup.c bitrate.c)
list(TRANSFORM dark_vorbis_sources PREPEND "${DARK_VORBIS_SOURCE}/lib/")
add_library(dark_vorbis STATIC ${dark_vorbis_sources})
if(UNIX)
  target_compile_definitions(dark_vorbis PRIVATE HAVE_ALLOCA_H)
endif()
target_include_directories(dark_vorbis
  PUBLIC "${DARK_VORBIS_SOURCE}/include"
  PRIVATE "${DARK_VORBIS_SOURCE}/lib")
target_link_libraries(dark_vorbis PUBLIC ogg)
if(NOT MSVC)
  target_link_libraries(dark_vorbis PRIVATE m)
endif()

add_library(dark_vorbisfile STATIC "${DARK_VORBIS_SOURCE}/lib/vorbisfile.c")
if(UNIX)
  target_compile_definitions(dark_vorbisfile PRIVATE HAVE_ALLOCA_H)
endif()
target_include_directories(dark_vorbisfile PUBLIC "${DARK_VORBIS_SOURCE}/include")
target_link_libraries(dark_vorbisfile PUBLIC dark_vorbis)
