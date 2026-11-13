set(dark_ui_sources
  bugterm.c
  buttpanl.cpp
  cmdterm.c
  joypoll.cpp
  movie.cpp
  panlmode.cpp
  panltool.cpp
  uiamov.cpp
  uianim.cpp
  uiapp.cpp
  uigame.cpp
  uiloop.cpp
)

target_sources(dark_game_core PRIVATE ${dark_ui_sources})
if(TARGET dark_dromed_core)
  target_sources(dark_dromed_core PRIVATE ${dark_ui_sources})
endif()
