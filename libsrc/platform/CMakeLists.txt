add_library(dark_platform STATIC)
if(WIN32)
 target_sources(dark_platform PRIVATE platform_win32.cpp)
else()
 target_sources(dark_platform PRIVATE platform_posix.cpp)
endif()
target_include_directories(dark_platform PUBLIC
  "${PROJECT_SOURCE_DIR}/h"
  "${PROJECT_SOURCE_DIR}/libsrc/cpptools"
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(dark_platform PUBLIC SDL3::SDL3)
target_link_libraries(dark_platform PRIVATE dark_sanitizers)

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
