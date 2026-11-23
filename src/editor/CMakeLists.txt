set(dark_editor_sources
  isdesced.cpp
  prompts.c
  sdesparm.cpp
  status.c
)

target_sources(dark_game_core PRIVATE ${dark_editor_sources})
if(TARGET dark_dromed_core)
  target_sources(dark_dromed_core PRIVATE ${dark_editor_sources})
endif()

if(TARGET dark_dromed_core)
  target_sources(dark_dromed_core PRIVATE
    alignpnp.c
    arcmd.cpp
    areapnp.c
    autovbr.cpp
    bldstair.c
    brinfo.c
    brlist.c
    brloop.c
    brobjpro.cpp
    brprop.cpp
    brquery.c
    brrend.c
    brundo.c
    brushgfh.c
    cmdbutts.c
    convpred.cpp
    cvtbrush.c
    doorblok.cpp
    dragbox.c
    editapp.c
    editbr.c
    editgeom.c
    editmode.c
    editobj.c
    editsave.c
    edmedmo.c
    famedit.cpp
    flowpnp.cpp
    fvaredit.cpp
    gedit.c
    gedmedmo.c
    ged_csg.cpp
    ged_line.c
    ged_rend.c
    ged_rmap.c
    ged_room.cpp
    ged_undo.c
    gfhfrobs.c
    gridpnp.c
    gridsnap.c
    hilight.c
    lightpnp.c
    linkdraw.cpp
    linkedit.cpp
    lnktrait.cpp
    medflbox.c
    imgui_motion_editor.cpp
    medplay.cpp
    modalui.c
    motedit.cpp
    namenum.c
    objed.cpp
    objhistp.cpp
    objpnp.c
    otreeed.cpp
    phproped.cpp
    pnptools.c
    imgui_pnp.cpp
    preload.c
    primal.c
    primshap.c
    proped.cpp
    proptest.cpp
    psdialog.cpp
    quesedit.cpp
    rendtest.c
    roompnp.c
    roomtowr.cpp
    saveload.c
    scrnedit.cpp
    scrpted.cpp
    simpwrap.cpp
    strprped.cpp
    stupidui.c
    swappnp.c
    swapregs.c
    txtpnp.c
    imgui_texture_palette.cpp
    undoredo.c
    userpnp.c
    vbrush.c
    vbr_math.c
    viewmgr.c
    vmwincfg.c
    vslider.c
    vumanui.c
  )
endif()

if(TARGET dark_dromed_core)
  target_sources(dark_dromed_core PRIVATE uiedit.c editorplatform.cpp objedit.cpp filemenu.cpp menubar.cpp
    imgui_editor_preview.cpp imgui_workspace.cpp menus.cpp)
endif()

if(TARGET dark_dromed_core)
  if(UNIX AND NOT APPLE)
    set(NFD_PORTAL ON CACHE BOOL "Use desktop portal file dialogs")
  endif()
  add_subdirectory("${PROJECT_SOURCE_DIR}/3rdparty/nativefiledialog-extended" "${PROJECT_BINARY_DIR}/nativefiledialog")
  target_link_libraries(dark_dromed_core PRIVATE nfd)
endif()
