add_library(dark_dispdev STATIC
  dispbase.cpp
  display.cpp
  sdl_window.cpp
  sdl_vulkan.cpp
)
dark_configure_library(dark_dispdev)

target_link_libraries(dark_dispdev PUBLIC Vulkan::Vulkan)
if(ANDROID)
  target_include_directories(dark_dispdev SYSTEM PRIVATE
    "${DARK_VULKAN_HEADERS_SOURCE}/include")
endif()
