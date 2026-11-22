add_library(dark_darkdlgs STATIC
  darkdlgs.cpp
  editlink.cpp
  editplst.cpp
  modal.cpp
  simplemenu.cpp
  structedit.cpp
)
target_link_libraries(dark_darkdlgs PRIVATE dark_engine_config dark_imgui)
