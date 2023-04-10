/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkinit.c,v 1.5 2000/02/19 13:25:30 toml Exp $

#include <comtools.h>
#include <aggmemb.h>
#include <appagg.h>
#include <aggmemb.h>

#include <loopapi.h>
#include <shkloop.h>

#include <shkinit.h>
#include <loopapp.h>
#include <init.h>

#include <gamemode.h>
#include <basemode.h>

#include <simpldmg.h>

#include <engfeat.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

////////////////////////////////////////////////////////////
// Constants
//

static const GUID* my_guid = &UUID_SHOCK;
        
static char* my_name = "SHOCK";  

static int my_priority = kPriorityNormal;          // my_priority


#define MY_CREATEFUNC CreateShockStuff

EXTERN void LGAPI MY_CREATEFUNC(void);

//
// Aggregate member creation
//

void LGAPI CreateShock(void)
{
   MY_CREATEFUNC();
   InitSimpleDamageModel();
}



////////////////////////////////////////////////////////////
// SHOCK CLIENT FACTORIES 
//

static const sLoopClientDesc* Factories[]=
{
   &ShockSimLoopClientDesc,
   &ShockRenderClientDesc,
   &ShockRender2ClientDesc,

   NULL
};

static ulong FactoryID;

#define NUM_ELEMS(x) (sizeof(x)/sizeof((x)[0]))

////////////////////////////////////////////////////////////
// SHOCK LOOPMODE DESCS
//

static sLoopModeDesc* Modes[] = 
{
   &ShockGameClients,
   &ShockBaseClients,
};

////////////////////////////////////////////////////////////
// INIT FUNC
//



#pragma off(unreferenced)
static STDMETHODIMP _InitFunc(IUnknown* goof)
{
   int i;
   ILoopManager* pLoopManager = AppGetObj(ILoopManager);
   ILoopClientFactory* factory = CreateLoopFactory(Factories);

   // Add client factories
   ILoopManager_AddClientFactory(pLoopManager,factory,&FactoryID);
   SafeRelease(factory);

   // Add loopmodes
   for (i = 0; i < NUM_ELEMS(Modes); i++)
   {
      ILoopManager_AddMode(pLoopManager,Modes[i]);
   }

   // Set up clients
   GameModeSetGameSpecClient(&LOOPID_ShockGameClients);
   BaseModeSetGameSpecClient(&LOOPID_ShockBaseClients);
   EngineFeaturesCreate();  // wooo wooo

   SafeRelease(pLoopManager);

   return kNoError;
}
#pragma on(unreferenced)

//////////////////////////////////////////////////////////////
// SHUTDOWN FUNC
//

#pragma off(unreferenced)
static STDMETHODIMP _ShutdownFunc(IUnknown* goof)
{
   ILoopManager* pLoopManager = AppGetObj(ILoopManager);
   ILoopManager_RemoveClientFactory(pLoopManager,FactoryID);
   
   SafeRelease(pLoopManager);
   return kNoError;
}
#pragma on(unreferenced)

////////////////////////////////////////////////////////////
// CONSTRAINTS
//

static sRelativeConstraint _Constraints[] = 
{
   // sample constraints:
   { kConstrainAfter, &IID_ILoopManager}, 
   { kConstrainBefore, &IID_LoopApp},   // must stuff loopmodes before they are initialized
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



