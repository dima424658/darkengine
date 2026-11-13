
if(TARGET dark_dromed_core)
  target_sources(dark_dromed_core PRIVATE
    bspdata.c
    csg.c
    csgalloc.c
    csgbbox.c
    csgbuild.c
    csgcheck.c
    csgclip.c
    csgcube.c
    csgemit.c
    csgfind.c
    csgmedia.c
    csgmerge.c
    csgutil.c
    hep.c
  )
endif()
