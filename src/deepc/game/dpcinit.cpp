/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// Deep Cover Initialization.
#include <comtools.h>
#include <aggmemb.h>
#include <appagg.h>
#include <aggmemb.h>

#include <loopapi.h>
#include <dpcloop.h>

#include <dpcinit.h>
#include <loopapp.h>
#include <init.h>

#include <gamemode.h>
#include <basemode.h>

#include <simpldmg.h>
#include <dpccmobj.h>
#include <dpccmapi.h>
#include <dpcpgapi.h>
#include <dpcplayr.h>

#include <engfeat.h>

////////////////////////////////////////////////////////////
// Constants
//

static const GUID* my_guid = &UUID_DEEPC;
        
static char* my_name = "DEEPC";

static int my_priority = kPriorityNormal;          // my_priority


#define MY_CREATEFUNC CreateDeepCoverStuff

EXTERN void LGAPI MY_CREATEFUNC(void);

//
// Aggregate member creation
//

void LGAPI CreateDeepCover(void)
{
   MY_CREATEFUNC();
   DPCPlayerCreate();
   InitSimpleDamageModel();
   DPCCameraCreate();
   
   DPCPlayerGunCreate();
   DPCCameraObjectsCreate();
}
 
////////////////////////////////////////////////////////////
// DEEPC CLIENT FACTORIES 
//

static const sLoopClientDesc* Factories[]=
{
   &DPCSimLoopClientDesc,
   &DPCRenderClientDesc,
   //&DPCRender2ClientDesc,

   NULL
};

static ulong FactoryID;

#define NUM_ELEMS(x) (sizeof(x)/sizeof((x)[0]))

////////////////////////////////////////////////////////////
// DEEPC LOOPMODE DESCS
//

static sLoopModeDesc* Modes[] = 
{
   &DPCGameClients,
   &DPCBaseClients,
};

////////////////////////////////////////////////////////////
// INIT FUNC
//

#pragma off(unreferenced)
static STDMETHODIMP _InitFunc(IUnknown* goof)
{
   ILoopManager* pLoopManager = AppGetObj(ILoopManager);
   ILoopClientFactory* factory = CreateLoopFactory(Factories);

   // Add client factories
   ILoopManager_AddClientFactory(pLoopManager,factory,&FactoryID);
   SafeRelease(factory);

   // Add loopmodes
   for (int i = 0; i < NUM_ELEMS(Modes); i++)
   {
      ILoopManager_AddMode(pLoopManager,Modes[i]);
   }

   // Set up clients
   GameModeSetGameSpecClient(&LOOPID_DPCGameClients);
   BaseModeSetGameSpecClient(&LOOPID_DPCBaseClients);
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
               NullFunc,            // connect
               NullFunc,            // post connect   
               _InitFunc,           // init
               _ShutdownFunc,       // end
               NullFunc);           // disconnect
   add_info.pID                 = my_guid;
   add_info.pszName             = my_name; 
   add_info.pControl            = InitObject._pAggregateControl;
   add_info.controlPriority     = my_priority;
   add_info.pControlConstraints = _Constraints;
   _AddToAggregate(app,&add_info,1);
   SafeRelease(InitObject._pAggregateControl);
   SafeRelease(app);
}
