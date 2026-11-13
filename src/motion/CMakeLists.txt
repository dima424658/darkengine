set(dark_motion_sources
  biploop.c
  cerebllm.cpp
  crattach.cpp
  creature.cpp
  cretprop.cpp
  crexp.cpp
  crjoint.cpp
  crwpnapi.cpp
  crwpnlst.cpp
  crwpnlup.cpp
  mcoord.cpp
  motdb.cpp
  motdesc.cpp
  motdmnge.c
  motmngr.cpp
  motprop.cpp
  motschem.cpp
  motset.cpp
  mputil.cpp
  mschutil.cpp
  mschyacc.c
  mskilset.cpp
  mskltree.cpp
  mvrcmbat.cpp
  mvrflags.cpp
  mvrflbow.cpp
  mvrgact.cpp
  mvrgloco.cpp
  mvrplyr.cpp
  mvrsngle.cpp
  mvrutils.cpp
  puppet.cpp
  pupprop.cpp
)

target_sources(dark_game_core PRIVATE ${dark_motion_sources})
if(TARGET dark_dromed_core)
  target_sources(dark_dromed_core PRIVATE ${dark_motion_sources})
endif()
