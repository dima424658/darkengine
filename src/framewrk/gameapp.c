/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/gameapp.c,v 1.47 2000/02/19 13:16:38 toml Exp $

#include <string.h>

#include <objbase.h>
#include <windows.h>
#include <aggmemb.h>
#include <appagg.h>
#include <loopapi.h>
#include <appapi.h>
#include <dappsink.h>

#include <uiapp.h>

#include <cfgtool.h>
#include <scrnmode.h>
#include <scrnman.h>
#include <gameapp.h>
#include <gamemode.h>
#include <init.h>
#include <command.h>
#include <dbfile.h>
#include <hotkey.h>
#include <gen_bind.h>
#include <backup.h>
#include <uigame.h>
#include <scrndump.h>
#include <netsynch.h>
#include <appsfx.h>

#include <mprintf.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

////////////////////////////////////////////////////////////
// GAME MODE AND GAME SYSTEM INITIALIZATION
////////////////////////////////////////////////////////////

extern void gedit_exit(void);

void do_game_switch(char* args)
{
   sScrnMode mode = { 0, 0, 0, }; 
   GameModeDesc desc = { NULL, };

   desc.scrnmode = &mode; 

   sscanf(args,"%d,%d",&mode.w,&mode.h);
   if (mode.w != 0 && mode.h != 0)
   {
      mode.valid_fields |= kScrnModeDimsValid;
   }

#ifdef NEW_NETWORK_ENABLED
   // Do network synchronization first
   SwitchToNetSynchMode(kLoopModeSwitch);
#else      
   // Just go straight to game mode
   {
      ILoop* looper = AppGetObj(ILoop);
      sLoopInstantiator* loop  = DescribeGameMode(mmGameDefault,&desc);
      eLoopModeChangeKind change = kLoopModeSwitch;

      ILoop_ChangeMode(looper,change,loop);
      SafeRelease(looper);
   }
#endif
}

// switch to game mode, with sound on
void do_game_switch_loud(char* args)
{   
   SFXInit();
   do_game_switch( args );
}

// switch to game mode, but with sound off
void do_game_switch_quiet(char* args)
{   
   SFXClose();
   do_game_switch( args );
}

static void end_mode(void)
{
   ILoop* looper = AppGetObj(ILoop);
   ILoop_EndMode(looper,0); 
   SafeRelease(looper);
}

void quit_game(void)
{
   IApplication* pApplication = AppGetObj(IApplication);
   IApplication_Quit(pApplication);
   IApplication_Release(pApplication);
}

//
// Dump screen
//

static void screen_dump(char *str)
{
   SetScreenDump(str); 
}


#ifdef PLAYTEST

#endif // PLAYTEST

static Command editor_commands[] = 
{
   { "game_mode", FUNC_STRING, do_game_switch_loud, "switch to game mode"},
   { "game_quiet", FUNC_STRING, do_game_switch_quiet, "switch to game mode, no sounds"},
};


static Command global_commands[] =
{
   { "quit_game", FUNC_VOID, quit_game, "Quit the game"},
   { "end_mode", FUNC_VOID, end_mode, "End the current major mode"},

   CONFIG_SET_CMD("set"),
   CONFIG_GET_CMD("get"),
   CONFIG_EVAL_CMD("eval"),
   CONFIG_IFDEF_CMD("ifdef"),
   CONFIG_IFNDEF_CMD("ifndef"),
   CONFIG_UNSET_CMD("unset"),
   { "screen_dump", FUNC_STRING, screen_dump, "make a screen shot" },



}; 


////////////////////////////////////////////////////////////
// Constants
//

static const GUID* my_guid = &IID_Game;
static char* my_name = "Game system init";
static int my_priority = kPriorityNormal;
#define MY_CREATEFUNC GameSysCreate

////////////////////////////////////////////////////////////
// INIT FUNC
//

#pragma off(unreferenced)
static STDMETHODIMP _InitFunc(IUnknown* goof)
{
   CommandInit();
   COMMANDS(editor_commands, HK_EDITOR);
   COMMANDS(global_commands, HK_ALL);
   command_context_ptr = &HotkeyContext;
   dbfile_setup_commands();

   AppAdvise();
   InitBackup();
   GameInitGUI(); 
   return kNoError;
}
#pragma on(unreferenced)

//////////////////////////////////////////////////////////////
// SHUTDOWN FUNC
//

#pragma off(unreferenced)
static STDMETHODIMP _ShutdownFunc(IUnknown* goof)
{
   TermBackup(); 
   GameTermGUI(); 


   return kNoError;
}
#pragma on(unreferenced)

////////////////////////////////////////////////////////////
// CONSTRAINTS
//

static sRelativeConstraint _Constraints[] = 
{
   { kConstrainAfter, &IID_UI },
   { kNullConstraint, }
}; 

////////////////////////////////////////////////////////////
// Everything below here is boiler plate code.
// Nothing needs to change, unless you want to add postconnect stuff
////////////////////////////////////////////////////////////

#pragma off(unreferenced)
static STDMETHODIMP NullFunc(IUnknown* goof)
{
   return kNoError;
}
#pragma on(unreferenced)

#pragma off(unreferenced)
static void STDMETHODCALLTYPE FinalReleaseFunc(IUnknown* goof)
{
}
#pragma on(unreferenced)

//////////////////////////////////////////////////////////////
// SysCreate()
//
// Called during AppCreateObjects, adds the uiSys system anonymously to the
// app aggregate
//

static struct _init_object
{
   DECLARE_C_COMPLEX_AGGREGATE_CONTROL();
} InitObject; 


void LGAPI MY_CREATEFUNC(void)
{
   sAggAddInfo add_info = { NULL, NULL, NULL, NULL, 0, NULL} ; 
   IUnknown* app = AppGetObj(IUnknown);
   INIT_C_COMPLEX_AGGREGATE_CONTROL(InitObject,
               FinalReleaseFunc,    // on final release
               NullFunc,      // connect
               NullFunc,      // post connect   
               _InitFunc,     // init
               _ShutdownFunc,    // end
               NullFunc);     // disconnect
   add_info.pID = my_guid;
   add_info.pszName = my_name; 
   add_info.pControl = InitObject._pAggregateControl;
   add_info.controlPriority = my_priority;
   add_info.pControlConstraints = _Constraints;
   _AddToAggregate(app,&add_info,1);
   SafeRelease(InitObject._pAggregateControl);
   SafeRelease(app);
}
