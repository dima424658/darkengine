# Generate each parser and its scanner token header together, outside the
# source tree. Keep the legacy prefixes used by the existing scanners.
dark_generate_parser(include_parser "${PROJECT_SOURCE_DIR}/src/inc.y"
  "inc.c" "inctok.h"
  COMPILE_FLAGS "-p inc")
dark_generate_parser(motion_schema_parser "${PROJECT_SOURCE_DIR}/src/mschema.y"
  "mschyacc.c" "mschtok.h"
  COMPILE_FLAGS "-p msch")
dark_generate_parser(sound_schema_parser "${PROJECT_SOURCE_DIR}/src/schema.y"
  "schyacc.c" "schtok.h")
dark_generate_parser(song_parser "${PROJECT_SOURCE_DIR}/src/songlang.y"
  "songyacc.c" "songtok.h"
  COMPILE_FLAGS "-p songyy")

set(dark_parser_sources
  ${include_parser_sources}
  ${motion_schema_parser_sources}
  ${sound_schema_parser_sources}
  ${song_parser_sources}
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
