/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkinit.c,v 1.8 2000/02/19 12:27:16 toml Exp $

#include <comtools.h>
#include <aggmemb.h>
#include <appagg.h>
#include <aggmemb.h>

#include <loopapi.h>
#include <drkloop.h>

#include <drkinit.h>
#include <loopapp.h>
#include <init.h>

#include <gamemode.h>
#include <basemode.h>
#include <engfeat.h>
#include <drkplinv.h>
#include <darkai.h>
#ifdef NEW_NETWORK_ENABLED
#include <drknetap.h>
#endif

#include <simpldmg.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

////////////////////////////////////////////////////////////
// Constants
//

static const GUID* my_guid = &UUID_Dark;
        
static char* my_name = "THE DARK PROJECT";  

static int my_priority = kPriorityNormal;          // my_priority


EXTERN void LGAPI CreateDarkStuff(void);

//
// Aggregate member creation
//


void LGAPI CreateDark(void)
{
   CreateDarkStuff();
   InitSimpleDamageModel();
   InventoryCreate(); 
   DarkAICreateAppServices();
#ifdef NEW_NETWORK_ENABLED
   DarkNetServicesCreate();
#endif
}



////////////////////////////////////////////////////////////
// DARK CLIENT FACTORIES 
//

static const sLoopClientDesc* Factories[]=
{
   &DarkSimLoopClientDesc,
   &DarkRenderClientDesc,

   NULL
};

static ulong FactoryID;

#define NUM_ELEMS(x) (sizeof(x)/sizeof((x)[0]))

////////////////////////////////////////////////////////////
// DARK LOOPMODE DESCS
//

static sLoopModeDesc* Modes[] = 
{
   &DarkGameClients,
   &DarkBaseClients,
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
   GameModeSetGameSpecClient(&LOOPID_DarkGameClients);
   BaseModeSetGameSpecClient(&LOOPID_DarkBaseClients);
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




void LGAPI CreateDarkStuff(void)
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




