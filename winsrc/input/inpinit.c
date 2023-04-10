// $Header: x:/prj/tech/libsrc/input/RCS/inpinit.c 1.2 1996/10/17 17:39:20 TOML Exp $

#include <aggmemb.h>
#include <appagg.h>
#include <inpinit.h>
#include <tminit.h>
#include <kb.h>
#include <mouse.h>

////////////////////////////////////////////////////////////
// Constants
//

static const GUID* my_guid = &UUID_InputLib;
static char* my_name = "Input Libraries";
static int my_priority = kPriorityLibrary;
#define MY_CREATEFUNC InputCreate

#define DEFAULT_SCREEN_X 1024
#define DEFAULT_SCREEN_Y 1024

////////////////////////////////////////////////////////////
// INIT FUNC
//

static char keybuf[512];

#pragma off(unreferenced)
static STDMETHODIMP _InitFunc(IUnknown* goof)
{
   kb_startup(keybuf);
   mouse_init(DEFAULT_SCREEN_X,DEFAULT_SCREEN_Y);

   return kNoError;
}
#pragma on(unreferenced)

//////////////////////////////////////////////////////////////
// SHUTDOWN FUNC
//

#pragma off(unreferenced)
static STDMETHODIMP _ShutdownFunc(IUnknown* goof)
{
   mouse_shutdown();
   kb_shutdown();
   return kNoError;
}
#pragma on(unreferenced)

////////////////////////////////////////////////////////////
// CONSTRAINTS
//

static sRelativeConstraint _Constraints[] =
{
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
				   FinalReleaseFunc,  	// on final release
				   NullFunc,  		// connect
				   NullFunc,  		// post connect
				   _InitFunc,  	// init
				   _ShutdownFunc, 	// end
				   NullFunc); 		// disconnect
   add_info.pID = my_guid;
   add_info.pszName = my_name;
   add_info.pControl = InitObject._pAggregateControl;
   add_info.controlPriority = my_priority;
   add_info.pControlConstraints = _Constraints;
   _AddToAggregate(app,&add_info,1);
   SafeRelease(app);
}

