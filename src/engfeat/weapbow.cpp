/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

////////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/engfeat/weapbow.cpp,v 1.15 1998/03/19 12:59:53 kate Exp $
//
// core weapon (hand-to-hand-combat) 

#include <lg.h>
#include <comtools.h>
#include <appagg.h>

#include <traitman.h>

#include <plyrhdlr.h>
#include <plycbllm.h>
#include <playrobj.h>

#include <weapon.h>
#include <weapbow.h>

#include <prjctile.h>
#include <crattach.h>
#include <objpos.h>
#include <physapi.h>

#include <osysbase.h>
#include <iobjsys.h>
#include <mprintf.h>
#include <meshapi.h>
#include <creatext.h>
#include <creatapi.h>
#include <camera.h>
#include <lazyagg.h>
#include <command.h>

#include <tweqctrl.h>

// Must be last header
#include <dbmem.h>

typedef struct
{
   BOOL equip_pending;
   BOOL equipped;
   BOOL unequip_pending;
} sBowWeaponInfo;

static sBowWeaponInfo BowWeaponInfo = {FALSE, FALSE};

#define PLAYER_BOW_HACK



EXTERN ObjID g_ArrowObj=OBJ_NULL; // gets accessed in drkgame.c

////////
//
//    BOW GLOBALS  
//

///////
//
// Guess?
//
struct cMxsVectorHack : public mxs_vector // for watcom compiler bug (toml 08-18-97)
{
   cMxsVectorHack(float _x, float _y, float _z) { z = _z; y = _y; x = _x; }
};

struct cMxsAngvecHack : public mxs_angvec // for watcom compiler bug (toml 08-18-97)
{
   cMxsAngvecHack(mxs_ang _x, mxs_ang _y, mxs_ang _z) { tz = _z; ty = _y; tx = _x; }
};



static cMxsVectorHack g_ArrowPosStart(0,0,0);
static cMxsVectorHack g_WristRelStart(0,0,0);
static cMxsVectorHack g_WristRelEnd(0,0,0);
static cMxsAngvecHack g_ArrowRotStart(0,0,0);

static cMxsVectorHack g_ArrowCamPosStart(-0.4,0.22,0.3);
static cMxsVectorHack g_ArrowCamPosEnd(-1.0,0.22,0.3);
static cMxsAngvecHack g_Arm2CamRot(0xfe12,0xfd30,0x3b0c);
static mxs_matrix g_Cam2WristMat;

static mxs_matrix g_ArrowMatrix;


BOOL g_ArrowRecentlyLaunched=FALSE;
static ObjID g_LastArrowLaunched=OBJ_NULL;
static int g_ArrowTimeSinceLaunched;


static void ArrowDestroy();
static void ArrowCreate();
static void ArrowPull(float power);
static void ArrowRelease();
static BOOL ArrowExists();


static LazyAggMember(IObjectSystem) pObjSys; 

void  BowModeEnterCallback(ePlayerMode mode, void *data)
{
   if (BowWeaponInfo.equip_pending)
   {
      BowWeaponInfo.equip_pending = FALSE;
      BowWeaponInfo.equipped = TRUE;
   }
}

void  BowModeExitCallback(ePlayerMode mode, void *data)
{
   if (BowWeaponInfo.unequip_pending)
   {
      BowWeaponInfo.unequip_pending = FALSE;
      BowWeaponInfo.equipped = FALSE;
   }
}

#define  ARROW_GRACE_PERIOD 200

BOOL BowFrameCallback(ulong dt,void *data)
{
   if (g_ArrowObj!=OBJ_NULL && !pObjSys->Exists(g_ArrowObj))
   {
      Warning(("Arrow destroyed while on bow!\n"));
      ArrowRelease();
      g_LastArrowLaunched=OBJ_NULL;
      g_ArrowRecentlyLaunched=FALSE;
      return FALSE;
   }

   float power;

   if(g_ArrowObj!=OBJ_NULL && PlayerCbllmGetPower(&power))
   {
      // move arrow back according to power
      ArrowPull(power);
   }

   if(g_ArrowRecentlyLaunched)
   {
      g_ArrowTimeSinceLaunched+=dt;
      if(g_ArrowTimeSinceLaunched>ARROW_GRACE_PERIOD)
      {
         g_LastArrowLaunched=OBJ_NULL;
         g_ArrowRecentlyLaunched=FALSE;
      }
   }
   return TRUE;
}

BOOL  EquipBow()
{
   // Set the callback for when it's done equipping
   PlayerHandlerSetEnterModeCallback(kPlayerModeFlexiBow, BowModeEnterCallback, NULL);
   PlayerHandlerSetFrameCallback(kPlayerModeFlexiBow, BowFrameCallback,NULL);

   BowWeaponInfo.equip_pending = TRUE;

   // setup arrow stuff
   mx_ang2mat(&g_Cam2WristMat,&g_Arm2CamRot);

   // Tell the cerebellum to switch modes
   PlayerSwitchMode(kPlayerModeFlexiBow);

   return TRUE;
}

BOOL  UnEquipBow()
{
   // Set the callback for when it's done unequipping
   PlayerHandlerSetLeaveModeCallback(kPlayerModeFlexiBow, BowModeExitCallback, NULL);
   BowWeaponInfo.unequip_pending = TRUE;

   // get rid of the arrow
   ArrowDestroy();

   // Tell the cerebellum to switch modes
   PlayerSwitchMode(kPlayerModeUnarmed);

   return TRUE;
}

BOOL  IsBowEquipped()
{
   return BowWeaponInfo.equipped&&!BowWeaponInfo.unequip_pending;
}

void  StartBowAttack()
{
   if (IsBowEquipped())
   {
      ArrowCreate();
      PlayerStartAction();
   }
}

void  FinishBowAttack()
{
   float power;

   if (IsBowEquipped())
   {
      if(PlayerCbllmGetPower(&power))
      {
         int flags;


         flags=PRJ_FLG_TELLAI|PRJ_FLG_FROMPOS;
         if (pObjSys->Exists(g_ArrowObj))
         {
            launchProjectileVel(PlayerObject(),g_ArrowObj,power,flags,NULL);

            TweqProcessAll(g_ArrowObj,kTweqDoActivate);

            g_ArrowRecentlyLaunched=TRUE;
            g_ArrowTimeSinceLaunched=0;
            g_LastArrowLaunched=g_ArrowObj;
         } else
         {
            Warning(("Arrow destroyed while on bow!\n"));
         }

         ArrowRelease();
      }
      PlayerFinishAction();
   }
}

//
// Current arrow stuff
//
static ObjID arrow_type = OBJ_NULL;

ObjID GetArrowArchetype()
{
   return arrow_type;
}

// HORROR! TODO: remove this and have this whole shoot arrow type thing go away
extern void drkgame_PlayerSwitchMode(ePlayerMode new_mode);

void set_arrow_arch(ObjID arch)
{
   Assert_(OBJ_IS_ABSTRACT(arch));

   arrow_type = arch;
}



BOOL SetCurrentArrow(ObjID arrow)
{
   AutoAppIPtr_(TraitManager, pTraitMan);

   ObjID arch = pTraitMan->GetArchetype(arrow);
   
   if (arch != OBJ_NULL)
   {
      set_arrow_arch(arch);
      if(ArrowExists())
         ArrowCreate();

      return TRUE;
   }
   else
      return FALSE;
}

static BOOL ArrowExists()
{
   return g_ArrowObj!=OBJ_NULL;
}

// takes vector rel to wrist position but in camera orientation, and
// transforms it to wrist orientation
static void CamToWristPos(mxs_vector *dst,mxs_vector *src)
{
   mx_mat_mul_vec(dst,&g_Cam2WristMat,src);
}

static void ArrowCreate()
{
   ObjID archetype=GetArrowArchetype();
   ObjPos *pPos;

   ArrowDestroy();
   if(archetype!=OBJ_NULL)
   {
      // create arrow
      g_ArrowObj=pObjSys->BeginCreate(archetype, kObjectConcrete);
      if(NULL!=(pPos=ObjPosGet(PlayerArm())))
      {
         ObjPosUpdate(g_ArrowObj,&pPos->loc.vec,&pPos->fac);
      } else
      {
         AssertMsg(FALSE,"Cannot create arrow when no player arm");
      }
      pObjSys->EndCreate(g_ArrowObj);
      if(PhysObjHasPhysics(g_ArrowObj)) // don't want it physical yet
         PhysDeregisterModel(g_ArrowObj);

      // attach arrow to player arm
      sCreatureAttachInfo attachInfo;

      attachInfo.jointID=3;
      attachInfo.weapon=kCrWeap_Invalid;

      CamToWristPos(&g_WristRelStart,&g_ArrowCamPosStart);
      CamToWristPos(&g_WristRelEnd,&g_ArrowCamPosEnd);
      mx_copy_vec(&attachInfo.relTrans.vec,&g_WristRelStart);

//      mx_ang2mat(&attachInfo.relTrans.mat,&g_ArrowRotStart);
//      mx_copy_mat(&attachInfo.relTrans.mat,&g_ArrowMatrix);
      mx_copy_mat(&attachInfo.relTrans.mat,&g_Cam2WristMat);

      CreatureAttachItem(PlayerArm(),g_ArrowObj,&attachInfo);
   }
}

static void ArrowPull(float power)
{
   if(power<0)
      power=0;
   else if (power>1.0)
      power=1.0;
   
   sCreatureAttachInfo attachInfo;

   attachInfo.jointID=3;
   attachInfo.weapon=kCrWeap_Invalid;

   mx_interpolate_vec(&attachInfo.relTrans.vec,&g_WristRelStart,&g_WristRelEnd,power);
   mx_copy_mat(&attachInfo.relTrans.mat,&g_Cam2WristMat);

//   mprintf("power: %g\n",power);
   CreatureAttachmentModify(PlayerArm(),g_ArrowObj,&attachInfo);
}

static void ArrowRelease()
{
   if(g_ArrowObj!=OBJ_NULL)
   {
      CreatureDetachItem(PlayerArm(),g_ArrowObj);
      // forget about arrow, since no longer attached or our responsibility
      g_ArrowObj=OBJ_NULL;
   }
}

static void ArrowDestroy()
{
   if(g_ArrowObj!=OBJ_NULL)
   {
      pObjSys->Destroy(g_ArrowObj);
      g_ArrowObj=OBJ_NULL;
   }
}

// still consider arrow to be player arrow for a short time after
// arrow has been launched
ObjID PlayerArrow()
{
   if(g_ArrowObj)
      return g_ArrowObj;
   if(g_ArrowRecentlyLaunched)
      return g_LastArrowLaunched;
   return OBJ_NULL;
}

#if 0
static void PrintMatrix(char *name, mxs_matrix *pMat)
{
   mprintf("%s : \n",name);
   mprintf("%g, %g, %g\n%g, %g, %g\n%g, %g, %g\n\n",pMat->el[0],pMat->el[3],\
   pMat->el[6],pMat->el[1],pMat->el[4],pMat->el[7],pMat->el[2],pMat->el[5],\
   pMat->el[8]);
}
#endif

EXTERN void PortalMovePointInsideCell(Location *loc); // from wrfunc.c in portal lib

EXTERN void PlayerBowAlignArrow()
{
   mxs_vector camVec;
   mxs_matrix mat,relMat;
//   mxs_angvec camAng;
   mxs_trans trans;
   ObjID armObj;
   IMesh *pMesh;

   if(g_ArrowObj==OBJ_NULL)
      return;
   if(OBJ_NULL==(armObj=PlayerArm() ) )
      return;

   if(NULL==(pMesh=CreatureGetMeshInterface(armObj)))
      return;
   pMesh->MeshJointPosCallback(3,&trans);
   
   Position camPos;
   Camera *pCam=PlayerCamera();
   mxs_vector camOffset;
//   mxs_matrix mat;

   // do silliness since camera jitters when crosses portal boundary
   camPos=*ObjPosGet(pCam->objid); 
   mx_sub_vec(&camOffset,&pCam->pos,&camPos.loc.vec);
   PortalMovePointInsideCell(&camPos.loc);
   mx_addeq_vec(&camPos.loc.vec,&camOffset);
   mx_copy_vec(&camVec,&camPos.loc.vec);
   mx_ang2mat(&mat,&pCam->ang);

#if 0
   PrintMatrix("Cw",&mat);
   PrintMatrix("Hw",&trans.mat);
#endif

//   CameraGetLocation(PlayerCamera(),&camPos,&camAng);

   // calc desired arrow orientation in world space
   // (since we know it should be 0,0,0 in camera space)
//   mx_ang2mat(&mat,&camAng);
   // hand-relative orientation is (Hand orientation in world)^-1*
   //                               (Arrow orientation in world).
   mx_inveq_mat(&trans.mat);
   mx_mul_mat(&relMat,&trans.mat,&mat);
   mx_mat2ang(&g_ArrowRotStart,&relMat);
   mx_copy_mat(&g_ArrowMatrix,&relMat);

   mxs_matrix testMat;
   // what arrow thinks rel orientation of wrist to camera is
   mx_inv_mat(&testMat,&g_ArrowMatrix); 

#if 0
   PrintMatrix("Hw^-1",&trans.mat);
   PrintMatrix("Ah",&g_ArrowMatrix);
   PrintMatrix("Ah^-1==Hc",&testMat);
#endif

   // move arrow on player arm
   sCreatureAttachInfo attachInfo;

   attachInfo.jointID=3;
   attachInfo.weapon=kCrWeap_Invalid;
   mx_copy_vec(&attachInfo.relTrans.vec,&g_ArrowPosStart);
   mx_copy_mat(&attachInfo.relTrans.mat,&g_ArrowMatrix);

   CreatureAttachmentModify(PlayerArm(),g_ArrowObj,&attachInfo);
   CreatureAttachmentsPosUpdate(armObj);

   mprintf("arrow rotation: %x, %x, %x\n",g_ArrowRotStart.tx,g_ArrowRotStart.ty,g_ArrowRotStart.tz);

}

#ifndef SHIP
Command WeapBowCommands[] =
{
   { "player_align_arrow", FUNC_VOID, PlayerBowAlignArrow, "calc arrow alignment so straight" },
};
#endif

void BowInit(void)
{
#ifndef SHIP
   COMMANDS(WeapBowCommands,HK_GAME_MODE);
#endif
}

void BowTerm(void)
{
}

   







