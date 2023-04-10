/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/motion/creature.cpp,v 1.206 2000/03/01 20:17:19 bfarquha Exp $
/////////
// ISSUES TO BE RESOLVED
//
// XXX Need to call "new" for all the SubFrameUpdate variables since can't
// make m_nJoints and m_nPhysModels const because
// Read from file constructor doesn't know type and objid going in.
// How to deal with changing creature types in general?  Not currently
// well supported
//
// XXX Need to get rid of the neck fixing hack, which means solving the
// underlying problem.
//
// NOTE: SphrSphereInWorld returns whether sphere intersects terrain.
//     must be passed a valid location (check using CellFromLoc).
//
// @TODO: get ballistic submodels of location controlled creatures working,
// so tails can wag as springs.

//#define PROFILE_ON

#include <cfgdbg.h>
#include <mprintf.h>
#include <timings.h>
#include <dynarray.h>

#include <meshapi.h>

#include <creature.h>
#include <creatur_.h>
#include <creatext.h>

#include <ai.h>
#include <aiapi.h>
#include <aiapiiai.h>

#include <cretprop.h>
#include <objtype.h>
#include <osysbase.h>
#include <traitman.h>
#include <traitbas.h>
#include <objquery.h>
#include <propman.h>
#include <objedit.h>

#include <objpos.h>
#include <editobj.h>
#include <phprop.h>
#include <physapi.h>
#include <physcast.h>
#include <phflist.h>
#include <collprop.h>
#include <collide.h>
#include <sndgen.h>
#include <dmgmodel.h>
#include <dmgbase.h>

#include <ghostmvr.h>
#include <crattach.h>
#include <command.h>
#include <plycbllm.h> // for debugging stuff
#include <mnumprop.h> // for model name listener
#include <mnamprop.h>
#include <posprop.h>  // for PROP_POSITION_NAME
#include <crjoint.h>
#include <simtime.h>

#include <resapi.h>
#include <binrstyp.h>

#include <playrobj.h>

#include <mvrutils.h> // for flag setup function
#include <motprop.h>  // for global timewarp

#include <iobjsys.h> // woo hoo comm-y stuff
#include <appagg.h>
#include <motbase.h> // for update_flag_fake. XXX should move this elsewhere KJ 10/97

#include <matrix.h>
#include <fix.h>
#include <qt.h>
#include <math.h>
#include <fltang.h>
#include <portal.h>
#include <port.h>

#include <phcore.h>
#include <phmods.h>
#include <phmod.h>
#include <phmterr.h>
#include <phmtprop.h>
#include <phref.h>

#include <phmodsph.h>
#include <sphrcst.h>
#include <sphrcsts.h>
#include <phclsn.h>
#include <phutils.h>

#include <crexp.h>   // exploding creatures system

// weapon stuff
#include <weapon.h>
#include <weaphit.h>
#include <weaphalo.h>
#include <weapprop.h>
#include <crwpnapi.h>
#include <crwpnlst.h>

#include <textarch.h> //for the is_terrain hack

#include <motdmnge.h>
#include <motdesc.h>
#include <motmngr.h>

#ifdef SHOCK
#include <shkcrdrd.h>
#endif

// must be last header
#include <dbmem.h>


//enable this if you want to have globals set with every motion started
// #define CUR_MOTION_TRACKING

static const int g_kCreatureVersion=10;

#define CREATURE_FROM_OBJID_UNSAFE(objID) (CreatureHandle(obj_chandle_id(objID))->pCreature)


static sCreatureDesc **g_pCreatureDescPtr=NULL;
static cCreatureFactory **g_pCreatureFactories;
static int g_nCreatureTypes=0;

typedef struct sPhysSubModInfo
{
   sPhysForce *pPhysForce;
   int nForces;
   mxs_vector endPos;
   mxs_vector terrForce;
   mxs_vector objColForce;
   mxs_real collTime;
} sPhysSubModInfo;

static ICreatureProperty *g_pICreatureProperty=NULL;

// Only to be called internall by creature system
static void SetCretObjPos(ObjID objID, const mxs_vector *pos, const mxs_angvec *rot);

///////////////////////////////////////////////////

EXTERN int g_SimRunning;
int g_SimRunning = FALSE;

static BOOL g_MotionEnded=FALSE;
static BOOL gUseHeadTracking = FALSE;

typedef struct sMotEndData
{
   int motion_num;
   int frame;
   ulong flags;
} sMotEndData;

static sMotEndData g_MotEndData;

int CreatureMotEndCallback(multiped *mp, int motion_num, int frame, float timeSlop, ulong flags)
{
   AssertMsg(mp->app_ptr,"No creature for multiped!");

   if(flags&MP_UPDATE_FLAG_FAKE)
      return 1;

   cCreature *pCreature=(cCreature *)mp->app_ptr;

   sCreatureMotionEndNode node;
   node.motionNum=motion_num;
   node.timeSlop=timeSlop;
   node.flags=flags;
   node.frame=frame;
   node.pNext=NULL;
   pCreature->QueueCompletedMotion(&node);

//   ConfigSpew("CreatureTrace",("%d: finished motion %d (%s)\n",pCreature->GetObjID(),motion_num,MotDescGetName(motion_num)));
   return 1;
}

///////////////////////////////////////////////////

IMesh *CreatureGetMeshInterface(const ObjID objID)
{
   // @OPTIMIZE: this isn't inline yet

   return CreatureFromObj(objID);
}

///////////////////////////////////////////////////

IMotor *CreatureGetMotorInterface(const ObjID objID)
{
   return CreatureFromObj(objID);
}

///////////////////////////////////////////////////

#ifdef PLAYTEST

BOOL g_AssertValidPosition = TRUE;

BOOL IsCreaturePositionValid(ObjID objID)
{
   cCreature *pCreature = CreatureFromObj(objID);
   Position *pPos = ObjPosGet(objID);
   cPhysModel *pModel = g_PhysModels.Get(objID);

   if (g_SimRunning && g_AssertValidPosition &&
       pPos && pModel && pCreature &&
       ComputeCellForLocation(&pPos->loc) != CELL_INVALID &&
       pCreature->GetType() != 10)  // rope magic number
   {
      return (PhysObjValidPos(objID, NULL));
   }

   return TRUE;
}

BOOL ValidateCreaturePosition(ObjID objID)
{
   if (!IsCreaturePositionValid (objID))
   {
      mxs_vector loc;
      PhysGetModLocation(objID, &loc);
      if (config_is_defined("bad_pos_assert"))
         CriticalMsg4("%s in bad cret pos: %g,%g,%g\n",
            ObjWarnName(objID), loc.x, loc.y, loc.z);
      else
      {
         mprintf("%s in bad cret pos: %g,%g,%g\n",
            ObjWarnName(objID), loc.x, loc.y, loc.z);
         mprintf("   rad: ");
         cPhysModel *pModel = g_PhysModels.Get(objID);
         for (int i=0; i<pModel->NumSubModels(); i++)
            mprintf("%g  ", ((cPhysSphereModel *)pModel)->GetRadius(i));
         mprintf("\n");
      }
      return FALSE;
   }
   return TRUE;
}

#endif

///////////////////////////////////////////////////

void CreatureFixup()
{
   sCreatureHandle *pCHandle;
   cCreature *pCreature;
   int max;
   int i;

   AutoAppIPtr_(PropertyManager, pPropMan);

   ICreaturePoseProperty *pPoseProp= (ICreaturePoseProperty *)pPropMan->GetPropertyNamed(PROP_CREATUREPOSE_NAME);
   ICreatureProperty *pCreatureProp = (ICreatureProperty *)pPropMan->GetPropertyNamed(PROP_CREATURE_NAME);

   cDynArray<ObjID> creatureFixupList;

   max = max_chandle_id();
   for (i = 0; i < max; i++)
   {
      pCHandle=CreatureHandle(i);

      if(!pCHandle)
         continue;

      AssertMsg1(pCHandle->pCreature,"no creature at entry %d",i);
      pCreature = pCHandle->pCreature;

      if (pCreature->GetType() == 10) // rope magic number, yippee
         continue;

      creatureFixupList.Append(pCreature->GetObjID());
   }

   for (i=0; i<creatureFixupList.Size(); i++)
   {
      pCreatureProp->Delete(creatureFixupList[i]);
      pCreatureProp->Create(creatureFixupList[i]);

      if (ObjIsPosed(creatureFixupList[i]))
      {
         sCreaturePose *pPose;

         if (!pPoseProp->Get(creatureFixupList[i], &pPose))
            pPose = NULL;

         pPoseProp->Delete(creatureFixupList[i]);

         if (pPose)
            pPoseProp->Set(creatureFixupList[i], pPose);
         else
            pPoseProp->Create(creatureFixupList[i]);
      }
   }

   creatureFixupList.SetSize(0);
}

///////////////////////////////////////////////////

BOOL CreatureMakeBallistic(ObjID objID, int style)
{
#ifndef SHIP
   mxs_vector pre_pos[8];
   mxs_vector post_pos[8];
   int i;
#endif

   cCreature *pCreature=CreatureFromObj(objID);
   if(pCreature)
   {
      const sCreatureDesc *pCDesc=pCreature->GetCreatureDesc();

      if (!PhysObjHasPhysics(objID))
         pCreature->MakePhysical();
      pCreature->MoveButt(&ObjPosGet(objID)->loc.vec, &ObjPosGet(objID)->fac, TRUE);

#ifndef SHIP
      cPhysModel *pModel = g_PhysModels.Get(objID);

      if (pModel)
      {
         Assert_((pModel->NumSubModels() < 8) || (pCDesc->nPhysModels < 8));

         for (i=0; i<pModel->NumSubModels() && i<pCDesc->nPhysModels; i++)
            PhysGetSubModLocation(objID, i, &pre_pos[i]);
      }
#endif

      BOOL retval;

      if (style == kCMB_Compressed)
         retval = pCreature->MakeBallistic(style, TRUE);
      else
         retval = pCreature->MakeBallistic(style);

#ifndef SHIP
      if (pModel)
      {
         Assert_((pModel->NumSubModels() < 8) || (pCDesc->nPhysModels < 8));

         for (i=0; i<pModel->NumSubModels() && i<pCDesc->nPhysModels; i++)
         {
            mxs_vector delta;

            PhysGetSubModLocation(objID, i, &post_pos[i]);
            mx_sub_vec(&delta, &pre_pos[i], &post_pos[i]);

            if (mx_mag2_vec(&delta) > 0.001)
               if (config_is_defined("bad_ballistic_assert"))
                  CriticalMsg2(" %s made a bad ballistic transition (mag %g)!\n", ObjWarnName(objID), mx_mag2_vec(&delta));
               else
                  Warning((" %s made a bad ballistic transition (mag %g)!\n", ObjWarnName(objID), mx_mag2_vec(&delta)));
         }
      }
#endif

      return retval;
   }
   else
      return FALSE;
}

///////////////////////////////////////////////////

void CreatureMakeNonBallistic(ObjID objID)
{
   cCreature *pCreature=CreatureFromObj(objID);

   if(pCreature)
      pCreature->MakeNonBallistic();
}

///////////////////////////////////////////////////

void CreaturePrepareToDie(ObjID objID)
{
   cCreature *pCreature=CreatureFromObj(objID);

   if(pCreature)
      pCreature->PrepareToDie();
}

///////////////////////////////////////////////////

void CreatureBeDead(ObjID objID)
{
   cCreature *pCreature=CreatureFromObj(objID);

   if(pCreature)
      pCreature->BeDead();
}

///////////////////////////////////////////////////

void CreatureMakePhysical(ObjID objID)
{
   cCreature *pCreature = CreatureFromObj(objID);

   if (pCreature)
      pCreature->MakePhysical();
}

///////////////////////////////////////////////////

void CreatureMakeNonPhysical(ObjID objID)
{
   cCreature *pCreature = CreatureFromObj(objID);

   if (pCreature)
      pCreature->MakeNonPhysical();
}

///////////////////////////////////////////////////

// @HACK, brutal. only works for human-ish creatures
BOOL CreatureGetHeadTransform(ObjID objID, mxs_trans *pHead)
{
   cCreature *pCreature=CreatureFromObj(objID);

   if(!pCreature || !pHead)
      return FALSE;
   const multiped *pM=pCreature->GetMultiped();
   mx_copy_vec(&pHead->vec,&pM->joints[9]);
   mx_copy_mat(&pHead->mat,&pM->orients[9]);
   return TRUE;
}

////////////////////////////////////////////////////////////
// Debugging functions

#ifdef PLAYTEST

static ObjID g_DebugObj=OBJ_NULL;

void CreatureDebugSetObj(ObjID obj)
{
   g_DebugObj=obj;
}

void CreatureDebugAbortWeapon()
{
   if(g_DebugObj!=OBJ_NULL)
   {
      CreatureAbortWeaponMotion(g_DebugObj,OBJ_NULL);
   }
}

void CreatureDebugAbortPlayerWeapon()
{
   ObjID pa;

   if(OBJ_NULL!=(pa=PlayerArm()))
   {
      CreatureAbortWeaponMotion(pa,OBJ_NULL);
   }
}

void CreatureDebugSetFocusObj(char *cmd_string)
{
   ObjID creature;
   ObjID focus;

   sscanf(cmd_string, "%d,%d", &creature, &focus);

   CreatureSetFocusObj(creature, focus);
}

void CreatureDebugSetFocusLoc(char *cmd_string)
{
   ObjID creature;
   ObjID focus;

   sscanf(cmd_string, "%d,%d", &creature, &focus);

   Position *pos = ObjPosGet(focus);

   if (pos)
      CreatureSetFocusLoc(creature, &pos->loc.vec);
}

extern cDynArray<ObjID> standableObjectList;

static void CreatureSpewStandable()
{
   mprintf("Standable AI objects:\n");

   for (int i=0; i<standableObjectList.Size(); i++)
      mprintf(" %s\n", ObjWarnName(standableObjectList[i]));
}

static int MotSwizzle(int mot,motion_callback *pcallback);

void CreatureDebugSaveLoadTest()
{
   cCreature *pCreature=CreatureFromObj(g_DebugObj);

   if(!pCreature)
      return;
   mprintf("testing mp saveload\n");
   multiped *mp=(multiped *)pCreature->GetMultiped();
   int bsize=mp_get_write_buffsize(mp);
   int *buf=new int[bsize];
   mp_write_multiped(mp,buf);
   mp_free_multiped(mp);
   mp_init_multiped(mp);
   mp_read_multiped(mp,buf,MotSwizzle);
   delete buf;
}

EXTERN BOOL g_mot_quat_debug;
static Command creature_debug_keys[] =
{
   { "cret_set_debug_obj", FUNC_INT, CreatureDebugSetObj, "set debug obj"},
   { "cret_weap_abort",    FUNC_VOID, CreatureDebugAbortWeapon, "stop swing"},
   { "player_weap_abort",  FUNC_VOID, CreatureDebugAbortPlayerWeapon, "stop swing"},
   { "cret_set_focus_obj", FUNC_STRING, CreatureDebugSetFocusObj, "set creature's focus obj"},
   { "cret_set_focus_loc", FUNC_STRING, CreatureDebugSetFocusLoc, "set creature's focus loc"},
   { "cret_saveload_test", FUNC_VOID, CreatureDebugSaveLoadTest, "test creature saveload"},
   { "toggle_mot_quat_debug", TOGGLE_BOOL, &g_mot_quat_debug, "toggle motion quaternion debugging"},
   { "fixup_creature_phys", FUNC_VOID, CreatureFixup, "fix up physics for all creatures"},
   { "spew_creature_standable", FUNC_VOID, CreatureSpewStandable, "spew all object AIs can stand on"},
};

#endif

///////////////////////////////////////////////////
// creature model name listener

static void LGAPI CreatureModelNameListener(sPropertyListenMsg* msg, PropListenerData data)
{
   if (msg->type & (kListenPropModify|kListenPropSet)) // had unset
   {
      cCreature *pCreature;

      if (OBJ_IS_CONCRETE(msg->obj))
      {
         if(NULL!=(pCreature=CreatureFromObj(msg->obj)))
            pCreature->ResetLengths();
      }
      else // set lengths for all descendents
      {
         AutoAppIPtr_(TraitManager,TraitMan);
         IObjectQuery* query = TraitMan->Query(msg->obj,kTraitQueryAllDescendents);
         for (; !query->Done(); query->Next())
         {
            ObjID obj = query->Object();
            if (OBJ_IS_CONCRETE(obj))
               if(NULL!=(pCreature=CreatureFromObj(obj)))
                  pCreature->ResetLengths();
         }
         SafeRelease(query);
      }
   }
}

///////////////////////////////////////////////////
// creature non-phys listener

static BOOL g_NonPhysOverride=FALSE; // needed so unset can make creature physical successfully (KJ 8/98)

static void LGAPI CreatureNonPhysListener(sPropertyListenMsg* msg, PropListenerData data)
{
   if (msg->type & (kListenPropModify|kListenPropUnset))
   {
      cCreature *pCreature;
      BOOL isNonPhys=(msg->type&kListenPropUnset)?FALSE:msg->value.intval;

      if (OBJ_IS_CONCRETE(msg->obj))
      {
         if(NULL!=(pCreature=CreatureFromObj(msg->obj)))
         {
            if(isNonPhys)
               pCreature->MakeNonPhysical();
         }
      } else // set lengths for all descendents
      {
         AutoAppIPtr_(TraitManager,TraitMan);
         IObjectQuery* query = TraitMan->Query(msg->obj,kTraitQueryAllDescendents);
         for (; !query->Done(); query->Next())
         {
            ObjID obj = query->Object();
            if (OBJ_IS_CONCRETE(obj))
            {
               if(NULL!=(pCreature=CreatureFromObj(obj)))
               {
                  if(isNonPhys)
                  {
                     pCreature->MakeNonPhysical();
                  } else if(!pCreature->IsPhysical())
                  {
                     g_NonPhysOverride=TRUE;
                     pCreature->MakePhysical();
                     g_NonPhysOverride=FALSE;
                  }
               }
            }
         }
         SafeRelease(query);
      }
   }
}

///////////////////////////////////////////////////

// THIS IS A SEMAPHORE!!!
BOOL g_CreatureIsMovingObj = FALSE; // True if this class is moving an obj.  Set by UpdateObjPos
BOOL g_CreaturePosPropSemaphore = FALSE; // this is also, as expected, a semaphore

static int gCrPosPropLock = 0;

// This annoyance should be called before and after the physics frame,
// because we don't want creatures changing our changes inside physics
void CreaturePosPropLock()
{
   gCrPosPropLock++;
}
void CreaturePosPropUnlock()
{
   gCrPosPropLock--;
}

// @YO OVER HERE
void LGAPI CrPosPropListener(sPropertyListenMsg* msg, PropListenerData data)
   // Listener for the position property.
   // Updates the creature position, if something other than this class moved it.
{
   if (g_CreaturePosPropSemaphore)
      return;
   if (g_CreatureIsMovingObj || OBJ_IS_ABSTRACT(msg->obj))
      return;
   cCreature *pCreature=CreatureFromObj(msg->obj);
   if (!pCreature)
      return;

   if (!pCreature->IsBallistic())
      if (gCrPosPropLock > 0)
         return;

   g_CreaturePosPropSemaphore=TRUE;

   // maybe we should zfloor here in the ballistic case, cause we are so wack

   Position *pPosition=ObjPosGet(msg->obj);

   // can cause creature to become tilted
   pCreature->MoveButt(&pPosition->loc.vec,&pPosition->fac, TRUE,TRUE);
   pCreature->FlushRelativeState();
   CreatureAttachmentsPosUpdate(pCreature->GetObjID());

   IMotorResolver *pResolver=pCreature->GetMotorResolver();
   if(pResolver)
      pResolver->NotifyAboutMotorStateChange();

   g_CreaturePosPropSemaphore=FALSE;
}

///////////////////////////////////////////////////

void CreaturesInit(int nCreatureTypes, const sCreatureDesc **ppCreatureDescs, const char **pCreatureTypeNames, const cCreatureFactory **ppCreatureFactories)
{
   InitCreatureAttachments();

#ifdef PLAYTEST
   COMMANDS(creature_debug_keys,HK_ALL);
#endif

   g_nCreatureTypes=nCreatureTypes;
   // note: I know it's bad to cast a const ptr to non-const, but I don't
   // modify it, honest.  KJ 4/98
   g_pCreatureDescPtr=(sCreatureDesc **)ppCreatureDescs;
   g_pCreatureFactories=(cCreatureFactory **)ppCreatureFactories;

   if((g_pICreatureProperty=CreaturePropertiesInit(nCreatureTypes,pCreatureTypeNames)) == NULL)
   {
        AssertMsg(FALSE,"Failed To Initialize Creature Property!");
   }

   IPropertyManager *propman = AppGetObj(IPropertyManager);
   // install listener on model name property
   IProperty *prop=propman->GetPropertyNamed(PROP_MODELNAME_NAME);
   if(prop)
   {
      prop->Listen(kListenPropModify|kListenPropSet|kListenPropUnset,CreatureModelNameListener,NULL);
      SafeRelease(prop);
   }

   // install listener on creature non-phys property
   prop=propman->GetPropertyNamed(PROP_CREATURENONPHYS_NAME);
   if(prop)
   {
      prop->Listen(kListenPropModify|kListenPropUnset,CreatureNonPhysListener,NULL);
      SafeRelease(prop);
   }

   // install listener on the position property
   prop=propman->GetPropertyNamed(PROP_POSITION_NAME);
   if(prop)
   {
      prop->Listen(kListenPropModify|kListenPropSet,CrPosPropListener,NULL);
      SafeRelease(prop);
   }

   SafeRelease(propman);

   CreatureExplodeInit();
}

///////////////////////////////////////////////////

void CreaturesClose()
{
   TermCreatureAttachments();

   CreaturePropertiesTerm();
   g_pICreatureProperty=NULL;
   g_pCreatureDescPtr=NULL;
   g_nCreatureTypes=0;

}

///////////////////////////////////////////////////

cCreature *CreatureCreate(int type, const ObjID objID)
{
   Assert_(type>=0&&type<g_nCreatureTypes);

   if(g_pCreatureFactories[type])
   {
      return g_pCreatureFactories[type]->Create(type,objID);
   } else
   {
      return new cCreature(type,objID);
   }
}

///////////////////////////////////////////////////

DECLARE_TIMER(CREAT_Total, Average);

void CreaturesUpdate(const ulong timeDelta)
{
   AUTO_TIMER(CREAT_Total);

   int i;
   int max;
   sCreatureHandle *pCHandle;

   IObjectSystem *pObjSys=AppGetObj(IObjectSystem);
   Assert_(pObjSys);
   pObjSys->Lock(); // don't want objects to get deleted during creature update

   AutoAppIPtr_(AIManager, pAIMan);

   max = max_chandle_id();
   for (i = 0; i < max; i++)
   {
      pCHandle=CreatureHandle(i);

      if(!pCHandle)
         continue;

      AssertMsg1(pCHandle->pCreature,"no creature at entry %d",i);

      ValidateCreaturePosition(pCHandle->pCreature->GetObjID());

      cAI *pAI = (cAI *)pAIMan->GetAI(pCHandle->pCreature->GetObjID());

      if (pAI)
      {
         eAIMode mode = pAI->GetState()->GetMode();

         SafeRelease(pAI);

         if ((mode == kAIM_Asleep) ||
             (mode == kAIM_SuperEfficient) ||
             (mode == kAIM_Efficient))
            continue;
      }
      else
      {
         if (ObjIsPosed(pCHandle->pCreature->GetObjID()))
            continue;
      }

      if(!pCHandle->pCreature->IsFrozen())
         pCHandle->pCreature->Update(timeDelta);
   }
   pObjSys->Unlock();
   SafeRelease(pObjSys);
}

///////////////////////////////////////////////////

BOOL CreatureExists(const ObjID obj)
{
   int idx = obj_chandle_id(obj);
   return idx > 0 && CreatureHandle(idx)->pCreature;
}

///////////////////////////////////////////////////

BOOL CreatureSelfPropelled(const ObjID objID)
{
   cCreature *pCreature=CreatureFromObj(objID);
   if(!pCreature)
      return FALSE;
   return !pCreature->IsBallistic();
}

///////////////////////////////////////////////////

void CreatureFreeze(ObjID objID)
{
   cCreature *pCreature=CreatureFromObj(objID);
   if(!pCreature) return;
   if(!pCreature->IsFrozen())
      pCreature->Freeze();
}

///////////////////////////////////////////////////

void CreatureUnFreeze(ObjID objID)
{
   cCreature *pCreature=CreatureFromObj(objID);
   if(!pCreature) return;
   if(pCreature->IsFrozen())
      pCreature->UnFreeze();
}

///////////////////////////////////////////////////

cCreature *CreatureFromObj(ObjID obj)
{
   int idx = obj_chandle_id(obj);
   if (idx > 0)
      return CreatureHandle(idx)->pCreature;
   return NULL;
}

///////////////////////////////////////////////////

#if 0
sDippyLocoState *CreatureGetLocoStatePointer(const ObjID objID)
{
   AssertMsg1(CreatureExists(objID),"no creature for obj %d",objID);

   cCreature *pCreature=CreatureFromObj(objID);

   return &pCreature->m_sLocoState;
}

float CreatureGetButtHeight(const ObjID objID)
{
   AssertMsg1(CreatureExists(objID),"no creature for obj %d",objID);

   cCreature *pCreature=CreatureFromObj(objID);

   return 3.375*pCreature->m_PrimScale;
}
#endif

///////////////////////////////////////////////////

BOOL CreatureGetRenderHandle(const ObjID objID, long *pHandle)
{
   // check that object is a creature
   if(!CreatureExists(objID))
      return FALSE;

   *pHandle=(long)CreatureFromObj(objID);
   return TRUE;
}

///////////////////////////////////////////////////

void CreatureMeshJointPosCallback(const mms_model *m, const int jointID, mxs_trans *pTrans)
{
   AssertMsg(m->app_data,"Invalid Creature Pointer In Joint Callback");

   ((cCreature *)(m->app_data))->MeshJointPosCallback(jointID,pTrans);
}

///////////////////////////////////////////////////

void CreatureMeshStretchyJointCallback(const mms_model *m, const int jointID, mxs_trans *pTrans, quat *pRot)
{
   AssertMsg(m->app_data,"Invalid Creature Pointer In Joint Callback");

   ((cCreature *)(m->app_data))->MeshStretchyJointCallback(jointID,pTrans,pRot);
}

///////////////////////////////////////////////////

BOOL CreatureGetDefaultLengthsName(int type, char *name)
{
   Assert_(g_pCreatureDescPtr&&type<g_nCreatureTypes);
   if(!g_pCreatureDescPtr[type]->defLengthsName)
      return FALSE;
   strcpy(name,g_pCreatureDescPtr[type]->defLengthsName);
   return TRUE;
}

///////////////////////////////////////////////////

void CreatureAbortWeaponMotion(ObjID creature, ObjID weapon)
{
   cCreature *pCreature=CreatureFromObj(creature);

   if(pCreature)
      pCreature->AbortWeaponMotion(weapon);
}

///////////////////////////////////////////////////

void GetSubModelPos(ObjID objID, int submod, mxs_vector *pPos)
{
   cCreature *pCreature = CreatureFromObj(objID);

   if (pCreature != NULL)
      pCreature->GetSubModelPos(submod, pPos);
}

static void GetPhysSubModelPos(mxs_vector *pJoints, sCrPhysModOffset *pOffset, mxs_vector *pPos)
{
   if(pOffset->frac)
   {
      mxs_vector tmp;

      mx_copy_vec(&tmp,&pJoints[pOffset->j1]);
      mx_scaleeq_vec(&tmp,1.0-pOffset->frac);
      mx_copy_vec(pPos,&pJoints[pOffset->j2]);
      mx_scaleeq_vec(pPos,pOffset->frac);
      mx_addeq_vec(pPos,&tmp);
   } else
   {
      mx_copy_vec(pPos,&pJoints[pOffset->j1]);
   }
}

///////////////////////////////////////////////////

void CreatureSetFocusObj(ObjID creature, ObjID focusObj)
{
   cCreature *pCreature = CreatureFromObj(creature);

   if (pCreature)
      pCreature->SetTrackingObj(focusObj);
}

///////////////////////////////////////////////////

void CreatureSetFocusLoc(ObjID creature, const mxs_vector *loc)
{
   cCreature *pCreature = CreatureFromObj(creature);

   if (pCreature)
      pCreature->SetTrackingLoc(*loc);
}

///////////////////////////////////////////////////

void CreatureSetNoFocus(ObjID creature)
{
   cCreature *pCreature = CreatureFromObj(creature);

   if (pCreature)
      pCreature->SetNoTracking();
}

///////////////////////////////////////////////////

typedef struct
{
   int mot;
   Label name;
} sMotSwizzle;

static sMotSwizzle *g_pMotSwizzleMap=NULL;
static int g_nSwizzleMots;

void CreaturesRead(fCreatureReadWrite func, eObjPartition partition)
{
   int i, ver, num;
   if(g_pMotSwizzleMap)
   {
      delete g_pMotSwizzleMap;
      g_pMotSwizzleMap=NULL;
      g_nSwizzleMots=0;
   }

   AutoAppIPtr_(ObjectSystem,pObjSys);

   func(&ver, sizeof(int),1);
   //   AssertMsg1(ver==g_kCreatureVersion||ver==1,"Creature version %d not supported",ver);

   if(ver>=7)
   {
      // read in mot swizzle map
      func((void *)&g_nSwizzleMots, sizeof(int),1);

      g_pMotSwizzleMap=new sMotSwizzle[g_nSwizzleMots];
      func((void *)g_pMotSwizzleMap,sizeof(*g_pMotSwizzleMap),g_nSwizzleMots);
   }

   // read number of creatures
   func((void *)&num, sizeof(int),1);

   // read in creature data
   for (i = 0; i < num; i++)
   {
      ObjID obj;
      int type;
      cCreature *pCreature;

      // Read general info
      func((void *)&obj, sizeof(ObjID),1);
      obj = pObjSys->RemapOnLoad(obj);

      func((void *)&type, sizeof(type),1);

      Assert_(type>=0&&type<g_nCreatureTypes);

      if(g_pCreatureFactories[type])
      {
         pCreature=g_pCreatureFactories[type]->CreateFromFile(type,obj,func,ver);
      } else
      {
         pCreature = new cCreature(type,obj,func,ver);
      }

      sCreatureHandle *pCHandle;
      ObjID objID=pCreature->GetObjID();

      if (!pObjSys->IsObjLoadPartition(objID,partition)
          || !g_pICreatureProperty->Get(objID,&pCHandle))
      {
         Warning(("can't load creature %d\n",objID));
         delete pCreature;
         continue;
      }
      pCHandle = CreatureHandle(obj_chandle_id(objID));

      AssertMsg(pCHandle,"something wacko in creature loading");
      pCHandle->pCreature = pCreature;
   }
}

///////////////////////////////////////////////////

void CreaturesWrite(fCreatureReadWrite func, eObjPartition partition)
{
   int i, max, num, nmot, mot, j;
   sCreatureHandle *pCH;
   IObjectSystem *pObjSys=AppGetObj(IObjectSystem);

   func((void *)&g_kCreatureVersion, sizeof(int),1);

   max = max_chandle_id();

   // calc number of creatures (might not be max id), and how many playing motions
   num=0;
   nmot=0;
   for (i = 0; i < max; i++)
   {
      pCH=CreatureHandle(i);

      if(pCH&&pCH->type!=kCreatureTypeInvalid)
      {
         if(pCH->pCreature && pObjSys->IsObjSavePartition(pCH->pCreature->GetObjID(),partition))
         {
            num++;
            j = 1;  // so mp_get_multiped_motions doesn't just return...
            mp_get_multiped_motions((multiped *)pCH->pCreature->GetMultiped(),&mot,&j);
            if(j)
               nmot++;
         }
      }
   }

   // number of motions in swizzle map
   func((void *)&nmot, sizeof(int),1);

   // write mappings for currently playing motions to motion names, for swizzling
   // iterate over creatures, writing out their data
   for (i = 0; i < max; i++)
   {
      pCH=CreatureHandle(i);

      if (pCH && pCH->type!=kCreatureTypeInvalid)
      {
         AssertMsg1(pCH->pCreature!=NULL,"Creature type %d property, but not class pointer",pCH->type);
         if(pCH->pCreature && pObjSys->IsObjSavePartition(pCH->pCreature->GetObjID(),partition))
         {
            multiped *mp=(multiped *)(pCH->pCreature->GetMultiped());
            j = 1;  // so mp_get_multiped_motions doesn't just return...
            mp_get_multiped_motions(mp,&mot,&j);
            if(j)
            {
               func((void *)&mot, sizeof(int),1);
               Label *pname=g_pMotionSet->GetName(mot);
               func((void *)pname,sizeof(*pname),1);
            }
         }
      }
   }

   // write number of creatures
   func((void *)&num, sizeof(int),1);
   // iterate over creatures, writing out their data
   for (i = 0; i < max; i++)
   {
      pCH=CreatureHandle(i);

      if (pCH && pCH->type!=kCreatureTypeInvalid)
      {
         AssertMsg1(pCH->pCreature!=NULL,"Creature type %d property, but not class pointer",pCH->type);
         if(pObjSys->IsObjSavePartition(pCH->pCreature->GetObjID(),partition))
            pCH->pCreature->Write(func);
      }
   }
   SafeRelease(pObjSys);
}



///////////////////////////////////////////////////
///////////////////////////////////////////////////
////// Creature class implementation
//////
///////////////////////////////////////////////////

///////////////////////////////////////////////////

static void InitMotorState(sMotorState *pMotorState)
{
   memset(pMotorState,0,sizeof(*pMotorState));
   pMotorState->ballisticRefCount=0;
}

// always does something safe, even if it can't find correct lengths values
static void GetLengths(ObjID obj_id, const sCreatureDesc *pCDesc,torso *pTorsos,limb *pLimbs,float *pScale)
{
   char name[38];
   void *pData = NULL;
   BOOL failed=FALSE;
   int nTorsos,nLimbs;
   IRes *pRes;

   AutoAppIPtr(ResMan);

   if(!ObjGetModelName(obj_id,name))
   {
      AssertMsg(pCDesc->defLengthsName,"No default lengths for creature type");
      strcpy(name,pCDesc->defLengthsName);
   }
   strcat(name,".cal");
   pRes = pResMan->Bind(name, RESTYPE_BINARY, NULL, "mesh\\");

   if (pRes) {
      pData = pRes->Lock();
   }

   if (pData && (*((int *)pData)==kMPCalibVersion))
   {
      uchar *pCur;

      pData=(((int *)pData)+1);
      nTorsos=((sCreatureLengths *)pData)->nTorsos;
      nLimbs=((sCreatureLengths *)pData)->nLimbs;

      // verify lengths data consistent with creature type
      //
      AssertMsg3(nTorsos == pCDesc->nTorsos, "nTorsos differs (%d vs %d) for %s",
                 nTorsos, pCDesc->nTorsos, ObjWarnName(obj_id));
      AssertMsg3(nLimbs == pCDesc->nLimbs, "nLimbs differs (%d vs %d) for %s",
                 nLimbs, pCDesc->nLimbs, ObjWarnName(obj_id));

      pCur=(((uchar *)pData)+CL_BASE_SIZE);
      memcpy(pTorsos,pCur,nTorsos*sizeof(torso));
      pCur+=(nTorsos*sizeof(torso));
      memcpy(pLimbs,pCur,nLimbs*sizeof(limb));
      pCur+=(nLimbs*sizeof(limb));
      *pScale=*((float *)(pCur));
   } else
   {
      failed=TRUE;
   }

   if (pData) {
      pRes->Unlock();
   }

   if (pRes) {
      SafeRelease(pRes);
   }

   if(failed) // fill in with dummy data
   {
      Warning(("creature.cpp: GetLengths - can't find lengths for obj %d\n",obj_id));
      *pScale=1.0;
      AssertMsg(pCDesc->pDefTorsos&&pCDesc->pDefLimbs,"creature.cpp: no default limb or torsos for type");
      memcpy(pTorsos,pCDesc->pDefTorsos,pCDesc->nTorsos*sizeof(torso));
      memcpy(pLimbs,pCDesc->pDefLimbs,pCDesc->nLimbs*sizeof(limb));
   }
}


cCreature::cCreature(int type, ObjID objID)
 : m_ObjID(objID),
   m_CreatureType(type),
   m_scalePhys(1.0),
   m_ballisticStyle(kCMB_Normal)
{
   Assert_(g_pCreatureDescPtr&&type<g_nCreatureTypes);

   const sCreatureDesc *pCDesc=g_pCreatureDescPtr[type];

   m_nJoints=g_pCreatureDescPtr[type]->nJoints;
   m_nPhysModels=g_pCreatureDescPtr[type]->nPhysModels;

// allocate necessary memory
//
   m_pJoints  = new mxs_vector[pCDesc->nJoints];
   m_pOrients = new mxs_matrix[pCDesc->nJoints];
   m_pTorsos  = new torso[pCDesc->nTorsos];
   m_pLimbs   = new limb[pCDesc->nLimbs];

   GetLengths(objID, pCDesc,m_pTorsos,m_pLimbs,&m_PrimScale);

   AssertMsg1(m_pTorsos[0].parent==-1,"Obj %d Torso does not follow convention",objID);
   m_iRootJoint=m_pTorsos[0].joint; // by convention

// setup multiped
//
   m_sMultiped.app_ptr=(void *)this;
//   m_sMultiped.app_ptr=(void *)objID;
   m_sMultiped.root_joint = m_iRootJoint;
   m_sMultiped.num_torsos = pCDesc->nTorsos;
   m_sMultiped.num_limbs  = pCDesc->nLimbs;
   m_sMultiped.num_joints = pCDesc->nJoints;
   m_sMultiped.torsos     = m_pTorsos;
   m_sMultiped.limbs      = m_pLimbs;
   m_sMultiped.joints     = m_pJoints;
   m_sMultiped.orients    = m_pOrients;
   // XXX TO DO: get rid of norm field in multiped struct, since never used.
   m_sMultiped.norm=NULL;

   // XXX should mp library do this?  it does orientation.. weird
   mx_zero_vec(&m_sMultiped.global_pos);
   mp_init_multiped(&m_sMultiped);

// initialize remaining data
//
   m_CreatureType=type;

   m_pMotorResolver=NULL;
   m_CompletedMotions=NULL;

   InitMotorState(&m_MotorState);

   const Position *pos=ObjPosGet(objID);

   if (pos)
   {
      MoveButt(&pos->loc.vec,&pos->fac,FALSE);

      ResetLengths();

      if(pCDesc->alwaysBallistic)
      {
         MakeBallistic(m_ballisticStyle, TRUE);  // do once, so makephysical does right thing.
      }
      else
      {
         MakeNonBallistic(TRUE);
      }

      ComputeBoundingData();
      m_DefaultButtZOffset=GetButtZOffset();
      m_MotorState.IsPhysical=MakePhysical();
   }
   else
   {
      Warning(("cCreature()#1: %s has no position\n", ObjWarnName(objID)));
   }

   MvrUtilsRegisterDefaultObjFlags(objID);

   m_trackingObj = OBJ_NULL;
   mx_zero_vec(&m_trackingLoc);
   mx_identity_mat(&m_headOrient);

   m_MaxAngVel = 0x3800;

   m_standingObj = OBJ_NULL;

   m_IsFrozen=FALSE;

}

///////////////////////////////////////////////////

static int MotSwizzle(int mot,motion_callback *pcallback)
{
   int i;
   sMotSwizzle *pSwizz;

   AssertMsg(g_pMotionSet,"can't swizzle mot when no motion set");
   if(pcallback)
      *pcallback=CreatureMotEndCallback;

   if(mot<0)
      return -1;

   // find swizzle entry for mot num
   pSwizz=g_pMotSwizzleMap;
   for(i=0;i<g_nSwizzleMots;i++,pSwizz++)
   {
      if(pSwizz->mot==mot)
         return g_pMotionSet->GetMotHandleFromName(&pSwizz->name);
   }
   return -1;
}

cCreature::cCreature(int type, ObjID objID, fCreatureReadWrite func, int version)
{
   AutoAppIPtr_(ObjectSystem,pObjSys);
   sCreatureLengths lengths;

   m_CreatureType=type;
   m_ObjID=objID;

   Assert_(g_pCreatureDescPtr&&m_CreatureType<g_nCreatureTypes);
   const sCreatureDesc *pCDesc=g_pCreatureDescPtr[m_CreatureType];

// allocate necessary memory
//
   m_pJoints = new mxs_vector[pCDesc->nJoints];
   m_pOrients= new mxs_matrix[pCDesc->nJoints];
   m_pTorsos = new torso[pCDesc->nTorsos];
   m_pLimbs = new limb[pCDesc->nLimbs];

   // Read Lengths
   switch (version)
   {
      case 10:
      case 9:
      case 8:
      case 7:
      case 6:
      case 5:
      case 4:
      case 3:
      {
         func((void *)&lengths.nTorsos,sizeof(int),1);
         func((void *)&lengths.nLimbs,sizeof(int),1);

         lengths.pTorsos = new torso[lengths.nTorsos];
         lengths.pLimbs = new limb[lengths.nLimbs];

         func((void *)lengths.pTorsos, sizeof(torso),lengths.nTorsos);
         func((void *)lengths.pLimbs, sizeof(limb),lengths.nLimbs);
         func((void *)&lengths.primScale, sizeof(float), 1);

         break;
      }

      case 2:
      {
         GetLengths(m_ObjID, pCDesc,m_pTorsos,m_pLimbs,&m_PrimScale);

         // read in old data so next read starts in right place, even though
         // it will be ignored
         func((void *)&lengths.nTorsos,sizeof(int),1);
         func((void *)&lengths.nLimbs,sizeof(int),1);

         lengths.pTorsos = new torso[lengths.nTorsos];
         lengths.pLimbs = new limb[lengths.nLimbs];

         // Load old torso
         int i;

         for (i=0; i<lengths.nTorsos; i++)
         {
            func((void *)&lengths.pTorsos[i].joint, sizeof(int), 1);
            func((void *)&lengths.pTorsos[i].parent, sizeof(int), 1);
            func((void *)&lengths.pTorsos[i].num_fixed_points, sizeof(int), 1);

            func((void *)lengths.pTorsos[i].joint_id, sizeof(int), 4);
            func((void *)lengths.pTorsos[i].pts, sizeof(mxs_vector), 4);
         }

         for (i=0; i<lengths.nLimbs; i++)
         {
             func((void *)&lengths.pLimbs[i].torso_id, sizeof(int), 1);
             func((void *)&lengths.pLimbs[i].bend, sizeof(int), 1);
             func((void *)&lengths.pLimbs[i].num_segments, sizeof(int), 1);

             func((void *)lengths.pLimbs[i].joint_id, sizeof(short), 5);
             func((void *)lengths.pLimbs[i].seg, sizeof(mxs_vector), 4);
             func((void *)lengths.pLimbs[i].seg_len, sizeof(mxs_real), 4);
         }

         func((void *)&lengths.primScale, sizeof(float), 1);

         break;
      }

      case 1:
      {
         AssertMsg(FALSE, "This is a really old level.  Creatures probably are broken.\n");

         GetLengths(m_ObjID, pCDesc,m_pTorsos,m_pLimbs,&m_PrimScale);

         // read in old data so next read starts in right place, even though
         // it will be ignored
         func((void *)&lengths.nTorsos,sizeof(int),1);
         func((void *)&lengths.nLimbs,sizeof(int),1);

         lengths.pTorsos = new torso[lengths.nTorsos];
         lengths.pLimbs = new limb[lengths.nLimbs];

         // old torso has one fewer fixed point
         func((void *)lengths.pTorsos, sizeof(torso)-((sizeof(mxs_vector)+sizeof(int)) * 13),lengths.nTorsos);
         // old torso has one fewer limb seg
         func((void *)lengths.pLimbs, sizeof(limb)-((sizeof(short)+sizeof(mxs_vector)+sizeof(mxs_real)) * 13),lengths.nLimbs);
         func((void *)&lengths.primScale, sizeof(float), 1);

         break;
      }

      default:
         CriticalMsg("Unknown version for loading creature lengths!\n");
   }

   if (version >= 4)
   {
      func((void *)&m_trackingObj, sizeof(ObjID), 1);
      m_trackingObj=pObjSys->RemapOnLoad(m_trackingObj);

      func((void *)&m_trackingLoc, sizeof(mxs_vector), 1);
      func((void *)&m_headOrient, sizeof(mxs_matrix), 1);
   }
   else
   {
      m_trackingObj = OBJ_NULL;
      mx_zero_vec(&m_trackingLoc);
      mx_identity_mat(&m_headOrient);
   }

   if (version == 4)
   {
      m_trackingObj = OBJ_NULL;
      mx_zero_vec(&m_trackingLoc);
      mx_identity_mat(&m_headOrient);
   }

   if (version >= 8)
      func((void *)&m_scalePhys, sizeof(float), 1);
   else
      m_scalePhys = 1.0;

   if (version >= 9)
      func((void *)&m_ballisticStyle, sizeof(int), 1);
   else
      m_ballisticStyle = kCMB_Normal;

   if(version >= 10)
      func((void *)&m_MaxAngVel, sizeof(fix), 1);
   else
      m_MaxAngVel = 0x3800;

   sCreatureLengths *pCreatureLengths=&lengths;

   // verify lengths data consistent with creature type
   //
   AssertMsg1(pCreatureLengths&&pCreatureLengths->nTorsos==pCDesc->nTorsos&&\
   pCreatureLengths->nLimbs==pCDesc->nLimbs,"Incompatable lengths for \
   creature type %d",m_CreatureType);

   memcpy(m_pTorsos,pCreatureLengths->pTorsos,pCDesc->nTorsos*sizeof(torso));
   memcpy(m_pLimbs,pCreatureLengths->pLimbs,pCDesc->nLimbs*sizeof(limb));
   m_PrimScale=pCreatureLengths->primScale;

   AssertMsg1(m_pTorsos[0].parent==-1,"Obj %d Torso does not follow convention",m_ObjID);
   m_iRootJoint=m_pTorsos[0].joint; // by convention

   // read in motor state
   InitMotorState(&m_MotorState);
   if(version>=6)
   {
      func((void *)&m_MotorState, sizeof(m_MotorState),1);
      // swizzle
      m_MotorState.focus=pObjSys->RemapOnLoad(m_MotorState.focus);
   }

// setup multiped
//
   m_sMultiped.app_ptr=(void *)this;
//   m_sMultiped.app_ptr=(void *)m_ObjID;
   m_sMultiped.root_joint=m_iRootJoint;
   m_sMultiped.num_torsos=pCDesc->nTorsos;
   m_sMultiped.num_limbs=pCDesc->nLimbs;
   m_sMultiped.num_joints=pCDesc->nJoints;
   m_sMultiped.torsos=m_pTorsos;
   m_sMultiped.limbs=m_pLimbs;
   m_sMultiped.joints=m_pJoints;
   m_sMultiped.orients=m_pOrients;
   // XXX TO DO: get rid of norm field in multiped struct, since never used.
   m_sMultiped.norm=NULL;

   // XXX should mp library do this?  it does orientation.. weird
   mx_zero_vec(&m_sMultiped.global_pos);
   mp_init_multiped(&m_sMultiped);

   // read in multiped save data
   if(version>=6)
   {
      int bufsize;

      func((void *)&bufsize,sizeof(int),1);
      int *buf=new int[bufsize];

      func((void *)buf,bufsize,1);

      mp_read_multiped(&m_sMultiped,buf,MotSwizzle);

      delete buf;
   }

// initialize remaining data
//
   m_nJoints=pCDesc->nJoints;
   m_nPhysModels=pCDesc->nPhysModels;

   m_pMotorResolver=NULL;
   m_CompletedMotions=NULL;

   m_IsFrozen=FALSE; // note this does not get saved/loaded


   const Position *pos=ObjPosGet(m_ObjID);

   if (pos)
   {
      // MoveButt will assume that the m_sMultiped position is where it is being
      // moved from, so we have to set it BEFORE calling MoveButt.
      mx_copy_vec(&m_sMultiped.global_pos, &pos->loc.vec);
      mx_mk_rot_z_mat(&m_sMultiped.global_orient, pos->fac.tz);

      MoveButt(&pos->loc.vec,&pos->fac,FALSE);

      ResetLengths();

      if (IsBallistic())
         MakeBallistic(m_ballisticStyle, TRUE);
      else
         MakeNonBallistic(TRUE);

      ComputeBoundingData();
      m_DefaultButtZOffset=GetButtZOffset();
      if (m_MotorState.IsPhysical)
      {
         MakePhysical();
      }
   }
   else
   {
      Warning(("cCreature()#2: %s has no position\n",ObjWarnName(m_ObjID)));
   }

   delete lengths.pTorsos;
   delete lengths.pLimbs;

   MvrUtilsRegisterDefaultObjFlags(m_ObjID);
}

///////////////////////////////////////////////////

cCreature::~cCreature()
{
   HackProcessCompletedMotions();
   if (CreatureWeaponObjDestroy)
   {
      (*CreatureWeaponObjDestroy)(m_ObjID);
   }

   if(m_pMotorResolver)
   {
      m_pMotorResolver->NotifyAboutMotorDestruction();
   }

   MakeNonPhysical();

   MvrUtilsUnRegisterDefaultObjFlags(m_ObjID);

   mp_stop_all_motions(&m_sMultiped);
   mp_free_multiped(&m_sMultiped);

   delete m_pJoints;
   delete m_pOrients;
   delete m_pTorsos;
   delete m_pLimbs;
}

///////////////////////////////////////////////////

void cCreature::SetMotorResolver(IMotorResolver *pResolver)
{
   m_pMotorResolver=pResolver;
//   m_sMultiped.app_ptr=(void *)pResolver;
}

///////////////////////////////////////////////////

int cCreature::GetActorType()
{
   Assert_(g_pCreatureDescPtr&&m_CreatureType>=0&&m_CreatureType<g_nCreatureTypes);
   return g_pCreatureDescPtr[m_CreatureType]->actorType;
}

///////////////////////////////////////////////////

void cCreature::Write(fCreatureReadWrite func)
{
   // Write General Info, for sanity checking
   func((void *)&m_ObjID, sizeof(m_ObjID),1);
   func((void *)&m_CreatureType, sizeof(m_CreatureType),1);

   // Write Lengths
   func((void *)&m_sMultiped.num_torsos,sizeof(m_sMultiped.num_torsos),1);
   func((void *)&m_sMultiped.num_limbs,sizeof(m_sMultiped.num_limbs),1);
   func((void *)m_pTorsos, sizeof(*m_pTorsos),m_sMultiped.num_torsos);
   func((void *)m_pLimbs, sizeof(*m_pLimbs),m_sMultiped.num_limbs);
   func((void *)&m_PrimScale, sizeof(m_PrimScale), 1);

   // Write tracking stuff
   func((void *)&m_trackingObj, sizeof(ObjID), 1);
   func((void *)&m_trackingLoc, sizeof(mxs_vector), 1);
   func((void *)&m_headOrient, sizeof(mxs_matrix), 1);

   func((void *)&m_scalePhys, sizeof(float), 1);
   func((void *)&m_ballisticStyle, sizeof(int), 1);

   func((void *)&m_MaxAngVel,sizeof(fix),1);

   // Write motor state
   m_MotorState.IsPhysical = PhysObjHasPhysics(m_ObjID);
   func((void *)&m_MotorState, sizeof(m_MotorState),1);

   // Write multiped motion stuff
   const int msize=mp_get_write_buffsize(&m_sMultiped);
   func((void *)&msize,sizeof(msize),1);
   uchar *buf= new uchar[msize];
   mp_write_multiped(&m_sMultiped, (void *)buf);
   func((void *)buf,msize,1);
   delete buf;
}

///////////////////////////////////////////////////

void cCreature::ResetLengths()
{
   quat *rot = new quat[m_sMultiped.num_joints+1];
   mxs_vector v;

   Assert_(g_pCreatureDescPtr&&m_CreatureType<g_nCreatureTypes);

   GetLengths(m_ObjID,g_pCreatureDescPtr[m_CreatureType],m_pTorsos,m_pLimbs,&m_PrimScale);

   Assert_(rot);

   // store off current rotations
   memcpy(rot,m_sMultiped.rel_orients,(m_sMultiped.num_joints+1)*sizeof(*rot));
   quat_from_matrix(&rot[m_sMultiped.num_joints],&m_sMultiped.global_orient);
   mx_zero_vec(&v);

   // want buttzoffset of multiped in calibration position, which may be
   // very different from current position (like if creature was posed).
   mp_initial_update(&m_sMultiped);
   ComputeBoundingData();
   m_DefaultButtZOffset=GetButtZOffset();

   mp_apply_motion(&m_sMultiped,rot,&v,-1);

   ComputeBoundingData();

   if (m_CreatureType != 10) // rope magic number
   {
      const Position *pPos=ObjPosGet(m_ObjID);
      SetCretObjPos(m_ObjID,&pPos->loc.vec,&pPos->fac); // this needed to update brush and refs
   }
   delete rot;
}

///////////////////////////////////////////////////

// NOTE: this doesn't do any physics checking to see if this is safe, so use
// at your own peril.  phys models get teleported, bbox gets updated etc.
void cCreature::PoseAtMotionFrame(int motNum, float frame)
{
   mps_motion_node node;
   mxs_vector xlat;

   if(motNum<0||motNum>mp_num_motions)
      return;
   if(frame<0||frame>mp_motion_list[motNum].info.num_frames-1)
   {
      frame=mp_motion_list[motNum].info.num_frames-1;
   }

   quat *rot=new quat[m_sMultiped.num_joints+1];

   // intialize xlat and rots, in case motion is an overlay
   mx_zero_vec(&xlat);
   int i;
   for(i=0;i<m_sMultiped.num_joints+1;i++)
   {
      quat_identity(&rot[i]);
   }

   memset(&node,0,sizeof(node));
   node.frame=frame;
   node.handle=motNum;

   // want these to be zero if global frame of ref (or special MFRT_Arm frame of reference).
   if(GetMotFrameOfReference()!=kMFRT_RelStart)
   {
      mx_zero_vec(&node.base_pos);
      quat_identity(&node.base_orient);
   } else
   {
      mx_copy_vec(&node.base_pos,&m_sMultiped.global_pos);
      quat_from_matrix(&node.base_orient,&m_sMultiped.global_orient);
   }

   MotDmngeLock(motNum);
   mp_evaluate_motion(&m_sMultiped,&node,rot,&xlat);
   mx_zero_vec(&xlat); // want creature to stay in place.
   mp_apply_motion(&m_sMultiped,rot,&xlat,-1);
   MotDmngeUnlock(motNum);

   ComputeBoundingData();
   if(PhysObjHasPhysics(m_ObjID)&&!IsBallistic())
      SlamPhysicsModels();
   CreatureAttachmentsPosUpdate(m_ObjID);

   delete rot;
}

///////////////////////////////////////////////////

int cCreature::SwizzleMotion(int motNum)
{
   return MotSwizzle(motNum,NULL);
}

///////////////////////////////////////////////////

int cCreature::GetCurrentMotion()
{
   int mot,maxmot=1;
   mp_get_multiped_motions(&m_sMultiped,&mot,&maxmot);
   if(maxmot)
      return mot;
   return -1;
}

///////////////////////////////////////////////////

// XXX TO DO: make this faster.  what are the fancy float things to do again?
void cCreature::ComputeBoundingData()
{
   mxs_vector bigun,max_off,min_off;
   mxs_vector *j;
   mxs_vector *bmin=&m_BMin;
   mxs_vector *bmax=&m_BMax;
   int i;

   mx_copy_vec(bmin,&m_pJoints[m_iRootJoint]);
   mx_copy_vec(bmax,&m_pJoints[m_iRootJoint]);

   j=m_pJoints;

   for(i=0;i<m_nJoints;i++,j++)
   {
      if(j->x<bmin->x)
         bmin->x=j->x;
      else if (j->x>bmax->x)
         bmax->x=j->x;
      if(j->y<bmin->y)
         bmin->y=j->y;
      else if (j->y>bmax->y)
         bmax->y=j->y;
      if(j->z<bmin->z)
         bmin->z=j->z;
      else if (j->z>bmax->z)
         bmax->z=j->z;
   }

   // calc radius
   mx_sub_vec(&max_off,bmax,&m_pJoints[m_iRootJoint]);
   mx_sub_vec(&min_off,bmin,&m_pJoints[m_iRootJoint]);
   for(i=0;i<3;i++)
   {
      if(fabs(max_off.el[i])>fabs(min_off.el[i]))
         bigun.el[i]=max_off.el[i];
      else
         bigun.el[i]=min_off.el[i];
   }
   m_Radius=mx_mag_vec(&bigun);
}

///////////////////////////////////////////////////

// XXX don't want this really.  Want offset computed by motion processor
inline void cCreature::GetFeetJointIDs(int *pnJoints, int **ppJointIDs) const
{
   Assert_(g_pCreatureDescPtr&&m_CreatureType<g_nCreatureTypes);
   *pnJoints=g_pCreatureDescPtr[m_CreatureType]->nFeet;
   *ppJointIDs=(int *)g_pCreatureDescPtr[m_CreatureType]->footJoints;
}

///////////////////////////////////////////////////

void cCreature::RecomputeJointPositions()
{
   quat *rot = new quat[m_sMultiped.num_joints+1];
   mxs_vector v;

   if(rot)
   {
      memcpy(rot,m_sMultiped.rel_orients,m_sMultiped.num_joints*sizeof(*rot));
      // global_orient and rel orient for root may be out of synch due to
      // MoveButt calls.  compensate here instead of in movebutt, because
      // otherwise movements are jerky
      quat_from_matrix(&rot[m_sMultiped.num_joints],&m_sMultiped.global_orient);
      mx_zero_vec(&v);
      mp_apply_motion(&m_sMultiped,rot,&v,-1);
      delete rot;
   } else
   {
      mp_initial_update(&m_sMultiped);
   }
}

///////////////////////////////////////////////////

// only valid for normalized vector
// WARNING: assumes normalized vec in X/Y plane.
static mxs_ang GetNormVecHeading(mxs_vector *vec)
{
   mxs_ang angle;

   angle=mx_rad2ang(acos(vec->x));
   if(angle>0)
   {
      if(vec->y<0) // -pi/2 < actual angle < 0
         angle=-angle;
   } else if (vec->y>0) // -pi < actual angle <= -pi/2
      angle=-angle;
   return angle;
}

static mxs_ang GetHeadingAngle(mxs_matrix *m)
{
   mxs_vector vec = m->vec[0];
   // Let's make this NOT quite zero, so normeq div0 won't occur.
   vec.z = 0.000001; // make sure it normalizes on X/Y plane. 'diff' is not used for anything else.
   mx_normeq_vec(&vec);

   return GetNormVecHeading(&vec);
}

#define CREATURE_FAC_ERR 0x0003

BOOL cCreature::MoveButt(const mxs_vector *pButtPos, const mxs_angvec *pAngles, BOOL updatePhysics, BOOL allowTilt)
{
   int i;
   mxs_vector *pJnt;
   mxs_vector d,old, trav;
   mxs_matrix mat,delt;
   quat dQ, tmp;
   mps_stack_node *mps;

   ConfigSpew("CreaturePosTrace",("%d moveButt: to %g, %g, %g, %x, mph=%d\n",m_ObjID,pButtPos->x,pButtPos->y,pButtPos->z,pAngles->tz,updatePhysics));

   mxs_vector new_butt_pos = *pButtPos;

   mx_copy_vec(&old,&m_sMultiped.global_pos);
   mx_copy_vec(&m_sMultiped.global_pos,&new_butt_pos);

   if(allowTilt)
   {
      mxs_matrix newor;

      mx_ang2mat(&newor,pAngles);
      mx_mult_mat(&mat,&newor,&m_sMultiped.global_orient);  // @TODO: this or tmul_mat?
      mx_copy_mat(&m_sMultiped.global_orient,&newor);
   } else
   {
      mxs_ang angle=GetHeadingAngle(&m_sMultiped.global_orient);

      if(((angle-pAngles->tz)>=0&&(angle-pAngles->tz)<CREATURE_FAC_ERR) || \
         ((angle-pAngles->tz)<0&&(angle-pAngles->tz)>-CREATURE_FAC_ERR))
      {
         mx_identity_mat(&mat);
      } else
      {
         mx_mk_rot_z_mat(&mat,pAngles->tz);
         mx_copy_mat(&m_sMultiped.global_orient,&mat);
         mx_mk_rot_z_mat(&delt,(mxs_ang)(-angle));
         mx_muleq_mat(&mat,&delt);
      }
   }

   mx_sub_vec(&d,&new_butt_pos,&old);

   quat_from_matrix(&dQ,&mat);
   mps=m_sMultiped.main_motion.head;
   while(mps)
   {
      if(mps->type==MN_MOTION)
      {
         // XXX TO DO: only modify if not a virtual motion

         mx_sub_vec(&trav,&old,&((mps_motion_node *)mps)->base_pos);
         mx_mat_muleq_vec(&mat,&trav);
         mx_sub_vec(&(((mps_motion_node *)mps)->base_pos),&new_butt_pos,&trav);
         //         mx_addeq_vec(&((mps_motion_node *)mps)->base_pos,&d);
         quat_copy(&tmp,&((mps_motion_node *)mps)->base_orient);
         quat_mul(&((mps_motion_node *)mps)->base_orient,&dQ,&tmp);
      }
      mps=mps->next;
   }

   pJnt=m_pJoints;
   for(i=0;i<m_nJoints;i++,pJnt++)
   {
      mx_sub_vec(&d,pJnt,&old);
      mx_mat_muleq_vec(&mat,&d);
      mx_add_vec(pJnt, &d, &m_sMultiped.global_pos);
      mx_mul_mat(&delt,&mat,&m_pOrients[i]);
      mx_copy_mat(&m_pOrients[i],&delt);
   }

   ComputeBoundingData();

   if (!g_SimRunning)
   {
      if (CreatureFromObj(m_ObjID) != NULL)
      {
         int head_joint = GetCreatureJointID(m_ObjID, kCJ_Neck);
         mx_copy_mat(&m_headOrient, &m_pOrients[head_joint]);
      }
      else
         mx_copy_mat(&m_headOrient, &m_sMultiped.global_orient);
   }

   ValidateCreaturePosition(m_ObjID);

   if(updatePhysics && PhysObjHasPhysics(m_ObjID) && !g_SimRunning) //&& IsBallistic()))
   {
      SlamPhysicsModels();
   }

   ValidateCreaturePosition(m_ObjID);

   // "floor" remote ghosts
   if (IsRemoteGhost(m_ObjID) && (CreatureFromObj(m_ObjID) != NULL))
   {
      Location start, end, hit;
      ObjID hit_obj;

      int filter = kCollideSphereHat | kCollideOBB | kCollideTerrain;

      if (GhostIsFloored(m_ObjID))
      {
         MakeHintedLocationFromVector(&start, pButtPos, &ObjPosGet(m_ObjID)->loc);
         MakeLocationFromVector(&end, pButtPos);
         end.vec.z -= 11.0;

         // Cast down to find floor height
         if (PhysRaycast(start, end, &hit, &hit_obj, 0, filter) != kCollideNone)
         {
            float height = start.vec.z - hit.vec.z;

            if ((height > 0) && (height < 10))
            {
               // Find height from butt to feet
               float left_butt_height, right_butt_height;

               mxs_vector butt, foot_left, foot_right;

               mx_copy_vec(&butt, &GetCreatureJointPos(m_ObjID, kCJ_Butt));
               mx_copy_vec(&foot_left, &GetCreatureJointPos(m_ObjID, kCJ_LToe));
               mx_copy_vec(&foot_right, &GetCreatureJointPos(m_ObjID, kCJ_RToe));

               left_butt_height = butt.z - foot_left.z;
               right_butt_height = butt.z - foot_right.z;

               if ((left_butt_height > 0) || (right_butt_height > 0))
               {
                  if (left_butt_height > right_butt_height)
                     new_butt_pos.z = hit.vec.z + left_butt_height;
                  else
                     new_butt_pos.z = hit.vec.z + right_butt_height;
               }
               else
               {
                  new_butt_pos.z = hit.vec.z + 1.0; // @TBD: lift a little here?
               }
            }
         }
      }
      else
      {
         MakeHintedLocationFromVector(&start, pButtPos, &ObjPosGet(m_ObjID)->loc);

         mxs_real left_z = 0;
         mxs_real right_z = 0;

         MakeLocationFromVector(&end, &GetCreatureJointPos(m_ObjID, kCJ_LToe));
         if (PhysRaycast(start, end, &hit, &hit_obj, 0, filter) != kCollideNone)
            left_z = hit.vec.z - end.vec.z;

         MakeLocationFromVector(&end, &GetCreatureJointPos(m_ObjID, kCJ_RToe));
         if (PhysRaycast(start, end, &hit, &hit_obj, 0, filter) != kCollideNone)
            right_z = hit.vec.z - end.vec.z;

         new_butt_pos.z += max(left_z, right_z);
      }

      mx_copy_vec(&old,&m_sMultiped.global_pos);
      mx_copy_vec(&m_sMultiped.global_pos,&new_butt_pos);
      mx_sub_vec(&d,&new_butt_pos,&old);

      pJnt=m_pJoints;
      for(i=0;i<m_nJoints;i++,pJnt++)
      {
         mx_sub_vec(&d,pJnt,&old);
         mx_mat_muleq_vec(&mat,&d);
         mx_add_vec(pJnt, &d, &m_sMultiped.global_pos);
         mx_mul_mat(&delt,&mat,&m_pOrients[i]);
         mx_copy_mat(&m_pOrients[i],&delt);
      }

      ComputeBoundingData();

      ValidateCreaturePosition(m_ObjID);

      if(updatePhysics && PhysObjHasPhysics(m_ObjID) && !g_SimRunning) //&& IsBallistic()))
      {
         SlamPhysicsModels();
      }

      ValidateCreaturePosition(m_ObjID);
   }

   mx_copy_vec(&m_MotorState.position,&new_butt_pos);
   m_MotorState.facing=*pAngles;
   return TRUE;
}

///////////////////////////////////////////////////

void cCreature::SlamPhysicsModels()
{
   int i;

   Assert_(g_pCreatureDescPtr && m_CreatureType<g_nCreatureTypes);
   const sCreatureDesc *pCDesc = g_pCreatureDescPtr[m_CreatureType];

   if (m_CreatureType == 10) // rope magic number
      return;

   ConfigSpew("BipedPhys",("%d: forcing phys model pos\n",m_ObjID));

   cPhysModel *pModel;
   if ((pModel = g_PhysModels.Get(m_ObjID)) == NULL)
      return;

   PosPropLock++;

   gCrPosPropLock++;

   PhysSetModLocation(m_ObjID, &m_sMultiped.global_pos);
   PhysSetModRotation(m_ObjID, &ObjPosGet(m_ObjID)->fac);

   if (!IsMovingTerrain(m_ObjID)&&!(IsBallistic()&&m_ballisticStyle==kCMB_Corpse))
   {
      for(i=0; i<pCDesc->nPhysModels && i<pModel->NumSubModels(); i++)
      {
         mxs_vector subPos;

         GetPhysSubModelPos(m_pJoints, &pCDesc->pCrPhysModOffsets[i], &subPos);

         if (IsBallistic())
            PhysSetSubModLocation(m_ObjID, i, &subPos);
         else
            PhysControlSubModLocation(m_ObjID, i, &subPos);

         ConfigSpew("BipedPhys",("sub %d: %g,%g,%g\n",i,subPos.x,subPos.y,subPos.z));
      }
   }

   gCrPosPropLock--;

   ValidateCreaturePosition(m_ObjID);

   PosPropLock--;
}

///////////////////////////////////////////////////


#define MAX_WORLD_SIZE 60000

// Only to be called internall by creature system
void SetCretObjPos(ObjID objID, const mxs_vector *pos, const mxs_angvec *rot)
{
   // check position is valid
   AssertMsg3(pos->x>-MAX_WORLD_SIZE&&pos->x<MAX_WORLD_SIZE&&\
      pos->y>-MAX_WORLD_SIZE&&pos->y<MAX_WORLD_SIZE,\
      "SetCretObjPos: invalid new position for %d: %g, %g\n",
              objID, pos->x, pos->y);

   PosPropLock++;
   g_CreatureIsMovingObj = TRUE;  // tell PosPropListener not to bother.
   ObjPosUpdateUnsafe(objID,pos,rot);
   PhysUpdateRefsObj(objID);
   g_CreatureIsMovingObj = FALSE;
   PosPropLock--;
}

BOOL cCreature::UpdateObjPosition()
{
   mxs_angvec fac;

   fac.tz = GetHeadingAngle(&m_sMultiped.global_orient);
   fac.tx = fac.ty = 0;

   SetCretObjPos(m_ObjID,&m_sMultiped.global_pos,&fac);

   mx_copy_vec(&m_MotorState.position,&m_sMultiped.global_pos);
   m_MotorState.facing=fac;

   return TRUE;
}

///////////////////////////////////////////////////

// for when you don't want to blend to new global position.
void cCreature::FlushRelativeState()
{
   mx_copy_vec(&m_sMultiped.rel_xlat,&m_sMultiped.global_pos);
}

///////////////////////////////////////////////////
static BOOL GetMpMainMotionBaseOrient(multiped *mp,quat **ppRot,mxs_vector **ppPos)
{
   mps_stack_node *pStackNode;

   pStackNode=mp->main_motion.head;
   while(pStackNode)
   {
      if(pStackNode->type==MN_MOTION)
      {
         if(((mps_motion_node *)pStackNode)->handle>0) // so not a pose
         {
            *ppRot= &(((mps_motion_node *)pStackNode)->base_orient);
            *ppPos= &(((mps_motion_node *)pStackNode)->base_pos);
            return TRUE;
         }
      }
      pStackNode=pStackNode->next;
   }
   return FALSE;
}

/////////////////////////////////////////////////////////

#define kMaxAngVel 0x3800   // in mxs_ang per sec

void cCreature::FilterMotionFrameData(const mps_motion_info *mi, quat *rot, mxs_vector *xlat)
{
#if 0
   ObjID focus=m_MotorState.focus;

   focus = gPlayerObj;

   // @TODO: focus stuff for head-turning
   // rotate base orientation to face focus.  (where base orientation is
   // starting orientation of motion.)
   // @NOTE that this won't work correctly for turns or motions that are being
   // bent, since global orientation varies over course of these motions instead
   // of always being base orientation.
   if(focus!=OBJ_NULL) // @TODO: don't use for ballistic creatures?
   {
      // set orientation of base joint

      quat *pBaseOrient;
      mxs_vector *pBasePos;

      if(GetMpMainMotionBaseOrient(&m_sMultiped,&pBaseOrient,&pBasePos))
      {
         ObjPos *pPos=ObjPosGet(focus); // @OPTIMIZE: store this off somewhere instead of computing every update
         // compute angle to focus obj
         mxs_vector diff;
         mxs_ang desFac,curFac,delta;
         mxs_matrix mat,mat2;
         fix maxTurn;  // needs to be fix due to overflow
         BOOL negRot=FALSE;

         mx_sub_vec(&diff,&pPos->loc.vec,&m_sMultiped.global_pos);
         // Let's make this NOT quite zero, so normeq div0 won't occur.
         diff.z = 0.000001; // make sure it normalizes on X/Y plane. 'diff' is not used for anything else.
         mx_normeq_vec(&diff);
         desFac=GetNormVecHeading(&diff);

         // compare to actual heading and clamp.
         curFac=GetHeadingAngle(&m_sMultiped.global_orient);
         delta=desFac-curFac;
         if(delta>MX_ANG_PI)
         {
            delta=-delta;
            negRot=TRUE;
         }
#if 0
         // @TODO: get msec from somewhere, do clamping
         maxTurn=fix_mul(kMaxAngVel,(fix_from_float(time_delta_ms/1000)));
         if(delta>maxTurn) // clamp
         {
            delta=maxTurn;
         }
#endif
         if(negRot)
            delta=-delta;
         desFac=curFac+delta;

         // modify base rotation
         // @TODO: take out base rotation
         quat rotOff;
         mxs_matrix matOff;

         GetFocusRotOffset(&rotOff,pBaseOrient,rot);
         quat_to_matrix(&matOff,&rotOff);
         mx_transeq_mat(&matOff);
         mx_mk_rot_z_mat(&mat,desFac);
         mx_mul_mat(&mat2,&matOff,&mat);
         quat_from_matrix(&rot[GetFocusJoint()],&mat2);
//         quat_from_matrix(pBaseOrient,&mat);
      }
   }
#endif
}

///////////////////////////////////////////////////

void cCreature::PostUpdateFilter(const ulong dt)
{
   const sCreatureDesc *pCDesc = g_pCreatureDescPtr[m_CreatureType];

   if (m_CreatureType == 10) // rope magic number
   {
      cPhysModel *pModel;
      int i;

      if ((pModel = g_PhysModels.Get(m_ObjID)) == NULL)
      {
         Warning(("cCreature::PostUpdateFilter: rope (obj %d) has no physics model?\n", m_ObjID));
         return;
      }

      mxs_matrix orien;
      mxs_vector vec;

      for (i=0; i<pModel->NumSubModels() + 1; i++)
      {
         Assert_(i < m_nJoints);

         if (i == 8)
            mx_copy_vec(&m_pJoints[8], &m_pJoints[7]);
         else
            mx_copy_vec(&m_pJoints[i], &pModel->GetLocationVec(i));

         // Root orientation is always the identity
         if (i >= 7)
         {
            mx_copy_mat(&m_pOrients[i], &m_pOrients[i-1]);
            mx_identity_mat(&m_pOrients[0]);
         }
         else
         {
            // Build matrix
            mx_sub_vec(&orien.vec[2], &pModel->GetLocationVec(i+1), &pModel->GetLocationVec(i));
            mx_normeq_vec(&orien.vec[2]);

            mx_unit_vec(&vec, 1);
            mx_cross_vec(&orien.vec[0], &vec, &orien.vec[2]);
            mx_normeq_vec(&orien.vec[0]);

            mx_cross_vec(&orien.vec[1], &orien.vec[2], &orien.vec[0]);
            mx_normeq_vec(&orien.vec[1]);
            mx_copy_mat(&m_pOrients[i], &orien);
         }
      }
   }
   else
   if (pCDesc->canHeadTrack)
   {
      if (m_trackingObj == 0)
         m_trackingObj = m_MotorState.focus;

      #define sq(x)  ((x) * (x))

      int head_joint_id = GetCreatureJointID(m_ObjID, kCJ_Neck);
      int neck_joint_id = GetCreatureJointID(m_ObjID, kCJ_Abdomen);

      mxs_matrix *head_mat = &m_pOrients[head_joint_id];
      mxs_matrix *neck_mat = &m_pOrients[neck_joint_id];

      mxs_matrix head_delta;
      mxs_real   head_track_rate = 100.0;

      if ((m_trackingObj == -1) || (m_trackingObj > 0))
      {
         if (!PlayerObjectExists())
            return;

         // Find vector to our target
         mxs_vector to_target;

         if (m_trackingObj == PlayerObject())
            PhysGetSubModLocation(PlayerObject(), PLAYER_HEAD, &to_target);
         else
         if (m_trackingObj > 0)
         {
            ObjPos * pPos = ObjPosGet(m_trackingObj);
            if (!pPos)
            {
               m_trackingObj = NULL;
               return;
            }
            to_target = pPos->loc.vec;
         }
         else
            to_target = m_trackingLoc;

         mx_subeq_vec(&to_target, &GetCreatureJointPos(m_ObjID, kCJ_Head));
         if (mx_mag2_vec(&to_target) < 0.0001)
            return;

         mx_normeq_vec(&to_target);

         const float kMinXYNeckDot = 0.0;
         const float kMaxZNeckDiff = 0.8;

         mxs_vector xy_to_target;
         mxs_vector xy_neck;

         mx_mk_vec(&xy_to_target, to_target.x, to_target.y, 0);
         mx_mk_vec(&xy_neck, neck_mat->vec[0].x, neck_mat->vec[0].y, 0);

         mx_normeq_vec(&xy_to_target);
         mx_normeq_vec(&xy_neck);

         mxs_real xy_dot = mx_dot_vec(&xy_to_target, &xy_neck);
         mxs_real z_diff = fabs(to_target.z - neck_mat->vec[0].z);

         // See if we've gone too far
         if ((xy_dot > kMinXYNeckDot) && (z_diff < kMaxZNeckDiff))
         {
            if ((fabs(to_target.z) > 0.999) || (mx_mag2_vec(&to_target) < 0.001))
               return;

            // Build new head orientation matrix
            mxs_matrix new_head_orient;
            mxs_vector z_vec;

            mx_unit_vec(&z_vec, 2);

            mx_copy_vec(&new_head_orient.vec[0], &to_target);
            mx_cross_vec(&new_head_orient.vec[1], &z_vec, &new_head_orient.vec[0]);
            mx_normeq_vec(&new_head_orient.vec[1]);
            mx_cross_vec(&new_head_orient.vec[2], &new_head_orient.vec[0], &new_head_orient.vec[1]);
            mx_normeq_vec(&new_head_orient.vec[2]);

            for (int i=0; i<3; i++)
               mx_sub_vec(&head_delta.vec[i], &new_head_orient.vec[i], &m_headOrient.vec[i]);
         }
         else
         {
            for (int i=0; i<3; i++)
               mx_sub_vec(&head_delta.vec[i], &head_mat->vec[i], &m_headOrient.vec[i]);
         }
      }
      else
      {
         for (int i=0; i<3; i++)
            mx_sub_vec(&head_delta.vec[i], &head_mat->vec[i], &m_headOrient.vec[i]);

         head_track_rate = 100.0;
      }

      float scale_amt;

      scale_amt = ((float)dt) / head_track_rate;
      if (scale_amt > 1.0)
         scale_amt = 1.0;

      // Move head towards desired orientation
      for (int i=0; i<3; i++)
      {
         mx_scaleeq_vec(&head_delta.vec[i], scale_amt);
         mx_add_vec(&m_headOrient.vec[i], &m_headOrient.vec[i], &head_delta.vec[i]);
         mx_normeq_vec(&m_headOrient.vec[i]);
      }
   }
}

///////////////////////////////////////////////////

static void mp_get_current_motrel_xlat(mxs_vector *dst, multiped *mp,mps_motion_node *mn)
{
   mps_motion_param * p;
   mps_motion * m;
   int cur_frame, motion_num;

   // Get translational distance at start frame.

	if(!mn)
      return;

   cur_frame = (int) floor(mn->frame + 0.5);
   motion_num = MP_MOTNUM_FROM_NODE_HANDLE(mn->handle);
   p = &mn->params;

   m = mp_motion_list + motion_num;

   mp_get_xlat_at_frame(dst, motion_num, cur_frame);

   if (p)
   {
      if (p->flags & MP_BEND)
      {
         mxs_matrix mat;

         mx_mk_rot_z_mat(&mat, mx_rad2ang(p->bend));
         mx_mat_muleq_vec(&mat, dst);
      }
      if (p->flags & MP_STRETCH) // only gets applied to x and y components
      {
//         mx_scaleeq_vec(dst, p->stretch);
         dst->x*=p->stretch;
         dst->y*=p->stretch;
      }
      if(p->flags & MP_VSTRETCH)
      {
         dst->z*=p->vstretch;
      }
      if(p->flags & MP_VINC)
      {
         dst->z += (float)(cur_frame-mn->start_frame)*
            (p->vinc/(m->info.num_frames-1-mn->start_frame));
      }
   }
}

///////////////////////////////////////////////////

BOOL cCreature::SetTransform(const mxs_vector *pButtPos, const mxs_angvec *pAngles, BOOL movePhys)
{
   if(IsBallistic()) // creature not self-controlling, so can't set own pos
      return FALSE;
   return MoveButt(pButtPos,pAngles,movePhys)&&UpdateObjPosition();
}

///////////////////////////////////////////////////

BOOL cCreature::SetPosition(const mxs_vector *pButtPos, BOOL movePhys)
{
   mxs_vector opos;
   mxs_angvec ofac;

   if(IsBallistic()) // creature not self-controlling, so can't set own pos
      return FALSE;
   GetTransform(&opos,&ofac);
   return MoveButt(pButtPos,&ofac,movePhys)&&UpdateObjPosition();
}

///////////////////////////////////////////////////

BOOL cCreature::SetOrientation(const mxs_angvec *pAngles, BOOL movePhys)
{
   mxs_vector opos;
   mxs_angvec ofac;

   if(IsBallistic()) // creature not self-controlling, so can't set own rot
      return FALSE;
   GetTransform(&opos,&ofac);
   ConfigSpew("CreaturePosTrace",("%d setOrient: rotating to %x\n",m_ObjID,pAngles->tz));
   return MoveButt(&opos,pAngles,movePhys)&&UpdateObjPosition();
}

///////////////////////////////////////////////////

void cCreature::GetTransform(mxs_vector *pButtPos, mxs_angvec *pAngles)
{
   Position *pPos=ObjPosGet(m_ObjID);

   AssertMsg1(pPos,"No position for obj %d",m_ObjID);

   *pButtPos=pPos->loc.vec;
   *pAngles=pPos->fac;
}

///////////////////////////////////////////////////

BOOL cCreature::MakePhysical()
{
   Assert_(g_pCreatureDescPtr&&m_CreatureType<g_nCreatureTypes);

   // don't to anything if property says it is never physical
   if(!g_NonPhysOverride&&ObjIsNonPhysicalCreature(m_ObjID))
      return FALSE;

   if (PhysObjHasPhysics(m_ObjID))
   {
      cPhysModel *pModel;

      if ((pModel = g_PhysModels.Get(m_ObjID)) != NULL)
         pModel->SetFlagState(kPMF_Creature, TRUE);

      return TRUE;
   }

   const sCreatureDesc *pCDesc=g_pCreatureDescPtr[m_CreatureType];
   int i;
   ObjID objID=m_ObjID;
   ulong flags=NULL;

   if(!pCDesc->nPhysModels)
      return FALSE;

   // create new phys models
   PhysRegisterSphere(objID,pCDesc->nPhysModels,flags,pCDesc->physRadius);
   cPhysModel *pModel = g_PhysModels.Get(objID);

   if (pModel == NULL)
      return FALSE;

   pModel->SetFlagState(kPMF_Creature, TRUE);

   if (pCDesc->hasSpecialPhysics)
   {
      pModel->SetFlagState(kPMF_Special, TRUE);
      UpdatePhysProperty(objID, PHYS_TYPE);
   }

   SetPhysSubModScale(m_scalePhys);

   SlamPhysicsModels();

   for(i=0;i<pCDesc->nPhysModels && i<pModel->NumSubModels();i++)
   {
      // activate location control
      if(PhysSubModIsBallistic(i))
         MakePhysSubModBallistic(i, m_ballisticStyle);
   }

   m_MotorState.IsPhysical=PhysObjHasPhysics(objID);

   ValidateCreaturePosition(m_ObjID);

   #ifndef SHIP
   if (config_is_defined("CreatureBallisticSpew"))
   {
      mprintf("%s physical\n", ObjWarnName(m_ObjID));
      cPhysModel *pModel = g_PhysModels.Get(m_ObjID);
      if (pModel)
      {
         for (int j=0; j<pModel->NumSubModels(); j++)
            mprintf(" [%d] %g %g %g\n", j, pModel->GetLocationVec(j).x,
                    pModel->GetLocationVec(j).y, pModel->GetLocationVec(j).z);
      }
   }
   #endif

   // Attach the weapon representation to the creature.
   if (CreatureWeaponObjSwizzle)
   {
      ((*CreatureWeaponObjSwizzle)(m_ObjID, kWeaponModeIdle));
   }

   return m_MotorState.IsPhysical;
}

///////////////////////////////////////////////////

void cCreature::MakeNonPhysical()
{
   if (!PhysObjHasPhysics(m_ObjID))
      return;

   ValidateCreaturePosition(m_ObjID);

#ifndef SHIP
   if (config_is_defined("CreatureBallisticSpew"))
   {
      mprintf("%s going non-physical\n", ObjWarnName(m_ObjID));
      cPhysModel *pModel = g_PhysModels.Get(m_ObjID);
      if (pModel)
      {
         for (int j=0; j<pModel->NumSubModels(); j++)
            mprintf(" [%d] %g %g %g\n", j, pModel->GetLocationVec(j).x,
                    pModel->GetLocationVec(j).y, pModel->GetLocationVec(j).z);
      }
   }

   SlamPhysicsModels();

   if (config_is_defined("CreatureBallisticSpew"))
   {
      mprintf("%s non-physical\n", ObjWarnName(m_ObjID));
      cPhysModel *pModel = g_PhysModels.Get(m_ObjID);
      if (pModel)
      {
         for (int j=0; j<pModel->NumSubModels(); j++)
            mprintf(" [%d] %g %g %g\n", j, pModel->GetLocationVec(j).x,
                    pModel->GetLocationVec(j).y, pModel->GetLocationVec(j).z);
      }
   }
#endif

   PhysDeregisterModel(m_ObjID);
   m_MotorState.IsPhysical=FALSE;
}

///////////////////////////////////////////////////

// shrink phys submodels
#define kDeathShrinkage 0.7

void cCreature::PrepareToDie()
{
   if(!IsPhysical())
      return;

   Assert_(g_pCreatureDescPtr&&m_CreatureType<g_nCreatureTypes);

   const sCreatureDesc *pCDesc=g_pCreatureDescPtr[m_CreatureType];

   if(!pCDesc->nPhysModels)
      return;

   SetPhysSubModScale(kDeathShrinkage);

   // @TODO:  Bodisafa 12/7/1999
   // This should be hooked up to a motion.
   if (CreatureWeaponObjDrop)
   {
      (*CreatureWeaponObjDrop)(m_ObjID);
   }
   return;
}

///////////////////////////////////////////////////

// After a creature dies and plays its death motion, we can either remove its
// physics or leave it there.  Leaving it there is cool if we want the player to be
// able to push or attack the dead body.  In any event, the code for not removing
// physics is kind of broken. (Generally, the mesh becomes totally separated from
// the object and its physics, and the physics does weird stuff like slide around.)
// So, we currently favor removal of the physics.
#define REMOVE_DEAD_PHYSICS
#ifdef REMOVE_DEAD_PHYSICS

void cCreature::BeDead()
{
   if(!IsPhysical())
      return;

   ConfigSpew("CreatureTrace",("%d: is dead\n",m_ObjID));

   PhysDeregisterModel(m_ObjID);

   m_MotorState.IsPhysical = PhysObjHasPhysics(m_ObjID);

   AssertMsg1 (!(m_MotorState.IsPhysical), "Failed to remove physics from recently deceased %s",
               ObjWarnName(m_ObjID));
}

#else // !REMOVE_DEAD_PHYSICS

// make ballistic and change to one phys model with rest axis
void cCreature::BeDead()
{
   if(!IsPhysical())
      return;

   ConfigSpew("CreatureTrace",("%d: is dead\n",m_ObjID));

#if 0  // this doesn't work with load/save, so made into ballistic mode instead
   // get rid of old creature physics models
   PhysDeregisterModel(m_ObjID);

   // create new non-creature, rest-axis physics model
   const sCreatureDesc *pCDesc=g_pCreatureDescPtr[m_CreatureType];

   if(!pCDesc->nPhysModels)
      return;

   // create new phys models
   PhysRegisterSphere(m_ObjID,1,NULL,pCDesc->physRadius*(kDeathShrinkage));
   cPhysModel *pModel = g_PhysModels.Get(m_ObjID);

   if(pModel)
   {
      pModel->SetRestAxes(ZAxis);
   }
   m_MotorState.IsPhysical=PhysObjHasPhysics(m_ObjID);
#endif

   MakeBallistic(kCMB_Corpse);

   ValidateCreaturePosition(m_ObjID);

}

#endif // REMOVE_DEAD_PHYSICS

///////////////////////////////////////////////////

BOOL cCreature::MakeBallistic(int style, BOOL force)
{
   if (!force && IsBallistic())
      return FALSE;

   #ifndef SHIP
   if (config_is_defined("CreatureBallisticSpew"))
   {
      mprintf("%s going ballistic\n", ObjWarnName(m_ObjID));
      cPhysModel *pModel = g_PhysModels.Get(m_ObjID);
      if (pModel)
      {
         for (int j=0; j<pModel->NumSubModels(); j++)
            mprintf(" [%d] %g %g %g\n", j, pModel->GetLocationVec(j).x,
                    pModel->GetLocationVec(j).y, pModel->GetLocationVec(j).z);
      }
   }
   #endif

   SetMotFrameOfReference(kMFRT_Global);
   if (m_MotorState.ballisticRefCount == 0)
      m_MotorState.ballisticRefCount++;

   m_ballisticStyle = style;

   // make phys models rigid
   if(IsPhysical())
   {
      const sCreatureDesc *pCDesc=g_pCreatureDescPtr[m_CreatureType];

      Assert_(pCDesc&&(pCDesc->nPhysModels==0||pCDesc->pCrPhysModOffsets));

      cPhysModel *pModel;

      if ((pModel = g_PhysModels.Get(m_ObjID)) == NULL)
         return FALSE;

      if(style==kCMB_Corpse)
         pModel->SetRestAxes(ZAxis);

      for(int i=0; i<pCDesc->nPhysModels && i<pModel->NumSubModels(); i++)
      {
         MakePhysSubModBallistic(i, style);
      }

      SlamPhysicsModels();
      mxs_vector z;
      mx_zero_vec(&z);
      PhysSetRotationalVelocity(m_ObjID,&z);
   }

   BOOL rv = PhysObjHasPhysics(m_ObjID) && PhysObjValidPos(m_ObjID, NULL);

#if 0
   if (rv != TRUE)
      MakeNonBallistic();
#endif

   #ifndef SHIP
   if (config_is_defined("CreatureBallisticSpew"))
   {
      mprintf("%s ballistic\n", ObjWarnName(m_ObjID));
      cPhysModel *pModel = g_PhysModels.Get(m_ObjID);
      if (pModel)
      {
         for (int j=0; j<pModel->NumSubModels(); j++)
            mprintf(" [%d] %g %g %g\n", j, pModel->GetLocationVec(j).x,
                    pModel->GetLocationVec(j).y, pModel->GetLocationVec(j).z);
      }
   }
   #endif

   ValidateCreaturePosition(m_ObjID);

   return rv;
}

///////////////////////////////////////////////////

void cCreature::MakePhysSubModBallistic(int index, int style)
{
   const sCreatureDesc *pCDesc=g_pCreatureDescPtr[m_CreatureType];

   mxs_vector subPos;
   mxs_vector world_subPos;

   PhysStopControlLocation(GetObjID());

   switch (style)
   {
      case kCMB_Normal:
      {
         GetPhysSubModelPos(m_pJoints, &pCDesc->pCrPhysModOffsets[index], &subPos);
         mx_subeq_vec(&subPos,&m_sMultiped.global_pos);
         mx_mat_tmul_vec(&world_subPos, &m_sMultiped.global_orient, &subPos);

         PhysSetSubModLocation(m_ObjID, index, &m_sMultiped.global_pos);
         PhysSetSubModRelLocation(m_ObjID, index, &world_subPos);

         SetPhysSubModScale(kDeathShrinkage, index);

         break;
      }

      case kCMB_Efficient:
      {
         mxs_angvec rot;

         rot = ObjPosGet(GetObjID())->fac;
         PhysControlRotation(GetObjID(), &rot);

         mxs_matrix orient;
         mxs_vector cur_unrot_offset;
         mxs_vector cur_offset;

         mx_ang2mat(&orient, &rot);
         PhysGetSubModLocation(m_ObjID, index, &cur_offset);
         mx_subeq_vec(&cur_offset, &ObjPosGet(m_ObjID)->loc.vec);

         mx_mat_tmul_vec(&cur_unrot_offset, &orient, &cur_offset);

         cPhysModel *pModel = g_PhysModels.Get(m_ObjID);

         if (pModel)
            pModel->SetSubModOffset(index, cur_unrot_offset);

         break;
      }

      case kCMB_WithFeet:
      {
         GetPhysSubModelPos(m_pJoints, &pCDesc->pCrPhysModOffsets[index], &subPos);
         mx_subeq_vec(&subPos,&m_sMultiped.global_pos);
         mx_mat_tmul_vec(&world_subPos, &m_sMultiped.global_orient, &subPos);

         if (index < 2)
         {
            PhysSetSubModLocation(m_ObjID, index, &m_sMultiped.global_pos);
            PhysSetSubModRelLocation(m_ObjID, index, &world_subPos);
         }
         else
         {
            Location start, end, hit;
            mxs_vector knee, foot;

            mx_add_vec(&knee, &world_subPos, &m_sMultiped.global_pos);

            if (pCDesc->footJoints)
            {
               mx_copy_vec(&subPos, &m_pJoints[pCDesc->footJoints[index-2]]);
               mx_subeq_vec(&subPos,&m_sMultiped.global_pos);
               mx_mat_tmul_vec(&world_subPos, &m_sMultiped.global_orient, &subPos);
               mx_add_vec(&foot, &world_subPos, &m_sMultiped.global_pos);
            }
            else
               mx_copy_vec(&foot, &knee);

            MakeLocationFromVector(&start, &knee);
            MakeLocationFromVector(&end, &foot);

            ComputeCellForLocation(&start);
            if (!PortalRaycast(&start, &end, &hit, TRUE))
            {
               mxs_vector shin;

               mx_sub_vec(&shin, &hit.vec, &knee);
               mx_scaleeq_vec(&shin, 0.99);
               mx_add_vec(&foot, &knee, &shin);
            }

            mx_subeq_vec(&foot, &m_sMultiped.global_pos);

            PhysSetSubModLocation(m_ObjID, index, &m_sMultiped.global_pos);
            PhysSetSubModRelLocation(m_ObjID, index, &foot);
         }

         break;
      }

      case kCMB_Posed:
      {
         GetPhysSubModelPos(m_pJoints, &pCDesc->pCrPhysModOffsets[index], &subPos);
         mx_subeq_vec(&subPos,&m_sMultiped.global_pos);
         mx_mat_tmul_vec(&world_subPos, &m_sMultiped.global_orient, &subPos);

         PhysSetSubModLocation(m_ObjID, index, &m_sMultiped.global_pos);
         PhysSetSubModRelLocation(m_ObjID, index, &world_subPos);

         break;
      }

      case kCMB_Compressed:
      {
         mxs_angvec rot;

         rot = ObjPosGet(GetObjID())->fac;
         PhysControlRotation(GetObjID(), &rot);

         mxs_vector loc;
         mxs_vector zero;

         PhysGetModLocation(m_ObjID, &loc);
         mx_zero_vec(&zero);

         PhysSetSubModRelLocation(m_ObjID, index, &zero);
         PhysSetSubModLocation(m_ObjID, index, &loc);

         SetPhysSubModScale(kDeathShrinkage, index);

         break;
      }
      case kCMB_Corpse:
      {
         mxs_vector loc;
         mxs_vector zero;

         PhysGetModLocation(m_ObjID, &loc);
         mx_zero_vec(&zero);

         PhysSetSubModRelLocation(m_ObjID, index, &zero);
         PhysSetSubModLocation(m_ObjID, index, &loc);

         SetPhysSubModScale(kDeathShrinkage, index);

         break;
      }
   }
}

///////////////////////////////////////////////////

void cCreature::MakeNonBallistic(BOOL force)
{
   if (!force && !IsBallistic())
      return;

   const sCreatureDesc *pCDesc=g_pCreatureDescPtr[m_CreatureType];

   Assert_(pCDesc);

   if(pCDesc->alwaysBallistic) // don't do anything if creature type always ballistic
      return;

   #ifndef SHIP
   if (config_is_defined("CreatureBallisticSpew"))
   {
      mprintf("%s going non-ballistic\n", ObjWarnName(m_ObjID));
      cPhysModel *pModel = g_PhysModels.Get(m_ObjID);
      if (pModel)
      {
         for (int j=0; j<pModel->NumSubModels(); j++)
            mprintf(" [%d] %g %g %g\n", j, pModel->GetLocationVec(j).x,
                    pModel->GetLocationVec(j).y, pModel->GetLocationVec(j).z);
      }
   }
   #endif

   ValidateCreaturePosition(m_ObjID);

   if(m_MotorState.ballisticRefCount>0)
      m_MotorState.ballisticRefCount--;

   if(m_MotorState.ballisticRefCount>0)   // still ballistic
   {
#ifndef SHIP
      mprintf("%s ballistic ref count still > 0? (%d specifically)\n", ObjWarnName(m_ObjID), m_MotorState.ballisticRefCount);
#endif
      return;
   }

   SetMotFrameOfReference(kMFRT_RelStart); // @TODO: make this creature type dependent

   // @TODO: notify physics system

   // @HACK: remove when we have different physics for posed corpse
   if (PhysObjHasPhysics(m_ObjID))
   {
      PhysStopControlRotation(m_ObjID);

      SetPhysSubModScale(m_scalePhys);
      SlamPhysicsModels();
   }

   #ifndef SHIP
   if (config_is_defined("CreatureBallisticSpew"))
   {
      mprintf("%s non-ballistic\n", ObjWarnName(m_ObjID));
      cPhysModel *pModel = g_PhysModels.Get(m_ObjID);
      if (pModel)
      {
         for (int j=0; j<pModel->NumSubModels(); j++)
            mprintf(" [%d] %g %g %g\n", j, pModel->GetLocationVec(j).x,
                    pModel->GetLocationVec(j).y, pModel->GetLocationVec(j).z);
      }
   }
   #endif

   ValidateCreaturePosition(m_ObjID);
}

void cCreature::SetPhysSubModScale(float scale, int submodel)
{
   const sCreatureDesc *pCDesc=g_pCreatureDescPtr[m_CreatureType];

   if (!PhysObjHasPhysics(m_ObjID))
      return;

   if (IsMovingTerrain(m_ObjID))  // um, don't ask
      return;

   if (submodel == -1)
   {
      cPhysModel *pModel;

      if ((pModel = g_PhysModels.Get(m_ObjID)) == NULL)
         return;

      for(int i=0; i<pCDesc->nPhysModels && i<pModel->NumSubModels(); i++)
         PhysSetSubModRadius(m_ObjID, i, pCDesc->pCrPhysModOffsets[i].radius * scale);

      m_scalePhys = scale;
   }
   else
      PhysSetSubModRadius(m_ObjID, submodel, pCDesc->pCrPhysModOffsets[submodel].radius * scale);

}

///////////////////////////////////////////////////
///////////////////////////////////////////////////
/////
///// Attachment/Weapon functions
/////
///////////////////////////////////////////////////
///////////////////////////////////////////////////

void cCreature::GetWeaponAttachDefaults(int wtype, sCreatureAttachInfo *pInfo)
{
   if(!pInfo)
   {
      return;
   }

   pInfo->weapon = wtype;
   pInfo->is_weapon = TRUE;

   mx_zero_vec(&pInfo->relTrans.vec);
   mx_identity_mat(&pInfo->relTrans.mat);

   Assert_(g_pCreatureDescPtr&&m_CreatureType<g_nCreatureTypes);
   const sCreatureDesc *pCDesc=g_pCreatureDescPtr[m_CreatureType];
   sCrPhysModOffsetTable *pWPO;
   sCrPhysModOffset *pMod;

   Assert_(wtype<pCDesc->nWeapons);

   if(!pCDesc || (NULL==(pWPO=pCDesc->pWeapPhysOffsets)) || wtype==kCrWeap_Invalid)
   {
      pInfo->joint=0;
      return;
   }
   pMod=pWPO[wtype].pPhysModels;

   if(!pMod)
   {
      pInfo->joint=0;
      return;
   }
   pInfo->joint = kCJ_Butt;
}

extern int g_CollisionObj1;
extern int g_CollisionObj2;
extern int g_CollisionSubmod1;
extern int g_CollisionSubmod2;
extern mxs_vector g_collision_location;

///////////////////////////////////////////////////////
//
// Default physcast callback
//

typedef struct sWeaponPhyscastData
{
   ObjID myID;
   ObjID ownerID;
} sWeaponPhyscastData;

static BOOL WeaponPhyscastCallback(ObjID objID, const cPhysModel* pModel, sWeaponPhyscastData* pData)
{
   // ignore myself
   if ((objID == pData->myID) || (objID == pData->ownerID))
      return FALSE;
   return TRUE;
}


BOOL cCreature::MakeWeaponPhysical(ObjID weapon, int wtype)
{
   Assert_(g_pCreatureDescPtr&&m_CreatureType<g_nCreatureTypes);
   const sCreatureDesc *pCDesc=g_pCreatureDescPtr[m_CreatureType];
   sCrPhysModOffsetTable *pWPO;
   sCrPhysModOffset *pMod;
   int nPhysMods,i;

   Assert_(wtype<pCDesc->nWeapons);

   // Check that we're a concrete object.  If we've switched weapons, during a swing, before our
   // previous weapon has become physical, then we get here with our new weapon as the "weapon"
   // parameter.  We really want to check that our weapon parameter matches our weapon at the
   // start of the swing, but simply bailing out if we've switched to an arrow works (though
   // you will be able to switch between the sword and the blackjack).  It's not the right
   // solution, but hey, 7 days till gold master.
   if (OBJ_IS_ABSTRACT(weapon))
      return FALSE;
   if(!pCDesc || (NULL==(pWPO=pCDesc->pWeapPhysOffsets)) || wtype==kCrWeap_Invalid)
      return FALSE;
   if((nPhysMods=pWPO[wtype].nPhysModels)<=0)
      return FALSE;
   pMod=pWPO[wtype].pPhysModels;
   if(!pMod)
      return FALSE;

   ConfigSpew("CreatureTrace",("%d: Making weapon physical\n",m_ObjID));

   // create new phys models
   PhysRegisterSphere(weapon,nPhysMods+kWH_NumModels,kPMCF_Creature|kPMCF_Weapon,pCDesc->physRadius);

   for(i=0;i<nPhysMods;i++,pMod++)
   {
      mxs_vector subPos;

      GetPhysSubModelPos(m_pJoints,pMod,&subPos);
      PhysSetSubModLocation(weapon,i,&subPos);
      PhysSetSubModRadius(weapon,i,pMod->radius * m_scalePhys);

      // activate location control
      PhysControlSubModLocation(weapon, i, &subPos);

      #if 0
      // check if it's a valid position
      mxs_vector zero;
      mx_zero_vec(&zero);  // this is really moronic
      if (!PhysObjValidPos(weapon, &zero))
      {
         ObjID owner;

         if ((owner = GetWeaponOwnerObjID(weapon)) != OBJ_NULL)
         {
            HandleWeaponHit(OBJ_NULL, weapon, owner, NULL);
            return FALSE;
         }
         else
            Warning(("Weapon started in invalid position with no owner?\n"));
      }
      #endif
   }

   ObjID ownerObj;

   if (GetObjID() == PlayerArm())
      ownerObj = PlayerObject();
   else
      ownerObj = GetObjID();

   Location owner_loc;
   Location submod_loc;
   Location hit_loc;
   ObjID    hit_obj;

   mxs_real min_hit_time = 1.0;
   Location min_hit_loc;
   ObjID    min_hit_obj = OBJ_NULL;
   int      min_hit_result;

   cPhysModel *pModel = g_PhysModels.Get(ownerObj);

   if (pModel == NULL)
      return FALSE;

   owner_loc = pModel->GetLocation();

   pMod=pWPO[wtype].pPhysModels;

   // physcast to our starting location, hitting anything in our way
   for (i=0; i<nPhysMods; i++)
   {
      PhysGetSubModLocation(weapon, i, &submod_loc.vec);
      MakeLocationFromVector(&submod_loc, &submod_loc.vec);

      int hit_result;

      if (ownerObj == PlayerObject())
      {
         sWeaponPhyscastData data = {weapon, ownerObj};
         PhysRaycastSetCallback(tPhyscastObjTest(WeaponPhyscastCallback), (void*)&data);
         hit_result = PhysRaycast(owner_loc, submod_loc, &hit_loc, &hit_obj, pMod[i].radius, kCollideSphere|kCollideSphereHat|kCollideOBB|kCollideTerrain);
         PhysRaycastClearCallback();
      }
      else
      {
         ObjID player_obj = PlayerObject();

         PhysRaycastSetObjlist(&player_obj, 1);
         hit_result = PhysRaycast(owner_loc, submod_loc, &hit_loc, &hit_obj, pMod[i].radius, kCollideSphere|kCollideSphereHat|kCollideOBB|kCollideTerrain);
         PhysRaycastClearObjlist();
      }

      //so if it's a weapon, and what it's hitting is terrain(has an obj
      //texture, to copy the trick in collide.cpp), and
      //the weapon is set to not collide with terrain, then don't
      //actually cause a hit.  Phew.  AMSD.
      if ((GetObjTextureIdx(hit_obj)>=0) && (IsWeapon(weapon)))
	  if (!GetWeaponTerrainCollision(weapon))
	     hit_result = kCollideNone;

      if (hit_result != kCollideNone)
      {
         mxs_real cast_len = mx_dist_vec(&owner_loc.vec, &submod_loc.vec);
         mxs_real hit_len = mx_dist_vec(&owner_loc.vec, &hit_loc.vec);

         if (cast_len == 0.0)
            continue;

         mxs_real hit_time = hit_len / cast_len;

         if (hit_time < min_hit_time)
         {
	    min_hit_loc = hit_loc;
            min_hit_time = hit_time;
            min_hit_obj = hit_obj;
            min_hit_result = hit_result;
         }
      }
   }

   if ((min_hit_obj != OBJ_NULL) &&
       (min_hit_obj != weapon) &&
       (min_hit_obj != ownerObj))
   {
      ObjID owner = GetWeaponOwnerObjID(weapon);

      if (owner != OBJ_NULL)
      {
         // cause the sound event
         int coll_type_hit = 0;
         int coll_type_weapon = 0;
         int bits = CollideTest(min_hit_obj, weapon);

         ObjGetCollisionType(min_hit_obj, &coll_type_hit);
         ObjGetCollisionType(weapon, &coll_type_weapon);

#define DEFAULT_IMPACT 20

         if (!(coll_type_weapon & COLLISION_NO_SOUND) ||
             !(coll_type_hit & COLLISION_NO_SOUND))
         {
            CollisionSoundObjects(weapon, min_hit_obj, bits, DEFAULT_IMPACT);
         }

         // Set up our completely hacked globals
         g_CollisionObj1 = weapon;
         g_CollisionObj2 = min_hit_obj;
         g_CollisionSubmod1 = 0;
         g_CollisionSubmod2 = 0;
	 mx_copy_vec(&g_collision_location, &min_hit_loc.vec);

         // cause an impact event
         sImpact impact = { DEFAULT_IMPACT };

         AutoAppIPtr_(DamageModel, pDamageModel);
         pDamageModel->HandleImpact(min_hit_obj, weapon, &impact, NULL);
      }
   }

   WeaponHaloInit(m_ObjID, weapon, nPhysMods);

   return PhysObjHasPhysics(weapon);
}

void cCreature::MakeWeaponNonPhysical(ObjID weapon)
{
   if(PhysObjHasPhysics(weapon))
   {
      PhysDeregisterModel(weapon);
      ConfigSpew("CreatureTrace",("%d: Making weapon non-physical\n",m_ObjID));
   }
   WeaponHaloTerm(m_ObjID, weapon);
}

// notify motor resolver, since that actually makes all stop motion
// decisions except in case of deletion
void cCreature::AbortWeaponMotion(ObjID weapon)
{
   if(!m_pMotorResolver)
      return;
   m_pMotorResolver->NotifyAboutMotionAbortRequest();
}

sCrPhysModOffsetTable *cCreature::GetWeaponPhysOffsets(int wtype)
{
   Assert_(g_pCreatureDescPtr&&m_CreatureType<g_nCreatureTypes);
   const sCreatureDesc *pCDesc=g_pCreatureDescPtr[m_CreatureType];
   sCrPhysModOffsetTable *pWPO;

   Assert_(wtype<pCDesc->nWeapons);

   if(!pCDesc || (NULL==(pWPO=pCDesc->pWeapPhysOffsets)) || wtype==kCrWeap_Invalid)
      return NULL;
   return &pWPO[wtype];
}

void cCreature::GetPhysSubModPos(sCrPhysModOffset *pOffset,mxs_vector *pPos)
{
   if(pOffset->frac)
   {
      mxs_vector tmp;

      mx_copy_vec(&tmp,&m_pJoints[pOffset->j1]);
      mx_scaleeq_vec(&tmp,1.0-pOffset->frac);
      mx_copy_vec(pPos,&m_pJoints[pOffset->j2]);
      mx_scaleeq_vec(pPos,pOffset->frac);
      mx_addeq_vec(pPos,&tmp);
   } else
   {
      mx_copy_vec(pPos,&m_pJoints[pOffset->j1]);
   }
}

void cCreature::GetSubModelPos(int submod, mxs_vector *pPos)
{
   GetPhysSubModelPos(m_sMultiped.joints, &g_pCreatureDescPtr[m_CreatureType]->pCrPhysModOffsets[submod], pPos);
}

BOOL cCreature::PhysSubModIsBallistic(int index)
{
   Assert_(g_pCreatureDescPtr&&m_CreatureType<g_nCreatureTypes);
   const sCreatureDesc *pCDesc=g_pCreatureDescPtr[m_CreatureType];

   return IsBallistic()||(pCDesc->pCrPhysModOffsets&&pCDesc->pCrPhysModOffsets[index].isBallistic);
}

///////////////////////////////////////////////////

///////////////////////////////////////////////////

#define E398_HACK

#ifdef E398_HACK

EXTERN BOOL g_DoRootHack; // from motset.cpp
EXTERN int  g_HackButtJointID; // from motset.cpp

#endif

#define kMaxAngVel 0x3800   // in mxs_ang per sec

// this should go away once motion are reprocessed so there is separate
// base rotation channel from butt rotation
// @HACK: E398 hack, really.
static uint HackMpUpdate(multiped * mp, float time_delta_ms, ulong app_flags, ObjID focus)
{
#ifdef E398_HACK
   // set global with butt joint id, so that capture callback knows to take
   // base rotation out of this.
   g_DoRootHack=TRUE;
   g_HackButtJointID=mp->root_joint;
#endif

   // rotate base orientation to face focus.  (where base orientation is
   // starting orientation of motion.)
   // @NOTE that this won't work correctly for turns or motions that are being
   // bent, since global orientation varies over course of these motions instead
   // of always being base orientation.
   if(focus!=OBJ_NULL && !((cCreature *)(mp->app_ptr))->IsBallistic())
   {
      quat *pBaseOrient;
      mxs_vector *pBasePos;

      if(GetMpMainMotionBaseOrient(mp,&pBaseOrient,&pBasePos))
      {
         ObjPos *pPos=ObjPosGet(focus); // @OPTIMIZE: store this off somewhere instead of computing every update

         if (pPos != NULL)
         {
            // compute angle to focus obj
            mxs_vector diff;
            mxs_ang desFac,curFac,delta;
            mxs_matrix mat;
            fix maxTurn;  // needs to be fix due to overflow
            BOOL negRot=FALSE;

            mx_sub_vec(&diff,&pPos->loc.vec,&mp->global_pos);
            // Let's make this NOT quite zero, so normeq div0 won't occur.
            diff.z = 0.000001; // make sure it normalizes on X/Y plane. 'diff' is not used for anything else.
            mx_normeq_vec(&diff);
            desFac=GetNormVecHeading(&diff);

            // compare to actual heading and clamp.
            curFac=GetHeadingAngle(&mp->global_orient);
            delta=desFac-curFac;
            if(delta>MX_ANG_PI)
            {
               delta=-delta;
               negRot=TRUE;
            }
            maxTurn=fix_mul(((cCreature *)(mp->app_ptr))->GetMaxAngVel(),(fix_from_float(time_delta_ms/1000)));
            if(delta>maxTurn) // clamp
            {
               delta=maxTurn;
            }
            if(negRot)
               delta=-delta;
            desFac=curFac+delta;

            // modify base rotation
            mx_mk_rot_z_mat(&mat,desFac);
            quat_from_matrix(pBaseOrient,&mat);

            // modify base position, so current position will remain unchanged
            mx_sub_vec(&diff,&mp->global_pos,pBasePos);
            mx_mk_rot_z_mat(&mat,delta);
            mx_mat_muleq_vec(&mat,&diff);
            mx_sub_vec(pBasePos,&mp->global_pos,&diff);
         }
      }
   }

   int retval=mp_update(mp,time_delta_ms,app_flags);

#ifdef E398_HACK
   g_DoRootHack=FALSE;
#endif

   return retval;
}

#define MOT_SCALE_TOLERANCE 0.1

#ifdef CUR_MOTION_TRACKING
char *cur_motion_name;
char  cur_motion_id;
#endif

#define kMaxTiltTolerance 0.8
#define kForceBlendLength 500

// This always applies a scale factor to stretch the motion based on
// creature size.  Stretch parameters computed by maneuvers should take
// this into account.
void cCreature::StartMotionWithParam(const int motionNum, mps_motion_param *pParam, ulong flags, int startFrame)
{
   mps_start_info info;
   mps_motion_param defParam;

   info.flags=NULL;

   if(motionNum<0 || motionNum>=mp_num_motions)
      Warning(("cCreature::StartMotionWithParam, bad motion number %d\n",motionNum));

#ifdef CUR_MOTION_TRACKING
   cur_motion_name=MotDescGetName(motionNum);
   cur_motion_id++;
#endif

   float scale = GetMotionScale();

   // if timeslop, offset start frame accordingly.
   const sMotorState *pState=GetMotorState();
   mps_motion *pMotion=((mps_motion *)mp_motion_list+motionNum);

   g_pMotionSet->GetStartEndFrames(motionNum,&info.start_frame,&info.callback_frame);
   if(flags&kMotStartFlag_SetStartFrame)
   {
      info.start_frame=startFrame;
   } else
   {
      if(pState->timeSlop>0&&pMotion->info.type==MT_CAPTURE)
      {
         float frame_offset;

         frame_offset = pState->timeSlop*pMotion->info.freq;
         if(pParam&&pParam->flags&MP_DURATION_SCALE)
         {
            frame_offset/=pParam->duration_scalar;
         }
         if((int)(frame_offset+1)<pMotion->info.num_frames-info.start_frame)
         {
            if(info.start_frame==0) // okay to keep xlat, since just for timeslop from beginning KJ 10/98
            {
               info.flags|=MStrtFlag_KeepXlat;
            }
            info.start_frame+=(int)(frame_offset);
         }
      }
   }
   if(info.callback_frame>=0 && info.start_frame>info.callback_frame)
   {
      info.start_frame=info.callback_frame;
   }
   info.callback = CreatureMotEndCallback;
   info.motion_num = motionNum;
   info.trans_duration=MotDescBlendLength(info.motion_num);
   if(flags&kMotStartFlag_ForceBlend)
   {
      if(info.trans_duration<kForceBlendLength)
         info.trans_duration=kForceBlendLength;
   }
   if(pParam)
   {
      defParam=*pParam;
   } else
   {
      defParam.flags=NULL;
   }
   if(IsBallistic()) // ignore params that affect xlat
   {
      defParam.flags&=~(MP_BEND|MP_STRETCH|MP_VSTRETCH|MP_VINC);
   }
   info.params=&defParam;
   info.callback_num=0;

   if(!IsBallistic() && (scale<1.0-MOT_SCALE_TOLERANCE || scale >1.0+MOT_SCALE_TOLERANCE))
   {
      // stretch motion
      if(info.params->flags&MP_STRETCH)
      {
         info.params->stretch*=scale;
      } else
      {
         info.params->stretch=scale;
         info.params->flags|=MP_STRETCH;
      }
   }
   // check for global timewarp
   float tw;
   if(ObjGetTimeWarp(m_ObjID,&tw))
   {
      if(info.params->flags&MP_DURATION_SCALE)
      {
         info.params->duration_scalar*=tw;
      } else
      {
         info.params->flags|=MP_DURATION_SCALE;
         info.params->duration_scalar=tw;
      }
   }

   mp_start_motion(&m_sMultiped, (mps_start_info *)&info);

   ConfigSpew("CreatureTrace",("%d starting motion %d (%s)\n",m_ObjID,motionNum,MotDescGetName(motionNum)));
}

///////////////////////////////////////////////////

// @NOTE: this actually stops all motions on creature because otherwise
// we run into problems if two full-body motions are playing, since mp_stop_motion
// will set the joint map to 255 but leave one motion on the multiped.
// The result being that the remain motions' rotations never get applied while it
// plays.  Alternative is to have StartMotion stop any other non-overlay motions
// that are playing before starting a new one (or do this mp_start_motion),
// or have mp_stop_motion check and do joint_map correctly.
// It's kind of mysterious, since only one maneuver executes a time, and I
// thought that each stopped all motions it started, so we should never have
// two non-overlay motions playing at once anyway.    (KJ 8/98)
void cCreature::StopMotion(const int motionNum)
{
//   mp_stop_motion(&m_sMultiped,motionNum);
   mp_stop_all_motions(&m_sMultiped); // see big comment above
   HackMpUpdate(&m_sMultiped,0,NULL,m_MotorState.focus);
   ConfigSpew("CreatureTrace",("%d CreatureTrace: stop mot %d\n",m_ObjID,motionNum));
}

///////////////////////////////////////////////////

void cCreature::Pause()
{
}

///////////////////////////////////////////////////
// Mesh shape functions
//
///////////////////////////////////////////////////

float cCreature::GetRadius()
{
   return m_Radius;
}

///////////////////////////////////////////////////

void cCreature::GetWorldBBox(mxs_vector *pBMin, mxs_vector *pBMax)
{
   mx_copy_vec(pBMin,&m_BMin);
   mx_copy_vec(pBMax,&m_BMax);
}



///////////////////////////////////////////////////
// Mesh renderer functions
//
///////////////////////////////////////////////////

void cCreature::MeshJointPosCallback(const int jointID, mxs_trans *pTrans)
{
   AssertMsg1(jointID>=0&&jointID<m_nJoints,"Invalid JointID %d for Creature",jointID);

   mx_copy_vec(&pTrans->vec,&m_pJoints[jointID]);

   if ((gUseHeadTracking || IsRemoteGhost(m_ObjID)) && (jointID == GetCreatureJointID(m_ObjID, kCJ_Neck)))
      mx_copy_mat(&pTrans->mat, &m_headOrient);
   else
      mx_copy_mat(&pTrans->mat,&m_pOrients[jointID]);
}

///////////////////////////////////////////////////

void cCreature::MeshStretchyJointCallback(const int jointID, mxs_trans *pTrans, quat *pRot)
{
   int iParentJoint;

   AssertMsg1(jointID>=0&&jointID<m_nJoints,"Invalid JointID %d for Creature",jointID);

   quat_copy(pRot,&m_sMultiped.rel_orients[jointID]);
   mx_copy_vec(&pTrans->vec,&m_pJoints[jointID]);

   iParentJoint=g_pCreatureDescPtr[m_CreatureType]->pJointParents[jointID];
   mx_copy_mat(&pTrans->mat,&m_pOrients[iParentJoint]);
}

///////////////////////////////////////////////////

void cCreature::QueueCompletedMotion(const sCreatureMotionEndNode *pNode)
{
   sCreatureMotionEndNode *pNewNode = new sCreatureMotionEndNode;
   sCreatureMotionEndNode **pHead;

   *pNewNode=*pNode;
   pNewNode->pNext=NULL;

   pHead=&m_CompletedMotions;
   while(*pHead)
   {
      pHead=&((*pHead)->pNext);
   }
   *pHead=pNewNode;
}

// XXX NOTE: this could get messed up if the motor resolver decides to respond
// to the motion ending by stopping some other motion.
// This is incorrect at the moment to emulate how things worked in the
// days of g_MotionEnded.  Really need to modify all the motor controllers
// to handle things robustly.
void cCreature::ProcessCompletedMotions()
{
   sCreatureMotionEndNode *pNode,*pNext;

   pNode=m_CompletedMotions;
   // give completion messages
   if(m_pMotorResolver)
   {
      while(pNode)
      {
         m_MotorState.timeSlop=pNode->timeSlop;
         // if frame!=-1, then motion may not actually be done yet, so
         // stop it.  Stop it here instead of in motendcallback because
         // want to make sure we're not in the middle of an mp_update
         // (frame!=-1 for motions which had a frame flagged
         // as end frame, instead of just ending at end).  (KJ 8/98)
         if(pNode->frame>=0)
            StopMotion(pNode->motionNum);
         m_pMotorResolver->NotifyAboutMotionEnd(pNode->motionNum, \
            pNode->frame, pNode->flags);
         pNode=pNode->pNext;
      }
   }
   // delete nodes
   pNode=m_CompletedMotions;
   while(pNode)
   {
      pNext=pNode->pNext;
      delete pNode;
      pNode=pNext;
   }
   m_CompletedMotions=NULL;
}

// This is here to process all motions that ended outside of creature update.
// Currently just spews and does nothing, to emulate behavior from
// g_MotionEnded days.  Really want to make motor controllers more robust
// so they can behave sensibly when motions stopped outside of frame, I
// think, and then regular ProcessCompletedMotions should be called instead.
void cCreature::HackProcessCompletedMotions()
{
   sCreatureMotionEndNode *pNode,*pNext;

   pNode=m_CompletedMotions;
   // give completion messages
   if(m_pMotorResolver)
   {
      while(pNode)
      {
         ConfigSpew("CreatureHack",("%d: motion %d ended outside of frame\n",GetObjID(),pNode->motionNum));
         pNode=pNode->pNext;
      }
   }
   // delete nodes
   pNode=m_CompletedMotions;
   while(pNode)
   {
      pNext=pNode->pNext;
      delete pNode;
      pNode=pNext;
   }
   m_CompletedMotions=NULL;
}

///////////////////////////////////////////////////

#define CREATURE_UPDATE_DT_MAX 500
#define CREATURE_FRAME_LEN     100

DECLARE_TIMER(CREAT_Update, Average);

void cCreature::Update(const ulong timeDelta)
{
   AUTO_TIMER(CREAT_Update);

#if 0
   ulong t,tRemaining;
#endif
   ulong dtThreshold = min(timeDelta, CREATURE_UPDATE_DT_MAX);
   BOOL moved = FALSE;

#if 0
   if(!m_pMotorResolver) // allowed, but can't update.  like for corpse
      return;
#endif

   // reset time slop, since no motions have ended yet this frame
   m_MotorState.timeSlop=0;

   // process motions that were stopped outside of creature frame
   // XXX could be troublesome.  "forget" about them for now for testing
   HackProcessCompletedMotions();

#if 0
   // Run multiped update frames
   for (t=CREATURE_FRAME_LEN; t<dtThreshold; t+=CREATURE_FRAME_LEN)
   {
      moved = SubFrameUpdate(CREATURE_FRAME_LEN) || moved;
   }

   tRemaining=dtThreshold-(t-CREATURE_FRAME_LEN);
   if(tRemaining>0)
   {
      moved = SubFrameUpdate(tRemaining)||moved;
   }
#endif

   gUseHeadTracking = TRUE;

   // Only run a single frame for now...
   moved = SubFrameUpdate(timeDelta);

   if(moved)
   {
      ComputeBoundingData();
      if (!IsBallistic() && !PhysObjIsRope(m_ObjID))
         UpdateObjPosition();
      CreatureAttachmentsPosUpdate(m_ObjID);
   }

   gUseHeadTracking = FALSE;
}


///////////////////////////////////////////////////

const sCreatureDesc *cCreature::GetCreatureDesc()
{
   return g_pCreatureDescPtr[m_CreatureType];
}

///////////////////////////////////////////////////


#if 0 // this not actually used

static BOOL StoreBackup(multiped *back, const multiped *src)
{
   mp_clone_multiped(back,(multiped *)src);
   return TRUE;
}

// XXX should probably make this smarter, since joint map and
// relorients don't need to be constantly freed and realloced, and
// list ptrs could just be swapped.
static BOOL RestoreFromBackup(multiped *dst, multiped *back)
{
   mp_free_multiped(dst);
   mp_clone_multiped(dst,back);
   return TRUE;
}

static void FreeBackup(multiped *back)
{
   mp_free_multiped(back);
}

#endif

///////////////////////////////////////////////////

void UpdateCreatureOnObject(cCreature *pCreature, ObjID standing_obj, mxs_vector *new_pos)
{
   if (standing_obj == OBJ_NULL)
      return;

   if (IsMovingTerrainMoving(standing_obj))
   {
      mx_add_vec(new_pos, &ObjPosGet(standing_obj)->loc.vec, &pCreature->GetStandingOffset());

      mxs_angvec orient;

      mx_mat2ang(&orient, &pCreature->GetMultiped()->global_orient);
      pCreature->MoveButt(new_pos, &orient, FALSE);
      pCreature->UpdateObjPosition();
      pCreature->SlamPhysicsModels();
   }
}

DECLARE_TIMER(CREAT_SubFrameUpdate, Average);
DECLARE_TIMER(CREAT_HackMpUpdate, Average);
DECLARE_TIMER(CREAT_UpdatePhysics, Average);
DECLARE_TIMER(CREAT_PostUpdate, Average);

BOOL cCreature::SubFrameUpdate(const ulong dt)
{
   AUTO_TIMER(CREAT_SubFrameUpdate);

   int flags;
   BOOL okay=TRUE;

   const sCreatureDesc *pCDesc=g_pCreatureDescPtr[m_CreatureType];

   char *collideSpeech[]={"OW! My groin!","OW! Me chest!","Arghhh matey!","No chicken feet!\n"};

   g_MotionEnded=FALSE;

   if (!m_pMotorResolver)
      goto post_update;

   // reset time slop, since no motions have ended yet this sub-frame
   // want to keep timeSlop zero if no motions ended
   if(m_MotorState.timeSlop>0)
      m_MotorState.timeSlop+=(((float)dt)/1000);

   m_pMotorResolver->NotifyAboutFrameUpdateBegin();

   if(!IsPlayingMotion())
   {
      if (IsBallistic())
         PostUpdateFilter(dt);

      mxs_vector dummy_pos;
      UpdateCreatureOnObject(this, GetStandingObj(), &dummy_pos);

      m_pMotorResolver->NotifyAboutFrameUpdateEnd();
      return FALSE;
   }

   // Don't do any of this for ropes -- the post update filter handles all the
   // positioning/orienting
   if (m_CreatureType == 10) // rope magic number
      goto post_update;

   // update is simple if creature not physical or ballistic
   // (since then not responsible for moving its own phys models
   if(!IsPhysical() || IsBallistic())
   {
      flags = HackMpUpdate(&m_sMultiped, dt,NULL,m_MotorState.focus); // need this to get flags, callbacks etc
      if (IsBallistic())
         MoveButt(&ObjPosGet(GetObjID())->loc.vec, &ObjPosGet(GetObjID())->fac, TRUE);
   }
   else
   {

      TIMER_Start(CREAT_HackMpUpdate);
      flags=HackMpUpdate(&m_sMultiped, dt,NULL,m_MotorState.focus);
      TIMER_MarkStop(CREAT_HackMpUpdate);

      mxs_vector new_pos;
      mx_copy_vec(&new_pos, &m_sMultiped.global_pos);

      UpdateCreatureOnObject(this, GetStandingObj(), &new_pos);

      ValidateCreaturePosition(m_ObjID);

      TIMER_Start(CREAT_UpdatePhysics);
      if (UpdateCreaturePhysics(m_ObjID, &new_pos))
      {
         mxs_angvec orient;

         mx_mat2ang(&orient, &m_sMultiped.global_orient);
         MoveButt(&new_pos, &orient, TRUE);
      }
      TIMER_MarkStop(CREAT_UpdatePhysics);

      ValidateCreaturePosition(m_ObjID);
   }
post_update:

   {
      AUTO_TIMER(CREAT_PostUpdate);

   PostUpdateFilter(dt);

   // XXX this should check if terrain collision occured when updating
   // physics models, and deal accordingly
   CreatureAttachmentsPhysUpdate(m_ObjID);

   // notify about flags reached during frame
   if(m_pMotorResolver && flags)
      m_pMotorResolver->NotifyAboutFrameFlags(flags);

   // process motions that ended during creature frame
   ProcessCompletedMotions();

   if (m_pMotorResolver)
      m_pMotorResolver->NotifyAboutFrameUpdateEnd();

   }
   return TRUE;
}







