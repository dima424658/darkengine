# SDL3/Vulkan runner and DromEd executable targets.
set(dark_sdl_provider_sources
  unixsrc/framewrk/diskfree.cpp
  libsrc/appcore/sdl_appcore.cpp
  libsrc/dispdev/display.cpp
  libsrc/dispdev/sdl_window.cpp
  libsrc/gameshel/gameshell.cpp
  libsrc/gameshel/sdl_events.cpp
  libsrc/input/gamepad.cpp
  libsrc/input/guids.c
  libsrc/input/input.cpp
  libsrc/actmovie/movie.cpp
  unixsrc/script/scrptmod.cpp
  libsrc/timer/timer.cpp
  libsrc/input/inpinit.c
  libsrc/input/kbasctab.cpp
  libsrc/input/kbcntry.c
)
if(WIN32)
  # Keep SDL3/Vulkan presentation and input on Windows. POSIX providers
  # include unistd.h and statvfs and cannot be compiled by MSVC.
  list(REMOVE_ITEM dark_sdl_provider_sources unixsrc/framewrk/diskfree.cpp)
  list(APPEND dark_sdl_provider_sources
    winsrc/framewrk/diskfree.c
  )
endif()
add_library(dark_sdl_providers STATIC ${dark_sdl_provider_sources})
target_link_libraries(dark_sdl_providers PUBLIC dark_engine_packed)
target_link_libraries(dark_sdl_providers PUBLIC ${DARK_SDL_TARGET})
target_link_libraries(dark_sdl_providers PUBLIC dark_lgd3d)
if(TARGET dark_vulkan_shaders)
  add_dependencies(dark_sdl_providers dark_vulkan_shaders)
endif()
if(TARGET dark_squirrel)
  target_compile_definitions(dark_sdl_providers PRIVATE
    DARK_ENABLE_SQUIRREL=1)
endif()
if(FFMPEG_FOUND)
  target_compile_definitions(dark_sdl_providers PRIVATE DARK_HAVE_FFMPEG=1)
  target_link_libraries(dark_sdl_providers PUBLIC PkgConfig::FFMPEG)
else()
  message(WARNING "FFmpeg development libraries were not found; movie playback will be disabled")
endif()
option(DARK_BUILD_THIEF2 "Compile the native SDL Thief 2 game sources" ON)
if(APPLE OR WIN32)
  set(dark_build_dromed_default ON)
else()
  set(dark_build_dromed_default OFF)
endif()
option(DARK_BUILD_DROMED "Compile the native SDL DromEd editor" ${dark_build_dromed_default})
if(DARK_BUILD_THIEF2)
  # The shared source manifest contains both the game and editor. Build the
  # game target without editor-only sources, then add its game bootstrap.
  set(dark_thief2_sources)
  foreach(source IN LISTS dark_engine_program_sources)
    file(RELATIVE_PATH source_rel "${CMAKE_CURRENT_SOURCE_DIR}"
      "${CMAKE_CURRENT_SOURCE_DIR}/${source}")
    file(TO_CMAKE_PATH "${source_rel}" source_rel)
    string(TOLOWER "${source_rel}" source_rel_lower)
    if(source_rel_lower MATCHES "^src/(editor|csg)/" OR
       source_rel_lower MATCHES "^src/framewrk/(initedit|report)[.]cpp$" OR
       source_rel_lower MATCHES "^src/sound/songedtr[.]cpp$")
      continue()
    endif()
    list(APPEND dark_thief2_sources "${source}")
  endforeach()
  set(dark_game_sources ${dark_thief2_sources})
  list(APPEND dark_game_sources
    "${CMAKE_CURRENT_SOURCE_DIR}/src/editor/isdesced.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/editor/prompts.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/editor/sdesparm.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/editor/status.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/framewrk/gameiid.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/framewrk/initgame.cpp"
  )
  list(REMOVE_DUPLICATES dark_game_sources)

  set(dark_dromed_sources ${dark_engine_program_sources})
  list(FILTER dark_dromed_sources EXCLUDE REGEX "/src/framewrk/initgame[.]cpp$")
  if(APPLE)
    list(APPEND dark_dromed_sources
      "${CMAKE_CURRENT_SOURCE_DIR}/unixsrc/editor/editor_macos.mm"
      "${CMAKE_CURRENT_SOURCE_DIR}/unixsrc/editor/objedit_macos.cpp")
  elseif(WIN32)
    # DromEd uses the SDL window/input providers on Windows as well. Keep
    # its dialogs and resize hooks on the shared SDL implementation instead
    # of linking the retired Win32 editor UI.
    list(APPEND dark_dromed_sources
      "${CMAKE_CURRENT_SOURCE_DIR}/src/editor/uiedit.c"
      "${CMAKE_CURRENT_SOURCE_DIR}/unixsrc/editor/editorplatform.cpp"
      "${CMAKE_CURRENT_SOURCE_DIR}/unixsrc/editor/objedit_macos.cpp"
      "${CMAKE_CURRENT_SOURCE_DIR}/unixsrc/editor/dialogs.cpp"
      "${CMAKE_CURRENT_SOURCE_DIR}/unixsrc/editor/filemenu.cpp"
      "${CMAKE_CURRENT_SOURCE_DIR}/unixsrc/editor/menus.cpp")
  elseif(NOT WIN32)
    list(APPEND dark_dromed_sources
      "${CMAKE_CURRENT_SOURCE_DIR}/src/editor/uiedit.c"
      "${CMAKE_CURRENT_SOURCE_DIR}/unixsrc/editor/objedit_macos.cpp"
      "${CMAKE_CURRENT_SOURCE_DIR}/unixsrc/editor/dialogs.cpp"
      "${CMAKE_CURRENT_SOURCE_DIR}/unixsrc/editor/editorplatform.cpp"
      "${CMAKE_CURRENT_SOURCE_DIR}/unixsrc/editor/filemenu.cpp"
      "${CMAKE_CURRENT_SOURCE_DIR}/unixsrc/editor/menus.cpp")
  endif()
  list(REMOVE_DUPLICATES dark_dromed_sources)

  # These generated scanners deliberately use the same private helper
  # names, so keep their scanner helpers static to each translation unit.
  set_source_files_properties(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/generated/inclex.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/generated/mschlex.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/generated/schlex.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/generated/songlex.c"
    PROPERTIES COMPILE_DEFINITIONS YY_STATIC
  )

  add_library(dark_game_core OBJECT ${dark_game_sources})
  target_link_libraries(dark_game_core PRIVATE dark_engine_packed)
  target_compile_definitions(dark_game_core PRIVATE
    PLAYTEST NEW_NETWORK_ENABLED THIEF THIEF2_GAME STRICT=1
  )
  if(NOT ANDROID)
    target_compile_definitions(dark_game_core PRIVATE
      PROPERTY_BLAME
    )
  endif()
  target_include_directories(dark_game_core BEFORE PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
  )
  if(TARGET dark_vorbisfile)
    target_compile_definitions(dark_game_core PRIVATE
      DARK_HAVE_VORBISFILE=1
    )
    target_link_libraries(dark_game_core PRIVATE dark_vorbisfile)
  endif()

  if(ANDROID)
    # SDLActivity loads libSDL3.so followed by libmain.so and resolves the
    # SDL_main entry point from this shared library.
    add_library(thief2 SHARED
      "${CMAKE_CURRENT_SOURCE_DIR}/unixsrc/entry/main.cpp"
      $<TARGET_OBJECTS:dark_game_core>
    )
  else()
    add_executable(thief2
      "${CMAKE_CURRENT_SOURCE_DIR}/unixsrc/entry/main.cpp"
      $<TARGET_OBJECTS:dark_game_core>
    )
  endif()
  # SDL and C++ standard-library headers require their native ABI packing.
  # Keep the legacy engine's byte packing out of this process entry point.
  if(MSVC)
    set(dark_entry_pack_option "/Zp8")
  else()
    set(dark_entry_pack_option "-fno-pack-struct")
  endif()
  set_source_files_properties(
    "${CMAKE_CURRENT_SOURCE_DIR}/unixsrc/entry/main.cpp"
    PROPERTIES COMPILE_OPTIONS "${dark_entry_pack_option}")
  set(dark_runner_libraries
    ${dark_engine_targets}
    dark_platform
    dark_compression
    dark_sdl_providers
    $<$<TARGET_EXISTS:dark_squirrel>:dark_squirrel>
    $<$<TARGET_EXISTS:dark_vorbisfile>:dark_vorbisfile>
    $<$<TARGET_EXISTS:dark_vorbis>:dark_vorbis>
    $<$<TARGET_EXISTS:ogg>:ogg>
  )
  # Apple ld rejects the empty template-generator archive's symbol member.
  list(REMOVE_ITEM dark_runner_libraries dark_templgen)

  if(DARK_BUILD_DROMED)
    add_library(dark_dromed_core OBJECT ${dark_dromed_sources})
    target_link_libraries(dark_dromed_core PRIVATE dark_engine_packed
      ${DARK_SDL_TARGET})
    target_compile_definitions(dark_dromed_core PRIVATE
      EDITOR REPORT PLAYTEST NEW_NETWORK_ENABLED THIEF STRICT=1 PROPERTY_BLAME
    )
    if(MSVC)
      target_compile_options(dark_dromed_core PRIVATE
        "/FI${CMAKE_CURRENT_SOURCE_DIR}/unixsrc/platform/editor_compat.h")
    else()
      target_compile_options(dark_dromed_core PRIVATE
        $<$<COMPILE_LANGUAGE:C>:-std=gnu89>
        $<$<COMPILE_LANGUAGE:C>:-fms-extensions>
        $<$<COMPILE_LANGUAGE:C>:-Wno-incompatible-pointer-types>
        "$<$<COMPILE_LANGUAGE:C>:-include${CMAKE_CURRENT_SOURCE_DIR}/unixsrc/platform/editor_compat.h>"
      )
    endif()
    if(APPLE)
      set_source_files_properties(
        "${CMAKE_CURRENT_SOURCE_DIR}/unixsrc/editor/editor_macos.mm"
        PROPERTIES COMPILE_OPTIONS "-fpack-struct=0")
      target_link_libraries(dark_dromed_core PRIVATE "-framework AppKit")
    endif()
    target_include_directories(dark_dromed_core BEFORE PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/src"
    )
    if(TARGET dark_vorbisfile)
      target_compile_definitions(dark_dromed_core PRIVATE
        DARK_HAVE_VORBISFILE=1
      )
      target_link_libraries(dark_dromed_core PRIVATE dark_vorbisfile)
    endif()

    add_executable(dromed
      "${CMAKE_CURRENT_SOURCE_DIR}/unixsrc/entry/main.cpp"
      $<TARGET_OBJECTS:dark_dromed_core>
    )
    set_source_files_properties(
      "${CMAKE_CURRENT_SOURCE_DIR}/unixsrc/entry/main.cpp"
      PROPERTIES COMPILE_OPTIONS "${dark_entry_pack_option}")
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
  if(NOT ANDROID)
    set_target_properties(thief2 PROPERTIES OUTPUT_NAME thief-runner)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
      install(FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/dark-engine-restored.desktop"
        DESTINATION share/applications
      )
      foreach(icon_size 16 24 32 48 64 128 256 512)
        install(FILES
          "${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/hicolor/${icon_size}x${icon_size}/apps/dark-engine-restored.png"
          DESTINATION "share/icons/hicolor/${icon_size}x${icon_size}/apps"
        )
      endforeach()
    endif()
  else()
    set_target_properties(thief2 PROPERTIES OUTPUT_NAME main)
    add_dependencies(thief2 OpenAL)
  endif()
endif()
