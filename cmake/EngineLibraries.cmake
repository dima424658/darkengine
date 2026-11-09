# Load the explicit, CMake-owned source lists for the engine targets.
include("${CMAKE_CURRENT_LIST_DIR}/EngineSourceManifest.cmake")

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
  # Shared SDL input providers and retained legacy keyboard/mouse headers.
  # Keep this last so its Windows input GUID header cannot shadow
  # compapis/inpguid.h.
  "${CMAKE_CURRENT_SOURCE_DIR}/libsrc/input"
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
  appcore compapis comtools config cpptools darkloop dev2d dispdev
  dstruct dump fix font g2 gadget gfile inputbnd lg lgalloc lgd3d matrix md
  mm mp mprintf namedres net prof r3d random recorder ref res rnd script
  sdesc skel sndsrc sndutil sound star tagfile templgen timer ui vec2
)
foreach(library IN LISTS dark_library_projects)
  set(source_variable "dark_sources_${library}")
  set(library_sources ${${source_variable}})
  if(library STREQUAL "appcore" AND NOT WIN32)
    list(FILTER library_sources EXCLUDE REGEX "/dappcore\\.cpp$")
  endif()
  if(library STREQUAL "g2")
    # Drop DOS/VGA initialization and retired x86 rasterizer patch wrappers.
    list(FILTER library_sources EXCLUDE REGEX "/(f16fl8ll|fl8fl8ll)\\.c$")
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

# Compile the Vulkan implementation with the native ABI settings, then fold
# its objects into lgd3d, whose legacy sources require byte-packed records.
add_library(dark_lgd3d_vulkan OBJECT ${dark_lgd3d_vulkan_sources})
target_link_libraries(dark_lgd3d_vulkan PRIVATE dark_engine_config)
target_link_libraries(dark_lgd3d_vulkan PRIVATE Vulkan::Vulkan ${DARK_SDL_TARGET})
target_include_directories(dark_lgd3d_vulkan PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/libsrc/lgd3d"
  "${CMAKE_CURRENT_SOURCE_DIR}/h")
if(ANDROID)
  target_include_directories(dark_lgd3d_vulkan SYSTEM PRIVATE
    "${DARK_VULKAN_HEADERS_SOURCE}/include")
endif()
if(DEFINED DARK_VULKAN_SHADER_DIR)
  target_compile_definitions(dark_lgd3d_vulkan PRIVATE
    DARK_VULKAN_SHADER_DIR="${DARK_VULKAN_SHADER_DIR}")
endif()
if(WIN32)
  target_compile_definitions(dark_lgd3d_vulkan PRIVATE
    NOMINMAX WIN32_LEAN_AND_MEAN)
endif()
target_sources(dark_lgd3d PRIVATE $<TARGET_OBJECTS:dark_lgd3d_vulkan>)
target_link_libraries(dark_lgd3d PUBLIC Vulkan::Vulkan ${DARK_SDL_TARGET})
if(TARGET dark_vulkan_shaders)
  add_dependencies(dark_lgd3d_vulkan dark_vulkan_shaders)
endif()

# SDL surface creation is platform display integration, not renderer state.
# Compile it with the native C++ ABI and attach its object to dispdev while
# keeping Vulkan resource ownership in lgd3d.
add_library(dark_dispdev_vulkan OBJECT libsrc/dispdev/sdl_vulkan.cpp)
target_link_libraries(dark_dispdev_vulkan PRIVATE dark_engine_config
  Vulkan::Vulkan ${DARK_SDL_TARGET})
target_include_directories(dark_dispdev_vulkan PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/libsrc/lgd3d"
  "${CMAKE_CURRENT_SOURCE_DIR}/h")
if(ANDROID)
  target_include_directories(dark_dispdev_vulkan SYSTEM PRIVATE
    "${DARK_VULKAN_HEADERS_SOURCE}/include")
endif()
target_sources(dark_dispdev PRIVATE
  $<TARGET_OBJECTS:dark_dispdev_vulkan>)
target_link_libraries(dark_dispdev PUBLIC Vulkan::Vulkan ${DARK_SDL_TARGET})

target_link_libraries(dark_cpptools PUBLIC dark_platform)
if(WIN32)
  target_compile_definitions(dark_lgalloc PRIVATE
    DARKENGINE_MODERN_CRT_ALLOCATOR=1
  )
endif()
target_link_libraries(dark_namedres PUBLIC dark_compression)
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
target_link_libraries(dark_res PUBLIC dark_compression)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/Squirrel.cmake")

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/OggVorbis.cmake")

