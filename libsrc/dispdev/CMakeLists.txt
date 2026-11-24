add_library(dark_dispdev STATIC
  dispbase.cpp
  display.cpp
  sdl_window.cpp
  sdl_vulkan.cpp
)
if(DARK_TARGET_VITA)
  target_sources(dark_dispdev PRIVATE vita_layout.cpp)
endif()
dark_configure_library(dark_dispdev)

if(DARK_TARGET_VITA)
  target_link_libraries(dark_dispdev PUBLIC dark_vita_gl)
else()
  target_link_libraries(dark_dispdev PUBLIC Vulkan::Vulkan)
endif()
if(ANDROID)
  target_include_directories(dark_dispdev SYSTEM PRIVATE
    "${DARK_VULKAN_HEADERS_SOURCE}/include")
endif()
