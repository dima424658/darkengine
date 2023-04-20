/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/dark/drkwbow.cpp,v 1.34 2000/03/03 18:15:34 patmac Exp $
//
// core weapon (hand-to-hand-combat)

#include <lg.h>
#include <comtools.h>
#include <appagg.h>
#include <config.h>
#include <cfgdbg.h>
#include <math.h>
#include <rand.h>
#include <mprintf.h>

#include <traitman.h>

#include <plyrhdlr.h>
#include <plycbllm.h>
#include <playrobj.h>
#include <plyablty.h>
#include <plyablt_.h>
#include <plyrvmot.h>
#include <plyrmode.h>

#include <weapon.h>
#include <weapcb.h>
#include <drkwbow.h>
#include <drkcret.h>
#include <simtime.h>
#include <crjoint.h>

#include <prjctile.h>
#include <crattach.h>
#include <objpos.h>
#include <physapi.h>
#include <objmedia.h>

#include <objedit.h>
#include <osysbase.h>
#include <iobjsys.h>
#include <meshapi.h>
#include <creatext.h>
#include <creatapi.h>
#include <camera.h>
#include <lazyagg.h>
#include <command.h>
#include <drkwbow_.h>
#include <rendprop.h>

#include <tweqctrl.h>
#include <contain.h>

#include <schema.h>
#include <schbase.h>

#include <gen_bind.h>

#include <crwpnapi.h>   // kWeaponModeRanged

// Must be last header
#include <dbmem.h>

typedef struct
{
   BOOL bKnocked;
   BOOL equip_pending;   // make this a flags field
   BOOL equipped;
   BOOL unequip_pending;
   BOOL nock_pending;
   BOOL frob_pending;
   BOOL down_pending;
   int WobbleStartTime;
   int CollapseTime;
   int RenockTime;
} sBowWeaponInfo;

static sBowWeaponInfo gBowWeaponInfo = {FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, 8000, 11000, 900};

#define PLAYER_BOW_HACK

EXTERN ObjID g_ArrowObj=OBJ_NULL; // gets accessed in drkgame.c

//#define BOW_DEBUG_HORROR
#ifdef BOW_DEBUG_HORROR
#define bow_mprintf(x) mprintf x
#else
#define bow_mprintf(x)
#endif
////////
// BOW GLOBALS

///////
// Guess
struct cMxsVectorHack : public mxs_vector // for watcom compiler bug (toml 08-18-97)
{
   cMxsVectorHack(float _x, float _y, float _z) { z = _z; y = _y; x = _x; }
};

struct cMxsAngvecHack : public mxs_angvec // for watcom compiler bug (toml 08-18-97)
{
   cMxsAngvecHack(mxs_ang _x, mxs_ang _y, mxs_ang _z) { tz = _z; ty = _y; tx = _x; }
};

static sPlayerAbilityDesc g_BowAbilityDesc={ {"bowsite"},kDCRTYPE_PlayerBowLimb,kPBS_Begin,kPBS_NumSkills,g_BowSkillDesc};
static cPlayerAbility g_BowAbility(&g_BowAbilityDesc);

// OTHER GLOBALS
static cMxsVectorHack g_ArrowPosStart(0,0,0);
static cMxsVectorHack g_WristRelStart(0,0,0);
static cMxsVectorHack g_WristRelEnd(0,0,0);
static cMxsAngvecHack g_ArrowRotStart(0,0,0);

static cMxsVectorHack g_ArrowCamPosStart(-0.4,0.22,0.3);
static cMxsVectorHack g_ArrowCamPosEnd(-1.0,0.22,0.3);
static cMxsAngvecHack g_Arm2CamRot(0xfe12,0xfd30,0x3b0c);
static mxs_matrix g_Cam2WristMat;

static mxs_matrix g_ArrowMatrix;

static ObjID g_PlayerRealArrowStack = OBJ_NULL;
static BOOL  g_ArrowRecentlyLaunched = FALSE;
static ObjID g_LastArrowLaunched = OBJ_NULL;
static BOOL  g_Straining = FALSE;

static long  g_StartAwayTime;
static long  g_ArrowTimeSinceLaunched;
static long  g_ArrowTimePoweringUp;

static float g_WobbleAmount;
static float g_XWobble;
static float g_YWobble;
static float g_XWobbleRate;
static float g_YWobbleRate;

static BOOL  zoom_active=TRUE;

static void ArrowDestroy(void);
static BOOL ArrowCreate(void);
static void ArrowPull(float power);
static void ArrowRelease(void);
static BOOL ArrowExists(void);
static void ArrowDecrementSourceAfterFire(void);
static void RelaxPlayerArmFilter(void);

static LazyAggMember(IObjectSystem) pObjSys;

void BowAbilityCallback(int mode, cPlayerAbility **pAbility, void *data)
{
   *pAbility=&g_BowAbility;
   bow_mprintf(("BAC %d %x\n",mode,data));
}

static BOOL first_frame = TRUE;

void BowModeEnterCallback(int mode, void *data)
{
   // If an unequip is pending, it probably means that we're
   // switching between arrows (in which case we don't get an
   // exit mode callback), so we can just do the unequip right
   // before we equip.

   bow_mprintf(("Enter %d : %d %d\n",mode,gBowWeaponInfo.unequip_pending,gBowWeaponInfo.equip_pending));
   if (gBowWeaponInfo.unequip_pending)
   {
      gBowWeaponInfo.unequip_pending = FALSE;
      gBowWeaponInfo.equipped = FALSE;
   }

   if (gBowWeaponInfo.equip_pending)
   {
      gBowWeaponInfo.equip_pending = FALSE;
      gBowWeaponInfo.equipped = TRUE;
      first_frame = TRUE;
   }

   // hack - query if the bow_zoom config is on
   zoom_active = (BOOL) atoi( g_pInputBinder->ProcessCmd ("echo $bow_zoom") );
}

void BowModeExitCallback(int mode, void *data)
{
   bow_mprintf(("Exit %d : %d %d\n",mode,gBowWeaponInfo.unequip_pending,gBowWeaponInfo.equip_pending));
   if (gBowWeaponInfo.unequip_pending)
   {
      gBowWeaponInfo.unequip_pending = FALSE;
      gBowWeaponInfo.equipped = FALSE;
   }
}

EXTERN float g_ElapsedTime; // from plyrvmot.cpp

static BOOL BowGetPower(float *pPower)
{
   float powerTime;

   if (!ArrowExists())
      return FALSE;
   if (gBowWeaponInfo.frob_pending||gBowWeaponInfo.down_pending)
      return FALSE; // ack
   if ((powerTime=g_BowAbility.GetPowerUpTime())==0)
      return FALSE;
   *pPower=g_ElapsedTime/powerTime;
   if(*pPower>1.0)
      *pPower=1.0;
   return TRUE;
}

static void ForgetRecentArrow()
{
   ObjSetHasRefs(g_LastArrowLaunched,TRUE); // no longer drawn as overlay, need
                                    // to sort in world.
   g_LastArrowLaunched=OBJ_NULL;
   g_ArrowRecentlyLaunched=FALSE;
}

// yea, yea, i suck
static float zoom_start_time=3.0;
static float zoom_cap=3.0;
static float zoom_per_s=1.5;
static float zoom_out_ms=3.5/1000.0;

static void ZoomController(BOOL active, float power)
{
   static float active_zoom=1.0, this_zoom_was=0.0;
   static float unzoom_start_time=0.0;

   if (!zoom_active)
      return;

   if ((active) && (power>0.4) && (g_ElapsedTime>zoom_start_time))
   {
      active_zoom=1.0+((g_ElapsedTime-zoom_start_time)*zoom_per_s);

      if (active_zoom>zoom_cap)
         active_zoom=zoom_cap;
   }
   else if (active_zoom>1.0)
   {
      float cur_time=GetSimTime();
      if (unzoom_start_time==0.0)
      {
         unzoom_start_time=cur_time;
         this_zoom_was=active_zoom;
      }
      else
         active_zoom=this_zoom_was-((cur_time-unzoom_start_time)*zoom_out_ms);
      if (active_zoom<1.04)
      {
         active_zoom=1.0;
         unzoom_start_time=0.0;
      }
   }
   Camera* player_cam = PlayerCamera();
   // @TODO: currently assumes we are only zooming element in system
   //  probably need a better way to do this which allows combination/base offset
   player_cam->zoom = active_zoom;
}

void ZoomClear(void)
{
   Camera* player_cam = PlayerCamera();
   // @TODO: currently assumes we are only zooming element in system
   //  probably need a better way to do this which allows combination/base offset
   player_cam->zoom = 1.0;
}

#define ARROW_GRACE_PERIOD 200

// this is the callback from the player arm/bow controller thing
BOOL BowFrameCallback(ulong dt,void *data)
{
   if (ArrowExists() && !pObjSys->Exists(g_ArrowObj))
   {
      Warning(("Arrow destroyed while on bow!\n"));
      ArrowRelease();
      g_LastArrowLaunched=OBJ_NULL;
      g_ArrowRecentlyLaunched=FALSE;
      return FALSE;
   }

   first_frame = FALSE;

   float power;

   if (BowGetPower(&power))
   {  // move arrow back according to power
      // @TODO: make sure this only modified when changes
      ArrowPull(power);
      ZoomController(TRUE,power);
   }
   else
   {
      ZoomController(FALSE,0);
      if (g_ArrowTimeSinceLaunched)
         g_ArrowTimeSinceLaunched+=dt;  // just keep track of this for a while...
   }

   if (g_ArrowRecentlyLaunched)
      if (g_ArrowTimeSinceLaunched>ARROW_GRACE_PERIOD)
          ForgetRecentArrow();

   return TRUE;
}

// this is now the PutTheBowAway code
void PutTheBowAway(BOOL abort)
{
   bow_mprintf(("PTBA %s (sat %d)\n",abort?"Abort":"Finish",g_StartAwayTime));
   if (ArrowExists())
      g_StartAwayTime=1;
   if (abort)
      PlayerAbortAction();
   else
      PlayerFinishAction();
}

// ok, really, force finish doesnt want to finish the shot quite yet
void ForceFinishBowAttack(BOOL abort)
{
   bow_mprintf(("FFBA %s\n",abort?"Abort":"Finish"));
   g_WobbleAmount = 0.0;
   g_Straining = FALSE;

   WeaponEvent(kEndAttack, PlayerObject(), NULL);

   if (abort)
      PutTheBowAway(abort);
   else
   {
      bow_mprintf(("Finish actions\n"));
      PlayerFinishAction();  // hoping to fire
      gBowWeaponInfo.down_pending=TRUE;
   }

   gBowWeaponInfo.bKnocked = FALSE;
}

BOOL EquipBow()
{
   bow_mprintf(("equipbow\n"));

   // set state right
   gBowWeaponInfo.equip_pending = TRUE;
   gBowWeaponInfo.equipped = FALSE;

   // get rid of the arrow, in case left over from previous equip
   ArrowDestroy();

   // tell weapon system
   if (GetArrowArchetype())
      SetWeapon(PlayerObject(),GetArrowArchetype(), kWeaponModeRanged);

   // setup arrow stuff
   mx_ang2mat(&g_Cam2WristMat,&g_Arm2CamRot);

   // Tell the cerebellum to switch modes
   PlayerSwitchMode(kPlayerModeFlexiBow);

   // @CHECKIN: can i set this to zero now
   g_ArrowTimeSinceLaunched=5000;  // sure, its been a while

   return TRUE;
}

BOOL UnEquipBow()
{
   bow_mprintf(("unequipbow\n"));
   g_PlayerRealArrowStack=OBJ_NULL;

   // end state
   gBowWeaponInfo.equip_pending = FALSE;
   gBowWeaponInfo.unequip_pending = TRUE;
   gBowWeaponInfo.down_pending = FALSE;
   gBowWeaponInfo.frob_pending = FALSE;
   g_StartAwayTime = 0;
   ZoomClear();

   // get rid of the arrow
   ArrowDestroy();

   if (GetWeaponObjID(PlayerObject())==GetArrowArchetype())
      UnSetWeapon(PlayerObject());

   // Tell the cerebellum to switch modes
   PlayerSwitchMode(kPlayerModeUnarmed);

   return TRUE;
}

BOOL IsBowEquipped()
{
   return (gBowWeaponInfo.equipped && !gBowWeaponInfo.unequip_pending && !first_frame);
}

BOOL IsBowArrowKnocked()
{
   return gBowWeaponInfo.bKnocked;
}

BOOL IsBowWobbling()
{
   return (g_WobbleAmount > 0.0);
}

static BOOL g_WobbleRelaxing=FALSE;

static void WobbleArmFilter(mxs_vector *loc, mxs_matrix *orient)
{
   if (IsBowWobbling()||g_WobbleRelaxing)
   {
      //mprintf("g_XWobble = %g (amt %g rate %g)\n", g_XWobble, g_WobbleAmount, g_XWobbleRate);
      //mprintf("g_YWobble = %g (amt %g rate %g)\n", g_YWobble, g_WobbleAmount, g_YWobbleRate);

      mxs_matrix new_orient;
      mxs_vector unit;
      mxs_vector x_vec;
      mxs_vector y_vec;

      mx_mk_vec(&unit, 0, g_XWobble, 0);
      mx_mat_mul_vec(&x_vec, orient, &unit);
      mx_mk_vec(&unit, 0, 0, g_YWobble);
      mx_mat_mul_vec(&y_vec, orient, &unit);

      mx_add_vec(&new_orient.vec[0], &orient->vec[0], &x_vec);
      mx_addeq_vec(&new_orient.vec[0], &y_vec);

      mx_cross_vec(&new_orient.vec[1], &orient->vec[2], &new_orient.vec[0]);
      mx_cross_vec(&new_orient.vec[2], &new_orient.vec[0], &new_orient.vec[1]);

      for (int i=0; i<3; i++)
      {
         if (mx_mag2_vec(&new_orient.vec[i]) < 0.0001)
            return;

         mx_normeq_vec(&new_orient.vec[i]);
      }

      mx_copy_mat(orient, &new_orient);
   }
   if (g_WobbleRelaxing)
   {  // @TODO: need DT in here!!!
      g_XWobble*=0.95;
      g_YWobble*=0.95;
      if (fabs(g_XWobble+g_YWobble)<=0.02)
      {
         ClearPlayerArmFilter();
         g_WobbleRelaxing=FALSE;
      }
   }
}

void RelaxPlayerArmFilter(void)
{
   g_WobbleRelaxing=TRUE;
}

void WobbleBow(long dt)
{
   if (g_WobbleAmount == 0.0)
   {
      SetPlayerArmFilter(WobbleArmFilter);
      g_XWobbleRate = (float)(Rand() % 256) / 64.0;
      g_YWobbleRate = (float)(Rand() % 256) / 64.0;
      g_WobbleRelaxing=FALSE;
   }

   g_WobbleAmount = (float)dt / 1000.0;

   g_XWobble = (sin(g_WobbleAmount * g_XWobbleRate)) * g_WobbleAmount * 0.02;
   g_YWobble = (sin(g_WobbleAmount * g_YWobbleRate)) * g_WobbleAmount * 0.02;
}

static float bow_renock_mag=1.0;
#define RENOCK_TIME_MS (gBowWeaponInfo.RenockTime)
BOOL RenockBow(long dt)
{
   BOOL final_phase=FALSE;

   if (!g_WobbleAmount)
   {
      bow_mprintf(("Starting Renock\n"));
      SetPlayerArmFilter(WobbleArmFilter);
      g_WobbleRelaxing=FALSE;
   }

   float dist = (((float)dt)/(float)(RENOCK_TIME_MS/3));
   if (dist<1.0)
      dist/=20;
   else if (dist<2.0)
      dist=0.05+(0.95*(1.0-((2.0-dist)*(2.0-dist))));
   else
   {
      dist=3.0-dist;
      dist*=dist;
      final_phase=TRUE;
      if (gBowWeaponInfo.frob_pending)
         if (!ArrowExists())  // if we dont have it yet
            if (!ArrowCreate())    // lets nock it
               gBowWeaponInfo.frob_pending=FALSE; // no arrow, give up
   }
   // @TODO: why isnt end of this smooth?
   g_XWobble = -0.15 * dist * bow_renock_mag;
   g_YWobble = -0.59 * dist * bow_renock_mag;
   g_WobbleAmount = 0.1; // just so IsBowWobbling says the right thing
   return final_phase;
}

BOOL DoBowAttack()
{
   if (IsBowEquipped() && ((GetPlayerMode() == kPM_Stand) || (GetPlayerMode() == kPM_Crouch)))
   {
      if (ArrowCreate())
      {
         gBowWeaponInfo.bKnocked = TRUE;
         PlayerStartAction();  // WHY doesnt this start RIGHT AWAY!!! HATE HATE HATE
         WeaponEvent(kStartWindup, PlayerObject(), NULL);
         g_ArrowTimePoweringUp = 0;
         g_WobbleAmount = 0.0;
         bow_mprintf(("Start attack\n"));
         return TRUE;
      }
      bow_mprintf(("Cant create arrow\n"));
   }
   bow_mprintf(("Cant attack\n"));
   return FALSE;
}

void StartBowAttack()
{
   if (!PlayerObjectExists())
      return;


   if (g_PlayerRealArrowStack==OBJ_NULL)
   {
      bow_mprintf(("no stack\n"));
      return;
   }

   gBowWeaponInfo.frob_pending = TRUE;
   bow_mprintf(("SBA\n"));
}

static int BowSound(const char *name)
{
   sSchemaCallParams callDat={SCH_SET_OBJ,};
   callDat.sourceID=PlayerObject();
   return SchemaPlay((Label *)name,&callDat);
}

BOOL AbortBowAttack()
{
   bow_mprintf(("ABA\n"));
   gBowWeaponInfo.frob_pending=FALSE;
   if (IsBowEquipped())
   {  // @BUG?  what if you arent really doing anything and script is confused???
      ForceFinishBowAttack(FALSE);
      return TRUE;
   }
   return FALSE;
}

BOOL FinishBowAttack()
{
   BOOL fired = FALSE;
   float power;

   bow_mprintf(("FBA\n"));

   gBowWeaponInfo.bKnocked = FALSE;

   if (gBowWeaponInfo.frob_pending)
   {
      gBowWeaponInfo.frob_pending = FALSE;
      return FALSE;  // if we are waiting on a frob, cancel the whole thing
   }

   if (IsBowEquipped())
   {
      if (BowGetPower(&power))
         if (power>0.15)
         {
            if (pObjSys->Exists(g_ArrowObj))
            {
               int flags=PRJ_FLG_TELLAI|PRJ_FLG_FROMPOS;
               if (power < 0.4)
                  power = 0.4;
               bow_mprintf(("ACTUAL FIRING (%d)",g_ArrowObj));
               launchProjectile(PlayerObject(),g_ArrowObj,power,flags,NULL,NULL,NULL);
               TweqProcessAll(g_ArrowObj,kTweqDoActivate);
               g_ArrowRecentlyLaunched=TRUE;
               g_LastArrowLaunched=g_ArrowObj;
               ArrowDecrementSourceAfterFire();
               WeaponEvent(kStartAttack, PlayerObject(), 0);
               fired=TRUE;
            }
#ifndef SHIP
            else
               mprintf("Arrow %d destroyed while on bow!\n",g_ArrowObj);
#endif
            ArrowRelease();  // release it from the creature
            g_ArrowObj=OBJ_NULL;
         }
         else
            bow_mprintf(("Half Cocked, no fire\n"));
      else
         bow_mprintf(("no raw bow power\n"));
      if (gBowWeaponInfo.frob_pending||gBowWeaponInfo.down_pending)
         bow_mprintf(("finish w/pendings\n"));
      else
      {
         if (fired)
         {
            BowSound("bowtwang_player");
            g_ArrowTimeSinceLaunched=1;
         }
         ForceFinishBowAttack(FALSE); // !fired);
      }
   }
   return FALSE;
}

// this is from drksloop, and is called every every frame!
void UpdateBowAttack(long dt)
{
   float power;

   if (IsBowEquipped())
   {
      if (BowGetPower(&power))
      {

		  //So, the player can fire the bow while in the water.
		  //and kill himself, besides.  Let's put a stop to that,
		  //shall we? AMSD
		  //not clear if we want to disable other modes besides swimming.
		 if (GetPlayerMode() == kPM_Swim)
		 {
			 ForceFinishBowAttack(TRUE);  //die die die
			 BowSound("bow_abort_auto"); //why the heck not?
			 return;  //ok, we're done.
		 }
         bow_mprintf(("."));
         g_ArrowTimePoweringUp += dt;
         if (g_ArrowTimePoweringUp > gBowWeaponInfo.WobbleStartTime)
         {
            WobbleBow(g_ArrowTimePoweringUp - gBowWeaponInfo.WobbleStartTime);
            if (!g_Straining)
            {
               BowSound("garstrain");
               g_Straining=TRUE;
            }
         }
         if (g_ArrowTimePoweringUp > gBowWeaponInfo.CollapseTime)
         {
            ForceFinishBowAttack(TRUE);
            BowSound("bow_abort_auto");
         }
      }
      else if ((g_ArrowTimeSinceLaunched!=0)&&
               (g_ArrowTimeSinceLaunched<RENOCK_TIME_MS))
      {
         bow_mprintf(("+"));
         ZoomController(FALSE,0);
         // in final phase
         if (RenockBow(g_ArrowTimeSinceLaunched))
            if ((!gBowWeaponInfo.frob_pending) && gBowWeaponInfo.down_pending) // and no frob...
            {
               g_ArrowTimeSinceLaunched=0; // hack hack hack
               bow_mprintf(("Attempt to abort\n"));
            }
      }                // so we fall through to the down pending case...
      else if (gBowWeaponInfo.frob_pending)
      {
         bow_mprintf(("FrobPend\n"));
         ClearPlayerArmFilter();
         ZoomClear();
         DoBowAttack();
         gBowWeaponInfo.frob_pending = FALSE;
         gBowWeaponInfo.down_pending = FALSE;
      }
      else if (gBowWeaponInfo.down_pending)
      {
         bow_mprintf(("DownPend\n"));
         RelaxPlayerArmFilter();
         ZoomController(FALSE,0);
         g_WobbleAmount = 0.0;
         PutTheBowAway(FALSE);
         gBowWeaponInfo.down_pending = FALSE;
      }
      else if (g_StartAwayTime)
      {
         bow_mprintf(("*"));
         g_StartAwayTime+=dt;
         if (g_StartAwayTime>500)
            ArrowDestroy();
      }
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

void set_arrow_arch(ObjID arch)
{
   Assert_(OBJ_IS_ABSTRACT(arch));

   arrow_type = arch;
}

// @TODO: fix this for NULL, general rightness
BOOL SetCurrentArrow(ObjID arrow)
{
   AutoAppIPtr_(TraitManager, pTraitMan);

   g_PlayerRealArrowStack=arrow;
   ObjID arch = pTraitMan->GetArchetype(arrow);
   if (arch != OBJ_NULL)
   {
      set_arrow_arch(arch);
      if (ArrowExists())
      {
         ArrowDestroy();
         ArrowCreate();
      }
      if (ArrowExists())
         SetWeapon(PlayerObject(),GetArrowArchetype(), kWeaponModeRanged);
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

static BOOL ArrowCreate()
{
   ObjID archetype=GetArrowArchetype();
   ObjPos *pPos;

   if (g_ArrowRecentlyLaunched) // can only draw one arrow as overlay at a time
   {
      ForgetRecentArrow();
      bow_mprintf(("using odd forget in create\n"));
   }

   g_StartAwayTime=0; // just to make sure - esp for this case!
   if (ArrowExists()) // hopefully from renock, i guess
   {
      bow_mprintf(("already have arrow %s in create\n",ObjEditName(g_ArrowObj)));
      return TRUE;
   }

   if (g_PlayerRealArrowStack==OBJ_NULL)
   {
      bow_mprintf(("Trying to create, no stack\n"));
      return FALSE;
   }

   if (PlayerArm()==OBJ_NULL)
   {
#ifndef SHIP
      mprintf("Hey, no arm, bad scene\n");
#endif
      return FALSE;
   }

   if (archetype!=OBJ_NULL)
   {  // get arrow from the stack
      AutoAppIPtr_(ContainSys,pContainSys);
      g_ArrowObj=pContainSys->RemoveFromStack(g_PlayerRealArrowStack,
                                              kStackRemoveNoDecrement|kStackRemoveAllowInfinite,1);

      if (g_ArrowObj==OBJ_NULL)
         return FALSE;

      ConfigSpew("create_arrow_spew",("Creating arrow %d, archetype %d\n",g_ArrowObj,archetype));

      if (NULL!=(pPos=ObjPosGet(PlayerArm())))
         ObjPosUpdate(g_ArrowObj,&pPos->loc.vec,&pPos->fac);
      else
         AssertMsg(FALSE,"Cannot create arrow when no player arm");

      if (PhysObjHasPhysics(g_ArrowObj)) // don't want it physical yet
         PhysDeregisterModel(g_ArrowObj);

      // attach arrow to player arm
      sCreatureAttachInfo attachInfo;

      attachInfo.joint = kCJ_RWrist;
      attachInfo.is_weapon = FALSE;
      attachInfo.weapon = kCrWeap_Invalid;

      CamToWristPos(&g_WristRelStart,&g_ArrowCamPosStart);
      CamToWristPos(&g_WristRelEnd,&g_ArrowCamPosEnd);
      mx_copy_vec(&attachInfo.relTrans.vec,&g_WristRelStart);
      mx_copy_mat(&attachInfo.relTrans.mat,&g_Cam2WristMat);
      CreatureAttachItem(PlayerArm(),g_ArrowObj,&attachInfo);

      ObjSetHasRefs(g_ArrowObj,FALSE); // don't want it sorted in world yet
   }
   bow_mprintf(("ArrowCreate %d\n",g_ArrowObj));
   return TRUE;
}

static void ArrowPull(float power)
{
   if (power<0)
      power=0;
   else if (power>1.0)
      power=1.0;

   sCreatureAttachInfo attachInfo;

   attachInfo.joint = kCJ_RWrist;
   attachInfo.is_weapon = FALSE;
   attachInfo.weapon = kCrWeap_Invalid;

   mx_interpolate_vec(&attachInfo.relTrans.vec,&g_WristRelStart,&g_WristRelEnd,power);
   mx_copy_mat(&attachInfo.relTrans.mat,&g_Cam2WristMat);

   CreatureAttachmentModify(PlayerArm(),g_ArrowObj,&attachInfo);
}

static void ArrowDecrementSourceAfterFire()
{
   bow_mprintf(("ArrowDecrement %d (stack %d)\n",g_ArrowObj,g_PlayerRealArrowStack));
   if (g_PlayerRealArrowStack)
   {
      AutoAppIPtr_(ContainSys,pContainSys);
      pContainSys->StackAdd(g_PlayerRealArrowStack,-1,kStackAddDestroyIfZero|kStackAddAllowInfinite);
   }
}

static void ArrowRelease()
{
   bow_mprintf(("ArrowRelease %d\n",g_ArrowObj));
   if (ArrowExists())
   {
      CreatureDetachItem(PlayerArm(),g_ArrowObj);
      g_ArrowObj=OBJ_NULL;
   }  // forget about arrow, since no longer attached or our responsibility
}

static void ArrowDestroy()
{
   bow_mprintf(("ArrowDestroy %d\n",g_ArrowObj));
   if (ArrowExists())
   {
      pObjSys->Destroy(g_ArrowObj);
      g_ArrowObj=OBJ_NULL;
      g_StartAwayTime=0;
   }
   else if (g_StartAwayTime)
      bow_mprintf(("Hey, ArrowDestroy, StartAwayTime, no arrow\n"));
}

// still consider arrow to be player arrow for a short time after
// arrow has been launched
ObjID PlayerArrow()
{
   if (g_ArrowObj)
      return g_ArrowObj;
   if (g_ArrowRecentlyLaunched)
      return g_LastArrowLaunched;
   return OBJ_NULL;
}

// for doing tests to get the bow correctly aligned in the world
//  so the parameters can be set right
#ifdef EDITOR

EXTERN void PortalMovePointInsideCell(Location *loc); // from wrfunc.c in portal lib

#if 0
static void PrintMatrix(char *name, mxs_matrix *pMat)
{
   mprintf("%s : \n",name);
   mprintf("%g, %g, %g\n%g, %g, %g\n%g, %g, %g\n\n",pMat->el[0],pMat->el[3],\
   pMat->el[6],pMat->el[1],pMat->el[4],pMat->el[7],pMat->el[2],pMat->el[5],\
   pMat->el[8]);
}
#endif

// actual command to do so
EXTERN void PlayerBowAlignArrow(void)
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

   // do silliness since camera jitters when crosses portal boundary
   camPos=*ObjPosGet(pCam->objid);
   mx_sub_vec(&camOffset,&pCam->pos,&camPos.loc.vec);
   PortalMovePointInsideCell(&camPos.loc);
   mx_addeq_vec(&camPos.loc.vec,&camOffset);
   mx_copy_vec(&camVec,&camPos.loc.vec);
   mx_ang2mat(&mat,&pCam->ang);

#if 0
   mxs_matrix mat;
   PrintMatrix("Cw",&mat);
   PrintMatrix("Hw",&trans.mat);
#endif

   // hand-relative orientation is (Hand orientation in world)^-1*
   //                               (Arrow orientation in world).
   mx_transeq_mat(&trans.mat); // invert
   mx_mul_mat(&relMat,&trans.mat,&mat);
   mx_mat2ang(&g_ArrowRotStart,&relMat);
   mx_copy_mat(&g_ArrowMatrix,&relMat);

#if 0
   mxs_matrix testMat;   // what arrow thinks rel orientation of wrist to camera is
   mx_trans_mat(&testMat,&g_ArrowMatrix);
   PrintMatrix("Hw^-1",&trans.mat);
   PrintMatrix("Ah",&g_ArrowMatrix);
   PrintMatrix("Ah^-1==Hc",&testMat);
#endif

   // move arrow on player arm
   sCreatureAttachInfo attachInfo;

   attachInfo.joint = kCJ_RWrist;
   attachInfo.is_weapon = FALSE;
   attachInfo.weapon = kCrWeap_Invalid;
   mx_copy_vec(&attachInfo.relTrans.vec,&g_ArrowPosStart);
   mx_copy_mat(&attachInfo.relTrans.mat,&g_ArrowMatrix);

   CreatureAttachmentModify(PlayerArm(),g_ArrowObj,&attachInfo);
   CreatureAttachmentsPosUpdate(armObj);

   mprintf("arrow rotation: %x, %x, %x\n",g_ArrowRotStart.tx,g_ArrowRotStart.ty,g_ArrowRotStart.tz);
}

Command WeapBowCommands[] =
{
   { "player_align_arrow", FUNC_VOID, PlayerBowAlignArrow, "calc arrow alignment so straight" },
};
#endif

void SetBowZoomActive (BOOL active)
{
   zoom_active = active;
}

void BowInit(void)
{
   // Set the callback for when it's done unequipping
   PlayerHandlerSetLeaveModeCallback(kPlayerModeFlexiBow, BowModeExitCallback, NULL);
   // Set the callback for when it's done equipping
   PlayerHandlerSetEnterModeCallback(kPlayerModeFlexiBow, BowModeEnterCallback, NULL);

   PlayerHandlerSetAbilityCallback(kPlayerModeFlexiBow, BowAbilityCallback, NULL);
   PlayerHandlerSetFrameCallback(kPlayerModeFlexiBow, BowFrameCallback, NULL);

#ifdef EDITOR
   COMMANDS(WeapBowCommands,HK_GAME_MODE);
   config_get_float("bow_zoom_start_time",&zoom_start_time);
   config_get_float("bow_zoom_cap",&zoom_cap);
   config_get_float("bow_zoom_per_s",&zoom_per_s);
   config_get_float("bow_zoom_out_ms",&zoom_out_ms);
   if (config_is_defined("zoom_deactivate"))
      zoom_active=FALSE;
   config_get_int("bow_wobble_start_ms",&gBowWeaponInfo.WobbleStartTime);
   config_get_int("bow_collapse_time",&gBowWeaponInfo.CollapseTime);
   config_get_int("bow_renock_time",&gBowWeaponInfo.RenockTime);
   config_get_float("bow_renock_mag",&bow_renock_mag);
#endif
}

void BowTerm(void)
{
}
