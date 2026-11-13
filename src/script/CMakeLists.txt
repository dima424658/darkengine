set(dark_script_sources
  arscript.cpp
  atkscrpt.cpp
  bodscrpt.cpp
  camscrpt.cpp
  contscrs.cpp
  datascrs.cpp
  dmgscrpt.cpp
  drscrpt.cpp
  engscrpt.cpp
  keyscpt.cpp
  linkscpt.cpp
  litscrpt.cpp
  lockscpt.cpp
  miscrprt.cpp
  mtscrpt.cpp
  netscrpt.cpp
  objscrpt.cpp
  pgrpscpt.cpp
  propscpt.cpp
  pupscrpt.cpp
  quesscpt.cpp
  rooscrpt.cpp
  scrptcmd.cpp
  scrptmod.cpp
  scrptnet.cpp
  scrptprp.cpp
  scrptst.cpp
  sndscrpt.cpp
  twqscrpt.cpp
)

target_sources(dark_game_core PRIVATE ${dark_script_sources})
if(TARGET dark_dromed_core)
  target_sources(dark_dromed_core PRIVATE ${dark_script_sources})
endif()
