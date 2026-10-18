cmake_minimum_required(VERSION 3.20)
project(DarkEnginePortability LANGUAGES C CXX)

set(DARK_SDL_PLATFORM OFF)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" OR ANDROID)
  set(DARK_SDL_PLATFORM ON)
endif()

if(DARK_SDL_PLATFORM)
  option(DARK_OPENGL_ES "Use the OpenGL ES 3 renderer instead of desktop OpenGL" OFF)
  if(ANDROID)
    set(DARK_OPENGL_ES ON CACHE BOOL
      "Use the OpenGL ES 3 renderer instead of desktop OpenGL" FORCE)
    if(NOT DARK_SDL2_SOURCE OR
       NOT EXISTS "${DARK_SDL2_SOURCE}/CMakeLists.txt")
      message(FATAL_ERROR
        "Android requires DARK_SDL2_SOURCE to name an extracted SDL2 source tree")
    endif()
    set(SDL_SHARED ON CACHE BOOL "Build the SDL shared library" FORCE)
    set(SDL_STATIC OFF CACHE BOOL "Do not build a second static SDL library" FORCE)
    set(SDL_TEST OFF CACHE BOOL "Do not build SDL tests" FORCE)
    set(SDL_TESTS OFF CACHE BOOL "Do not build SDL tests" FORCE)
    add_subdirectory("${DARK_SDL2_SOURCE}"
      "${CMAKE_BINARY_DIR}/third_party/SDL2" EXCLUDE_FROM_ALL)

    if(NOT DARK_OPENAL_SOFT_SOURCE OR
       NOT EXISTS "${DARK_OPENAL_SOFT_SOURCE}/CMakeLists.txt")
      message(FATAL_ERROR
        "Android requires DARK_OPENAL_SOFT_SOURCE to name an extracted OpenAL Soft source tree")
    endif()
    set(ALSOFT_UTILS OFF CACHE BOOL "Do not build OpenAL Soft utilities" FORCE)
    set(ALSOFT_EXAMPLES OFF CACHE BOOL "Do not build OpenAL Soft examples" FORCE)
    set(ALSOFT_TESTS OFF CACHE BOOL "Do not build OpenAL Soft tests" FORCE)
    set(ALSOFT_INSTALL OFF CACHE BOOL "Do not create OpenAL Soft install rules" FORCE)
    set(ALSOFT_DLOPEN OFF CACHE BOOL "Link the Android OpenSL backend directly" FORCE)
    set(ALSOFT_BACKEND_OBOE OFF CACHE BOOL "Do not require the separate Oboe dependency" FORCE)
    set(ALSOFT_BACKEND_OPENSL ON CACHE BOOL "Use Android OpenSL for playback" FORCE)
    set(ALSOFT_REQUIRE_OPENSL ON CACHE BOOL "Require Android OpenSL support" FORCE)
    set(ALSOFT_BACKEND_WAVE OFF CACHE BOOL "Do not build the file writer backend" FORCE)
    add_subdirectory("${DARK_OPENAL_SOFT_SOURCE}"
      "${CMAKE_BINARY_DIR}/third_party/openal-soft" EXCLUDE_FROM_ALL)

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

    # namedres deliberately includes <libpng16/png.h> because it also owns a
    # public header named png.h.  Recreate the installed libpng header layout
    # inside the build tree so the bundled source can be consumed directly.
    set(DARK_PNG_COMPAT_INCLUDE_DIR
      "${CMAKE_BINARY_DIR}/third_party/libpng-include")
    file(MAKE_DIRECTORY "${DARK_PNG_COMPAT_INCLUDE_DIR}/libpng16")
    configure_file("${DARK_PNG_SOURCE}/png.h"
      "${DARK_PNG_COMPAT_INCLUDE_DIR}/libpng16/png.h" COPYONLY)
    configure_file("${DARK_PNG_SOURCE}/pngconf.h"
      "${DARK_PNG_COMPAT_INCLUDE_DIR}/libpng16/pngconf.h" COPYONLY)
    configure_file("${CMAKE_BINARY_DIR}/third_party/libpng/pnglibconf.h"
      "${DARK_PNG_COMPAT_INCLUDE_DIR}/libpng16/pnglibconf.h" COPYONLY)

    find_library(DARK_GLES3_LIBRARY NAMES GLESv3 REQUIRED)
    find_library(DARK_ANDROID_LOG_LIBRARY NAMES log REQUIRED)
  elseif(DARK_OPENGL_ES)
    find_path(DARK_GLES3_INCLUDE_DIR GLES3/gl3.h REQUIRED)
    find_library(DARK_GLES3_LIBRARY NAMES GLESv2 GLESv3 REQUIRED)
  else()
    find_package(OpenGL REQUIRED)
  endif()
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_package(PNG REQUIRED)
    find_package(SDL2 REQUIRED)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
      pkg_check_modules(FFMPEG QUIET IMPORTED_TARGET
        libavformat libavcodec libavutil libswscale libswresample)
    endif()
  endif()
endif()

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)

if(WIN32)
  set(DARK_PLATFORM_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/winsrc/platform")
  set(DARK_PLATFORM_SOURCE "${DARK_PLATFORM_ROOT}/platform_services.cpp")
else()
  set(DARK_PLATFORM_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/unixsrc/platform")
  set(DARK_PLATFORM_SOURCE "${DARK_PLATFORM_ROOT}/platform_services.cpp")
endif()

add_library(dark_platform STATIC
  ${DARK_PLATFORM_SOURCE}
  libsrc/cpptools/dynfunc.cpp
)
target_include_directories(dark_platform PUBLIC
  h
  libsrc/cpptools
  ${DARK_PLATFORM_ROOT}
)
if(NOT WIN32)
  target_link_libraries(dark_platform PUBLIC ${CMAKE_DL_LIBS})
  if(DARK_SDL_PLATFORM)
    target_link_libraries(dark_platform PUBLIC SDL2::SDL2)
  endif()
  if(ANDROID)
    target_link_libraries(dark_platform PUBLIC "${DARK_ANDROID_LOG_LIBRARY}")
  endif()
endif()

add_library(dark_compression STATIC 3rdparty/blast/blast.c)
target_include_directories(dark_compression PUBLIC 3rdparty/blast)

if(DARK_SDL_PLATFORM)
  set(dark_build_engine_default ON)
else()
  set(dark_build_engine_default OFF)
endif()
option(DARK_BUILD_ENGINE "Compile the platform-neutral engine libraries" ${dark_build_engine_default})

if(DARK_BUILD_ENGINE)
  # Keep the Visual Studio manifests as the single source of truth.  This
  # deliberately small reader understands the simple ClCompile entries used
  # by the legacy projects and ignores files disabled in every configuration.
  function(dark_resolve_source out_var source base_dir)
    string(REPLACE "\\" "/" source "${source}")
    string(REPLACE "/" ";" source_parts "${source}")
    cmake_path(NORMAL_PATH base_dir OUTPUT_VARIABLE resolved)
    foreach(part IN LISTS source_parts)
      if(part STREQUAL "" OR part STREQUAL ".")
        continue()
      elseif(part STREQUAL "..")
        get_filename_component(resolved "${resolved}" DIRECTORY)
        continue()
      endif()

      if(EXISTS "${resolved}/${part}")
        set(resolved "${resolved}/${part}")
        continue()
      endif()

      string(TOLOWER "${part}" part_lower)
      file(GLOB siblings "${resolved}/*")
      foreach(candidate IN LISTS siblings)
        get_filename_component(candidate_name "${candidate}" NAME)
        string(TOLOWER "${candidate_name}" candidate_name_lower)
        if(candidate_name_lower STREQUAL part_lower)
          set(resolved "${candidate}")
          break()
        endif()
      endforeach()
    endforeach()
    cmake_path(NORMAL_PATH resolved)
    # Keep manifest paths in CMake's portable slash form so the source filters
    # behave identically on Windows and Linux hosts.
    file(TO_CMAKE_PATH "${resolved}" resolved)
    set(${out_var} "${resolved}" PARENT_SCOPE)
  endfunction()

  function(dark_sources_from_vcxproj out_var project_file)
    get_filename_component(project_dir "${project_file}" DIRECTORY)
    file(STRINGS "${project_file}" project_lines)
    set(result)
    set(current_source)
    set(current_excluded FALSE)
    foreach(line IN LISTS project_lines)
      if(line MATCHES "<ClCompile Include=\"([^\"]+)\"[ ]*/>")
        set(source "${CMAKE_MATCH_1}")
        string(REPLACE "\\" "/" source "${source}")
        if(NOT source MATCHES "(^|/)winsrc/")
          dark_resolve_source(source "${source}" "${project_dir}")
          list(APPEND result "${source}")
        endif()
      elseif(line MATCHES "<ClCompile Include=\"([^\"]+)\"[ ]*>")
        set(current_source "${CMAKE_MATCH_1}")
        set(current_excluded FALSE)
      elseif(current_source AND line MATCHES "<ExcludedFromBuild[^>]*>true</ExcludedFromBuild>")
        set(current_excluded TRUE)
      elseif(current_source AND line MATCHES "</ClCompile>")
        string(REPLACE "\\" "/" current_source "${current_source}")
        if(NOT current_excluded AND NOT current_source MATCHES "(^|/)winsrc/")
          dark_resolve_source(current_source "${current_source}" "${project_dir}")
          list(APPEND result "${current_source}")
        endif()
        unset(current_source)
      endif()
    endforeach()
    set(${out_var} "${result}" PARENT_SCOPE)
  endfunction()

  file(GLOB dark_library_dirs LIST_DIRECTORIES TRUE "${CMAKE_CURRENT_SOURCE_DIR}/libsrc/*")
  list(FILTER dark_library_dirs INCLUDE REGEX "[^.]$")
  set(dark_include_dirs
    "${CMAKE_CURRENT_SOURCE_DIR}/h"
    "${CMAKE_CURRENT_SOURCE_DIR}/inc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/actreact"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ai"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/csg"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/editor"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/engfeat"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/framewrk"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/motion"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/object"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/physics"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/portal"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/render"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/script"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/sim"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/sound"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/shock"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/dark"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/deepc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/deepc/motion"
    "${CMAKE_CURRENT_SOURCE_DIR}/libsrc/object/property"
    "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/blast"
    "${DARK_PLATFORM_ROOT}"
    ${dark_library_dirs}
    # Transitional home of legacy keyboard/mouse public headers. Keep this
    # last so its Windows input GUID header cannot shadow compapis/inpguid.h.
    "${CMAKE_CURRENT_SOURCE_DIR}/winsrc/input"
  )

  add_library(dark_engine_config INTERFACE)
  target_include_directories(dark_engine_config INTERFACE ${dark_include_dirs})
  target_compile_definitions(dark_engine_config INTERFACE
    RGB_LIGHTING BIG_LIGHTMAPS SOFTWARE_RGB SCHEMA_LOADING
    $<$<PLATFORM_ID:Linux>:_DEFAULT_SOURCE>
  )
  # A debuggable APK keeps native symbols without enabling the legacy DEBUG
  # allocator. Do not define SHIP there: some old loop diagnostics are part of
  # the engine ABI even when their debug instrumentation is disabled.
  if(NOT ANDROID)
    target_compile_definitions(dark_engine_config INTERFACE
      $<$<CONFIG:Debug>:DEBUG;DBG_ON;WARN_ON;SPEW_ON>
    )
  endif()
  if(MSVC)
    target_compile_options(dark_engine_config INTERFACE /FItypes.h)
  else()
    target_compile_options(dark_engine_config INTERFACE
      "SHELL:-include types.h -include platform_io.h"
      # Keep diagnostics useful around deliberate legacy idioms: reflected
      # inherited records, integer NULL values, the old strstream wrapper,
      # and void I/O adapters whose callers have no error-return channel.
      -Wno-unused-result
      -Wno-pointer-arith
      -Wno-deprecated
      $<$<COMPILE_LANGUAGE:CXX>:-Wno-conversion-null>
      $<$<COMPILE_LANGUAGE:CXX>:-Wno-invalid-offsetof>
    )
  endif()

  # Legacy engine records use byte packing, but native platform and library
  # headers must retain their host ABI. Keep packing out of the shared config
  # so providers can safely include SDL2, OpenGL, and the C++ standard library.
  add_library(dark_engine_packed INTERFACE)
  target_link_libraries(dark_engine_packed INTERFACE dark_engine_config)
  if(MSVC)
    target_compile_options(dark_engine_packed INTERFACE /Zp1)
  else()
    target_compile_options(dark_engine_packed INTERFACE -fpack-struct=1)
  endif()

  set(dark_library_projects
    appcore arq compapis comtools config cpptools darkloop dev2d dispdev
    dstruct dump fix font g2 gadget gfile inputbnd lg lgalloc lgd3d matrix md
    mm mp mprintf namedres net prof r3d random recorder ref res rnd script
    sdesc skel sndsrc sndutil sound star tagfile templgen timer ui vec2
  )
  foreach(library IN LISTS dark_library_projects)
    set(project_file "${CMAKE_CURRENT_SOURCE_DIR}/libsrc/${library}/${library}.vcxproj")
    dark_sources_from_vcxproj(library_sources "${project_file}")
    if(library STREQUAL "appcore" AND NOT WIN32)
      list(FILTER library_sources EXCLUDE REGEX "/dappcore\\.cpp$")
    endif()
    if(library STREQUAL "dev2d" AND NOT WIN32)
      # Hardware-specific DOS/VGA device tables.  The portable CPU canvas
      # remains; these backends cannot run on a protected-mode host.
      list(FILTER library_sources EXCLUDE REGEX "/(fl8dtab|svgainit|vesac|vgadtab|vsadtab|w32dtab)\\.c$")
    endif()
    if(library STREQUAL "g2" AND NOT WIN32)
      # These wrappers patch constants embedded in the retired x86 rasterizer
      # assembly and have no native implementation to call on Linux.
      list(FILTER library_sources EXCLUDE REGEX "/(f16fl8ll|fl8fl8ll|svgainit)\\.c$")
    endif()
    if(library STREQUAL "gfile")
      list(FILTER library_sources EXCLUDE REGEX "/gftest\\.c$")
    endif()
    if(library_sources)
      add_library("dark_${library}" STATIC ${library_sources})
      list(APPEND dark_engine_targets "dark_${library}")
      target_include_directories("dark_${library}" BEFORE PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/libsrc/${library}"
      )
      target_link_libraries("dark_${library}" PUBLIC dark_engine_packed)
    endif()
  endforeach()

  target_link_libraries(dark_cpptools PUBLIC dark_platform)
  if(NOT WIN32)
    target_sources(dark_cpptools PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/unixsrc/cpptools/filesystem.cpp"
    )
    target_sources(dark_lgalloc PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/unixsrc/lgalloc/stdalloc.cpp"
    )
  endif()
  target_link_libraries(dark_namedres PUBLIC dark_compression)
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(dark_namedres PUBLIC PNG::PNG)
  elseif(ANDROID)
    target_include_directories(dark_namedres PRIVATE
      "${DARK_PNG_COMPAT_INCLUDE_DIR}"
    )
    target_link_libraries(dark_namedres PUBLIC png_static)
  endif()
  target_link_libraries(dark_res PUBLIC dark_compression)

  set(DARK_SQUIRREL_SOURCE "" CACHE PATH
      "Path to an external Squirrel 3.2 source checkout")
  if(DARK_SQUIRREL_SOURCE)
    if(NOT EXISTS "${DARK_SQUIRREL_SOURCE}/include/squirrel.h")
      message(FATAL_ERROR
        "DARK_SQUIRREL_SOURCE does not contain include/squirrel.h: ${DARK_SQUIRREL_SOURCE}")
    endif()
    set(dark_squirrel_sources
      sqapi.cpp sqbaselib.cpp sqclass.cpp sqcompiler.cpp sqdebug.cpp
      sqfuncstate.cpp sqlexer.cpp sqmem.cpp sqobject.cpp sqstate.cpp
      sqtable.cpp sqvm.cpp
    )
    list(TRANSFORM dark_squirrel_sources
      PREPEND "${DARK_SQUIRREL_SOURCE}/squirrel/")
    set(dark_squirrel_stdlib_sources
      sqstdaux.cpp sqstdblob.cpp sqstdio.cpp sqstdmath.cpp sqstdrex.cpp
      sqstdstream.cpp sqstdstring.cpp sqstdsystem.cpp
    )
    list(TRANSFORM dark_squirrel_stdlib_sources
      PREPEND "${DARK_SQUIRREL_SOURCE}/sqstdlib/")
    add_library(dark_squirrel STATIC
      ${dark_squirrel_sources}
      ${dark_squirrel_stdlib_sources}
    )
    target_include_directories(dark_squirrel PUBLIC
      "${DARK_SQUIRREL_SOURCE}/include"
    )
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
      target_compile_definitions(dark_squirrel PUBLIC _SQ64)
    endif()
    target_compile_definitions(dark_script PRIVATE
      DARK_ENABLE_SQUIRREL=1
    )
    target_link_libraries(dark_script PRIVATE dark_squirrel)
  endif()

  set(DARK_OGG_SOURCE "" CACHE PATH
      "Path to an external libogg 1.3.5 source tree")
  set(DARK_VORBIS_SOURCE "" CACHE PATH
      "Path to an external libvorbis 1.3.7 source tree")
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
      codebook.c sharedbook.c lookup.c bitrate.c
    )
    list(TRANSFORM dark_vorbis_sources
      PREPEND "${DARK_VORBIS_SOURCE}/lib/")
    add_library(dark_vorbis STATIC ${dark_vorbis_sources})
    target_include_directories(dark_vorbis
      PUBLIC "${DARK_VORBIS_SOURCE}/include"
      PRIVATE "${DARK_VORBIS_SOURCE}/lib"
    )
    target_link_libraries(dark_vorbis PUBLIC ogg PRIVATE m)

    add_library(dark_vorbisfile STATIC
      "${DARK_VORBIS_SOURCE}/lib/vorbisfile.c")
    target_include_directories(dark_vorbisfile PUBLIC
      "${DARK_VORBIS_SOURCE}/include")
    target_link_libraries(dark_vorbisfile PUBLIC dark_vorbis)
  endif()

  if(DARK_SDL_PLATFORM)
    add_library(dark_unix_providers STATIC
      unixsrc/appcore/appcore.cpp
      unixsrc/dispdev/display.cpp
      unixsrc/framewrk/diskfree.cpp
      unixsrc/gameshel/gameshell.cpp
      unixsrc/input/gamepad.cpp
      unixsrc/input/guids.c
      unixsrc/input/input.cpp
      unixsrc/movie/movie.cpp
      unixsrc/render/opengl.cpp
      unixsrc/script/scrptmod.cpp
      unixsrc/sound/factory.cpp
      unixsrc/sound/openal_loader.cpp
      unixsrc/sound/platform.cpp
      unixsrc/timer/timer.cpp
      winsrc/input/inpinit.c
      winsrc/input/kbasctab.cpp
      winsrc/input/kbcntry.c
    )
    target_link_libraries(dark_unix_providers PUBLIC dark_engine_packed)
    target_link_libraries(dark_unix_providers PUBLIC SDL2::SDL2)
    if(TARGET dark_squirrel)
      target_compile_definitions(dark_unix_providers PRIVATE
        DARK_ENABLE_SQUIRREL=1)
    endif()
    if(FFMPEG_FOUND)
      target_compile_definitions(dark_unix_providers PRIVATE DARK_HAVE_FFMPEG=1)
      target_link_libraries(dark_unix_providers PUBLIC PkgConfig::FFMPEG)
    else()
      message(WARNING "FFmpeg development libraries were not found; movie playback will be disabled")
    endif()
    if(DARK_OPENGL_ES)
      target_compile_definitions(dark_unix_providers PRIVATE DARK_OPENGL_ES=1)
      if(DARK_GLES3_INCLUDE_DIR)
        target_include_directories(dark_unix_providers PRIVATE "${DARK_GLES3_INCLUDE_DIR}")
      endif()
      target_link_libraries(dark_unix_providers PUBLIC "${DARK_GLES3_LIBRARY}")
    else()
      target_link_libraries(dark_unix_providers PUBLIC OpenGL::GL)
    endif()

    option(DARK_BUILD_THIEF2 "Compile the native SDL Thief 2 game sources" ON)
    if(DARK_BUILD_THIEF2)
      dark_sources_from_vcxproj(dark_game_sources
        "${CMAKE_CURRENT_SOURCE_DIR}/src/dromed.vcxproj")

      # The Visual Studio project contains both DromEd and Thief2. Match its
      # BuildThief2 selection without maintaining a second source manifest.
      set(dark_thief2_sources)
      foreach(source IN LISTS dark_game_sources)
        file(RELATIVE_PATH source_rel "${CMAKE_CURRENT_SOURCE_DIR}" "${source}")
        file(TO_CMAKE_PATH "${source_rel}" source_rel)
        if(source_rel MATCHES "^src/(editor|csg)/" OR
           source_rel MATCHES "^src/framewrk/(initedit|report)[.]cpp$" OR
           source_rel MATCHES "^src/sound/songedtr[.]cpp$")
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

      # These generated scanners deliberately use the same private helper
      # names.  The Visual Studio project compiles them with YY_STATIC; keep
      # that source-level setting when consuming the project from CMake.
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
        # SDLActivity loads libSDL2.so followed by libmain.so and resolves the
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
      target_link_libraries(thief2 PRIVATE
        dark_engine_config
        -Wl,--start-group
        ${dark_engine_targets}
        dark_platform
        dark_compression
        dark_unix_providers
        $<$<TARGET_EXISTS:dark_squirrel>:dark_squirrel>
        $<$<TARGET_EXISTS:dark_vorbisfile>:dark_vorbisfile>
        $<$<TARGET_EXISTS:dark_vorbis>:dark_vorbis>
        $<$<TARGET_EXISTS:ogg>:ogg>
        -Wl,--end-group
        m
      )
      if(NOT ANDROID)
        target_link_libraries(thief2 PRIVATE pthread)
        set_target_properties(thief2 PROPERTIES OUTPUT_NAME thief-runner)
        if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
          install(TARGETS thief2 RUNTIME DESTINATION bin)
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
  endif()
endif()
