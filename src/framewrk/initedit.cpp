/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/initedit.cpp,v 1.26 2000/02/19 13:16:19 toml Exp $

#include <comtools.h>


#include <gshelapi.h>
#include <appapi.h>
#include <appagg.h>
#include <loopapi.h>
#include <config.h>
#include <render_backend.h>
#include <stdlib.h>

#include <init.h>
#include <editapp.h>
#include <editmode.h>
#include <gamemode.h>
#include <gamescrn.h>
#include <gameapp.h>
#include <scrnmode.h>
#include <cfgtool.h>
#include <dbfile.h>
#include <dbasemsg.h>
#include <objmodel.h>
#include <edittool.h>
#include <sdestool.h>
#include <iobjed.h>
#ifdef THIEF2_GAME
#include <motmngr.h>
#endif

// must be last header
#include <dbmem.h>

// huh?
#ifdef __WATCOMC__
#pragma warning 555 9
#endif

////////////////////////////////////////////////////////////
// EDITOR INITIALIZATION
//

//
// This file specifies aggregate creation for the editor executable
//

///////////////////////////////////////////////////////////////
// COM object initializaiton
//

//
// Called to notify start-up code is complete, COM initialization is
// pending.
// The application uses this to call the COM "Create" functions
//



tResult LGAPI AppCreateObjects(int argc, const char *argv[])
{
   CoreEngineCreateObjects(argc,argv);
   EditorSysCreate();
   GameSysCreate();
   EditToolsCreate();
   SdescToolsCreate();
   ObjEditorsCreate();

   return NOERROR;
}

////////////////////////////////////////////////////////////
// Init helper funcs
//

static void setup_edit_mode(void)
{
   EditModeDesc desc;
   sScrnMode mode = {0 };

   memset(&desc,0,sizeof(desc));
   ScrnModeGetConfig(&mode,"edit_");

   // DromEd shares the modern renderer with the game executable.  Keep the
   // legacy canvas at a layout the editor understands, but render its 3D
   // views through lgd3d's D3D11 implementation and present them in a normal
   // resizable desktop window.  An explicit edit_screen_size remains useful
   // for choosing another editor layout.
   if (!(mode.valid_fields & kScrnModeDimsValid))
   {
      mode.w = 1024;
      mode.h = 768;
      mode.valid_fields |= kScrnModeDimsValid;
   }
   mode.bitdepth = 16;
   mode.valid_fields |= kScrnModeBitDepthValid;
   mode.flags &= ~kScrnModeFullScreen;
   mode.flags |= kScrnModeWindowed | kScrnMode2dDriver | kScrnMode3dDriver;
   mode.valid_fields |= kScrnModeFlagsValid;

   desc.scrnmode = &mode;
   gPrimordialMode = DescribeEditMode(mmEditDefault,&desc);
}

static void setup_game_mode(void)
{
   GameModeDesc desc;
   sScrnMode mode;

   memset(&desc,0,sizeof(desc));
   desc.scrnmode = ScrnModeGetConfig(&mode,"game_");
   gPrimordialMode = DescribeGameMode(mmGameDefault,&desc);
}

static void constrain_editor_game_mode(sScrnMode* mode)
{
   // Game mode is a preview hosted by DromEd, not a separate exclusive-mode
   // application. Keep the legacy render canvas compatible with the D3D11
   // presenter and retain the editor's normal resizable top-level window.
   mode->bitdepth = 16;
   mode->flags &= ~kScrnModeFullScreen;
   mode->flags |= kScrnModeWindowed | kScrnMode2dDriver | kScrnMode3dDriver;
   mode->valid_fields |= kScrnModeBitDepthValid | kScrnModeFlagsValid;
}

//
// Called to notify COM initialization is complete.  The application
// uses this time to acquire pointers to the available components if
// using globals.  Other initialization may also be placed here.
//

//@TODO: make a dbNew and remove this
EXTERN void new_world(void);

tResult LGAPI AppInit()
{
   RenderBackendTraceReset();
#ifndef THIEF2_GAME
   // DromEd's render canvas tracks its resizable client area. Keep the
   // presenter pixel-aligned; the game executable retains the default
   // aspect-fit scaling for its fixed-resolution menus and gameplay canvas.
   RenderBackendSetScaleToWindow(FALSE);
   RenderBackendSetPreserveCanvas(TRUE);
#endif

#ifndef THIEF2_GAME
   ConstrainGameScreenMode(constrain_editor_game_mode);
#endif
   CoreEngineAppInit();

#ifdef THIEF2_GAME
   // The playtest startup path can load a command-line mission before the
   // biped loop client has created its motion database on modern builds.
   if (!g_pMotionSet)
      MotionManagerInit();
#endif

   if (!gPrimordialMode)
   {
#ifdef THIEF2_GAME
      setup_game_mode();
#else
      // DromEd must enter its normal editor mode before starting a playtest.
      // Making game mode primordial skips the editor-to-game handoff (mission
      // backup, simulation setup, UI teardown, etc.) and can start AI work
      // while simulation time is already advancing.  The editor loop consumes
      // start_game_mode after its first fully initialized frame instead.
      setup_edit_mode();
#endif
   }


   char buf[260];
   const char *load_var = config_is_defined("render_test") ? "render_test" : "file";
   if (config_get_raw(load_var,buf,sizeof(buf)))
   {
      edbFiletype loaded = dbLoad(buf,kFiletypeAll);
      RenderBackendTrace("database-load var=%s file=%s result=0x%x",
                       load_var,buf,loaded);
   }
   else
      new_world();


   return NOERROR;
}

//
// Call to notify COM close-down is pending.  A good time for the
// application to Release() interfaces acquired during AppInit(), plus
// any other clean-up desired.
//

// @TODO: remove this extern
EXTERN bool cow_autosaves;

tResult LGAPI AppExit()
{
   if (config_is_defined("save_on_exit"))
      dbSave("exit.cow",kFiletypeAll);
   dbReset();
   objmodelFreeAllModels();
   CoreEngineAppExit();
   return NOERROR;
}




/*
Local Variables:
typedefs:("config_write_spec" "uint" "ushort")
End:
*/
