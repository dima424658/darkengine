add_library(dark_platform STATIC)

if(WIN32)
  set(DARK_PLATFORM_ROOT "${PROJECT_SOURCE_DIR}/winsrc/platform")
else()
  set(DARK_PLATFORM_ROOT "${PROJECT_SOURCE_DIR}/unixsrc/platform")
endif()

target_include_directories(dark_platform PUBLIC
  "${PROJECT_SOURCE_DIR}/h"
  "${PROJECT_SOURCE_DIR}/libsrc/cpptools"
  ${DARK_PLATFORM_ROOT}
)

target_link_libraries(dark_platform PUBLIC SDL3::SDL3)

target_compile_definitions(dark_platform INTERFACE
  $<$<PLATFORM_ID:Linux>:_DEFAULT_SOURCE>
)

target_link_libraries(dark_platform PUBLIC
  $<$<PLATFORM_ID:Windows>:advapi32 user32 ws2_32>
  ${CMAKE_DL_LIBS}
)

if(ANDROID)
  find_library(DARK_ANDROID_LOG_LIBRARY NAMES log REQUIRED)
  target_link_libraries(dark_platform PUBLIC "${DARK_ANDROID_LOG_LIBRARY}")
endif()

set_target_properties(dark_platform PROPERTIES
  ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}"
)

set(dark_include_dirs
  "${PROJECT_SOURCE_DIR}/h"
  "${PROJECT_SOURCE_DIR}/inc"
  "${PROJECT_SOURCE_DIR}/src"
  "${PROJECT_SOURCE_DIR}/src/actreact"
  "${PROJECT_SOURCE_DIR}/src/ai"
  "${PROJECT_SOURCE_DIR}/src/csg"
  "${PROJECT_SOURCE_DIR}/src/editor"
  "${PROJECT_SOURCE_DIR}/src/engfeat"
  "${PROJECT_SOURCE_DIR}/src/framewrk"
  "${PROJECT_SOURCE_DIR}/src/motion"
  "${PROJECT_SOURCE_DIR}/src/object"
  "${PROJECT_SOURCE_DIR}/src/physics"
  "${PROJECT_SOURCE_DIR}/src/portal"
  "${PROJECT_SOURCE_DIR}/src/render"
  "${PROJECT_SOURCE_DIR}/src/script"
  "${PROJECT_SOURCE_DIR}/src/sim"
  "${PROJECT_SOURCE_DIR}/src/sound"
  "${PROJECT_SOURCE_DIR}/src/shock"
  "${PROJECT_SOURCE_DIR}/src/ui"
  "${PROJECT_SOURCE_DIR}/src/dark"
  "${PROJECT_SOURCE_DIR}/src/deepc"
  "${PROJECT_SOURCE_DIR}/src/deepc/motion"
  "${DARK_PLATFORM_ROOT}"
)

add_library(dark_engine_config INTERFACE)
target_include_directories(dark_engine_config INTERFACE ${dark_include_dirs})
target_compile_definitions(dark_engine_config INTERFACE
  RGB_LIGHTING BIG_LIGHTMAPS SCHEMA_LOADING
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
