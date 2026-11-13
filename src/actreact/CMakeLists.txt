set(dark_actreact_sources
  arloop.cpp
  contag8r.cpp
  dumbprox.cpp
  mediag8r.cpp
  periog8r.cpp
  pg8rnull.cpp
  propag8n.cpp
  propag8r.cpp
  radiag8r.cpp
  reaction.cpp
  ssrclife.cpp
  stimbase.cpp
  stimguid.c
  stimsens.cpp
  stimsrc.cpp
  stimul8r.cpp
  stimuli.cpp
)

target_sources(dark_game_core PRIVATE ${dark_actreact_sources})
if(TARGET dark_dromed_core)
  target_sources(dark_dromed_core PRIVATE ${dark_actreact_sources})
endif()
