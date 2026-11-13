set(dark_portal_sources
  animlit.c
  bspsphr.c
  char.c
  duv.c
  oracle.c
  portal.c
  portbg.c
  portclip.c
  portdraw.c
  porthw.c
  portlit.c
  portpick.c
  portsky.c
  portsurf.c
  portwatr.c
  ptdrawhw.c
  ptsurf.cpp
  pt_clut.c
  pt_mem.c
  surfaces.c
  wrcast.c
  wrdbg.c
  wrfunc.c
)

target_sources(dark_game_core PRIVATE ${dark_portal_sources})
if(TARGET dark_dromed_core)
  target_sources(dark_dromed_core PRIVATE ${dark_portal_sources})
endif()
