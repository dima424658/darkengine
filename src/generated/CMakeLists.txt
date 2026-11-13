set(dark_generated_sources
  inclex.c
  mschlex.c
  schlex.c
  songlex.c
)

target_sources(dark_game_core PRIVATE ${dark_generated_sources})
if(TARGET dark_dromed_core)
  target_sources(dark_dromed_core PRIVATE ${dark_generated_sources})
endif()

# Scanner helper names must remain private to each translation unit. Source
# properties are scoped to the directory where the core targets were created.
set(dark_scanner_targets dark_game_core)
if(TARGET dark_dromed_core)
  list(APPEND dark_scanner_targets dark_dromed_core)
endif()
set_source_files_properties(${dark_generated_sources}
  TARGET_DIRECTORY ${dark_scanner_targets}
  PROPERTIES COMPILE_DEFINITIONS YY_STATIC
)
