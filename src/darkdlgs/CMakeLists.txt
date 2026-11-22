add_library(dark_darkdlgs STATIC
  darkdlgs.cpp
  editlink.cpp
  editplst.cpp
  simplemenu.cpp
)
target_link_libraries(dark_darkdlgs PRIVATE dark_engine_config dark_imgui)
