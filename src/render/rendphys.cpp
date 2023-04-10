/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/rendphys.cpp,v 1.12 1998/10/05 17:27:35 mahk Exp $

#include <lg.h>
#include <phcore.h>
#include <phmods.h>
#include <phmodsph.h>
#include <phmodobb.h>
#include <rendphys.h>
#include <creatext.h>
#include <creature.h>
#include <creatur_.h>

// Must be last header 
#include <dbmem.h>


BOOL RendPhysGetSphereModelInfo(ObjID obj, mxs_vector *pos, mxs_real *size, int *maxSubModels)
{
   cPhysModel *pModel;
   tPhysSubModId numSubModels;
   int i;

   if ((pModel = g_PhysModels.GetActive(obj)) == NULL)
   {
      *maxSubModels = 0;
      return FALSE;
   }

   if ((pModel->GetType(0) != kPMT_Sphere) && (pModel->GetType(0) != kPMT_Point) && (pModel->GetType(0) != kPMT_SphereHat))
   {
      *maxSubModels = 0;
      return FALSE;
   }

   numSubModels = pModel->NumSubModels();
   if (numSubModels < *maxSubModels)
      *maxSubModels = numSubModels;

   for(i=0; i<*maxSubModels; i++)
   {
      pos[i] = pModel->GetLocationVec(i);
      size[i] = ((cPhysSphereModel *)pModel)->GetRadius(i);
   }

   return TRUE;
}


BOOL RendPhysGetOBBModelInfo(ObjID obj, mxs_vector *pos, mxs_angvec *fac, mxs_vector *bbox)
{
   cPhysModel *pModel;

   if ((pModel = g_PhysModels.GetActive(obj)) == NULL)
      return FALSE;

   if (pModel->GetType(0) != kPMT_OBB)
      return FALSE;

   *fac = pModel->GetRotation();

   mxs_matrix obb_rot;
   mxs_vector obb_offset;

   mx_ang2mat(&obb_rot, fac);
   mx_mat_mul_vec(&obb_offset, &obb_rot, &((cPhysOBBModel *)pModel)->GetOffset());

   mx_add_vec(pos, &pModel->GetLocationVec(), &obb_offset);
   mx_copy_vec(bbox, &((cPhysOBBModel *)pModel)->GetEdgeLengths());   
   return TRUE;
}


BOOL RendPhysGetVectors(ObjID obj, mxs_vector *pos, mxs_vector *velocity, mxs_vector *forces, mxs_vector constraints[], int *num_constraints)
{
   cPhysModel   *pModel;
   cPhysDynData *pDynamics;

   if ((pModel = g_PhysModels.GetActive(obj)) == NULL)
      return FALSE;

   pDynamics = pModel->GetDynamics();   

   *pos = pModel->GetLocationVec();     
   *velocity = pDynamics->GetVelocity();
   *forces = pDynamics->GetAcceleration();

   pModel->GetConstraints(constraints, num_constraints);

   return TRUE;
}

BOOL RendPhysGetPhysBBox(ObjID obj, mxs_vector *pos, mxs_angvec *fac, mxs_vector *bbox)
{
   cPhysModel *pModel = g_PhysModels.Get(obj);

   if (pModel == NULL)
      return FALSE;

   mxs_vector minvec;
   mxs_vector maxvec;
   PhysGetAABBox(obj, &minvec, &maxvec);

   mx_add_vec(pos, &minvec, &maxvec);
   mx_scaleeq_vec(pos, 0.5);

   mx_mk_angvec(fac, 0, 0, 0);

   mx_sub_vec(bbox, &maxvec, &minvec);

   return TRUE;
}


BOOL RendCreatureGetJointInfo(ObjID obj, mxs_vector *pos, int *num)
{
   cCreature *pCreature;
   const sCreatureDesc *pCreatureDesc;

   if ((pCreature = CreatureFromObj(obj)) != NULL)
   {
      pCreatureDesc = pCreature->GetCreatureDesc();

      for (int i=0; (i<pCreatureDesc->nJoints) && (i<*num); i++)
      {
         pos[i] = pCreature->GetJoint(i);
      }

      if (pCreatureDesc->nJoints < *num)
         *num = pCreatureDesc->nJoints;

      return TRUE; 
   }
   else
      return FALSE;
}


