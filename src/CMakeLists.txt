if(APPLE OR WIN32)
  set(dark_build_dromed_default ON)
else()
  set(dark_build_dromed_default OFF)
endif()

option(DARK_BUILD_DROMED "Compile the native SDL DromEd editor" ${dark_build_dromed_default})

add_library(dark_game_core OBJECT)
target_link_libraries(dark_game_core PRIVATE dark_engine_config)
target_compile_definitions(dark_game_core PRIVATE
  PLAYTEST NEW_NETWORK_ENABLED THIEF THIEF2_GAME STRICT=1
)

if(NOT ANDROID)
  target_compile_definitions(dark_game_core PRIVATE
    PROPERTY_BLAME
  )
endif()

target_include_directories(dark_game_core BEFORE PRIVATE
  "${PROJECT_SOURCE_DIR}/src"
)

target_link_libraries(dark_game_core PRIVATE dark_vorbisfile)

if(DARK_BUILD_DROMED)
  add_library(dark_dromed_core OBJECT)
  target_link_libraries(dark_dromed_core PRIVATE dark_engine_config
    SDL3::SDL3)
  target_compile_definitions(dark_dromed_core PRIVATE
    EDITOR REPORT PLAYTEST NEW_NETWORK_ENABLED THIEF STRICT=1 PROPERTY_BLAME
  )

  if(MSVC)
    target_compile_options(dark_dromed_core PRIVATE
      "/FI${PROJECT_SOURCE_DIR}/unixsrc/platform/editor_compat.h")
  else()
    target_compile_options(dark_dromed_core PRIVATE
      $<$<COMPILE_LANGUAGE:C>:-std=gnu89>
      $<$<COMPILE_LANGUAGE:C>:-fms-extensions>
      $<$<COMPILE_LANGUAGE:C>:-Wno-incompatible-pointer-types>
      "$<$<COMPILE_LANGUAGE:C>:-include${PROJECT_SOURCE_DIR}/unixsrc/platform/editor_compat.h>"
    )
  endif()

  if(APPLE)
    target_link_libraries(dark_dromed_core PRIVATE "-framework AppKit")
  endif()

  target_include_directories(dark_dromed_core BEFORE PRIVATE
    "${PROJECT_SOURCE_DIR}/src"
  )

  target_link_libraries(dark_dromed_core PRIVATE dark_vorbisfile)
endif()

set(dark_root_sources
  camguids.c
  darktabl.c
  frezreac.cpp
  goof.c
  inctab.cpp
  killme.cpp
  testloop.c
  testmode.c
  version.c
)

target_sources(dark_game_core PRIVATE ${dark_root_sources})

if(TARGET dark_dromed_core)
  target_sources(dark_dromed_core PRIVATE ${dark_root_sources})
endif()

add_subdirectory(actreact)
add_subdirectory(ai)
add_subdirectory(csg)
add_subdirectory(dark)
add_subdirectory(deepc)
add_subdirectory(editor)
add_subdirectory(engfeat)
add_subdirectory(framewrk)
add_subdirectory(generated)
add_subdirectory(motion)
add_subdirectory(object)
add_subdirectory(physics)
add_subdirectory(portal)
add_subdirectory(render)
add_subdirectory(script)
add_subdirectory(shock)
add_subdirectory(sim)
add_subdirectory(sound)
add_subdirectory(ui)

if(ANDROID)
  # SDLActivity loads libSDL3.so followed by libmain.so and resolves the
  # SDL_main entry point from this shared library.
  add_library(thief2 SHARED
    $<TARGET_OBJECTS:dark_game_core>
  )
else()
  add_executable(thief2
    $<TARGET_OBJECTS:dark_game_core>
  )
endif()

set(dark_runner_libraries
  ${dark_engine_targets}
  dark_platform
  $<$<TARGET_EXISTS:dark_squirrel>:dark_squirrel>
  dark_vorbisfile
  dark_vorbis
  ogg
)
# Apple ld rejects the empty template-generator archive's symbol member.
list(REMOVE_ITEM dark_runner_libraries dark_templgen)

if(DARK_BUILD_DROMED)
  add_executable(dromed
    $<TARGET_OBJECTS:dark_dromed_core>
  )

  if(APPLE)
    target_link_libraries(dromed PRIVATE dark_engine_config
      ${dark_runner_libraries} ${dark_runner_libraries} m "-framework AppKit")
    target_link_options(dromed PRIVATE -Wl,-no_fixup_chains)
  elseif(MSVC)
    target_link_libraries(dromed PRIVATE dark_engine_config
      ${dark_runner_libraries} ${dark_runner_libraries})
  else()
    target_link_libraries(dromed PRIVATE dark_engine_config
      -Wl,--start-group ${dark_runner_libraries} -Wl,--end-group m)
  endif()

  if(NOT WIN32)
    target_link_libraries(dromed PRIVATE pthread)
  endif()

  if(TARGET SDL3::SDL3main)
    target_link_libraries(dromed PRIVATE SDL3::SDL3main)
  endif()

  set_target_properties(dromed PROPERTIES OUTPUT_NAME DromEd)
endif()

# The legacy template generator object contains no symbols. Apple ld
# rejects the empty archive's GNU-style symbol-table member.
list(REMOVE_ITEM dark_runner_libraries dark_templgen)
target_link_libraries(thief2 PRIVATE dark_engine_config)

if(APPLE)
  # Apple ld does not support GNU --start-group/--end-group. Repeating
  # the static archives resolves the same legacy circular references.
  target_link_libraries(thief2 PRIVATE
    ${dark_runner_libraries} ${dark_runner_libraries} m)
  # Chained fixups require naturally aligned pointers. The legacy engine
  # deliberately packs global descriptors, so use classic dyld fixups.
  target_link_options(thief2 PRIVATE -Wl,-no_fixup_chains)
elseif(MSVC)
  target_link_libraries(thief2 PRIVATE
    ${dark_runner_libraries} ${dark_runner_libraries})
else()
  target_link_libraries(thief2 PRIVATE
    -Wl,--start-group ${dark_runner_libraries} -Wl,--end-group m)
endif()

if(NOT ANDROID AND NOT WIN32)
  target_link_libraries(thief2 PRIVATE pthread)
endif()

if(TARGET SDL3::SDL3main)
  target_link_libraries(thief2 PRIVATE SDL3::SDL3main)
endif()

if(WIN32)
  # SDL3's imported shared target links through its import library, but
  # the DLL must also be next to the executable at launch time. CMake
  # does not copy imported runtime artifacts as part of an ordinary
  # target build.
  foreach(sdl_runtime_target SDL3::SDL3 SDL3::SDL3-shared)
    if(TARGET ${sdl_runtime_target})
      get_target_property(sdl_runtime_type ${sdl_runtime_target} TYPE)
      if(sdl_runtime_type STREQUAL "SHARED_LIBRARY")
        install(FILES "$<TARGET_FILE:${sdl_runtime_target}>"
          DESTINATION bin)
        foreach(runtime_target thief2 dromed)
          if(TARGET ${runtime_target})
            add_custom_command(TARGET ${runtime_target} POST_BUILD
              COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:${sdl_runtime_target}>"
                "$<TARGET_FILE_DIR:${runtime_target}>"
              VERBATIM)
          endif()
        endforeach()
        break()
      endif()
    endif()
  endforeach()
endif()

if(ANDROID)
  set_target_properties(thief2 PROPERTIES OUTPUT_NAME main)
  add_dependencies(thief2 OpenAL)
else()
  set_target_properties(thief2 PROPERTIES OUTPUT_NAME thief-runner)
endif()

if(NOT ANDROID)
  install(TARGETS thief2 RUNTIME DESTINATION bin)
  if(TARGET dromed)
    install(TARGETS dromed RUNTIME DESTINATION bin)
  endif()
endif()
