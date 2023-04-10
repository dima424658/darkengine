/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/loopapp.c,v 1.48 2000/02/19 13:16:21 toml Exp $

#include <loopapi.h>
#include <aggmemb.h>
#include <appagg.h>

#include <config.h>

#include <loopapp.h>
#include <init.h>
#include <scrnloop.h>
#include <testloop.h>
#include <resloop.h>
#include <testmode.h>
#include <editmode.h>
#include <basemode.h>
#include <uiloop.h>
#include <editgeom.h>
#include <gamemode.h>
#include <biploop.h>
#include <objloop.h>
#include <brloop.h>
#include <ailoop.h>
#include <physloop.H>
#include <plyrloop.h>
#include <simloop.h>
#include <sndloop.h>
#include <rendloop.h>
#include <wrloop.h>
#include <schloop.h>
#include <simtloop.h>
#include <arloop.h>
#include <roomloop.h>
#include <quesloop.h>
#include <antxloop.h>
#include <fvarloop.h>
#include <crwpnlup.h>

#ifdef NEW_NETWORK_ENABLED
#include <netloop.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 
#endif
  
//////////////////////////////////////////////////////////////////
// MASTER LOOP FACTORY FUNCTION LIST
//

static sLoopClientDesc* MasterClientsList[]=
{
   &ScrnManClientDesc,
   &TestLoopClientDesc,
   &ResLoopClientDesc,
   &uiLoopClientDesc,
   &GameLoopClientDesc,
   &BaseLoopClientDesc,
   &BipedLoopClientDesc,
   &ObjSysLoopClientDesc,
   &g_AILoopClientDesc,
   &PhysicsLoopClientDesc,
   &PhysicsBaseClientDesc,
   &PlayerLoopClientDesc,
   &SimLoopClientDesc,
   &SoundLoopClientDesc,
   &RenderLoopClientDesc,
   &RenderBaseLoopClientDesc,
   &WrLoopClientDesc,
   &SchemaLoopClientDesc,
   &SimTimeLoopClientDesc,
   &ActReactLoopClientDesc,
   &RoomLoopClientDesc,
   &QuestLoopClientDesc,
   &AnimTextureLoopClientDesc,
   &FileVarClientDesc, 
   &CreatureWeaponLoopClientDesc,

#ifdef NEW_NETWORK_ENABLED
   &NetworkLoopClientDesc,
   &NetworkResetLoopClientDesc,
   &ObjNetLoopClientDesc,
   &NetworkSimLoopClientDesc,
   &GhostLoopClientDesc,
#endif //NETWORK_ENABLED

#ifdef EDITOR
   &EditGeomClientDesc,
   &EditorLoopClientDesc,
   &BrushListLoopClientDesc,
#endif

   NULL
};


static ulong MasterFactoryID;

////////////////////////////////////////////////////////////
//
// MASTER LOOP MODE LIST
//
//

EXTERN sLoopModeDesc AllClientsMode;

static sLoopModeDesc* MasterDescList[] =
{
   &TestLoopMode,
   &GameLoopMode,
   &BaseLoopMode,

#ifdef EDITOR
   &EditLoopMode,
#endif 


   // keep this last
   &AllClientsMode,
};

#define NUM_LOOPMODE_DESCS (sizeof(MasterDescList)/sizeof(MasterDescList[0]))


////////////////////////////////////////////////////////////
// "ALL CLIENTS" Loopmode
//

//
// Since Modes can include other modes in their client list,
// we'll just build a "mode list" to make our lives easier
//

// subtract one to disclude the "all clients mode"
#define NUM_MODES (sizeof(MasterDescList)/sizeof(MasterDescList[0])-1)


static tLoopClientID* Clients[NUM_MODES];




sLoopModeDesc AllClientsMode =
{
   &LOOPID_AllClients, "All Clients Mode",
   Clients,
   sizeof(Clients)/sizeof(Clients[0])
};

static void build_all_clients(void)
{
   int i;
   for (i = 0; i < NUM_MODES; i++)
   {
      Clients[i] = MasterDescList[i]->name.pID;
   }
}

////////////////////////////////////////

#ifndef SHIP

static void set_loopmode_noise(void)
{
   char buf[12];
   ulong messes;
   unsigned diags;
   // Setup loopmode debuggin (must happen before dispatch)
   if (config_get_raw("dispatch_noise",buf,sizeof(buf)))
   {
      ILoop * pLoop = AppGetObj(ILoop);

      ILoop_GetDiagnostics(pLoop, &diags, &messes);
      sscanf(buf,"%lx",&messes);
      ILoop_SetDiagnostics(pLoop, diags|kLoopDiagTracking, messes);
      ILoop_Release(pLoop);
   }
}

#else
#define set_loopmode_noise()
#endif

//////////////////////////////////////////////////////////////
// INIT FUNC
//


#pragma off(unreferenced)
STDMETHODIMP LoopAppInitFunc(IUnknown* goof)
{
   ILoopClientFactory* factory = CreateLoopFactory(MasterClientsList);
   ILoopManager* loopman = AppGetObj(ILoopManager);
   int i;

#ifndef SHIP
   ILoop* pLoop = AppGetObj(ILoop);
   ulong messes;
   unsigned diags;

   ILoop_GetDiagnostics(pLoop, &diags, &messes);
   if (config_is_defined("looptrack"))
      diags |= kLoopDiagTracking;
   if (config_is_defined("looptime"))
      diags |= kLoopDiagTimings;
   if (config_is_defined("loopchk"))
   {
      int level = 0;
      config_get_int("loopchk", &level);
      if (level == 1)
         diags |= kLoopDiagClientHeapchk;
      else
         diags |= kLoopDiagFrameHeapchk;
   }
   ILoop_SetDiagnostics(pLoop, diags, (ulong)kMsgsAll);
   SafeRelease(pLoop);
#endif

   // Add the factories
   ILoopManager_AddClientFactory(loopman, factory,&MasterFactoryID);
   SafeRelease(factory);

   // build the "all clients" mode
   build_all_clients();

   // Add the loopmodes
   for (i = 0; i < NUM_LOOPMODE_DESCS; i++)
   {
      ILoopManager_AddMode(loopman,MasterDescList[i]);
   }

   ILoopManager_SetBaseMode(loopman,&LOOPID_BaseMode);

   set_loopmode_noise();

   SafeRelease(loopman);

   return kNoError;
}
#pragma on(unreferenced)

//////////////////////////////////////////////////////////////
// SHUTDOWN FUNC
//


#pragma off(unreferenced)
STDMETHODIMP LoopAppShutdownFunc(IUnknown* goof)
{
   ILoopManager* loopman = AppGetObj(ILoopManager);
   ILoopManager_RemoveClientFactory(loopman,MasterFactoryID);

   SafeRelease(loopman);
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
// LoopAppSysCreate()
//
// Called during AppCreateObjects, adds the LoopApp system anonymously to the
// app aggregate
//

static struct _init_object
{
   DECLARE_C_COMPLEX_AGGREGATE_CONTROL();
} InitObject;


static sRelativeConstraint LoopAppConstraints[] =
{
   { kConstrainAfter, &IID_ILoopManager},
   { kNullConstraint, }
};


void LGAPI LoopAppCreate(void)
{
   sAggAddInfo add_info = { NULL, NULL, NULL, NULL, 0, NULL} ;
   IUnknown* app = AppGetObj(IUnknown);
   INIT_C_COMPLEX_AGGREGATE_CONTROL(InitObject,
               FinalReleaseFunc,    // on final release
               NullFunc,      // connect
               NullFunc,      // post connect
               LoopAppInitFunc,     // init
               LoopAppShutdownFunc,    // end
               NullFunc);     // disconnect
   add_info.pID = &IID_LoopApp;
   add_info.pszName = "LoopApp";
   add_info.pControl = InitObject._pAggregateControl;
   add_info.controlPriority = kPriorityNormal;
   add_info.pControlConstraints = LoopAppConstraints;
   _AddToAggregate(app,&add_info,1);
   SafeRelease(InitObject._pAggregateControl);
   SafeRelease(app);
}

