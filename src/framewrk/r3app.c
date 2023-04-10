/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/r3app.c,v 1.9 2000/02/19 13:16:29 toml Exp $

#include <aggmemb.h>
#include <appagg.h>
#include <r3d.h>

#include <2dapp.h>
#include <r3app.h>
#include <init.h>
#include <mm.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 


// The sim isn't in an r3 frame.  So this is a spare context for
// when we want to use the r3 during the sim.
r3s_context *sim_context;

//////////////////////////////////////////////////////////////
// INIT FUNC
//

#pragma off(unreferenced)
static STDMETHODIMP _InitFunc(IUnknown* goof)
{
   r3_init_defaults(-1,12,-1,-1,-1);
   sim_context = r3_alloc_context(NULL, 0);

   mm_init();
   return kNoError;
}
#pragma on(unreferenced)

//////////////////////////////////////////////////////////////
// SHUTDOWN FUNC
//

#pragma off(unreferenced)
static STDMETHODIMP _ShutdownFunc(IUnknown* goof)
{
   mm_close();
   r3_close();

   return kNoError;
}
#pragma on(unreferenced)


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
// uiSysSysCreate()
//
// Called during AppCreateObjects, adds the uiSys system anonymously to the
// app aggregate
//

static struct _init_object
{
   DECLARE_C_COMPLEX_AGGREGATE_CONTROL();
} InitObject; 


static sRelativeConstraint _Constraints[] = 
{
   { kConstrainAfter, &IID_Gr2d},
   { kNullConstraint, }
}; 


void LGAPI r3SysCreate(void)
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
   add_info.pID = &IID_Gr3d;
   add_info.pszName = "Gr3d"; 
   add_info.pControl = InitObject._pAggregateControl;
   add_info.controlPriority = kPriorityLibrary; 
   add_info.pControlConstraints = _Constraints;
   _AddToAggregate(app,&add_info,1);
   SafeRelease(InitObject._pAggregateControl);
   SafeRelease(app);
}

