/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/2dapp.c,v 1.10 2000/02/19 12:29:35 toml Exp $

#include <2d.h>
#include <aggmemb.h>
#include <appagg.h>
#include <dispguid.h>
#include <gshlguid.H>
#include <resguid.h>

#include <2dapp.h>
#include <init.h>

#include <mallocdb.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

//////////////////////////////////////////////////////////////
// ALLOCATION OVERRIDES
//
#ifndef SHIP

void * dark_gr_malloc(int n)
{
   return malloc_db((size_t)n, "gr_malloc()", 0);
}

void dark_gr_free(void *p)
{
   free_db(p, "gr_free()", 0);
}

#endif

//////////////////////////////////////////////////////////////
// INIT FUNC
//

#pragma off(unreferenced)
STDMETHODIMP Gr2dInitFunc(IUnknown* goof)
{
#ifndef SHIP
   gr_set_malloc(dark_gr_malloc);
   gr_set_free(dark_gr_free);
#endif
   gr_init();

   return kNoError;
}
#pragma on(unreferenced)

//////////////////////////////////////////////////////////////
// SHUTDOWN FUNC
//


#pragma off(unreferenced)
STDMETHODIMP Gr2dShutdownFunc(IUnknown* goof)
{
   gr_close();

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
// Gr2dSysCreate()
//
// Called during AppCreateObjects, adds the Gr2d system anonymously to the
// app aggregate
//

static struct _init_object
{
   DECLARE_C_COMPLEX_AGGREGATE_CONTROL();
} InitObject;

extern const GUID UUID_Res;

static sRelativeConstraint Gr2dConstraints[] =
{
   { kConstrainAfter, &UUID_Res},
   { kConstrainAfter, &IID_IDisplayDevice},
   { kConstrainAfter, &IID_IGameShell},
   { kNullConstraint, }
};


void LGAPI Gr2dCreate(void)
{
   sAggAddInfo add_info = { NULL, NULL, NULL, NULL, 0, NULL} ;
   IUnknown* app = AppGetObj(IUnknown);
   INIT_C_COMPLEX_AGGREGATE_CONTROL(InitObject,
               FinalReleaseFunc,    // on final release
               NullFunc,      // connect
               NullFunc,      // post connect
               Gr2dInitFunc,     // init
               Gr2dShutdownFunc,    // end
               NullFunc);     // disconnect
   add_info.pID = &IID_Gr2d;
   add_info.pszName = "Gr2d";
   add_info.pControl = InitObject._pAggregateControl;
   add_info.controlPriority = kPriorityLibrary;
   add_info.pControlConstraints = Gr2dConstraints;
   _AddToAggregate(app,&add_info,1);
   SafeRelease(InitObject._pAggregateControl);
   SafeRelease(app);
}


