/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/initedit.cpp,v 1.26 2000/02/19 13:16:19 toml Exp $

#include <windows.h>
#include <dynfunc.h>

#include <comtools.h>


#include <gshelapi.h>
#include <appapi.h>
#include <appagg.h>
#include <loopapi.h>
#include <config.h>
#include <stdlib.h>

#include <init.h>
#include <editapp.h>
#include <editmode.h>
#include <gamemode.h>
#include <gameapp.h>
#include <scrnmode.h>
#include <cfgtool.h>
#include <dbfile.h>
#include <dbasemsg.h>
#include <objmodel.h>
#include <edittool.h>
#include <sdestool.h>
#include <iobjed.h>

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

#ifdef _WIN32
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
   desc.scrnmode = ScrnModeGetConfig(&mode,"edit_");
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

////////////////////////////////////////

//
// Editor DLL loading
//


static void LGAPI woe(IUnknown *)
{
   Warning(("Cannot find darkdlgs.dll.  Dialogs will not be available\n"));
}

DeclDynFunc_(void, LGAPI, DarkDlgsSetAggregate, (IUnknown *));
ImplDynFunc(DarkDlgsSetAggregate, "darkdlgs.dll", "_DarkDlgsSetAggregate@4", woe);

#define DynDarkDlgsSetAggregate (DynFunc(DarkDlgsSetAggregate).GetProcAddress())

static void load_dlg_lib(void)
{
   AutoAppIPtr_(Unknown,pUnk);
   DynDarkDlgsSetAggregate(pUnk);
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
   CoreEngineAppInit();

   if (!gPrimordialMode)
   {
      if (config_is_defined("start_game_mode"))
         setup_game_mode();
      else
         setup_edit_mode();
   }

   load_dlg_lib();

   char buf[80];
   if (config_get_raw("file",buf,80))
      dbLoad(buf,kFiletypeAll);
   else new_world();


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




#endif




/*
Local Variables:
typedefs:("config_write_spec" "uint" "ushort")
End:
*/
