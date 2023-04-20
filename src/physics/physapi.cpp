/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/physics/physapi.cpp,v 1.179 2000/03/20 09:44:15 adurant Exp $
//
// Public physics functions
//
//
// Notes:
//  - All axis-aligned operations are based on the object's orientation
//  - All functions in this file are C-callable

#include <math.h>

#include <lg.h>
#include <command.h>
#include <config.h>
#include <rand.h>

#include <autolink.h>
#include <objnotif.h>
#include <objpos.h>
#include <objshape.h>  // ObjGetRadius() && ObjGetUnscaledDims()
#include <objscale.h>
#include <objedit.h>   // ObjWarnName()
#include <objarray.h>
#include <propbase.h>
#include <collprop.h>  // ObjSetCollisionType()
#include <portal.h>    // CellFromLoc()
#include <medmotn.h>   // g_aMedMoCellMotion
#include <sphrcst.h>   // SphrSphereInWorld()
#include <playrobj.h>
#include <creatext.h>
#include <cretprop.h>
#include <textarch.h>
#include <mnamprop.h>  // ObjSetModelName()
#include <mnumprop.h>  // ObjLoadModel()
#include <doorphys.h>  // for IsDoor()
#include <phmedtr.h>
#include <media.h>
#include <plyrmode.h>
#include <plyrmov.h>
#include <rooms.h>
#include <roomprop.h>
#include <sphrcst.h>
#include <sphrcsts.h>
#include <doorprop.h>

#include <appagg.h>
#include <comtools.h>
#include <property.h>
#include <propman.h>
#include <posprop.h>
#include <osysbase.h>

#include <subscrip.h>

#include <physapi.h>
#include <phclimb.h>
#include <phconst.h>
#include <phcore.h>
#include <phmods.h>
#include <phmod.h>
#include <phmodsph.h>
#include <phmodobb.h>
#include <phmodutl.h>
#include <phmsg.h>
#include <phdyn.h>
#include <phctrl.h>
#include <phprop.h>
#include <phoprop.h>
#include <phmtprop.h>
#include <phmterr.h>
#include <phutils.h>
#include <phref.h>
#include <phreport.h>
#include <phtxtprp.h>
#include <phnet.h>
#include <phlistn_.h>

#include <phmoapi.h>

#include <phdbg.h>
#include <phystest.h>

#include <netman.h>
#include <iobjnet.h>

#include <mprintf.h>

// must be last header
#include <dbmem.h>

static INetManager *g_pNetMan;
static IObjectNetworking *g_pObjNet;

static cPhysModel    *g_pModel;

// note fullwise, moved this to be in ship builds so that we can call it on dbReset for real
// Xemu 6/19/99
void PhysResetListeners(void)
{
   g_pPhysListeners->Reset();
}

#ifndef SHIP

void LocationControlOBBs()
{
   cPhysModel *pModel;

   pModel = g_PhysModels.GetFirstActive();
   while (pModel != NULL)
   {
      if (pModel->GetType(0) == kPMT_OBB)
      {
         pModel->GetControls()->ControlLocation(pModel->GetLocationVec());
         pModel->GetControls()->ControlRotation(pModel->GetRotation());
      }
    
      pModel = g_PhysModels.GetNextActive(pModel);
   }
}

void InitOBBDimsObj(ObjID objID)
{
   cPhysModel *pModel;

   pModel = g_PhysModels.GetActive(objID);

   if (!pModel || (pModel->GetType(0) != kPMT_OBB))
      return;

   mxs_vector size, scale;
   
   ObjGetUnscaledDims(objID, &size);
   if (ObjGetScale(objID, &scale))
   {
      size.x *= scale.x;
      size.y *= scale.y;
      size.z *= scale.z;
   }
  
   cPhysDimsProp *pDimsProp;

   if (!g_pPhysDimsProp->Get(objID, &pDimsProp))
   {
      g_pPhysDimsProp->Create(objID);
      g_pPhysDimsProp->Get(objID, &pDimsProp);
   }

   pDimsProp->size = size;

   g_pPhysDimsProp->Set(objID, pDimsProp);
   UpdatePhysModel(objID, PHYS_DIMS);
}

void InitOBBDims(ObjID objID)
{
   if (objID == -1)
   {
      cPhysModel *pModel;

      pModel = g_PhysModels.GetFirstActive();
      while (pModel)
      {
         if (pModel->GetType(0) == kPMT_OBB)
            InitOBBDimsObj(pModel->GetObjID());

         pModel = g_PhysModels.GetNextActive(pModel);
      }
   }
   else
      InitOBBDimsObj(objID);
}

void InitSphereHatDimsObj(ObjID objID)
{
   cPhysModel *pModel;

   pModel = g_PhysModels.GetActive(objID);

   if (!pModel || (pModel->NumSubModels() < 2) || (pModel->GetType(1) != kPMT_SphereHat))
      return;

   mxs_vector size, scale;
   
   ObjGetUnscaledDims(objID, &size);
   if (ObjGetScale(objID, &scale))
   {
      size.x *= scale.x;
      size.y *= scale.y;
      size.z *= scale.z;
   }
  
   cPhysDimsProp *pDimsProp;

   if (!g_pPhysDimsProp->Get(objID, &pDimsProp))
   {
      g_pPhysDimsProp->Create(objID);
      g_pPhysDimsProp->Get(objID, &pDimsProp);
   }

   pDimsProp->radius[0] = size.z / 2;

   g_pPhysDimsProp->Set(objID, pDimsProp);
   UpdatePhysModel(objID, PHYS_DIMS);
}

void InitSphereHatDims(ObjID objID)
{
   if (objID == -1)
   {
      cPhysModel *pModel;

      pModel = g_PhysModels.GetFirstActive();
      while (pModel)
      {
         if ((pModel->NumSubModels() == 2) && (pModel->GetType(1) == kPMT_SphereHat))
            InitSphereHatDimsObj(pModel->GetObjID());

         pModel = g_PhysModels.GetNextActive(pModel);
      }
   }
   else
      InitSphereHatDimsObj(objID);
}

void SpewOutOfWorld()
{
   cPhysModel *pModel;

   mprintf("Objects out of world:\n");

   pModel = g_PhysModels.GetFirstActive();
   while (pModel != NULL)
   {
      if (!PhysObjValidPos(pModel->GetObjID(), NULL) &&
          !(pModel->IsRotationControlled() && pModel->IsLocationControlled()))
         mprintf("  %s\n", ObjWarnName(pModel->GetObjID()));

      pModel = g_PhysModels.GetNextActive(pModel);
   }

   pModel = g_PhysModels.GetFirstInactive();
   while (pModel != NULL)
   {
      if (!PhysObjValidPos(pModel->GetObjID(), NULL) &&
          !(pModel->IsRotationControlled() && pModel->IsLocationControlled()))
         mprintf("  %s\n", ObjWarnName(pModel->GetObjID()));

      pModel = g_PhysModels.GetNextInactive(pModel);
   }
}

void CheckDoors()
{
   cPhysModel *pModel;
   BOOL ok = TRUE;

   for (int i=0; i<gMaxObjID; i++)
   {
      if ((pModel = g_PhysModels.Get(i)) != NULL)
      {
         if (pModel->IsDoor() && !g_pRotDoorProperty->IsRelevant(i) && !g_pTransDoorProperty->IsRelevant(i))
         {
            ok = FALSE;
            mprintf("obj %d door disagrees!\n", i);
         }
      }

      if (g_pRotDoorProperty->IsRelevant(i) || g_pTransDoorProperty->IsRelevant(i))
      {
         if ((pModel = g_PhysModels.Get(i)) == NULL)
         {
            ok = FALSE;
            mprintf("door obj %d has no physics!\n", i);
         }
         else
         {
            if (!pModel->IsDoor())
            {
               ok = FALSE;
               mprintf("door obj %d has physics, but isn't a door!\n", i);
            }
         }
      }
   }

   if (ok)
      mprintf("Looks good, boss.\n");
}

void CheckMovingTerrain()
{
   cPhysModel *pModel;
   BOOL ok = TRUE;

   for (int i=0; i<gMaxObjID; i++)
   {
      if ((pModel = g_PhysModels.Get(i)) != NULL)
      {
         if (pModel->IsMovingTerrain() && !g_pMovingTerrainProperty->IsRelevant(i))
         {
            ok = FALSE;
            mprintf("obj %d moving terrain disagrees!\n", i);
         }
      }

      if (g_pMovingTerrainProperty->IsRelevant(i))
      {
         if ((pModel = g_PhysModels.Get(i)) == NULL)
         {
            ok = FALSE;
            mprintf("moving terrain obj %d has no physics!\n", i);
         }
         else
         {
            if (!pModel->IsMovingTerrain())
            {  
               ok = FALSE;
               mprintf("moving terrain obj %d has physics, but isn't moving terrain!\n", i);
            }
         }
      }
   }

   if (ok)
      mprintf("Looks good, boss.\n");
}

void SpewAwakeObjects()
{
   cPhysModel *pModel = g_PhysModels.GetFirstMoving();

   mprintf("Active models:\n");
   while (pModel)
   {
      mprintf("  %s\n", ObjWarnName(pModel->GetObjID()));
      pModel = g_PhysModels.GetNextMoving(pModel);
   }
}

void SpewAICollidesObjects()
{
   cPhysModel *pModel = g_PhysModels.GetFirstActive();

   mprintf("AI Collision models:\n");
   while (pModel)
   {
      if (pModel->AICollides())
         mprintf("  %s\n", ObjWarnName(pModel->GetObjID()));
      pModel = g_PhysModels.GetNextActive(pModel);
   }
}

void SpewStats()
{
   g_PhysModels.SpewStats();
}

void ExplodeMe(mxs_real magnitude, mxs_real radius_squared)
{
   PhysExplode(&ObjPosGet(PlayerObject())->loc.vec, magnitude, radius_squared);
}

void RopeHit(ObjID objID)
{
   float mag = ((float)RandRange(0, 40));

   PhysHitRope(objID, mag);
}

void LockAttachments()
{
   cAutoLinkQuery query(g_pPhysAttachRelation);

   for (; !query->Done(); query->Next())
   {
      sLink link;
      sPhysAttachData *data = (sPhysAttachData *)query->Data();
      query->Link(&link);

      mx_sub_vec(&data->offset, &ObjPosGet(link.source)->loc.vec, &ObjPosGet(link.dest)->loc.vec);

      cPhysModel *pAttach = g_PhysModels.Get(link.source);

      if (pAttach)
         pAttach->GetControls()->StopControlLocation();
   }
}

void AutoJiggle()
{
   cPhysModel *pModel;

   pModel = g_PhysModels.GetFirstActive();
   while (pModel)
   {
      ObjTranslate(pModel->GetObjID(), &ObjPosGet(pModel->GetObjID())->loc.vec);
      pModel = g_PhysModels.GetNextActive(pModel);
   }
}

void PhysDup()
{
   cPhysModel *pCurModel;
   cPhysModel *pModel;
   int i;

   cDynArray<ObjID> dupList;

   for (pCurModel = g_PhysModels.GetFirstActive();
        pCurModel != NULL;
        pCurModel = g_PhysModels.GetNextActive(pCurModel))
   {
      ObjID curObj = pCurModel->GetObjID();
      int   objCount = 0;

      // skip objects we've already spewed about dups
      BOOL skip = FALSE;
      for (i=0; i<dupList.Size(); i++)
      {
         if (curObj == dupList[i])
         {
            skip = TRUE;
            break;
         }
      }
      
      if (skip)
         continue;

      // scan for matching objids
      for (pModel = g_PhysModels.GetFirstActive();
           pModel != NULL;
           pModel = g_PhysModels.GetNextActive(pModel))
      {
         if (pModel == pCurModel)
            continue;

         if (pModel->GetObjID() == curObj)
         {
            if (objCount == 0)
               dupList.Append(curObj);

            objCount++;
         }
      }

      if (objCount > 0)
         mprintf("%s has %d duplicates\n", ObjEditName(curObj), objCount);
   }

   for (i=0; i<dupList.Size(); i++)
   {
      // Get the "real" physics for this objID
      pCurModel = g_PhysModels.Get(dupList[i]);

      // We restart the iteration from the beginning every time we remove
      // a model from the list, so we don't run into any delete-while-iterating
      // problems.
      BOOL done_looping = FALSE;
      while (!done_looping)
      {
         done_looping = TRUE;

         for (pModel = g_PhysModels.GetFirstActive();
              pModel != NULL;
              pModel = g_PhysModels.GetNextActive(pModel))
         {
            if ((pModel != pCurModel) && (dupList[i] == pModel->GetObjID()))
            {
               // this is a fake one that needs to be removed
               g_PhysModels.HackListRemove(pModel);
               done_looping = FALSE;
               break;
            }
         }
      }
   }
}

extern BOOL g_PhysicsOn;

extern void PhysClearTimers();

static Command g_PhysicsCommands[] =
{
   {"physics",           TOGGLE_BOOL, &g_PhysicsOn,             "Activate/Deactivate physics", HK_ALL},

   {"start_control",     FUNC_FLOAT, PhysicsSetControl,         "Start obj 3 under ctrl",     HK_ALL},
   {"stop_control",      FUNC_VOID,  PhysicsStopControl,        "Stop control of obj 3",      HK_ALL},
   {"launch_sphere",     FUNC_FLOAT, PhysicsLaunchSphere,       "Launch a sphere object",     HK_ALL},
   {"create_sphere",     FUNC_INT,   PhysRegisterSphereDefault, "Create a sphere model",      HK_ALL},
   {"launch_obb",        FUNC_FLOAT, PhysicsLaunchOBB,          "Launch a obb object",        HK_ALL},
   {"create_obb",        FUNC_INT,   PhysRegisterOBBDefault,    "Create a obb model",         HK_ALL},

   {"reset_moving_terrain", FUNC_VOID, ResetMovingTerrain,      "Reset all moving terrain",   HK_ALL},
   {"lock_attachments",     FUNC_VOID, LockAttachments,         "Lock all attachments in place", HK_ALL},
   {"auto_jiggle",       FUNC_VOID,  AutoJiggle,                "Touch ObjPos of all phys objs", HK_ALL}, 
   {"phys_dup",          FUNC_VOID,  PhysDup,                   "Check for duplicate physics",   HK_ALL},

   {"phys_spew_info",    FUNC_INT,   PhysSpewInfo,              "Spew Info on an object",     HK_ALL},
   {"phys_spew_player",  FUNC_VOID,  PhysSpewPlayer,            "Spew Info on the player",    HK_ALL},

   {"hit_rope",          FUNC_INT,   RopeHit,                   "Hit a rope",                 HK_ALL},

   {"phys_joyride",      FUNC_VOID,  PhysicsJoyride,            "",                           HK_ALL},
   {"phys_raycast",      FUNC_INT,   PhysicsTestRaycast,        "",                           HK_ALL},
   {"explode_me",        FUNC_FLOAT, ExplodeMe,                 "BOOM",                       HK_ALL},

   {"loc_control_obbs",  FUNC_VOID,  LocationControlOBBs,       "Location control all OBBs",  HK_ALL},
   {"spew_out_of_world", FUNC_VOID,  SpewOutOfWorld,            "Spew all objs out of world", HK_ALL},
   {"spew_awake_objects", FUNC_VOID, SpewAwakeObjects,          "Spew all awake objects",     HK_ALL},
   {"spew_ai_collides",  FUNC_VOID,  SpewAICollidesObjects,     "Spew all objects AI collide with", HK_ALL},

   {"init_obb_dims",     FUNC_INT,   InitOBBDims,               "Init the dims of obb(s)",    HK_ALL},
   {"init_spherehat_dims", FUNC_INT, InitSphereHatDims,         "Init the dims of sphere hat(s)", HK_ALL},

   {"clear_phys_timers", FUNC_VOID,  PhysClearTimers,           "Clear profiling timers", HK_ALL},

   {"check_doors",       FUNC_VOID,  CheckDoors,                "Make sure door state is in sync", HK_ALL},
   {"check_mterr",       FUNC_VOID,  CheckMovingTerrain,        "Make sure mterr state is in sync", HK_ALL},

   {"phys_stats",        FUNC_VOID,  SpewStats,                 "Spew memory usage stats", HK_ALL},

   {"phys_reset_listeners", FUNC_VOID,  PhysResetListeners,     "Reset listener subscriptions", HK_ALL},
};
#endif


void LockAttachmentObj(ObjID objID)
{
   cAutoLinkQuery query(g_pPhysAttachRelation, objID);

   if (!query->Done())
   {
      sLink link;
      sPhysAttachData *data = (sPhysAttachData *)query->Data();
      query->Link(&link);

      mx_sub_vec(&data->offset, &ObjPosGet(link.source)->loc.vec, &ObjPosGet(link.dest)->loc.vec);

      cPhysModel *pAttach = g_PhysModels.Get(link.source);

      if (pAttach)
         pAttach->GetControls()->StopControlLocation();
   }
}

extern void RemoveObjectCollisions(ObjID objID);

//
// Checks for moveable model, provide warnings, and sets up
// pModel for the caller
//
static BOOL CheckMoveable(ObjID objID, const char *func_name)
{
   if ((g_pModel = g_PhysModels.Get(objID)) == NULL)
   {
      Warning(("%s: obj %s has no physics model\n", func_name, ObjWarnName(objID)));
      return FALSE;
   }
   if (!g_pModel->IsMoveable())
   {
      Warning(("%s: obj %s is not moveable\n", func_name, ObjWarnName(objID)));
      return FALSE;
   }

   return TRUE;
}

///////////////////////////////////////////////////////////////////////////////.

#ifndef SHIP

static cObjArray<bool,kOAF_Static|kOAF_NoAbstracts> gObjAlreadyWarned;
static BOOL connect_once = FALSE; 

void ResetObjectWarnings()
{
   if (!connect_once)
      gObjAlreadyWarned.AutoConnect(); 

   connect_once = TRUE; 

   gObjAlreadyWarned.Clear(); 

}

void SetObjectWarned(ObjID objID)
{
   gObjAlreadyWarned[objID] = TRUE;
}

BOOL HasObjectWarned(ObjID objID)
{
   if (config_is_defined("phys_always_warn"))
      return FALSE;

   return gObjAlreadyWarned[objID];
}

#endif

///////////////////////////////////////////////////////////////////////////////.

EXTERN BOOL g_SimRunning;
BOOL PosPropLock = 0;

static void LGAPI PosPropListener(sPropertyListenMsg* msg, PropListenerData data)
{
   if (PosPropLock || OBJ_IS_ABSTRACT(msg->obj))
      return;

   cPhysModel *pModel;

   if ((pModel = g_PhysModels.Get(msg->obj)) == NULL)
      return;

   Position *pos = (Position *)msg->value.ptrval;

   if ((msg->type & (kListenPropSet|kListenPropModify)) && !(msg->type & kListenPropLoad))
   {
      PosPropLock++;
      
      if (pModel->IsCreature())
      {
         if (!CreatureSelfPropelled(msg->obj)) // I'm aware this sucks but there's no good way around it
         {
            PhysSetModRotation(msg->obj, &pos->fac);
            PhysSetModLocation(msg->obj, &pos->loc.vec);
         }
      }
      else
      {
         PhysSetModRotation(msg->obj, &pos->fac);
         PhysSetModLocation(msg->obj, &pos->loc.vec);
      }

      if (!(msg->type & kListenPropLoad))
      {
         if (pModel->IsRope())
            DeployRope(msg->obj);
      }

      PosPropLock--;
   }

   PhysUpdateRefsObj(msg->obj);
}

void UpdateObjRoomGravity(ObjID objID, cRoom *enterRoom, cRoom *exitRoom)
{
   cPhysModel *pModel = g_PhysModels.Get(objID);

   if (pModel == NULL)
      return;

   pModel->SetGravity(pModel->GetGravity() / GetRoomGravity(exitRoom->GetObjID()));
   pModel->SetGravity(pModel->GetGravity() * GetRoomGravity(enterRoom->GetObjID()));
}

// Determine whether this object needs to broadcast changes to its control velocities.
// This is where we put in cutoffs for specially networked systems like doors and players.
static BOOL ShouldBroadcastControlFor(ObjID objID)
{
   return g_pObjNet->ObjHostedHere(objID) && !IsDoor(objID) && objID != PlayerObject();
}


//////////////////////////////////////////////////////////////////////////////
//
// Initialization, I/O, and Cleanup
//
//////////////////////////////////////////////////////////////////////////////

extern int sim_frames;
extern int phys_frames;

extern int  inworld_checks;
extern BOOL net_cap_physics;

//
// Init the system
//
void PhysInit()
{
   AutoAppIPtr_(PropertyManager, pPropMan);

   g_pNetMan = AppGetObj(INetManager);
   g_pObjNet = AppGetObj(IObjectNetworking);

#ifdef SHIP
   net_cap_physics = TRUE;
#else
   net_cap_physics = !config_is_defined("phys_no_net_cap");
#endif

   sim_frames = 0;
   phys_frames = 0;

   IProperty *PosProp = pPropMan->GetPropertyNamed(PROP_POSITION_NAME);
   PosProp->Listen(kListenPropSet | kListenPropModify | kListenPropLoad, PosPropListener, NULL);
   SafeRelease(PosProp);

   // Set up friction
   kFrictionFactor = 0.03;
   config_get_float("friction", &kFrictionFactor);

   kMaxFrameLen = 80;
   config_get_int("phys_max_frame_len", &kMaxFrameLen);

   mx_mk_vec(&kGravityDir, 0, 0, -1);
   mx_normeq_vec(&kGravityDir);
   kGravityAmt = 32.0;

   // some constants are more constant than others.
   if (config_is_defined("springcap"))
   {	  
	   float UserMag = kSpringCapMag;	  
	   config_get_float("max_spring_vel",&UserMag);	  
	   if (UserMag>0.0)		
		   kSpringCapMag = UserMag;
   }


   inworld_checks = !config_is_defined("no_check_in_world");

   InitPhysReports();

   if (g_pRooms)
      g_pRooms->AddTransitionCallback(UpdateObjRoomGravity);

   // Initialize properties
   InitPhysicsProperties();
   InitMovingTerrainProps();
   PhysTexturePropsInit(); 
   PhysExplodePropInit();
   PhysInitVelPropInit();
   PhysFaceVelPropInit();
   PhysRopePropInit();
   PhysPPlatePropInit();
   PhysAICollidePropInit();
   PhysMediaTransitionsInit(); 

   // Initialize relations
   InitTerrainPathLinks();
   PhysAttachRelationInit();

   // Set up networking
   PhysNetInit();

   ResetObjectWarnings();

   Assert_(pPhysSubscribeService == NULL);
   pPhysSubscribeService = new cSubscribeService(3);

   if (g_pPhysListeners == NULL)
      g_pPhysListeners = new cPhysListeners();

#ifndef SHIP
   COMMANDS(g_PhysicsCommands, HK_ALL);
#endif
}

//
// Clean up the system
//
void PhysTerm()
{
   g_PhysModels.DestroyAll();

   TermPhysReports();

   if (g_pRooms)
      g_pRooms->RemoveTransitionCallback(UpdateObjRoomGravity);

   // Terminate properties
   TermPhysicsProperties();
   TermMovingTerrainProps();
   PhysTexturePropsTerm(); 
   PhysExplodePropTerm();
   PhysInitVelPropTerm();
   PhysFaceVelPropTerm();
   PhysRopePropTerm();
   PhysPPlatePropTerm();
   PhysAICollidePropTerm();

   // Terminate relations
   PhysAttachRelationTerm();

   PhysNetTerm();

   PhysMediaTransitionsTerm(); 

   Assert_(pPhysSubscribeService != NULL);
   delete pPhysSubscribeService;

   if (g_pPhysListeners != NULL)
   {
      delete g_pPhysListeners;
      g_pPhysListeners = NULL;
   }

   #ifndef SHIP
   if (config_is_defined("phys_max_frame_len") && (sim_frames > 0))
      mprintf("%g average physics frames per sim frame\n", (float) phys_frames / sim_frames);
   #endif
}

BOOL HeadLimitFunc(ObjID objID);

void PhysPostLoad()
{
   if (g_pRooms)
   {
      g_pRooms->RemoveTransitionCallback(UpdateObjRoomGravity);
      g_pRooms->AddTransitionCallback(UpdateObjRoomGravity);
   }

   cPhysModel *pModel;

   if (PlayerObjectExists())
   {
      pModel = g_PhysModels.Get(PlayerObject());

      if (pModel != NULL)
      {
         // Set angle limits
         pModel->AddAngleLimit(1, 90, HeadLimitFunc);
         pModel->AddAngleLimit(1, 270, HeadLimitFunc);
         pModel->AddAngleLimit(0, 90, HeadLimitFunc);
         pModel->AddAngleLimit(0, 270, HeadLimitFunc);
      }

      AutoAppIPtr_(ObjectSystem, pObjSys);
      pModel->SetClimbingObj(pObjSys->RemapOnLoad(pModel->GetClimbingObj()));
   }

   mxs_vector offset;
   mx_zero_vec(&offset);

   while ((pModel = g_PhysModels.GetFirstInactive()) != NULL)
   {
      pModel->Activate();

      for (int i=0; i<pModel->NumSubModels(); i++)
         pModel->SetSubModOffset(i, offset);

      PhysSetModLocation(pModel->GetObjID(), &ObjPosGet(pModel->GetObjID())->loc.vec);

      UpdatePhysProperty(pModel->GetObjID(), PHYS_STATE | PHYS_DIMS);
   }

   pModel = g_PhysModels.GetFirstActive();
   while (pModel != NULL)
   {
      if (pModel->IsRope())
         DeployRope(pModel->GetObjID());

      pModel = g_PhysModels.GetNextActive(pModel);
   }

   ResetObjectWarnings();
}

//
// Load physics from disk
//
void PhysRead(PhysReadWrite func, eObjPartition part)
{
   func(&g_PhysVersion, sizeof(int), 1);

   if (g_PhysVersion < 5)
   {
      AssertMsg(FALSE, "Attempt to load outdated physics data.  Ignoring.");
      g_PhysVersion = g_PhysCurrentVersion;
      return;
   }

   if (g_PhysVersion != g_PhysCurrentVersion)
      Warning(("Internal Physics Version: %d, loading: %d.\n", g_PhysCurrentVersion, g_PhysVersion));

   g_PhysModels.Read(func,part); 

   if (g_PhysVersion >= 27)
      g_PhysContactLinks.Read(func);

   if (g_PhysVersion >= 17)
   {
      pPhysSubscribeService->Reset();
      pPhysSubscribeService->Read(func);
   }

   /*
   if (g_PhysVersion >= 30)
      g_pPhysListeners->Read(func);
   else
      g_pPhysListeners->Reset();
   */

   g_PhysVersion = g_PhysCurrentVersion;
}

//
// Save physics to disk
//
void PhysWrite(PhysReadWrite func, eObjPartition part)
{
   func(&g_PhysVersion, sizeof(int), 1);
   g_PhysModels.Write(func,part);

   // Don't save contact info in the briefcase
   if (part == (kObjPartConcrete | kObjPartBriefcase))
      g_PhysContactLinks.WriteDone(func);
   else
   {
      if (g_PhysVersion >= 27)
         g_PhysContactLinks.Write(func);
   }

   pPhysSubscribeService->Write(func);
   // Physics listeners are currently not saved because none of the clients
   // require it. It should be safe to re-enable this if necessary.
//   g_pPhysListeners->Write(func);
}

//
// Update properties of all physics objects
//
void PhysUpdateProperties()
{
   cPhysModel *pModel;
   cPhysModel *pModelNext;
   cPhysTypeProp *pTypeProp;

   extern BOOL PhysListenerLock;

   PhysListenerLock = TRUE;

   pModel = g_PhysModels.GetFirstActive();
   while (pModel != NULL)
   {
      pModelNext = g_PhysModels.GetNextActive(pModel);

      if (!g_pPhysTypeProp->Get(pModel->GetObjID(), &pTypeProp))
      {
         g_pPhysTypeProp->Create(pModel->GetObjID());
         g_pPhysTypeProp->Get(pModel->GetObjID(), &pTypeProp);

         switch (pModel->GetType(0))
         {
            case kPMT_OBB:       pTypeProp->type = kOBBProp;  break;
            case kPMT_Sphere:
            case kPMT_Point:     pTypeProp->type = kSphereProp; break;
            case kPMT_SphereHat: pTypeProp->type = kSphereHatProp; break;
         }

         pTypeProp->num_submodels = pModel->NumSubModels();
      
         g_pPhysTypeProp->Set(pModel->GetObjID(), pTypeProp);
      }

      UpdatePhysProperty(pModel->GetObjID());

      pModel = pModelNext;
   }

   PhysListenerLock = FALSE;
}

//////////////////////////////////////////////////////////////////////////////
//
// Public Get/Set Model and Submodel Locations and Rotations
//
//////////////////////////////////////////////////////////////////////////////

//
// Set a model's location (preserves submodel offsets)
//
void PhysSetModLocation(ObjID objID, mxs_vector *loc)
{
   mxs_vector submod_loc;
   mxs_matrix obj_rotation;
   mxs_vector rot_submod_loc;

   if (!CheckMoveable(objID, "PhysSetModLocation"))
      return;

   if (g_pModel->IsPlayer())
      g_pModel->SetLocationVec(*loc);
   else
      g_pModel->SetLocationVec(*loc, TRUE);

   mx_ang2mat(&obj_rotation, &g_pModel->GetRotation());

   for (int i=0; i<g_pModel->NumSubModels(); i++)
   {
      mx_copy_vec(&submod_loc, &g_pModel->GetSubModOffset(i));
      if (g_pModel->IsPlayer())
      {
         mxs_vector mo_offset;
         PlayerMotionGetOffset((short)i, &mo_offset);
         mx_addeq_vec(&submod_loc, &mo_offset);
      }
      mx_mat_mul_vec(&rot_submod_loc, &obj_rotation, &submod_loc);
      mx_addeq_vec(&rot_submod_loc, loc); 
      g_pModel->SetLocationVec(i, rot_submod_loc);
   }

   UpdatePhysProperty(objID, PHYS_STATE);
}

//
// Set a submodel's location in world coordinates
//
void PhysSetSubModLocation(ObjID objID, tPhysSubModId subModId, mxs_vector *loc)
{
   if (!CheckMoveable(objID, "PhysSetSubModLocation"))
      return;

   if (!EqualVectors(*loc, g_pModel->GetLocationVec(subModId)))
   {
      g_pModel->SetLocationVec(subModId, *loc);

      UpdatePhysProperty(objID, PHYS_DIMS);
   }
}

//
// Set a submodel's location relative to its current location
//
void PhysSetSubModRelLocation(ObjID objID, tPhysSubModId subModId, mxs_vector *offset)
{
   if (!CheckMoveable(objID, "PhysSetSubModRelLocation"))
      return;

   mxs_vector loc;

   mx_copy_vec(&loc, (mxs_vector *)&g_pModel->GetLocationVec(subModId));
   mx_addeq_vec(&loc, offset);

   g_pModel->SetLocationVec(subModId, loc);
   g_pModel->SetSubModOffset(subModId, *offset);

   UpdatePhysProperty(objID, PHYS_DIMS);
}

//
// Get a model's location
//
void PhysGetModLocation(ObjID objID, mxs_vector *loc)
{
   if (!CheckMoveable(objID, "PhysGetModLocation"))
   {
      mx_zero_vec(loc);
      return;
   }

   mx_copy_vec(loc, (mxs_vector *) &g_pModel->GetLocationVec());
}

//
// Get a submodel's location
//
void PhysGetSubModLocation(ObjID objID, tPhysSubModId subModId, mxs_vector *loc)
{
   if (!CheckMoveable(objID, "PhysGetSubModLocation"))
   {
      mx_zero_vec(loc);
      return;
   }

   mx_copy_vec(loc, (mxs_vector *) &g_pModel->GetLocationVec(subModId));
}

//
// Set a model's rotation
//
void PhysSetModRotation(ObjID objID, mxs_angvec *ang)
{
   if (!CheckMoveable(objID, "PhysSetModRotation"))
      return;

   mxs_angvec cur_ang = g_pModel->GetRotation();
   if ((cur_ang.tx != ang->tx) || (cur_ang.ty != ang->ty) || (cur_ang.tz != ang->tz))
   {
      if (g_pModel->IsPlayer())
         g_pModel->SetRotation(*ang);
      else
         g_pModel->SetRotation(*ang, TRUE);

      UpdatePhysProperty(objID, PHYS_STATE);
   }
}

//
// Get a model's rotation
//
void PhysGetModRotation(ObjID objID, mxs_angvec *ang)
{
   if (!CheckMoveable(objID, "PhysGetModRotation"))
   {
      mx_mk_angvec(ang, 0, 0, 0);
      return;
   }

   mx_mk_angvec(ang, g_pModel->GetRotation().tx,
                     g_pModel->GetRotation().ty,
                     g_pModel->GetRotation().tz);
}

// 
// Set a submodel's rotation
//
void PhysSetSubModRotation(ObjID objID, tPhysSubModId subModId, mxs_angvec *ang)
{
   if (!CheckMoveable(objID, "PhysSetSubModRotation"))
      return;

   mxs_angvec cur_ang = g_pModel->GetRotation(subModId);
   if ((cur_ang.tx != ang->tx) || (cur_ang.ty != ang->ty) || (cur_ang.tz != ang->tz))
   {
      g_pModel->SetRotation(subModId, *ang);

      UpdatePhysProperty(objID, PHYS_DIMS);
   }
}

//
// Get a submodel's rotation
//
void PhysGetSubModRotation(ObjID objID, tPhysSubModId subModId, mxs_angvec *ang)
{
   if (!CheckMoveable(objID, "PhysGetSubModRotation"))
   {
      mx_mk_angvec(ang, 0, 0, 0);
      return;
   }

   mx_mk_angvec(ang, g_pModel->GetRotation(subModId).tx,   
                     g_pModel->GetRotation(subModId).ty,    
                     g_pModel->GetRotation(subModId).tz);
}

//////////////////////////////////////////////////////////////////////////////
//
// Public Velocity/Speed functions
//
//////////////////////////////////////////////////////////////////////////////

//
// Set velocity along a specified axis
//
void PhysAxisSetVelocity(ObjID objID, int axis, mxs_real speed)
{
   mxs_matrix mat;
   mxs_vector velocity;

   if (!CheckMoveable(objID, "PhysAxisSetVelocity"))
      return;

   if (g_pModel->IsLocationControlled())
      return;

   mx_ang2mat(&mat, (mxs_angvec *) &g_pModel->GetRotation(0));
   mx_scale_vec(&velocity, &mat.vec[axis], speed);

   cPhysDynData *pDyn = g_pModel->GetDynamics();
   pDyn->SetVelocity(velocity);

   UpdatePhysProperty(objID, PHYS_STATE);
}

//
// Set velocity along a specified vecotor (magnitude = speed)
//
void PhysSetVelocity(ObjID objID, mxs_vector *velocity)
{
   if (!CheckMoveable(objID, "PhysSetVelocity"))
      return;

   if (g_pModel->IsLocationControlled())
      return;

   cPhysDynData *pDyn = g_pModel->GetDynamics();
   pDyn->SetVelocity(*velocity);

   for (int i=0; i<g_pModel->NumSubModels(); i++)
   {
      pDyn = g_pModel->GetDynamics(i);
      pDyn->SetVelocity(*velocity);
   }

   UpdatePhysProperty(objID, PHYS_STATE);
}

//
// Get velocity (all submodels must have same velocities)
//
void PhysGetVelocity(ObjID objID, mxs_vector *velocity)
{
   if (!CheckMoveable(objID, "PhysGetVelocity"))
   {
      mx_zero_vec(velocity);
      return;
   }

   mx_copy_vec(velocity, &g_pModel->GetVelocity());
}

//
// Set rotational velocity (x, y, z of ang_velocity are the rotations
// around those axes in rad/s)
//
void PhysSetRotationalVelocity(ObjID objID, mxs_vector *ang_velocity)
{
   if (!CheckMoveable(objID, "PhysSetRotationalVelocity"))
      return;

   if (g_pModel->IsRotationControlled())
      return;

   cPhysDynData *pDyn = g_pModel->GetDynamics();
   pDyn->SetRotationalVelocity(*ang_velocity);

   UpdatePhysProperty(objID, PHYS_STATE);
}

//
// Get rotational velocity (all submodels must have same rotational velocities)
//
void PhysGetRotationalVelocity(ObjID objID, mxs_vector *ang_velocity)
{
   if (!CheckMoveable(objID, "PhysGetRotationalVelocity"))
   {
      mx_zero_vec(ang_velocity);
      return;
   }

   cPhysDynData *pDyn = g_pModel->GetDynamics();
   mx_copy_vec(ang_velocity, (mxs_vector *)&pDyn->GetRotationalVelocity());
}

//
// Set submodel rotational velocity
//
void PhysSetSubModRotationalVelocity(ObjID objID, tPhysSubModId subModId, mxs_vector *ang_velocity)
{
   if (!CheckMoveable(objID, "PhysSetSubModRotationalVelocity"))
      return;

   if (g_pModel->IsRotationControlled())
      return;

   cPhysDynData *pDyn = g_pModel->GetDynamics(subModId);
   pDyn->SetRotationalVelocity(*ang_velocity);

   UpdatePhysProperty(objID, PHYS_STATE);
}

// 
// Get submodel rotational velocity
//
void PhysGetSubModRotationalVelocity(ObjID objID, tPhysSubModId subModId, mxs_vector *ang_velocity)
{
   if (!CheckMoveable(objID, "PhysGetSubModRotationalVelocity"))
   {
      mx_zero_vec(ang_velocity);
      return;
   }

   cPhysDynData *pDyn = g_pModel->GetDynamics(subModId);
   mx_copy_vec(ang_velocity, &pDyn->GetRotationalVelocity());
}

//////////////////////////////////////////////////////////////////////////////
//
// Public Control functions
//
//////////////////////////////////////////////////////////////////////////////

//
// Set a control velocity along a specified axis
//
void PhysAxisControlVelocity(ObjID objID, int axis, mxs_real speed)
{
   if (!CheckMoveable(objID, "PhysAxisControlVelocity"))
      return;

   cPhysCtrlData *pCtrl = g_pModel->GetControls();
#ifdef NEW_NETWORK_ENABLED
   if (g_pNetMan->Networking() && ShouldBroadcastControlFor(objID)
       && pCtrl->GetControlAxisVelocity(axis) != speed)
   {
      PhysBroadcastAxisControlVelocity(objID, axis, speed);
   }
#endif
   pCtrl->SetControlVelocity(axis, speed);

   UpdatePhysProperty(objID, PHYS_CONTROL);
}

//
// Set a velocity control along a specified vector (magnitude == speed)
//
void PhysControlVelocity(ObjID objID, mxs_vector *velocity)
{
   if (!CheckMoveable(objID, "PhysControlVelocity"))
      return;

   cPhysCtrlData *pCtrl = g_pModel->GetControls();
#ifdef NEW_NETWORK_ENABLED
   if (g_pNetMan->Networking() && ShouldBroadcastControlFor(objID))
   {
      mxs_vector cur = pCtrl->GetControlVelocity();
      if (!mx_is_identical(&cur,velocity,0.01))
          PhysBroadcastControlVelocity(objID, velocity);
   }
#endif
   pCtrl->SetControlVelocity(*velocity);

   UpdatePhysProperty(objID, PHYS_CONTROL);
}

//
// Stop velocity control on a specific axis
//
void PhysStopAxisControlVelocity(ObjID objID, int axis)
{
   if (!CheckMoveable(objID, "PhysStopAxisControlVelocity"))
      return;

   cPhysCtrlData *pCtrl = g_pModel->GetControls();
#ifdef NEW_NETWORK_ENABLED
   if (g_pNetMan->Networking() && ShouldBroadcastControlFor(objID)
       && pCtrl->GetControlAxisVelocity(axis) != 0)
   {
      PhysBroadcastAxisControlVelocity(objID, axis, 0);
   }
#endif
   pCtrl->StopControlVelocity(axis);

   UpdatePhysProperty(objID, PHYS_CONTROL);
}

//
// Stop all non-rotational velocity controls
//
void PhysStopControlVelocity(ObjID objID)
{
   if (!CheckMoveable(objID, "PhysStopControlVelocity"))
      return;

   cPhysCtrlData *pCtrl = g_pModel->GetControls();
#ifdef NEW_NETWORK_ENABLED
   if (g_pNetMan->Networking() && ShouldBroadcastControlFor(objID))
   {
      mxs_vector zero_vec;
      mx_zero_vec(&zero_vec);
      mxs_vector cur = pCtrl->GetControlVelocity();
      if (!mx_is_identical(&cur,&zero_vec,0.0))
         PhysBroadcastControlVelocity(objID, &zero_vec);
   }
#endif
   pCtrl->StopControlVelocity();

   UpdatePhysProperty(objID, PHYS_CONTROL);
}

//
// Set a rotational control velocity around a specified axis
//
void PhysAxisControlRotationalVelocity(ObjID objID, int axis, mxs_real speed)
{
   mxs_matrix obj_rot;
   mxs_vector rot;

   if (!CheckMoveable(objID, "PhysAxisControlRotationalVelocity"))
      return;

   mx_ang2mat(&obj_rot, &g_pModel->GetRotation());

   mx_scale_vec(&rot, &obj_rot.vec[axis], speed);

   cPhysCtrlData *pCtrl = g_pModel->GetControls();
#ifdef NEW_NETWORK_ENABLED
   if (g_pNetMan->Networking() && ShouldBroadcastControlFor(objID))
   {
      mxs_vector cur = pCtrl->GetControlRotationalVelocity();
      if (!mx_is_identical(&cur,&rot,0.01))
          PhysBroadcastControlRotationalVelocity(objID, &rot);
   }
#endif
   pCtrl->SetControlRotationalVelocity(rot);

   UpdatePhysProperty(objID, PHYS_CONTROL);
}

//
// Set a rotational control velocity around a specified axis on a submod
//
void PhysAxisControlSubModRotationalVelocity(ObjID objID, tPhysSubModId subModId, int axis, mxs_real speed)
{
   mxs_vector rot;

   if (!CheckMoveable(objID, "PhysAxisControlSubModRotationalVelocity"))
      return;

   mx_unit_vec(&rot, axis);
   mx_scaleeq_vec(&rot, speed);

   cPhysCtrlData *pCtrl = g_pModel->GetControls(subModId);
   pCtrl->SetControlRotationalVelocity(rot);

   UpdatePhysProperty(objID, PHYS_CONTROL);
}

//
// Set a velocity control around a specified vector (magnitude == speed)
//
void PhysControlRotationalVelocity(ObjID objID, mxs_vector *velocity)
{
   if (!CheckMoveable(objID, "PhysControlRotationalVelocity"))
      return;

   cPhysCtrlData *pCtrl = g_pModel->GetControls();
#ifdef NEW_NETWORK_ENABLED
   if (g_pNetMan->Networking() && ShouldBroadcastControlFor(objID))
   {
      mxs_vector cur = pCtrl->GetControlRotationalVelocity();
      if (!mx_is_identical(&cur,velocity,0.01))
          PhysBroadcastControlRotationalVelocity(objID, velocity);
   }
#endif
   pCtrl->SetControlRotationalVelocity(*velocity);

   UpdatePhysProperty(objID, PHYS_CONTROL);
}

//
// Stop all rotational velocity controls
//
void PhysStopControlRotationalVelocity(ObjID objID)
{
   if (!CheckMoveable(objID, "PhysStopControlRotationalVelocity"))
      return;

   cPhysCtrlData *pCtrl = g_pModel->GetControls();
   pCtrl->StopControlRotationalVelocity();

   for (int i=0; i<g_pModel->NumSubModels(); i++)
   {
      pCtrl = g_pModel->GetControls(i);
      pCtrl->StopControlRotationalVelocity();
   }

   UpdatePhysProperty(objID, PHYS_CONTROL);
}

//
// Set a model location control at a specified location (preserves offsets)
//
void PhysControlLocation(ObjID objID, mxs_vector *location)
{
   if (!CheckMoveable(objID, "PhysControlLocation"))
      return;

   cPhysCtrlData *pCtrl = g_pModel->GetControls();
   pCtrl->ControlLocation(*location);

   UpdatePhysProperty(objID, PHYS_CONTROL);
}

//
// Set a submodel location control at a specified location
//
void PhysControlSubModLocation(ObjID objID, tPhysSubModId subModId, mxs_vector *location)
{
   if (!CheckMoveable(objID, "PhysControlSubModLocation"))
      return;

   cPhysCtrlData *pCtrl = g_pModel->GetControls(subModId);
   pCtrl->ControlLocation(*location);

   UpdatePhysProperty(objID, PHYS_CONTROL);
}

//
// Stop all location controls
//
void PhysStopControlLocation(ObjID objID)
{
   if (!CheckMoveable(objID, "PhysStopControlLocation"))
      return;

   cPhysCtrlData *pCtrl = g_pModel->GetControls();
   pCtrl->StopControlLocation();

   UpdatePhysProperty(objID, PHYS_CONTROL);
}

//
// Set a rotation control at a specified rotation
//
void PhysControlRotation(ObjID objID, mxs_angvec *rotation)
{
   if (!CheckMoveable(objID, "PhysControlRotation"))
      return;

   cPhysCtrlData *pCtrl = g_pModel->GetControls();
   pCtrl->ControlRotation(*rotation);

   UpdatePhysProperty(objID, PHYS_CONTROL);
}

//
// Stop all rotation controls
//
void PhysStopControlRotation(ObjID objID)
{
   if (!CheckMoveable(objID, "PhysStopControlRotation"))
      return;

   cPhysCtrlData *pCtrl = g_pModel->GetControls();
   pCtrl->StopControlRotation();

   UpdatePhysProperty(objID, PHYS_CONTROL);
}


//////////////////////////////////////////////////////////////////////////////
//
// Public sphere-specific functions
//
//////////////////////////////////////////////////////////////////////////////

//
// Create a default sphere with a hat
//
BOOL PhysRegisterSphereHatDefault(ObjID objID)
{
   return PhysRegisterSphereHat(objID, kPMCF_Default, ObjGetRadius(objID));
}

//
// Create a sphere with a hat
//
BOOL PhysRegisterSphereHat(ObjID objID, unsigned flags, mxs_real radius)
{
   cPhysSphereModel *pModel;

   if (PhysObjHasPhysics(objID))
      return FALSE;

   AutoAppIPtr_(ObjectSystem, pObjSys);
   if (!pObjSys->Exists(objID))
      return FALSE;

#ifdef NEW_NETWORK_ENABLED
   if (g_pNetMan->Networking() && g_pObjNet->ObjHostedHere(objID))
      PhysBroadcastRegisterSphereHat(objID, flags, radius);
#endif

   PhysMessageMadePhysical(objID);

   pModel = new cPhysSphereModel(objID, 2, TRUE, flags, radius, 1);


   cPhysTypeProp typeProp;

   typeProp.type = kSphereHatProp;
   typeProp.num_submodels = 2;
   g_pPhysTypeProp->Set(objID, &typeProp);

   PhysUpdateRefs(pModel);

   // Notify media about creation
   sPhysMediaEvent event;

   event.obj = objID;
   event.old_med = kMS_Invalid;

   Position *pPos = ObjPosGet(objID);
   ComputeCellForLocation(&pPos->loc);
   
   if (pPos && (pPos->loc.cell != CELL_INVALID))
   {
      event.cell = pPos->loc.cell;
      event.new_med = PortalMediumToObjMedium(WR_CELL(event.cell)->medium);
   }
   else
   {
      event.cell = CELL_INVALID;
      event.new_med = kMS_Invalid;
   }

   PhysSendMediaEvent(&event);

   return TRUE;
}     

//
// Create a default sphere
//
BOOL PhysRegisterSphereDefault(ObjID objID)
{
   return PhysRegisterSphere(objID, 1, kPMCF_Default, ObjGetRadius(objID));
}

//
// Create a sphere
//
BOOL PhysRegisterSphere(ObjID objID, tPhysSubModId numSubModels, unsigned flags, mxs_real radius)
{
   cPhysSphereModel *pModel;

   if (PhysObjHasPhysics(objID))
      return FALSE;

   AutoAppIPtr_(ObjectSystem, pObjSys);
   if (!pObjSys->Exists(objID))
      return FALSE;

#ifdef NEW_NETWORK_ENABLED
   if (g_pNetMan->Networking() && g_pObjNet->ObjHostedHere(objID))
       PhysBroadcastRegisterSphere(objID, numSubModels, flags, radius);
#endif

   PhysMessageMadePhysical(objID);

   pModel = new cPhysSphereModel(objID, numSubModels, FALSE, flags, radius, 1);

   cPhysTypeProp typeProp;
   typeProp.type = kSphereProp;
   typeProp.num_submodels = numSubModels;
   g_pPhysTypeProp->Set(objID, &typeProp);

   PhysUpdateRefs(pModel);

   // Notify media about creation
   sPhysMediaEvent event;

   event.obj = objID;
   event.old_med = kMS_Invalid;

   Position *pPos = ObjPosGet(objID);
   ComputeCellForLocation(&pPos->loc);
   
   if (pPos && (pPos->loc.cell != CELL_INVALID))
   {
      event.cell = pPos->loc.cell;
      event.new_med = PortalMediumToObjMedium(WR_CELL(event.cell)->medium);
   }
   else
   {
      event.cell = CELL_INVALID;
      event.old_med = kMS_Invalid;
   }

   PhysSendMediaEvent(&event);

   return TRUE;
}

//
// Set submodel radius
//
void PhysSetSubModRadius(ObjID objID, tPhysSubModId subModId, mxs_real radius)
{
   cPhysModel *pModel;

   if ((pModel = g_PhysModels.Get(objID)) == NULL)
   {
      Warning(("PhysSetSubModRadius: obj %d has no physics model\n", objID));
      return;
   }
   if ((pModel->GetType(subModId) != kPMT_Sphere) &&
       (pModel->GetType(subModId) != kPMT_Point))
   {
      Warning(("PhysSetSubModRadius: obj %d does not have a sphere or point physics model\n", objID));
      return;
   }
   ((cPhysSphereModel *)pModel)->SetRadius(subModId, radius);

   UpdatePhysProperty(objID, PHYS_DIMS);
   PhysUpdateRefs(pModel);
}

//////////////////////////////////////////////////////////////////////////////
//
// Public OBB-specific functions
//
//////////////////////////////////////////////////////////////////////////////

//
// Create a default OBB
//
BOOL PhysRegisterOBBDefault(ObjID objID)
{
   return PhysRegisterOBB(objID, kPMCF_Default);
}

//
// Create an OBB
//
BOOL PhysRegisterOBB(ObjID objID, unsigned flags)
{
   cPhysOBBModel *pModel;

   if (PhysObjHasPhysics(objID))
      return FALSE;

   AutoAppIPtr_(ObjectSystem, pObjSys);
   if (!pObjSys->Exists(objID))
      return FALSE;


#ifdef NEW_NETWORK_ENABLED
   if (g_pNetMan->Networking() && g_pObjNet->ObjHostedHere(objID))
      PhysBroadcastRegisterOBB(objID, flags);
#endif

   PhysMessageMadePhysical(objID);

   PHYSICS_REGISTER_SPEW(objID, ("Created sphere model for obj %d\n", objID));

   // make sure the object model has been loaded (since we're going to init
   // off its size)
   ObjLoadModel(objID);

   // models self-thread to g_physmodels
   pModel = new cPhysOBBModel(objID, 6, flags);

   cPhysTypeProp typeProp;
   typeProp.type = kOBBProp;
   typeProp.num_submodels = 6;
   g_pPhysTypeProp->Set(objID, &typeProp);

   PhysUpdateRefs(pModel);

   // Notify media about creation
   sPhysMediaEvent event;

   event.obj = objID;
   event.old_med = kMS_Invalid;

   Position *pPos = ObjPosGet(objID);
   ComputeCellForLocation(&pPos->loc);
   
   if (pPos && (pPos->loc.cell != CELL_INVALID))
   {
      event.cell = pPos->loc.cell;
      event.new_med = PortalMediumToObjMedium(WR_CELL(event.cell)->medium);
   }
   else
   {
      event.cell = CELL_INVALID;
      event.old_med = kMS_Invalid;
   }

   PhysSendMediaEvent(&event);

   return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
//
// Public generic functions
//
//////////////////////////////////////////////////////////////////////////////

//
// Test for physics model
//
BOOL PhysObjHasPhysics(ObjID objID)
{
   return (g_PhysModels.Get(objID) != NULL);
}

//
// Test for physics model in world
BOOL PhysObjValidPos(ObjID objID, mxs_vector *delta)
{
   cPhysModel *pModel;
   mxs_vector  real_delta;
   mxs_vector  new_vec;
   Location    new_loc;

   if ((pModel = g_PhysModels.Get(objID)) == NULL)
   {
      Warning(("PhysObjValidPos: obj %d has no physics model\n", objID));
      return FALSE;
   }

   if (delta == NULL)
   {
      mx_zero_vec(&real_delta);
      delta = &real_delta;
   }

   mx_add_vec(&new_vec, (mxs_vector *) &pModel->GetLocationVec(), delta);
   MakeLocationFromVector(&new_loc, &new_vec);
   if (CellFromLoc(&new_loc) == CELL_INVALID)
      return FALSE;

   if (pModel->IsFancyGhost())
   {
      float radius = ((cPhysSphereModel *)pModel)->GetRadius(0);

      return SphrSphereInWorld(&new_loc, (radius > 1.0) ? 1.0 : radius, 0);
   }

   for (int i=0; i<pModel->NumSubModels(); i++)
   {
      mx_add_vec(&new_vec, (mxs_vector *) &pModel->GetLocationVec(i), delta); 
      MakeLocationFromVector(&new_loc, &new_vec);

      switch (pModel->GetType(i))
      {
         case kPMT_SphereHat:
         {
            if (i == 0)
            {
               if (pModel->IsPointVsTerrain())
               {
                  if (CellFromLoc(&new_loc) == CELL_INVALID)
                     return FALSE;
               }
               else
               {
                  if (!SphrSphereInWorld(&new_loc, ((cPhysSphereModel *)pModel)->GetRadius(i), 0))
                     return FALSE;
               }
            }
            break;
         }

         case kPMT_Sphere:
         {
            if (pModel->IsPointVsTerrain())
            {
               if (CellFromLoc(&new_loc) == CELL_INVALID)
                  return FALSE;
            }
            else
            {
               if (!SphrSphereInWorld(&new_loc, ((cPhysSphereModel *)pModel)->GetRadius(i), 0))
                  return FALSE;
            }
            break;
         }

         case kPMT_Point:
         {
            if (CellFromLoc(&new_loc) == CELL_INVALID)
               return FALSE;

            break;
         }
      }
   }

   return TRUE;
}

//
// Test for sphere intersecting doors
//
BOOL PhysSphereIntersectsDoor(ObjID objID)
{
   cPhysSphereModel *pModel = (cPhysSphereModel *)g_PhysModels.Get(objID);

   mxs_vector norm_list[6];
   mxs_real   const_list[6];

   if (pModel == NULL)
      return FALSE;

   switch (pModel->GetType(0))
   {
      case kPMT_Sphere:
      case kPMT_SphereHat:
      case kPMT_Point:
      {
         cPhysModel *pIterModel;
         cPhysOBBModel *pDoorModel;

         pIterModel = g_PhysModels.GetFirstActive();
         while (pIterModel)
         {
            if (pIterModel->IsDoor())
            {
               pDoorModel = (cPhysOBBModel *)pIterModel;

               if (mx_dist_vec(&pModel->GetLocationVec(), &pDoorModel->GetLocationVec()) <
                   (pModel->GetRadius(0) + mx_mag_vec(&pDoorModel->GetEdgeLengths())))
               {
                  pDoorModel->GetNormals(norm_list);
                  pDoorModel->GetConstants(const_list, norm_list);

                  BOOL inside = TRUE;

                  for (int i=0; i<6 && inside; i++)
                  {
                     mxs_real dist = mx_dot_vec(&pModel->GetLocationVec(), &norm_list[i]) - 
                                     (const_list[i] + pModel->GetRadius(0));

                     if (dist > 0.0)
                     {
                        inside = FALSE;
                        break;
                     }
                  }

                  if (inside)
                     return TRUE;
               }
            }

            pIterModel = g_PhysModels.GetNextActive(pIterModel);
         }

         return FALSE;
      }

      default:
         return FALSE;
   }
}

//
// Test for physics model on ground
//
BOOL PhysObjOnGround(ObjID objID)
{
   cPhysModel         *pModel;
   cFaceContactList   *pFaceContactList;
   cEdgeContactList   *pEdgeContactList;
   cVertexContactList *pVertexContactList;
   cPhysSubModelInst  *pSubModel;

   if ((pModel = g_PhysModels.GetActive(objID)) == NULL)
   {
      Warning(("PhysObjOnGround: %s has no active physics model\n", ObjWarnName(objID)));
      return FALSE;
   }

   for (int i=0; i<pModel->NumSubModels(); i++)
   {
      if (pModel->GetFaceContacts(i, &pFaceContactList))
      {
         cFaceContact *pFaceContact = pFaceContactList->GetFirst();

         while (pFaceContact != NULL)
         {
            if (mx_dot_vec(&pFaceContact->GetNormal(), &kGravityDir) < 0.0)
               return TRUE;

            pFaceContact = pFaceContact->GetNext();
         }
      }

      if (pModel->GetEdgeContacts(i, &pEdgeContactList))
      {
         cEdgeContact *pEdgeContact;

         pEdgeContact = pEdgeContactList->GetFirst();
         while (pEdgeContact != NULL)
         {
            if (mx_dot_vec(&pEdgeContact->GetNormal(pModel->GetLocationVec(i)), &kGravityDir) < 0.0)
               return TRUE;

            pEdgeContact = pEdgeContact->GetNext();
         }
      }

      if (pModel->GetVertexContacts(i, &pVertexContactList))
      {
         cVertexContact *pVertexContact;

         pVertexContact = pVertexContactList->GetFirst();
         while (pVertexContact != NULL)
         {
            if (mx_dot_vec(&pVertexContact->GetNormal(pModel->GetLocationVec(i)), &kGravityDir) < 0.0)
               return TRUE;

            pVertexContact = pVertexContact->GetNext();
         }
      }

      if (GetObjectContacts(pModel->GetObjID(), i, pModel, &pSubModel))
      {
         while (pSubModel != NULL)
         {
            cPhysModel *pSubMod = pSubModel->GetPhysicsModel();

            if (pSubMod->GetType(0) == kPMT_OBB)
            {
               mxs_vector norm;

               ((cPhysOBBModel *)pSubMod)->GetNormal(pSubModel->GetSubModId(), &norm);

               if (mx_dot_vec(&norm, &kGravityDir) < 0.0)
                  return TRUE;
            }
            else
            if ((pSubMod->GetType(pSubModel->GetSubModId()) == kPMT_SphereHat) &&
                (pSubModel->GetSubModId() == 1))
            {
               cFaceContact faceContact((cPhysSphereModel *)pSubMod, ((cPhysSphereModel *)pSubMod)->GetRadius(0));

               if (mx_dot_vec(&faceContact.GetNormal(), &kGravityDir) < 0.0)
                  return TRUE;
            }
            else
            if ((pSubMod->GetType(0) == kPMT_Sphere) ||  
                ((pSubMod->GetType(0) == kPMT_SphereHat) && (pSubModel->GetSubModId() == 0)))
            {
               mxs_vector normal;
               PhysGetObjsNorm(pModel, i, pSubMod, pSubModel->GetSubModId(), normal);

               if (mx_dot_vec(&normal, &kGravityDir) < 0.0)
                  return TRUE;
            }

            pSubModel = pSubModel->GetNext();
         }
      }
   }
   return FALSE;
}


// 
// Test for physics model on OBB
//
BOOL PhysObjOnPlatform(ObjID objID)
{
   cPhysModel        *pModel;
   cPhysSubModelInst *pSubModel;

   if ((pModel = g_PhysModels.GetActive(objID)) == NULL)
   {
      Warning(("PhysObjOnPlatform: %s has no active physics model\n", ObjWarnName(objID)));
      return FALSE;
   }

   for (int i=0; i<pModel->NumSubModels(); i++)
   {
      if (GetObjectContacts(pModel->GetObjID(), i, pModel, &pSubModel))
      {
         while (pSubModel != NULL)
         {
            cPhysModel *pSubMod = pSubModel->GetPhysicsModel();

            if (pSubMod->GetType(0) == kPMT_OBB)
            {
               cFaceContact faceContact((cPhysOBBModel *)pSubMod, pSubModel->GetSubModId());

               if (faceContact.GetNormal().z > 0)
                  return TRUE;
            }

            pSubModel = pSubModel->GetNext();
         }
      }
   }
   return FALSE;
}

void PhysSetFlag(ObjID objID, int flag, BOOL state)
{
   cPhysModel *pModel = g_PhysModels.Get(objID);

   if (pModel != NULL)
      pModel->SetFlagState(flag, state);
   else
      Warning(("PhysSetFlag: %s has no physics\n", ObjWarnName(objID)));
}      


//
// Get the media state of an object
//
eMediaState PhysGetObjMediaState(ObjID objID)
{
   cPhysModel *pModel = g_PhysModels.Get(objID);

   if (pModel == NULL)
      return kMS_Invalid;

   return (eMediaState)pModel->GetMediaState();
} 

BOOL PhysObjInWater(ObjID objID)
{
   if (PlayerObjectExists() && (objID == PlayerObject()))
   {
      cPhysModel *pModel = g_PhysModels.Get(objID);
      if (pModel == NULL)
      {
#ifndef SHIP
         mprintf("PhysObjInWater: no physics on obj %d\n", objID);
#endif
         return FALSE;
      }

      Location cog_loc;

      MakeHintedLocationFromVector(&cog_loc, &pModel->GetCOG(), &pModel->GetLocation(PLAYER_BODY));
      if (CellFromLoc(&cog_loc) == CELL_INVALID)
         return FALSE;

      return (WR_CELL(cog_loc.cell)->medium == MEDIA_WATER);
   }
   else
      return (PhysGetObjMediaState(objID) == kMS_Liquid_Submerged);
}

//
// Get the flow of the cell containing the physics model
//
void PhysObjGetFlow(ObjID objID, mxs_vector *flow)
{
   cPhysModel *pModel;
   Location    loc;
   mxs_vector  cog;

   mx_zero_vec(flow);

   if ((pModel = g_PhysModels.GetActive(objID)) == NULL)
   {
      Warning(("PhysObjGetFlow: obj %d does not have an active model\n", objID));
      return;
   }

   cog = pModel->GetCOG();
   MakeHintedLocationFromVector(&loc, &cog, (Location *) &pModel->GetLocation());

   if (CellFromLoc(&loc) != -1)
   {
      uchar motion_index;

      motion_index = WR_CELL(loc.cell)->motion_index;
      if (motion_index != 0)
         mx_copy_vec(flow, &g_aMedMoCellMotion[motion_index].center_change);
        
   }
}

////////////////////////////////////////////////////////////////////////////////

void PhysGetAABBox(ObjID objID, mxs_vector *minvec, mxs_vector *maxvec)
{
   cPhysModel *pModel = g_PhysModels.Get(objID);

   if (pModel == NULL)
   {
      mx_zero_vec(minvec);
      mx_zero_vec(maxvec);
      return;
   }

   switch (pModel->GetType(0))
   {
      case kPMT_Sphere:
      case kPMT_Point:
      {
         mx_copy_vec(minvec, &pModel->GetLocationVec(0));
         mx_copy_vec(maxvec, &pModel->GetLocationVec(0));

         mxs_vector submod_min;
         mxs_vector submod_max;

         mxs_vector sphere_bbox;
         mxs_real   radius;

         for (int i=0; i<pModel->NumSubModels(); i++)
         {
            mx_copy_vec(&submod_min, &pModel->GetLocationVec(i));
            mx_copy_vec(&submod_max, &pModel->GetLocationVec(i));

            radius = ((cPhysSphereModel *)pModel)->GetRadius(i) * (pModel->IsCreature() ? 1.1 : 1.0);
            mx_mk_vec(&sphere_bbox, radius, radius, radius);
            mx_addeq_vec(&submod_max, &sphere_bbox);
            mx_subeq_vec(&submod_min, &sphere_bbox);

            for (int j=0; j<3; j++)
            {
               if (submod_min.el[j] < minvec->el[j])
                  minvec->el[j] = submod_min.el[j];
               if (submod_max.el[j] > maxvec->el[j])
                  maxvec->el[j] = submod_max.el[j];
            }
         }

         break;
      }

      case kPMT_OBB:
      {
         int i, j, k, l;
         mxs_matrix mat;
         mxs_vector edge_lengths;
         mxs_vector rot_offset;

         mx_ang2mat(&mat, &pModel->GetRotation());
         mx_copy_vec(&edge_lengths, &((cPhysOBBModel *)pModel)->GetEdgeLengths());

         mx_mat_mul_vec(&rot_offset, &mat, &((cPhysOBBModel *)pModel)->GetOffset());

         mx_add_vec(minvec, &pModel->GetLocationVec(), &rot_offset);
         mx_add_vec(maxvec, &pModel->GetLocationVec(), &rot_offset);

         for (i=0; i<3; i++)
            mx_scaleeq_vec(&mat.vec[i], edge_lengths.el[i] / 2);

         mxs_vector max_ext;
         mxs_vector min_ext;

         mx_zero_vec(&max_ext);
         mx_zero_vec(&min_ext);

         // find corners
         mxs_vector corner;
         mxs_vector x_dir;
         mxs_vector y_dir;
         mxs_vector z_dir;

         for (i=0; i<2; i++)
         {
            mx_copy_vec(&x_dir, &mat.vec[0]);
            if (i == 1)
               mx_scaleeq_vec(&x_dir, -1.0);

            for (j=0; j<2; j++)
            {
               mx_copy_vec(&y_dir, &mat.vec[1]);
               if (j == 1)
                  mx_scaleeq_vec(&y_dir, -1.0);

               for (k=0; k<2; k++)
               {
                  mx_copy_vec(&z_dir, &mat.vec[2]);
                  if (k == 1)
                     mx_scaleeq_vec(&z_dir, -1.0);

                  mx_add_vec(&corner, &x_dir, &y_dir);
                  mx_addeq_vec(&corner, &z_dir);

                  for (l=0; l<3; l++)
                  {
                     if (corner.el[l] > max_ext.el[l])
                        max_ext.el[l] = corner.el[l];
                     if (corner.el[l] < min_ext.el[l])
                        min_ext.el[l] = corner.el[l];
                  }
               }
            }
         }

         mx_addeq_vec(minvec, &min_ext);
         mx_addeq_vec(maxvec, &max_ext);

         break;
      }

      case kPMT_SphereHat:
      {
         mxs_vector submod_min;
         mxs_vector submod_max;
         mxs_vector sphere_bbox;

         mx_copy_vec(minvec, &pModel->GetLocationVec());
         mx_copy_vec(maxvec, &pModel->GetLocationVec());

         mxs_real radius = ((cPhysSphereModel *)pModel)->GetRadius(0) * 1.3;
         mx_mk_vec(&sphere_bbox, radius, radius, radius);
         mx_add_vec(&submod_max, maxvec, &sphere_bbox);
         mx_sub_vec(&submod_min, minvec, &sphere_bbox);

         for (int i=0; i<3; i++)
         {
            if (submod_min.el[i] < minvec->el[i])
               minvec->el[i] = submod_min.el[i];
            if (submod_max.el[i] > maxvec->el[i])
               maxvec->el[i] = submod_max.el[i];
         }
         
         break;
      }
   }

}

////////////////////////////////////////////////////////////////////////////////

void PhysSetGravity(ObjID objID, mxs_real gravity)
{
   cPhysModel *pModel;

   if ((pModel = g_PhysModels.GetActive(objID)) == NULL)
   {
      Warning(("PhysSetGravity: %s has no active physics model\n", ObjWarnName(objID)));
      return;
   }

   pModel->SetGravity(gravity);

#ifdef NEW_NETWORK_ENABLED
   PhysNetBroadcastGravity(objID, gravity);
#endif

   UpdatePhysProperty(objID, PHYS_ATTR);
}

////////////////////////////////////////

mxs_real PhysGetGravity(ObjID objID)
{
   cPhysModel *pModel;

   if ((pModel = g_PhysModels.GetActive(objID)) == NULL)
   {
      Warning(("PhysGetGravity: %s has no active physics model\n", ObjWarnName(objID)));
      return 1.0;
   }

   return pModel->GetGravity();
}

////////////////////////////////////////////////////////////////////////////////

void PhysSetMass(ObjID objID, mxs_real mass)
{
   cPhysModel *pModel;
   if ((pModel = g_PhysModels.GetActive(objID)) == NULL)
   {
      Warning(("PhysSetMass: %s has no active physics model\n", ObjWarnName(objID)));
      return;
   }

   pModel->GetDynamics()->SetMass(mass);

   UpdatePhysProperty(objID, PHYS_ATTR);
}

////////////////////////////////////////

mxs_real PhysGetMass(ObjID objID)
{
   cPhysModel *pModel;
   if ((pModel = g_PhysModels.GetActive(objID)) == NULL)
   {
      Warning(("PhysGetMass: %s has no active physics model\n", ObjWarnName(objID)));
      return 1.0;
   }

   return pModel->GetDynamics()->GetMass();
}

////////////////////////////////////////////////////////////////////////////////

void PhysSetElasticity(ObjID objID, mxs_real elasticity)
{
   cPhysModel *pModel;
   if ((pModel = g_PhysModels.GetActive(objID)) == NULL)
   {
      Warning(("PhysSetElasticity: %s has no active physics model\n", ObjWarnName(objID)));
      return;
   }

   pModel->GetDynamics()->SetElasticity(elasticity);

   UpdatePhysProperty(objID, PHYS_ATTR);
}

////////////////////////////////////////

mxs_real PhysGetElasticity(ObjID objID)
{
   cPhysModel *pModel;
   if ((pModel = g_PhysModels.GetActive(objID)) == NULL)
   {
      Warning(("PhysGetElasticity: %s has no active physics model\n", ObjWarnName(objID)));
      return 1.0;
   }

   return pModel->GetDynamics()->GetElasticity();
} 


////////////////////////////////////////////////////////////////////////////////

void PhysSetDensity(ObjID objID, mxs_real density)
{
   cPhysModel *pModel;
   if ((pModel = g_PhysModels.GetActive(objID)) == NULL)
   {
      Warning(("PhysSetDensity: %s has no active physics model\n", ObjWarnName(objID)));
      return;
   }

   pModel->GetDynamics()->SetDensity(density);

   UpdatePhysProperty(objID, PHYS_ATTR);
}

////////////////////////////////////////

mxs_real PhysGetDensity(ObjID objID)
{
   cPhysModel *pModel;
   if ((pModel = g_PhysModels.GetActive(objID)) == NULL)
   {
      Warning(("PhysGetDensity: %s has no active physics model\n", ObjWarnName(objID)));
      return 1.0;
   }

   return pModel->GetDynamics()->GetDensity();
} 

////////////////////////////////////////

void PhysSetSubModSpringTension(ObjID objID, tPhysSubModId subModId, mxs_real tension)
{
   cPhysModel *pModel;
   if ((pModel = g_PhysModels.GetActive(objID)) == NULL)
   {
      Warning(("PhysSetSubModSpringTension: %s has no active physics model\n", ObjWarnName(objID)));
      return;
   }

   pModel->SetSpringTension(subModId, tension);
}

////////////////////////////////////////

mxs_real PhysGetSubModSpringTension(ObjID objID, tPhysSubModId subModId)
{
   cPhysModel *pModel;
   if ((pModel = g_PhysModels.GetActive(objID)) == NULL)
   {
      Warning(("PhysGetSubModSpringTension: %s has no active physics model\n", ObjWarnName(objID)));
      return 0.0;
   }

   return pModel->GetSpringTension(subModId);
}

////////////////////////////////////////

void PhysSetSubModSpringDamping(ObjID objID, tPhysSubModId subModId, mxs_real damping)
{
   cPhysModel *pModel;
   if ((pModel = g_PhysModels.GetActive(objID)) == NULL)
   {
      Warning(("PhysSetSubModSpringDamping: %s has no active physics model\n", ObjWarnName(objID)));
      return;
   }

   pModel->SetSpringDamping(subModId, damping);
}

////////////////////////////////////////

mxs_real PhysGetSubModSpringDamping(ObjID objID, tPhysSubModId subModId)
{
   cPhysModel *pModel;
   if ((pModel = g_PhysModels.GetActive(objID)) == NULL)
   {
      Warning(("PhysGetSubModSpringDamping: %s has no active physics model\n", ObjWarnName(objID)));
      return 0.0;
   }

   return pModel->GetSpringDamping(subModId);
}

////////////////////////////////////////////////////////////////////////////////

ObjID PhysGetClimbingObj(ObjID climber)
{
   cPhysModel *pModel;

   if ((pModel = g_PhysModels.GetActive(climber)) == NULL)
      return OBJ_NULL;

   return pModel->GetClimbingObj();
}

//////////////////////////////////////////////////

BOOL PhysObjIsRope(ObjID objID)
{
   cPhysModel *pModel;
   if ((pModel = g_PhysModels.GetActive(objID)) == NULL)
      return FALSE;

   return pModel->IsRope();
}

//////////////////////////////////////////////////

void PhysHitRope(ObjID objID, int magnitude)
{
   mxs_vector dir;
   mxs_vector vel;

   cPhysModel *pModel = g_PhysModels.Get(objID);
   if (pModel == NULL)
      return;

   int submod = 2; //RandRange(0, pModel->NumSubModels() - 1);

   dir.x = ((mxs_real)RandRange(0, magnitude)) - magnitude / 2;
   dir.y = ((mxs_real)RandRange(0, magnitude)) - magnitude / 2;
   //this was unitialized.
   dir.z = 0; //really we don't care about Z on ropes much...  

   vel = pModel->GetVelocity(submod);

   mx_addeq_vec(&vel, &dir);

   pModel->GetDynamics(submod)->SetVelocity(vel);
}

////////////////////////////////////////////////////////////////////////////////

void PhysExplode(mxs_vector *location, mxs_real magnitude, mxs_real radius_squared)
{
   cPhysModel *pModel;
   mxs_real power;
   int i;

   mxs_real explode_torque = 500;
   config_get_float("explode_torque", &explode_torque);

   Location explode_loc;

   MakeLocationFromVector(&explode_loc, location);
   ComputeCellForLocation(&explode_loc);

   // Build a list of all affected objects.  We can't just iterate over
   // the dlist, because it may get modified turing the iteration.
   cDynArray_<cPhysModel *, 32> explodeObjList;

   pModel = g_PhysModels.GetFirstActive();
   while (pModel)
   {
      // Is this object in range?
      // @TODO: compute range based on magnitude
      if ((mx_dist2_vec(location, &pModel->GetLocationVec()) < radius_squared) && 
          !pModel->IsProjectile() && !pModel->IsMovingTerrain() && !pModel->IsDoor() &&
          !pModel->IsFancyGhost())
      {
         // Do we have visibility to this object?
         Location obj, hit;

         MakeLocationFromVector(&obj, &pModel->GetLocationVec());

         if (PortalRaycast(&explode_loc, &obj, &hit, 0))
         {
            explodeObjList.Append(pModel);
         }
      }
      pModel = g_PhysModels.GetNextActive(pModel);
   }

   for (int obj_index=0; obj_index < explodeObjList.Size(); obj_index++)
   {
      pModel = explodeObjList[obj_index];

      // Apply a force
      mxs_vector force_dir;
      mxs_real   distance;

      mx_sub_vec(&force_dir, &pModel->GetLocationVec(), location);
      if (!IsZeroVector(force_dir))
      {
         distance = mx_normeq_vec(&force_dir);
         power = magnitude / (pModel->GetDynamics()->GetMass() * distance);
         mx_scaleeq_vec(&force_dir, power);

         // Cap force dir at 400 ft/s
         if (mx_mag2_vec(&force_dir) > (400 * 400))
         {
            mx_normeq_vec(&force_dir);
            mx_scaleeq_vec(&force_dir, 400);
         }

         if (pModel->IsLocationControlled() || pModel->IsPressurePlate())
         {
            if (pModel->IsRope())
            {
               pModel->SetSleep(FALSE);
               
               for (i=1; i<pModel->NumSubModels(); i++)
               {
                  mx_sub_vec(&force_dir, &pModel->GetLocationVec(i), location);
                  if (!IsZeroVector(force_dir))
                  {
                     distance = mx_normeq_vec(&force_dir);
                     power = magnitude / (pModel->GetDynamics()->GetMass() * distance);
                     mx_scaleeq_vec(&force_dir, power);

                     if (mx_mag2_vec(&force_dir) > (400 * 400))
                     {
                        mx_normeq_vec(&force_dir);
                        mx_scaleeq_vec(&force_dir, 400);
                     }
                     g_PhysModels.AddDeferredVelocity(pModel->GetObjID(), i, force_dir);
                  }
               }
            }
         }
         else
         {
            g_PhysModels.AddDeferredVelocity(pModel->GetObjID(), force_dir);
            
            if (!pModel->IsPlayer())
            {
               for (i=0; i<pModel->NumSubModels(); i++)
               {
                  DestroyAllObjectContacts(pModel->GetObjID(), i, pModel);
                  pModel->DestroyAllTerrainContacts(i);
               }
            }
         }
      }
         
      // And a random torque
      if (!pModel->IsRotationControlled() && !pModel->IsPlayer() && !pModel->IsRope())
      {
         mxs_vector cur_rot_vel;
         mxs_vector add_rot_vel;

         for (i=0; i<3; i++)
         {
            if (pModel->GetRotAxes() & (1 << i))
            {
               add_rot_vel.el[i] = power * ((float)(RandRange(0, explode_torque) - (explode_torque / 2))) / explode_torque;
               if (add_rot_vel.el[i] > 4 * MX_REAL_2PI)
                  add_rot_vel.el[i] = 4 * MX_REAL_2PI;
               else
                  if (add_rot_vel.el[i] < -4 * MX_REAL_2PI)
                     add_rot_vel.el[i] = -4 * MX_REAL_2PI;
            }
            else
               add_rot_vel.el[i] = 0;
         }

         mx_copy_vec(&cur_rot_vel, &pModel->GetDynamics()->GetRotationalVelocity());
         mx_addeq_vec(&cur_rot_vel, &add_rot_vel);
         pModel->GetDynamics()->SetRotationalVelocity(cur_rot_vel);
      }
   }

   explodeObjList.SetSize(0);
}

////////////////////////////////////////////////////////////////////////////////

BOOL HeadLimitFunc(ObjID objID)
{
   // we want it to be a hard limit
   return TRUE;
}

//
// Create the default player
//
void PhysCreateDefaultPlayer(ObjID objID)
{
   cPhysModel *pModel;
   mxs_vector  offset;

   // Remove existing model
   if (PhysObjHasPhysics(objID))
      PhysDeregisterModel(objID);

   // Create spheres and place them
   PhysRegisterSphere(objID, 5, kPMCF_Player, 1);

   mx_mk_vec(&offset, 0, 0, PLAYER_HEAD_POS);
   PhysSetSubModRelLocation(objID, PLAYER_HEAD, &offset);

   mx_mk_vec(&offset, 0, 0, PLAYER_BODY_POS);
   PhysSetSubModRelLocation(objID, PLAYER_BODY, &offset);

   mx_mk_vec(&offset, 0, 0, PLAYER_KNEE_POS);
   PhysSetSubModRelLocation(objID, PLAYER_KNEE, &offset);

   mx_mk_vec(&offset, 0, 0, PLAYER_SHIN_POS);
   PhysSetSubModRelLocation(objID, PLAYER_SHIN, &offset);

   mx_mk_vec(&offset, 0, 0,  PLAYER_FOOT_POS);
   PhysSetSubModRelLocation(objID, PLAYER_FOOT, &offset);

   pModel = g_PhysModels.GetActive(objID);

   // Set COG a little below the player's head
   mx_mk_vec(&offset, 0, 0, (PLAYER_HEIGHT / 2) - PLAYER_RADIUS);
   pModel->SetCOGOffset(offset);

   // Set radii
   PhysSetSubModRadius(objID, PLAYER_HEAD, PLAYER_RADIUS);
   PhysSetSubModRadius(objID, PLAYER_BODY, PLAYER_RADIUS);
   PhysSetSubModRadius(objID, PLAYER_KNEE, 0.0);
   PhysSetSubModRadius(objID, PLAYER_SHIN, 0.0);
   PhysSetSubModRadius(objID, PLAYER_FOOT, 0.0);

   mxs_angvec zeroang;
   mx_mk_angvec(&zeroang, 0, 0, 0);
   pModel->SetRotation(PLAYER_HEAD, zeroang); 

   // Set elasticity
   pModel->GetDynamics()->SetElasticity(0.0);
   pModel->GetDynamics()->SetMass(180);

   // Set density
   pModel->GetDynamics()->SetDensity(0.9);

   // Set head springiness
   pModel->SetSpringTension(PLAYER_HEAD, DEFAULT_SPRING_TENSION);
   pModel->SetSpringDamping(PLAYER_HEAD, DEFAULT_SPRING_DAMPING);

   // Set angle limits
   pModel->AddAngleLimit(1, 90, HeadLimitFunc);
   pModel->AddAngleLimit(1, 270, HeadLimitFunc);
   pModel->AddAngleLimit(0, 90, HeadLimitFunc);
   pModel->AddAngleLimit(0, 270, HeadLimitFunc);

   // Reset the player motion system
   ResetPlayerMotion();

   UpdatePhysProperty(objID);
}

//
// Remove a model
//
void PhysDeregisterModel(ObjID objID)
{
   int i;
   cPhysModel *pModel;

   if (!PhysObjHasPhysics(objID))
      return;

#ifdef NEW_NETWORK_ENABLED
   if (g_pNetMan->Networking() && g_pObjNet->ObjHostedHere(objID))
       PhysBroadcastDeregister(objID);
#endif

   PhysMessageMadeNonPhysical(objID);

   PHYSICS_REGISTER_SPEW(objID, ("Destroyed model for obj %d\n", objID));

   if((pModel = g_PhysModels.GetActive(objID)) != NULL)
   {
      // Remove it from all pending collisions
      RemoveObjectCollisions(objID);

      // Remove all its submodels from contact lists
      for (i=0; i<pModel->NumSubModels(); i++)
      {
         // Terrain
         pModel->DestroyAllTerrainContacts(i);
         // Another object
         DestroyAllObjectContacts(objID, i, pModel);
      }
   }

   // Notify media about deletion
   sPhysMediaEvent event;

   event.obj = objID;
   event.new_med = kMS_Invalid;

   Position *pPos = ObjPosGet(objID);
   ComputeCellForLocation(&pPos->loc);
   
   if (pPos && (pPos->loc.cell != CELL_INVALID))
   {
      event.cell = pPos->loc.cell;
      event.old_med = PortalMediumToObjMedium(WR_CELL(event.cell)->medium);
   }
   else
   {
      event.cell = CELL_INVALID;
      event.old_med = kMS_Invalid;
   }

   PhysSendMediaEvent(&event);

   // Remove all physics properties
   while ((pModel = g_PhysModels.Get(objID)) != NULL)
   {
      g_PhysModels.Remove(pModel);
      delete pModel;

      g_pPhysTypeProp->Delete(objID);
      g_pPhysAttrProp->Delete(objID);
      g_pPhysStateProp->Delete(objID);
      g_pPhysControlProp->Delete(objID);
      g_pPhysDimsProp->Delete(objID);
   }
}

//
// Remove all models in the world
//
void PhysDeregisterAllModels(void)
{
   cPhysModel *pModel;

   pModel = g_PhysModels.GetFirstActive();
   while (g_PhysModels.GetFirstActive() != NULL)
   {
      if (g_PhysModels.GetActive(pModel->GetObjID()))
         PhysDeregisterModel(pModel->GetObjID());
      else
      {
         g_PhysModels.Remove(pModel);
         delete pModel;
      }

      pModel = g_PhysModels.GetFirstActive();
   }

   pModel = g_PhysModels.GetFirstInactive();
   while (g_PhysModels.GetFirstInactive() != NULL)
   {
      if (g_PhysModels.GetInactive(pModel->GetObjID()))
         PhysDeregisterModel(pModel->GetObjID());
      else
      {
         g_PhysModels.Remove(pModel);
         delete pModel;
      }

      pModel = g_PhysModels.GetFirstInactive();
   }
}

//
// Get the friction acting on a model
//
BOOL PhysGetFriction(ObjID objID, mxs_real *friction)
{
   cFaceContactList   *pFaceContactList;
   cEdgeContactList   *pEdgeContactList;
   cVertexContactList *pVertexContactList;
   cPhysSubModelInst  *pSubModel;
   cPhysModel         *pModel;
   int i;

   mxs_real climbability_factor = 0;
   *friction = 0;

   if ((pModel = g_PhysModels.GetActive(objID)) == NULL)
   {
      Warning(("PhysGetFriction: %s has no active model\n", ObjWarnName(objID)));
      return FALSE;
   }

   if (pModel->IsMovingTerrain())
   {
      *friction = pModel->GetBaseFriction();
      return TRUE;
   }

   for (i=0; i<pModel->NumSubModels(); i++)
   {
      if (pModel->GetFaceContacts(i, &pFaceContactList))
      {
         cFaceContact *pFaceContact;

         pFaceContact = pFaceContactList->GetFirst();
         while (pFaceContact != NULL)
         {
            mxs_real friction_pct;
            mxs_real friction_scale = GetObjFriction(pFaceContact->GetObjID());

            friction_pct = -min(0, mx_dot_vec(&pFaceContact->GetNormal(), &kGravityDir));
            *friction += kFrictionFactor * friction_scale * friction_pct * kGravityAmt;
            climbability_factor += GetObjClimbability(pFaceContact->GetObjID());

            pFaceContact = pFaceContact->GetNext();
         }
      }

      if (pModel->GetEdgeContacts(i, &pEdgeContactList))
      {
         cEdgeContact *pEdgeContact;

         pEdgeContact = pEdgeContactList->GetFirst();
         while (pEdgeContact != NULL)
         {
            mxs_real friction_pct;

            friction_pct = -min(0, mx_dot_vec(&pEdgeContact->GetNormal(pModel->GetLocationVec(i)), &kGravityDir));
            *friction += kFrictionFactor * friction_pct * kGravityAmt;

            pEdgeContact = pEdgeContact->GetNext();
         }
      }

      if (pModel->GetVertexContacts(i, &pVertexContactList))
      {
         cVertexContact *pVertexContact;

         pVertexContact = pVertexContactList->GetFirst();
         while (pVertexContact != NULL)
         {
            mxs_real friction_pct;

            friction_pct = -min(0, mx_dot_vec(&pVertexContact->GetNormal(pModel->GetLocationVec(i)), &kGravityDir));
            *friction += kFrictionFactor * friction_pct * kGravityAmt;

            pVertexContact = pVertexContact->GetNext();
         }
      }

      if(GetObjectContacts(pModel->GetObjID(), i, pModel, &pSubModel))
      {
         while (pSubModel != NULL)
         {
            cPhysModel *pSubMod = pSubModel->GetPhysicsModel();

            if (pSubMod->GetType(0) == kPMT_OBB)
            {
               mxs_vector face_normal;

               ((cPhysOBBModel *)pSubMod)->GetNormal(pSubModel->GetSubModId(), &face_normal);

               *friction += kFrictionFactor * -min(0, mx_dot_vec(&face_normal, &kGravityDir)) * kGravityAmt;
            }
            else
            {
               mxs_vector norm;

               PhysGetObjsNorm(pModel, i, pSubMod, pSubModel->GetSubModId(), norm);

               *friction += kFrictionFactor * -min(0, mx_dot_vec(&norm, &kGravityDir)) * kGravityAmt;
            }
            pSubModel = pSubModel->GetNext();
         }
      }
   }

   if (pModel->IsClimbing() || pModel->IsMantling())
      *friction += kFrictionFactor * kGravityAmt;

   // Factor in texture climbability
   mxs_real friction_delta = (kFrictionFactor * kGravityAmt) - *friction;

   if (friction_delta > 0)
   {
      if (climbability_factor >= 1.0)
         *friction += friction_delta;
      else
         *friction += friction_delta * climbability_factor;
   }

   // Don't forget base friction
   if ((*friction == 0) && (pModel->GetBaseFriction() > 0))
   {
      *friction = pModel->GetBaseFriction();
      return TRUE;
   }

   return FALSE;
}


//
// Get base friction value for model
//
void PhysGetBaseFriction(ObjID objID, mxs_real *base_friction)
{
   cPhysModel *pModel;
   if ((pModel = g_PhysModels.Get(objID)) == NULL)
   {
      Warning(("PhysGetBaseFriction: obj %d has no physics model\n", objID));
      *base_friction = 0.0;
      return;
   }

   *base_friction = pModel->GetBaseFriction();
}

//
// Set base friction value for model
//
void PhysSetBaseFriction(ObjID objID, mxs_real base_friction)
{
   cPhysModel *pModel;
   if ((pModel = g_PhysModels.Get(objID)) == NULL)
   {
      Warning(("PhysSetBaseFriction: obj %d has no physics model\n", objID));
      return;
   }

   pModel->SetBaseFriction(base_friction);
}

void PhysPlayerJump(ObjID player, mxs_real jump_speed)
{
   cPhysModel *pModel;

   if ((pModel = g_PhysModels.GetActive(player)) == NULL)
   {
      Warning(("PhysPlayerJump: %s has no active physics model.\n", ObjWarnName(player)));
      return;
   }

   mxs_real friction;
   PhysGetFriction(player, &friction);

   if (friction > 0.5)
   {
      mxs_vector velocity;
      mxs_vector grav_comp;

      if (PhysObjInWater(player))
         jump_speed *= 0.5;

      mx_copy_vec(&velocity, &pModel->GetVelocity());

      mx_scale_vec(&grav_comp, &kGravityDir, mx_dot_vec(&kGravityDir, &velocity));
      mx_subeq_vec(&velocity, &grav_comp);
      mx_scaleeq_vec(&velocity, 1.05);
      mx_scale_addeq_vec(&velocity, &kGravityDir, -jump_speed);

      if (friction <= 1.0)
         mx_scaleeq_vec(&velocity, friction);

      pModel->GetDynamics()->SetVelocity(velocity);

      if ((g_pPlayerMode->GetMode() == kPM_Stand) || (g_pPlayerMode->GetMode() == kPM_Crouch))
         g_pPlayerMode->SetMode(kPM_Jump);

      pModel->DestroyAllTerrainContacts(PLAYER_FOOT);
      g_pPlayerMovement->LeaveGround();
   }

   if (pModel->IsClimbing())
      BreakClimb(player, TRUE, FALSE);
   else
      CheckClimb(player);

   if (pModel->IsMantling())
      BreakMantle(player);
   else
      CheckMantle(player);
}

// 
// Make us a projectile
//
void PhysSetProjectile(ObjID objID)
{
   cPhysModel *pModel;
   if ((pModel = g_PhysModels.Get(objID)) == NULL)
   {
      Warning(("PhysSetProjectile: obj %d has no physics model\n", objID));
      return;
   }
   pModel->SetProjectile(TRUE);
}

//
// Are we a projectile?
// 
BOOL PhysIsProjectile(ObjID objID)
{
   cPhysModel *pModel;
   if ((pModel = g_PhysModels.Get(objID)) == NULL)
   {
      Warning(("PhysIsProjectile: obj %d has no physics model\n", objID));
      return FALSE;
   }
   return pModel->IsProjectile();
}

//
// Wake up all sleeping physics models
//
void PhysWakeUpAll()
{
   cPhysModel *pModel;
   cPhysModel *pModelNext;

   // Activate all models 
   pModel = g_PhysModels.GetFirstInactive();
   while (pModel != NULL)
   {
      pModelNext = g_PhysModels.GetNextInactive(pModel);

      pModel->Activate();

      pModel = pModelNext;
   }

   // Deactivate all models outside of world
   pModel = g_PhysModels.GetFirstActive();
   while (pModel != NULL)
   {
      pModelNext = g_PhysModels.GetNextActive(pModel);

      if (!pModel->IsLocationControlled() && 
          !PhysObjValidPos(pModel->GetObjID(), NULL) &&
          !pModel->IsPlayer() && !pModel->IsRope() &&
          !pModel->IsMovingTerrain())
         pModel->Deactivate();

      pModel = pModelNext;
   }

   pModel = g_PhysModels.GetFirstStationary();
   while (pModel != NULL)
   {
      if (!pModel->IsLocationControlled() || pModel->IsRope())
         pModel->SetSleep(FALSE);
      pModel = g_PhysModels.GetNextStationary(pModel);
   }
}

BOOL PhysIsSleeping(ObjID objID)
{
   cPhysModel *pModel = g_PhysModels.Get(objID);

   if (pModel == NULL)
   {
      Warning(("PhysIsSleeping called on non-physical %s\n", ObjWarnName(objID)));
      return FALSE;
   }

   return pModel->IsSleeping();
}

//
// Find the most we can move an object along a dir
//
BOOL PhysObjMoveDir(ObjID objID, Location *start, Location *end, Location *hit)
{
   cPhysModel *pModel = g_PhysModels.Get(objID);

   if (pModel == NULL)  // sure, move it, hell if I care
      return TRUE;
   
   if (pModel->GetType(0) == kPMT_OBB)  // we aren't down with OBBs
      return TRUE;

   Location submod_start, submod_end;
   
   mxs_vector smallest_offset;
   mxs_vector orig_offset;

   mx_sub_vec(&smallest_offset, &end->vec, &start->vec);
   mx_copy_vec(&orig_offset, &smallest_offset);

   for (int i=0; i<pModel->NumSubModels(); i++)
   {
      if ((pModel->GetType(i) == kPMT_SphereHat) && (i == 1))  // ignore the hat
         continue;
     
      MakeHintedLocationFromVector(&submod_start, &pModel->GetLocationVec(i), &pModel->GetLocation(i));
      MakeHintedLocationFromVector(&submod_end, &submod_start.vec, &submod_start);
      mx_addeq_vec(&submod_end.vec, &smallest_offset);

      mxs_real radius = ((cPhysSphereModel *)pModel)->GetRadius(i);
      
      if (radius > 0)
      {
         int contact_count = SphrSpherecastStatic(&submod_start, &submod_end, radius, 0);

         if (contact_count > 0)
         {
            mxs_vector cur_offset;
            sSphrContact *pContact = &gaSphrContact[0];

            for (int j=1; j<contact_count; j++)  // find lowest timestamp
            {
               if (gaSphrContact[j].time < pContact->time)
                  pContact = &gaSphrContact[j];
            }

            mx_sub_vec(&cur_offset, &pContact->point_on_ray, &submod_start.vec);
            mx_scaleeq_vec(&cur_offset, 0.95);

            if (mx_mag2_vec(&cur_offset) < mx_mag2_vec(&smallest_offset))
               mx_copy_vec(&smallest_offset, &cur_offset);
         }
      }
      else
      {
         mxs_vector cur_offset;
         Location rc_hit;

         if (!PortalRaycast(&submod_start, &submod_end, &rc_hit, 1))
         {
            mx_sub_vec(&cur_offset, &rc_hit.vec, &submod_start.vec);
            mx_scaleeq_vec(&cur_offset, 0.99);

            if (mx_mag2_vec(&cur_offset) < mx_mag2_vec(&smallest_offset))
               mx_copy_vec(&smallest_offset, &cur_offset);
         }
      }
   }

   if (!EqualVectors(smallest_offset, orig_offset))
   {
      mx_add_vec(&hit->vec, &start->vec, &smallest_offset);
      return FALSE;
   }
   else
      return TRUE;
}

//
// Spew some physics info on an object
//
void PhysSpewInfo(ObjID objID)
{
   cPhysModel *pModel;

   pModel = g_PhysModels.Get(objID);

   if (pModel == NULL)
   {
      mprintf("Object %d has no physics physics model.\n", objID);
      return;
   }

   mprintf("\n");
   mprintf("[Object %d] (%d submodels) %s\n", objID, pModel->NumSubModels(), pModel->IsActive() ? "" : "-inactive-");
   mprintf(" Position = %g %g %g\n", pModel->GetLocationVec().x, pModel->GetLocationVec().y, pModel->GetLocationVec().z);
   mprintf(" Velocity = %g %g %g\n", pModel->GetVelocity().x, pModel->GetVelocity().y, pModel->GetVelocity().z);
   mprintf(" Mass = %g\n", pModel->GetDynamics()->GetMass());
   mprintf(" Elasticity = %g\n", pModel->GetDynamics()->GetElasticity());
   mprintf(" Base Friction = %g\n", pModel->GetBaseFriction());
   mprintf(" [%c] Velocity Controlled\n", pModel->IsVelocityControlled() ? 'X' : ' ');
   mprintf(" [%c] Rotational Velocity Controlled\n", pModel->IsRotationalVelocityControlled() ? 'X' : ' ');
   mprintf(" [%c] Location Controlled\n", pModel->IsLocationControlled() ? 'X' : ' ');
   mprintf(" [%c] Rotation Controlled\n", pModel->IsRotationControlled() ? 'X' : ' ');
}

void PhysSpewPlayer()
{
   PhysSpewInfo(PlayerObject());
}

//
// Listening
//

////////////////////////////////////////
// Name is just used for save/load matching
// Clearly, we could provide an API by name if needed

PhysListenerHandle PhysCreateListener(const char* pName, PhysListenMsgSet set, PhysListenFunc func, PhysListenerData data)
{
   // @HACK: due to initialization constraints, AI wants to create listener before physics initialized
   if (g_pPhysListeners == NULL)
      g_pPhysListeners = new cPhysListeners;
   return g_pPhysListeners->CreateListener(pName, set, func, data);
}

////////////////////////////////////////

void PhysDestroyListener(PhysListenerHandle handle)
{
   // we may have already shut down physics, in which cast all listeners should have been destroyed already
   if (g_pPhysListeners)
      g_pPhysListeners->DestroyListener(handle);
}

////////////////////////////////////////

void PhysListen(PhysListenerHandle handle, ObjID objID)
{
   Assert_(g_pPhysListeners);
   g_pPhysListeners->Listen(handle, objID);
}

////////////////////////////////////////

void PhysUnlisten(PhysListenerHandle handle, ObjID objID)
{
   Assert_(g_pPhysListeners);
   g_pPhysListeners->Unlisten(handle, objID);
}



