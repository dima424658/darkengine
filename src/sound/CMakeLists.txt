set(dark_sound_sources
  ambient.c
  ambprop.cpp
  appsfx.cpp
  auxsnd.cpp
  esnd.cpp
  esndprop.cpp
  esndreac.cpp
  metasnd.cpp
  psnd.cpp
  psndapi.cpp
  psndinst.cpp
  schdb.cpp
  schema.cpp
  schfile.cpp
  schloop.c
  schmsg.cpp
  schprop.cpp
  schsamps.cpp
  schyacc.c
  sndframe.cpp
  sndgen.cpp
  sndloop.cpp
  sndnet.cpp
  sndprop.cpp
  songfile.cpp
  songi.cpp
  songid.cpp
  songmiss.cpp
  songplri.cpp
  songutil.cpp
  spchdom.cpp
  spchprop.cpp
  spchvoc.cpp
  speech.cpp
  vocore.cpp
)

target_sources(dark_game_core PRIVATE ${dark_sound_sources})
if(TARGET dark_dromed_core)
  target_sources(dark_dromed_core PRIVATE ${dark_sound_sources})
endif()

if(TARGET dark_dromed_core)
  target_sources(dark_dromed_core PRIVATE
    songedtr.cpp
  )
endif()
