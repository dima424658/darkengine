/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

////////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/physics/phmoapi.cpp,v 1.8 1999/12/20 16:46:24 adurant Exp $
//
// Physics motion api
//

#include <lg.h>
#include <matrix.h>
#include <playrobj.h>

#include <config.h>

#include <phcore.h>
#include <phmods.h>
#include <phmod.h>

#include <phconst.h>
#include <phmoapi.h>
#include <phmotion.h>

// Must be last header
#include <dbmem.h>

////////////////////////////////////////////////////////////////////////////////

static float gEyeLoc;

void PhysAdjustPlayerHead(mxs_vector *pos, mxs_angvec *ang)
{
   cPhysModel *pModel;

   mxs_matrix upright_orien;
   mxs_matrix head_orien;
   mxs_matrix new_orien;
   mxs_matrix delta_orien;

   mxs_vector eye_offset;
   mxs_vector cheat;
   mxs_vector neck;
   mxs_vector obj_neck;
   mxs_vector forward;    // forward (absolute unit vec)
   mxs_vector x_axis;     // forward (new)
   mxs_vector y_axis;     // left ear

   mxs_angvec facing;

   // Adjust eye offset
   mx_mk_vec(&eye_offset, 0, 0, gEyeLoc); // .8);
   mx_addeq_vec(pos, &eye_offset);

   if (!PlayerObjectExists())
      return;

   pModel = g_PhysModels.GetActive(PlayerObject());
   if ((pModel == NULL) || (pModel->NumSubModels() < 3) || (pModel->IsMantling()))
      return;

   // Find neck vector ("up" for head)
   mx_sub_vec(&neck, &pModel->GetLocationVec(0), &pModel->GetLocationVec());
   mx_scale_vec(&cheat, &kGravityDir, -30);
   mx_addeq_vec(&neck, &cheat);
   mx_normeq_vec(&neck);

   // Rotate neck to object's z-orientation
   mx_mk_angvec(&facing, 0, 0, (short)-ang->tz);
   mx_ang2mat(&upright_orien, &facing);
   mx_mat_mul_vec(&obj_neck, &upright_orien, &neck);

   // Compute "left ear" vector by crossing forward looking and neck
   mx_unit_vec(&forward, 0);
   mx_cross_vec(&y_axis, &obj_neck, &forward);

   // Compute "forward" vector by crossing neck and right ear
   mx_cross_vec(&x_axis, &y_axis, &obj_neck);

   // Build matrix from this
   mx_copy_vec(&delta_orien.vec[0], &x_axis);
   mx_copy_vec(&delta_orien.vec[1], &y_axis);
   mx_copy_vec(&delta_orien.vec[2], &obj_neck);

   mx_ang2mat(&head_orien, ang);

   // Rotate the offset by the head angle
   mx_mul_mat(&new_orien, &head_orien, &delta_orien);

   mx_mat2ang(ang, &new_orien);
}

////////////////////////////////////////////////////////////////////////////////

cPlayerMotion *pPlayerMotion = NULL;

void InitPlayerMotion()
{
   Assert_(pPlayerMotion == NULL);

   pPlayerMotion = new cPlayerMotion;

   gEyeLoc = 0.8;
   config_get_float("eyeloc",&gEyeLoc);
}

////////////////////////////////////////

void TermPlayerMotion()
{
   Assert_(pPlayerMotion != NULL);

   delete pPlayerMotion;
   pPlayerMotion = NULL;
}

////////////////////////////////////////

void ResetPlayerMotion()
{
   if (pPlayerMotion != NULL)
      TermPlayerMotion();
   InitPlayerMotion();
}

////////////////////////////////////////////////////////////////////////////////

void PlayerMotionActivate(ePlayerMotion motion)
{
   Assert_(pPlayerMotion != NULL);

   pPlayerMotion->Activate(motion);
}

////////////////////////////////////////

ePlayerMotion PlayerMotionGetActive()
{
   Assert_(pPlayerMotion != NULL);

   return pPlayerMotion->GetActive();
}

////////////////////////////////////////

void PlayerMotionActivateList(ePlayerMotion *motions, int size)
{
   Assert_(pPlayerMotion != NULL);
   
   pPlayerMotion->ActivateList(motions, size);
}

////////////////////////////////////////

void PlayerMotionSetRest(ePlayerMotion restMotion)
{
   Assert_(pPlayerMotion != NULL);

   pPlayerMotion->SetRest(restMotion);
}

////////////////////////////////////////

ePlayerMotion PlayerMotionGetRest()
{
   Assert_(pPlayerMotion != NULL);

   return pPlayerMotion->GetRest();
}

////////////////////////////////////////////////////////////////////////////////

void PlayerMotionSetOffset(short submod, mxs_vector *offset)
{
   Assert_(pPlayerMotion != NULL);

   pPlayerMotion->SetOffset(submod, offset);
}

////////////////////////////////////////

void PlayerMotionGetOffset(short submod, mxs_vector *offset)
{
   Assert_(pPlayerMotion != NULL);

   pPlayerMotion->GetOffset(submod, offset);
}

////////////////////////////////////////////////////////////////////////////////

void PlayerMotionUpdate(mxs_real dt)
{
   Assert_(pPlayerMotion != NULL);
 
   pPlayerMotion->Update(dt);
}

////////////////////////////////////////////////////////////////////////////////

mxs_vector PlayerGetEyeOffset()
{
  mxs_vector eye_offset;
  mx_mk_vec(&eye_offset,0,0,gEyeLoc); 
  return eye_offset;
}

////////////////////////////////////////////////////////////////////////////////







