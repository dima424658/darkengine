if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui.cpp")
  message(FATAL_ERROR
    "DromEd requires Dear ImGui. Run git submodule update --init 3rdparty/imgui.")
endif()

add_library(dark_imgui STATIC
  imgui/imgui.cpp
  imgui/imgui_draw.cpp
  imgui/imgui_tables.cpp
  imgui/imgui_widgets.cpp
  imgui/backends/imgui_impl_sdl3.cpp
  imgui/backends/imgui_impl_vulkan.cpp
)
target_include_directories(dark_imgui SYSTEM PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/imgui")
target_link_libraries(dark_imgui PUBLIC SDL3::SDL3 Vulkan::Vulkan)
