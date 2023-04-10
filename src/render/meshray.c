/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/meshray.c,v 1.2 2000/02/19 12:35:27 toml Exp $

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\
   meshray.c

   This is the RPG engine wrapper for the MM-ray intersector.
   Although it takes an mms_ray_hit_detect_info as an argument,
   it overwrites that structure's input fields.  So it's really
   only used for output, in the case where the ray hit the model.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */


#include <r3d.h>

#include <creatext.h>
#include <mnamprop.h>
#include <objmodel.h>
#include <objpos.h>
#include <objshape.h>
#include <wrtype.h>

#include <meshray.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 


// @TODO: At the moment these are in rendobj.c, though that may change
// when we reorganize our object model management.  When that's done,
// we should be able to get these from a header somewhere.  (Mat, 1/2/98)
extern void stupid_jointpos_callback(mms_model *m, int joint_id,
                                     mxs_trans *t);
extern void stupid_stretchy_joint_callback(mms_model *m, int joint_id,
                                           mxs_trans *t, quat *rot);


BOOL MeshRayIntersection(mxs_vector *pStart, mxs_vector *pEnd,
                         BOOL bEarlyOut, ObjID Obj,
                         mms_ray_hit_detect_info *pDetectInfo)
{
   char szModelName[80];
   mms_model *pModel;
   BOOL bResult;
   long iAppData;
   int iModelIdx;
   Position *pPos;

   // @TODO: this is really sick--can we do better?
   ObjGetModelName(Obj, szModelName);
   iModelIdx = objmodelGetIdx(szModelName);
   pModel = (mms_model *) objmodelGetModel(iModelIdx);

#ifndef SHIP
   AssertMsg1(objmodelGetModelType(iModelIdx) == OM_TYPE_MESH,
              "MeshRayIntersection: object %d has no mesh model.\n", Obj);
#endif // ~SHIP

   if (CreatureGetRenderHandle(Obj, &iAppData)) {
      pModel->app_data = iAppData;
      mm_set_joint_pos_cback(CreatureMeshJointPosCallback);
      mm_set_stretchy_joint_cback(CreatureMeshStretchyJointCallback);
   } else {
      pModel->app_data = (long) Obj;
      mm_set_joint_pos_cback(stupid_jointpos_callback);
      mm_set_stretchy_joint_cback(stupid_stretchy_joint_callback);
   }

   pDetectInfo->start = pStart;
   pDetectInfo->end = pEnd;
   pDetectInfo->early_out = bEarlyOut;

   pPos = ObjPosGet(Obj);

#ifndef SHIP
   AssertMsg1(pPos != NULL,
              "MeshRayIntersection: object %d has no position\n", Obj);
#endif // ~SHIP

   pDetectInfo->bound_center = &pPos->loc.vec;
   pDetectInfo->bound_radius = ObjGetRadius(Obj);

   r3_start_frame();
   bResult = mm_ray_hit_detect(pModel, pDetectInfo);
   r3_end_frame();

   return bResult;
}
