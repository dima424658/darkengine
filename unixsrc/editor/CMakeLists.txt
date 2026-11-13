if(APPLE)
  target_sources(dark_dromed_core PRIVATE
    editor_macos.mm
    objedit_macos.cpp
  )
else()
  # Windows also uses the shared SDL editor UI providers.
  target_sources(dark_dromed_core PRIVATE
    editorplatform.cpp
    objedit_macos.cpp
    winui.cpp
    filemenu.cpp
    menus.cpp
  )
endif()
