// $Header: x:/prj/tech/libsrc/res/RCS/resagg.c 1.2 1997/02/24 23:49:38 TOML Exp $

#include <config.h>
#include <res.h>
#include <aggmemb.h>
#include <appagg.h>

#include <res_.h>
#include <resagg.h>
#include <resguid.h>

//////////////////////////////////////////////////////////////
// INIT FUNC
//


#pragma off(unreferenced)
static STDMETHODIMP ResSysInitFunc(IUnknown* goof)
{
   ResInitInternal();

   return kNoError;
}
#pragma on(unreferenced)

//////////////////////////////////////////////////////////////
// SHUTDOWN FUNC
//


#pragma off(unreferenced)
static STDMETHODIMP ResSysShutdownFunc(IUnknown* goof)
{
   ResTerm();

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
// ResCreate()
//
// Called during AppCreateObjects, adds the ResSys system anonymously to the
// app aggregate
//

static struct _init_object
{
   DECLARE_C_COMPLEX_AGGREGATE_CONTROL();
} InitObject; 


static sRelativeConstraint ResSysConstraints[] = 
{
   { kNullConstraint, }
}; 


void LGAPI ResCreate(void)
{
   sAggAddInfo add_info = { NULL, NULL, NULL, NULL, 0, NULL} ; 
   IUnknown* app = AppGetObj(IUnknown);
   INIT_C_COMPLEX_AGGREGATE_CONTROL(InitObject,
               FinalReleaseFunc,    // on final release
               NullFunc,      // connect
               NullFunc,      // post connect   
               ResSysInitFunc,   // init
               ResSysShutdownFunc,  // end
               NullFunc);     // disconnect
   add_info.pID = &UUID_Res;
   add_info.pszName = "Res"; 
   add_info.pControl = InitObject._pAggregateControl;
   add_info.controlPriority = kPriorityLibrary; 
   add_info.pControlConstraints = ResSysConstraints;
   _AddToAggregate(app,&add_info,1);
}



