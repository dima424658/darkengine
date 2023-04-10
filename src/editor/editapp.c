/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/editapp.c,v 1.50 2000/02/19 12:27:57 toml Exp $

#include <string.h>
#include <ctype.h>

#include <aggmemb.h>
#include <appagg.h>
#include <config.h>
#include <memstat.h>

#include <editapp.h>
#include <gameapp.h>
#include <editmode.h>
#include <init.h>
#include <contexts.h>
#include <command.h>
#include <cmdterm.h>
#include <gen_bind.h>
#include <status.h>
#include <cfgtool.h>
#include <viewmgr.h>
#include <resapp.h>
#include <preload.h>
#include <objedit.h>
#include <uiedit.h>
#include <ged_room.h>
#include <fvaredit.h>
#include <scrnedit.h>

#include <prof.h>
#include <motedit.h> // for motedit_cmd

#include <mprintf.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

// Should this be here?  Probably not
void MonoDebug (void)
{
   DbgMonoConfig ();
}

////////////////////////////////////////////////////////////
// Constants
//

static const GUID* my_guid = &IID_Editor;
static char* my_name = "Editor Init";
static int my_priority = kPriorityNormal;
#define MY_CREATEFUNC EditorSysCreate

////////////////////////////////////////////////////////////
// INIT FUNC
//

void edit_setup_commands(void);

#pragma off(unreferenced)
static STDMETHODIMP _InitFunc(IUnknown* goof)
{
   extern void setup_gfh_commands(void);

   setup_gfh_commands();
   vm_init();
   vm_init_cameras();
   ged_room_init();
   initProfiler();
   edit_setup_commands();
   history_start();
   ObjEditInit();
   FileVarEditInit(); 
   ScrnEditInit(); 

   return kNoError;
}
#pragma on(unreferenced)

//////////////////////////////////////////////////////////////
// SHUTDOWN FUNC
//

#pragma off(unreferenced)
static STDMETHODIMP _ShutdownFunc(IUnknown* goof)
{
   vm_term();
   preload_free_all();
   history_free_all();
   ScrnEditTerm(); 

   return kNoError;
}
#pragma on(unreferenced)

////////////////////////////////////////////////////////////
// CONSTRAINTS
//

static sRelativeConstraint _Constraints[] =
{
   { kConstrainBefore, &IID_Game},
   { kConstrainAfter,  &IID_Res},
   { kNullConstraint, }
};

////////////////////////////////////////////////////////////
// Commands
//

extern void enter_edit_mode(char*);


EXTERN void texture_pal_cmd(void);

static Command commands[] =
{
   { "edit_mode", FUNC_STRING, enter_edit_mode, "Switch to editor mode: edit_mode <wid>,<hgt>", HK_ALL},
   { "mono_debug", FUNC_VOID, MonoDebug, "Enter monochrome debug screen", HK_ALL},
   { "texture_pal", FUNC_VOID, texture_pal_cmd, "Bring up the texture palette", HK_EDITOR}, 
   { "motedit", FUNC_VOID, motedit_cmd, "Bring up the motion editor", HK_EDITOR}, 
   { "redraw_all", FUNC_VOID, redraw_all_cmd, "Redraw the editor screen", HK_EDITOR}, 
   CONFIG_SET_CMD("set"),
   CONFIG_GET_CMD("get"),
   CONFIG_EVAL_CMD("eval"),
   CONFIG_IFDEF_CMD("ifdef"),
   CONFIG_IFNDEF_CMD("ifndef"),
   CONFIG_UNSET_CMD("unset"),

};

void edit_setup_commands(void)
{
   COMMANDS(commands,HK_ALL);
}

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
