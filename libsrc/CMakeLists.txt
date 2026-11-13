# Explicit directories keep source discovery and header search order predictable.
set(dark_library_projects
  actmovie
  appcore
  compapis
  comtools
  config
  cpptools
  darkloop
  dev2d
  dispdev
  dstruct
  dump
  fix
  font
  g2
  gadget
  gameshel
  gfile
  input
  inputbnd
  lg
  lgalloc
  lgd3d
  matrix
  md
  mm
  mp
  mprintf
  namedres
  net
  object
  prof
  r3d
  random
  recorder
  ref
  res
  rnd
  script
  sdesc
  skel
  sndsrc
  sndutil
  sound
  star
  tagfile
  templgen
  timer
  ui
  vec2
)

target_include_directories(dark_engine_config INTERFACE
  "${CMAKE_CURRENT_SOURCE_DIR}/actmovie"
  "${CMAKE_CURRENT_SOURCE_DIR}/appcore"
  "${CMAKE_CURRENT_SOURCE_DIR}/compapis"
  "${CMAKE_CURRENT_SOURCE_DIR}/comtools"
  "${CMAKE_CURRENT_SOURCE_DIR}/config"
  "${CMAKE_CURRENT_SOURCE_DIR}/cpptools"
  "${CMAKE_CURRENT_SOURCE_DIR}/darkloop"
  "${CMAKE_CURRENT_SOURCE_DIR}/dev2d"
  "${CMAKE_CURRENT_SOURCE_DIR}/dispdev"
  "${CMAKE_CURRENT_SOURCE_DIR}/dstruct"
  "${CMAKE_CURRENT_SOURCE_DIR}/dump"
  "${CMAKE_CURRENT_SOURCE_DIR}/fix"
  "${CMAKE_CURRENT_SOURCE_DIR}/font"
  "${CMAKE_CURRENT_SOURCE_DIR}/g2"
  "${CMAKE_CURRENT_SOURCE_DIR}/gadget"
  "${CMAKE_CURRENT_SOURCE_DIR}/gameshel"
  "${CMAKE_CURRENT_SOURCE_DIR}/gfile"
  "${CMAKE_CURRENT_SOURCE_DIR}/input"
  "${CMAKE_CURRENT_SOURCE_DIR}/inputbnd"
  "${CMAKE_CURRENT_SOURCE_DIR}/lg"
  "${CMAKE_CURRENT_SOURCE_DIR}/lgalloc"
  "${CMAKE_CURRENT_SOURCE_DIR}/lgd3d"
  "${CMAKE_CURRENT_SOURCE_DIR}/matrix"
  "${CMAKE_CURRENT_SOURCE_DIR}/md"
  "${CMAKE_CURRENT_SOURCE_DIR}/mm"
  "${CMAKE_CURRENT_SOURCE_DIR}/mp"
  "${CMAKE_CURRENT_SOURCE_DIR}/mprintf"
  "${CMAKE_CURRENT_SOURCE_DIR}/namedres"
  "${CMAKE_CURRENT_SOURCE_DIR}/net"
  "${CMAKE_CURRENT_SOURCE_DIR}/object"
  "${CMAKE_CURRENT_SOURCE_DIR}/object/property"
  "${CMAKE_CURRENT_SOURCE_DIR}/prof"
  "${CMAKE_CURRENT_SOURCE_DIR}/r3d"
  "${CMAKE_CURRENT_SOURCE_DIR}/random"
  "${CMAKE_CURRENT_SOURCE_DIR}/recorder"
  "${CMAKE_CURRENT_SOURCE_DIR}/ref"
  "${CMAKE_CURRENT_SOURCE_DIR}/res"
  "${CMAKE_CURRENT_SOURCE_DIR}/rnd"
  "${CMAKE_CURRENT_SOURCE_DIR}/script"
  "${CMAKE_CURRENT_SOURCE_DIR}/sdesc"
  "${CMAKE_CURRENT_SOURCE_DIR}/skel"
  "${CMAKE_CURRENT_SOURCE_DIR}/sndsrc"
  "${CMAKE_CURRENT_SOURCE_DIR}/sndutil"
  "${CMAKE_CURRENT_SOURCE_DIR}/sound"
  "${CMAKE_CURRENT_SOURCE_DIR}/star"
  "${CMAKE_CURRENT_SOURCE_DIR}/tagfile"
  "${CMAKE_CURRENT_SOURCE_DIR}/templgen"
  "${CMAKE_CURRENT_SOURCE_DIR}/timer"
  "${CMAKE_CURRENT_SOURCE_DIR}/ui"
  "${CMAKE_CURRENT_SOURCE_DIR}/vec2"
  # Keep legacy input GUID headers after compapis/inpguid.h.
  "${CMAKE_CURRENT_SOURCE_DIR}/input"
  $<$<PLATFORM_ID:Windows>:"${PROJECT_SOURCE_DIR}/winsrc">
)

# Apply shared settings before a library adds its own dependencies, preserving
# include-directory and transitive link order.
function(dark_configure_library target)
  target_include_directories(${target} BEFORE PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
  target_link_libraries(${target} PUBLIC dark_engine_config dark_platform)
  set_target_properties(${target} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")
endfunction()

foreach(library IN LISTS dark_library_projects)
  add_subdirectory(${library})
  # Some directories, such as object/, supply headers without a library.
  if(TARGET "dark_${library}")
    list(APPEND dark_engine_targets "dark_${library}")
  endif()
endforeach()
set(dark_engine_targets ${dark_engine_targets} PARENT_SCOPE)
