/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/plyrvmot.cpp,v 1.12 1998/09/19 23:19:51 CCAROLLO Exp $
//
// implementations of "virtual motion" functions used by player
//
// @TODO: need to come up with way to make virtual motions not need
// to have global scope

#include <plyrvmot.h>
#include <mp.h>
#include <camera.h>
#include <objtype.h>
#include <playrobj.h>
#include <fix.h>
#include <label.h>
#include <motmngr.h>
#include <camera.h>
#include <playrobj.h>
#include <wrfunc.h>


// must be last header
#include <dbmem.h>

static mxs_vector g_StartPos;
static mxs_angvec g_StartAng;
static mxs_vector g_EndPos;
static mxs_angvec g_EndAng;

static float g_Duration=0; // in seconds
EXTERN float g_curFrame=0;
EXTERN float g_ElapsedTime=0;

EXTERN float g_ElapsedFlexTime=0;
EXTERN float g_FlexDuration=0; // in seconds


// XXX jittery camera hack.
EXTERN void PortalMovePointInsideCell(Location *loc); // from wrfunc.c in portal lib

EXTERN BOOL g_PlayerArmAttached=TRUE; 

// XXX these should get folded into maneuvers..
// will leave here for now KJ 12/97


static PlayerArmFilterFunc gPlayerArmFilter = NULL;

void SetPlayerArmFilter(PlayerArmFilterFunc filterFunc)
{
   gPlayerArmFilter = filterFunc;
}

void ClearPlayerArmFilter()
{
   gPlayerArmFilter = NULL;
}

bool BaseMotion(multiped *mp, mps_motion_info *m, int frame, quat *rot, mxs_vector *trans)
{
   Position camPos;
   Camera *pCam=PlayerCamera();
   mxs_vector camOffset;
   mxs_vector cpos;
   mxs_angvec cang;
   mxs_matrix mat;
   int cell;
   quat camRot,armRot;

   // do silliness since camera jitters when crosses portal boundary

   if(g_PlayerArmAttached)
   {
      CameraGetLocation(pCam, &cpos, &cang);
      MakePositionFromVectors(&camPos, &cpos, &cang);
      cell=CellFromLoc(&camPos.loc); // need to call this to calc cell in position struct

      mx_sub_vec(&camOffset,&pCam->pos,&camPos.loc.vec);
      if (cell != CELL_INVALID)
         PortalMovePointInsideCell(&camPos.loc);
      mx_addeq_vec(&camPos.loc.vec,&camOffset);
      mx_ang2mat(&mat,&pCam->ang);
   } else
   {
      mx_identity_mat(&mat);
      mx_zero_vec(&camPos.loc.vec);
   }

   if (gPlayerArmFilter)
      gPlayerArmFilter(&camPos.loc.vec, &mat);

    quat_from_matrix(&camRot,&mat);

   int i;
   quat *pRot=rot;

   g_ElapsedTime=frame/((float)m->freq);

   // it matters that these get set because otherwise
   // on frame that overlay ends, these will get applied before new overlay
   // can start up.
   for(i=0;i<mp->num_joints;i++,pRot++)
   {
      quat_copy(pRot,&mp->rel_orients[i]);
   }
   // set base orientation
   quat_identity(&rot[mp->num_joints]);

   mx_mat_mul_vec(&camOffset,&mat,&g_StartPos);
   mx_add_vec(trans,&camOffset,&camPos.loc.vec);
   mx_ang2mat(&mat,&g_StartAng);
   quat_from_matrix(&armRot,&mat);
   quat_mul(&rot[mp->root_joint],&armRot,&camRot);

   return TRUE;
}

// this isn't what we really want it initialized to, but 
// need to have it initialized here so that compiler knows where memory
// should live, since EXTERNED (right?).  Watcom bug only lets me initialize to 0
EXTERN quat g_FlexTopStartRot={0,0,0,0};  
EXTERN quat g_FlexTopEndRot={0,0,0,0};
EXTERN quat g_FlexBotStartRot={0,0,0,0};
EXTERN quat g_FlexBotEndRot={0,0,0,0};

// full "body" virtual motion that includes flexing
bool FlexBowMotion(multiped *mp, mps_motion_info *m, int frame, quat *rot, mxs_vector *trans)
{
   int i;
   quat *pRot=rot;
   float frac=1.0;

   Position camPos;
   Camera *pCam=PlayerCamera();
   mxs_vector camOffset;
   mxs_vector cpos;
   mxs_angvec cang;
   mxs_matrix mat;
   int cell;
   quat camRot,armRot;

   if(g_PlayerArmAttached)
   {
      // do silliness since camera jitters when crosses portal boundary
      CameraGetLocation(pCam, &cpos, &cang);
      MakePositionFromVectors(&camPos, &cpos, &cang);
      cell=CellFromLoc(&camPos.loc); // need to call this to calc cell in position struct

      mx_sub_vec(&camOffset,&pCam->pos,&camPos.loc.vec);
      if (cell != CELL_INVALID)
         PortalMovePointInsideCell(&camPos.loc);
      mx_addeq_vec(&camPos.loc.vec,&camOffset);
      mx_ang2mat(&mat,&pCam->ang);

   } else
   {
      mx_identity_mat(&mat);
      mx_zero_vec(&camPos.loc.vec);
   }

   if (gPlayerArmFilter)
      gPlayerArmFilter(&camPos.loc.vec, &mat);

   quat_from_matrix(&camRot,&mat);

   g_ElapsedTime=frame/((float)m->freq);

   // it matters that these get set because otherwise
   // on frame that overlay ends, these will get applied before new overlay
   // can start up.
   for(i=0;i<mp->num_joints;i++,pRot++)
   {
      quat_copy(pRot,&mp->rel_orients[i]);
   }
   // set base orientation
   quat_identity(&rot[mp->num_joints]);


   mx_mat_mul_vec(&camOffset,&mat,&g_StartPos);
   mx_add_vec(trans,&camOffset,&camPos.loc.vec);
   mx_ang2mat(&mat,&g_StartAng);
   quat_from_matrix(&armRot,&mat);
   quat_mul(&rot[mp->root_joint],&armRot,&camRot);

   // do flexing calcs
   if(mp->num_joints!=8)
   {
      Warning(("FlexMotion: wrong multiped\n"));
      return FALSE;
   }
   // use current frame to figure out how much time has elapsed, and hence
   // how far through motion we are.
   if(g_FlexDuration<=0) 
   {
      frac=0;
      quat_copy(&rot[4],&g_FlexTopStartRot);
      quat_copy(&rot[6],&g_FlexBotStartRot);
   } else
   {
      g_ElapsedFlexTime=frame/((float)m->freq);
      frac=g_ElapsedFlexTime/g_FlexDuration;
      if(frac>=1.0)
      {
         frac=1.0;
         quat_copy(&rot[4],&g_FlexTopEndRot);
         quat_copy(&rot[6],&g_FlexBotEndRot);
      } else
      {
         quat_slerp(&rot[4],&g_FlexTopStartRot,&g_FlexTopEndRot,frac);
         quat_slerp(&rot[6],&g_FlexBotStartRot,&g_FlexBotEndRot,frac);
      }
   }
   return TRUE;
}

////////////////////////////////////////////////
////////////////////////////////////////////////
///
///   Externally accessable functions
///
////////////////////////////////////////////////
////////////////////////////////////////////////

EXTERN void PlyrVMotSetOffsets(mxs_vector *pPos, mxs_angvec *pAng)
{
   mx_copy_vec(&g_StartPos,pPos);
   g_StartAng=*pAng;
   mx_copy_vec(&g_EndPos,pPos);
   g_EndAng=*pAng;
}

EXTERN void PlyrVMotSetStartOffsets(mxs_vector *pPos, mxs_angvec *pAng)
{
   mx_copy_vec(&g_StartPos,pPos);
   g_StartAng=*pAng;
}

EXTERN void PlyrVMotSetEndOffsets(mxs_vector *pPos, mxs_angvec *pAng)
{
   mx_copy_vec(&g_EndPos,pPos);
   g_EndAng=*pAng;
}

EXTERN void PlyrVMotSetDuration(float seconds)
{
   g_Duration=seconds;
}

EXTERN void PlyrVMotRegisterMotions()
{
   mps_motion m;

   Label synchName={"camSynch"};
   Label flexName={"bowFlex"};

   // initialize globals
   mx_zero_vec(&g_StartPos);
   mx_zero_vec(&g_EndPos);
   g_StartAng.tx=g_StartAng.ty=g_StartAng.tz=0;
   g_EndAng.tx=g_EndAng.ty=g_EndAng.tz=0;
   quat_identity(&g_FlexTopStartRot);
   quat_identity(&g_FlexTopEndRot);
   quat_identity(&g_FlexBotStartRot);
   quat_identity(&g_FlexBotEndRot); 	

   // Camera tracking motion
   m.info.type = MT_VIRTUAL;
   m.info.num_frames = -1;
	m.info.freq = 100;
	m.virtual_update = BaseMotion;
   m.info.sig=AllJointSig(5); // input is num joints..
	m.num_flags = 0;
	m.flags = NULL;
   g_pMotionSet->RegisterVirtualMotion(&synchName,&m);

   // Camera tracking motion for flexi-bow
	m.info.type = MT_VIRTUAL;
	m.info.num_frames = -1;
	m.info.freq = 100;
	m.virtual_update = FlexBowMotion;
   m.info.sig=AllJointSig(8); // input is num joints..
	m.num_flags = 0;
	m.flags = NULL;
   g_pMotionSet->RegisterVirtualMotion(&flexName,&m);
}

int PlyrVMotNumMotions()
{
   return 4;
}
