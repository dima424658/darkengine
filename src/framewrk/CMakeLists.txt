set(dark_framewrk_sources
  2dapp.c
  appname.c
  backup.cpp
  basemode.c
  buftagf.cpp
  cfgtool.c
  chainevt.cpp
  command.c
  contentctx.cpp
  ctag.cpp
  ctagnet.cpp
  ctagset.cpp
  dappsink.cpp
  dbfile.cpp
  diskfree.cpp
  dispatch.cpp
  dlgmode.c
  fcytagdb.cpp
  filetool.c
  filevar.cpp
  fltang.cpp
  appmenu.cpp
  gameapp.c
  gameinfo.c
  gamepad.cpp
  gamemode.c
  gamescrn.cpp
  gamestr.cpp
  gametool.cpp
  gen_bind.cpp
  imgsrc.cpp
  init.cpp
  loopapp.c
  lresdisk.c
  main.cpp
  namemap.cpp
  netman.cpp
  netmsg.cpp
  netprops.cpp
  netsim.cpp
  netsynch.cpp
  pickgame.cpp
  r3app.c
  rand.cpp
  reprthlp.cpp
  reputil.cpp
  resapp.c
  resdata.c
  resloop.c
  scrndump.cpp
  subscrip.cpp
  tagdbdat.cpp
  tagdbin.cpp
  tagdbkey.cpp
  tagdbkst.cpp
  tagdbout.cpp
  tagpatch.cpp
  tagremap.cpp
)

target_sources(dark_game_core PRIVATE ${dark_framewrk_sources})
if(TARGET dark_dromed_core)
  target_sources(dark_dromed_core PRIVATE ${dark_framewrk_sources})
endif()

target_sources(dark_game_core PRIVATE
  gameiid.c
  initgame.cpp
)

if(TARGET dark_dromed_core)
  target_sources(dark_dromed_core PRIVATE
    initedit.cpp
    report.cpp
  )
endif()
