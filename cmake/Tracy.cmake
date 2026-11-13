option(DARK_ENABLE_TRACY "Enable Tracy CPU profiling" OFF)

if(NOT EXISTS "${PROJECT_SOURCE_DIR}/3rdparty/tracy/public/tracy/Tracy.hpp")
  message(FATAL_ERROR
    "Tracy source is missing. Run git submodule update --init 3rdparty/tracy.")
endif()

add_library(dark_profiling INTERFACE)
target_include_directories(dark_profiling SYSTEM INTERFACE
  "${PROJECT_SOURCE_DIR}/3rdparty/tracy/public")
if(DARK_ENABLE_TRACY)
  set(TRACY_ENABLE ON)
  set(TRACY_STATIC ON)
  set(TRACY_ON_DEMAND ON CACHE BOOL "Collect Tracy events only while connected")
  add_subdirectory("${PROJECT_SOURCE_DIR}/3rdparty/tracy"
    "${CMAKE_BINARY_DIR}/third_party/tracy" EXCLUDE_FROM_ALL)
  # The static client is also linked into Android's shared libmain.so.
  set_target_properties(TracyClient PROPERTIES POSITION_INDEPENDENT_CODE ON)
  target_link_libraries(dark_profiling INTERFACE Tracy::TracyClient)
endif()
