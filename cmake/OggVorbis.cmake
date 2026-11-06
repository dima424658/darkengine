# Optional source builds for fan-mission Ogg Vorbis audio support.
set(DARK_OGG_SOURCE "" CACHE PATH "Path to an external libogg 1.3.5 source tree")
set(DARK_VORBIS_SOURCE "" CACHE PATH "Path to an external libvorbis 1.3.7 source tree")
if(DARK_OGG_SOURCE OR DARK_VORBIS_SOURCE)
  if(NOT DARK_OGG_SOURCE OR NOT DARK_VORBIS_SOURCE)
    message(FATAL_ERROR
      "DARK_OGG_SOURCE and DARK_VORBIS_SOURCE must be provided together")
  endif()
  if(NOT EXISTS "${DARK_OGG_SOURCE}/include/ogg/ogg.h")
    message(FATAL_ERROR
      "DARK_OGG_SOURCE does not contain include/ogg/ogg.h: ${DARK_OGG_SOURCE}")
  endif()
  if(NOT EXISTS "${DARK_VORBIS_SOURCE}/include/vorbis/vorbisfile.h")
    message(FATAL_ERROR
      "DARK_VORBIS_SOURCE does not contain include/vorbis/vorbisfile.h: ${DARK_VORBIS_SOURCE}")
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
  target_include_directories(dark_vorbis
    PUBLIC "${DARK_VORBIS_SOURCE}/include"
    PRIVATE "${DARK_VORBIS_SOURCE}/lib")
  target_link_libraries(dark_vorbis PUBLIC ogg PRIVATE m)

  add_library(dark_vorbisfile STATIC "${DARK_VORBIS_SOURCE}/lib/vorbisfile.c")
  target_include_directories(dark_vorbisfile PUBLIC "${DARK_VORBIS_SOURCE}/include")
  target_link_libraries(dark_vorbisfile PUBLIC dark_vorbis)
endif()
