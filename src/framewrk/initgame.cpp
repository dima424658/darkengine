/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/initgame.cpp,v 1.3 1998/10/05 17:26:17 mahk Exp $

#include <comtools.h>


#include <gshelapi.h>
#include <appapi.h>
#include <appagg.h>
#include <loopapi.h>
#include <config.h>
#include <stdlib.h>

#include <init.h>
#include <gamemode.h>
#include <gameapp.h>
#include <scrnmode.h>
#include <cfgtool.h>
#include <dbfile.h>
#include <dbasemsg.h>
#include <objmodel.h>
#include <sdestool.h>
#include <contentctx.h>
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
   GameSysCreate();
   SdescToolsCreate(); 

   return NOERROR;
}

////////////////////////////////////////////////////////////
// Init helper funcs
//


static void setup_game_mode(void)
{
   GameModeDesc desc;
   sScrnMode mode; 

   memset(&desc,0,sizeof(desc));
   desc.scrnmode = ScrnModeGetConfig(&mode,"game_");
   gPrimordialMode = DescribeGameMode(mmGameDefault,&desc);
}

////////////////////////////////////////

//
// Called to notify COM initialization is complete.  The application
// uses this time to acquire pointers to the available components if
// using globals.  Other initialization may also be placed here.
//

tResult LGAPI AppInit()
{
   const BOOL quickstart = config_is_defined("quickstart");

   CoreEngineAppInit();

#ifdef THIEF2_GAME
   // Direct command-line mission loads occur before the biped loop client
   // has had an opportunity to create the motion database.
   if (!g_pMotionSet)
      MotionManagerInit();
#endif

   // quickstart=<mission> is a deterministic developer path: bypass the
   // mission loop, movies, and menus and make gameplay the primordial mode.
   if (quickstart)
      setup_game_mode();
   else if (!gPrimordialMode)
      setup_game_mode();   

   char buf[260];
   const char *load_var = quickstart ? "quickstart" : "file";
   if (config_get_raw(load_var,buf,sizeof(buf)))
      dbLoad(buf,kFiletypeAll);
   else if (!DarkContentShowPicker())
      dbBuildDefault();

   return NOERROR;
}

//
// Call to notify COM close-down is pending.  A good time for the
// application to Release() interfaces acquired during AppInit(), plus
// any other clean-up desired.
//


tResult LGAPI AppExit()
{
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
