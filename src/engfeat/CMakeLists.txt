set(dark_engfeat_sources
  campaign.cpp
  contain.cpp
  contprop.cpp
  dmgmodel.cpp
  dmgprop.cpp
  dmgreact.cpp
  engfeat.cpp
  flinder.cpp
  frobctrl.cpp
  frobprop.cpp
  gunflash.cpp
  gunprop.cpp
  hpprop.cpp
  invlimb.cpp
  invrndpr.cpp
  inv_rend.c
  keysys.cpp
  lockprop.cpp
  ownprop.cpp
  pick.cpp
  preldprp.cpp
  projlink.cpp
  projprop.cpp
  propreac.cpp
  quesloop.cpp
  quesreac.cpp
  quest.cpp
  questprp.cpp
  simpdmg.cpp
  susprop.cpp
  trapprop.cpp
  tweqctrl.cpp
  tweqprop.cpp
  tweqrep.cpp
  weapcb.cpp
  weaphalo.cpp
  weaphit.cpp
  weapon.cpp
  weapprop.cpp
  weapreac.cpp
  x10guish.cpp
)

target_sources(dark_game_core PRIVATE ${dark_engfeat_sources})
if(TARGET dark_dromed_core)
  target_sources(dark_dromed_core PRIVATE ${dark_engfeat_sources})
endif()
