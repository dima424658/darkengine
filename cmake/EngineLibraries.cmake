# Legacy engine libraries and source manifest integration.
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
# so providers can safely include SDL3, Vulkan, and the C++ standard library.
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
if(CMAKE_SYSTEM_NAME MATCHES "^(Linux|Darwin)$" OR ANDROID OR WIN32)
  if(ANDROID)
    target_include_directories(dark_namedres PRIVATE
      "${DARK_PNG_COMPAT_INCLUDE_DIR}"
    )
    target_link_libraries(dark_namedres PUBLIC png_static)
  elseif(APPLE)
    # CMake's macOS ZLIB finder may expose the SDK's entire usr/include
    # directory, which shadows libc++ headers with non-Apple Clang toolchains.
    # The compiler already searches the SDK sysroot; expose only libpng's
    # Homebrew include directory and keep the discovered libraries.
    target_include_directories(dark_namedres PRIVATE "${PNG_PNG_INCLUDE_DIR}")
    target_link_libraries(dark_namedres PUBLIC ${PNG_LIBRARIES})
  else()
    target_link_libraries(dark_namedres PUBLIC PNG::PNG)
  endif()
endif()
target_link_libraries(dark_res PUBLIC dark_compression)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/Squirrel.cmake")

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/OggVorbis.cmake")

