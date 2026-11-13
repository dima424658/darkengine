find_package(BISON 2.3 REQUIRED)

# Generate each parser and its scanner token header together, outside the
# source tree. Keep the legacy prefixes used by the existing scanners.
BISON_TARGET(IncludeParser "${PROJECT_SOURCE_DIR}/src/inc.y"
  "${CMAKE_CURRENT_BINARY_DIR}/inc.c"
  DEFINES_FILE "${CMAKE_CURRENT_BINARY_DIR}/inctok.h"
  COMPILE_FLAGS "-p inc")
BISON_TARGET(MotionSchemaParser "${PROJECT_SOURCE_DIR}/src/mschema.y"
  "${CMAKE_CURRENT_BINARY_DIR}/mschyacc.c"
  DEFINES_FILE "${CMAKE_CURRENT_BINARY_DIR}/mschtok.h"
  COMPILE_FLAGS "-p msch")
BISON_TARGET(SoundSchemaParser "${PROJECT_SOURCE_DIR}/src/schema.y"
  "${CMAKE_CURRENT_BINARY_DIR}/schyacc.c"
  DEFINES_FILE "${CMAKE_CURRENT_BINARY_DIR}/schtok.h")
BISON_TARGET(SongParser "${PROJECT_SOURCE_DIR}/src/songlang.y"
  "${CMAKE_CURRENT_BINARY_DIR}/songyacc.c"
  DEFINES_FILE "${CMAKE_CURRENT_BINARY_DIR}/songtok.h"
  COMPILE_FLAGS "-p songyy")

set(dark_parser_sources
  ${BISON_IncludeParser_OUTPUTS}
  ${BISON_MotionSchemaParser_OUTPUTS}
  ${BISON_SoundSchemaParser_OUTPUTS}
  ${BISON_SongParser_OUTPUTS}
)
# Both cores share one generation step; scanner compilation must wait for
# the token headers even on a clean parallel build.
add_custom_target(dark_generated_parsers DEPENDS ${dark_parser_sources})

set(dark_scanner_sources
  inclex.c
  mschlex.c
  schlex.c
  songlex.c
)

# Scanner helper names must remain private to each translation unit. Source
# properties are scoped to the directory where the core targets were created.
set(dark_scanner_targets dark_game_core)
if(TARGET dark_dromed_core)
  list(APPEND dark_scanner_targets dark_dromed_core)
endif()
foreach(target IN LISTS dark_scanner_targets)
  target_sources(${target} PRIVATE ${dark_parser_sources} ${dark_scanner_sources})
  target_include_directories(${target} BEFORE PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
  add_dependencies(${target} dark_generated_parsers)
endforeach()

set_source_files_properties(${dark_scanner_sources}
  TARGET_DIRECTORY ${dark_scanner_targets}
  PROPERTIES COMPILE_DEFINITIONS YY_STATIC
)
