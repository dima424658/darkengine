/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/physics/phmodutl.cpp,v 1.17 2000/02/19 12:32:20 toml Exp $
//
//
//

#include <lg.h>
#include <matrix.h>
#include <matrixs.h>

#include <objtype.h>
#include <phystyp2.h>
#include <phcore.h>
#include <phprop.h>
#include <phcontct.h>

#include <phmod.h>
#include <phmods.h>
#include <phmodutl.h>
#include <phmodsph.h>
#include <phmodbsp.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

///////////////////////////////////////////////////////////////////////////////
//
// Does this object have an active model? If so, find it.
//

BOOL PhysHasActiveModel(ObjID objID, cPhysModel ** ppModel)
{
#if 0
   if (g_pHighModelProperty->Get(objID, ppModel) &&
       ((*ppModel)->IsActive()))
      return TRUE;
   if (g_pLowModelProperty->Get(objID, ppModel) &&
       ((*ppModel)->IsActive()))
      return TRUE;
   return FALSE;
#else
   *ppModel = g_PhysModels.GetActive(objID);
   return !!(*ppModel);
#endif
}

///////////////////////////////////////////////////////////////////////////////

typedef mxs_real(*GetObjectsNormalFn) (cPhysModel * pModel1, tPhysSubModId subModId1,
                                       cPhysModel * pModel2, tPhysSubModId subModId2,
                                       mxs_vector & normal);

///////////////////////////////////////

mxs_real GetSphereVsSphereNormal(cPhysSphereModel * pModel1, tPhysSubModId subModId1,
                                 cPhysSphereModel * pModel2, tPhysSubModId subModId2,
                                 mxs_vector & normal)
{
   mxs_real mag;
   mx_sub_vec(&normal, (mxs_vector *) & pModel1->GetLocationVec(subModId1),
              (mxs_vector *) & pModel2->GetLocationVec(subModId2));
   mag = mx_mag_vec(&normal);
   mx_scaleeq_vec(&normal, 1 / mag);

   float radius1 = pModel1->GetRadius(subModId1);
   float radius2 = pModel2->GetRadius(subModId2);

   if (pModel1->IsFancyGhost() && (radius1 > 1.0) && !pModel2->IsPlayer())
      radius1 = 1.0;
   if (pModel2->IsFancyGhost() && (radius2 > 1.0) && !pModel1->IsPlayer())
      radius2 = 1.0;

   return mag - radius1 - radius2;
}

///////////////////////////////////////

mxs_real GetSphereVsBSPNormal(cPhysModel * pModel1, tPhysSubModId subModId1,
                              cPhysModel * pModel2, tPhysSubModId subModId2,
                              mxs_vector & normal)
{
   CriticalMsg("GetSphereVsBSPNormal: not implemented yet\n");
   return 0;
}

///////////////////////////////////////

mxs_real GetSphereVsOBBNormal(cPhysModel * pModel1, tPhysSubModId subModId1,
                              cPhysModel * pModel2, tPhysSubModId subModId2,
                              mxs_vector & normal)
{
   cFaceContact faceContact((cPhysOBBModel *)pModel2, subModId2);

   mx_copy_vec(&normal, &faceContact.GetNormal());

   float radius = ((cPhysSphereModel *)pModel1)->GetRadius(subModId1);

   if (pModel1->IsFancyGhost() && (radius > 1.0))
      radius = 1.0;

   return mx_dot_vec(&pModel1->GetLocationVec(subModId1), &faceContact.GetNormal()) - faceContact.GetPlaneConst() - 
      radius;
}

///////////////////////////////////////

mxs_real GetBSPVsSphereNormal(cPhysModel * pModel1, tPhysSubModId subModId1,
                              cPhysModel * pModel2, tPhysSubModId subModId2,
                              mxs_vector & normal)
{
   CriticalMsg("GetBSPVsSphereNormal: not implemented yet\n");
   return 0;
}

///////////////////////////////////////

mxs_real GetBSPVsBSPNormal(cPhysModel * pModel1, tPhysSubModId subModId1,
                           cPhysModel * pModel2, tPhysSubModId subModId2,
                           mxs_vector & normal)
{
   CriticalMsg("GetBSPVsBSPNormal: not implemented yet\n");
   return 0;
}

///////////////////////////////////////

mxs_real GetBSPVsOBBNormal(cPhysModel * pModel1, tPhysSubModId subModId1,
                           cPhysModel * pModel2, tPhysSubModId subModId2,
                           mxs_vector & normal)
{
   CriticalMsg("GetBSPVsOBBNormal: not implemented yet\n");
   return 0;
}

///////////////////////////////////////

mxs_real GetOBBVsSphereNormal(cPhysModel * pModel1, tPhysSubModId subModId1,
                              cPhysModel * pModel2, tPhysSubModId subModId2,
                              mxs_vector & normal)
{
   cFaceContact faceContact((cPhysOBBModel *)pModel1, subModId1);

   mx_copy_vec(&normal, &faceContact.GetNormal());
   mx_scaleeq_vec(&normal, -1.0);   

   float radius = ((cPhysSphereModel *)pModel2)->GetRadius(subModId2);

   if (pModel2->IsFancyGhost() && (radius > 1.0))
      radius = 1.0;

   return mx_dot_vec(&pModel2->GetLocationVec(subModId2), &faceContact.GetNormal()) - faceContact.GetPlaneConst() - 
      radius;
}

///////////////////////////////////////

mxs_real GetOBBVsBSPNormal(cPhysModel * pModel1, tPhysSubModId subModId1,
                           cPhysModel * pModel2, tPhysSubModId subModId2,
                           mxs_vector & normal)
{
   CriticalMsg("GetOBBVsBSPNormal: not implemented yet\n");
   return 0;
}

///////////////////////////////////////

mxs_real GetOBBVsOBBNormal(cPhysModel * pModel1, tPhysSubModId subModId1,
                           cPhysModel * pModel2, tPhysSubModId subModId2,
                           mxs_vector & normal)
{
   CriticalMsg("GetOBBVsOBBNormal: not implemented yet\n");
   return 0;
}

///////////////////////////////////////

mxs_real GetSphereVsSphereHatNormal(cPhysModel * pModel1, tPhysSubModId subModId1,
                                    cPhysModel * pModel2, tPhysSubModId subModId2,
                                    mxs_vector & normal)
{
   if (subModId2 == 0)
      return GetSphereVsSphereNormal((cPhysSphereModel *)pModel1, subModId1, (cPhysSphereModel *)pModel2, subModId2, normal);
   else
   {
      cFaceContact faceContact((cPhysSphereModel *)pModel2, ((cPhysSphereModel *)pModel2)->GetRadius(0));

      if (faceContact.GetPoly() != NULL)
      {
         mx_copy_vec(&normal, &faceContact.GetNormal());

         float radius = ((cPhysSphereModel *)pModel1)->GetRadius(subModId1);

         if (pModel1->IsFancyGhost() && (radius > 1.0))
            radius = 1.0;

         return mx_dot_vec(&pModel1->GetLocationVec(subModId1), &faceContact.GetNormal()) - 
            faceContact.GetPlaneConst() - radius;
      }
      else
      {
         return GetSphereVsSphereNormal((cPhysSphereModel *)pModel1, subModId1, (cPhysSphereModel*)pModel2, subModId2, normal);
      }
   }
}

///////////////////////////////////////

mxs_real GetBSPVsSphereHatNormal(cPhysModel * pModel1, tPhysSubModId subModId1,
                                 cPhysModel * pModel2, tPhysSubModId subModId2,
                                 mxs_vector & normal)
{
   CriticalMsg("GetBSPVsSphereHatNormal: not implemented yet\n");
   return 0;
}

///////////////////////////////////////

mxs_real GetOBBVsSphereHatNormal(cPhysModel * pModel1, tPhysSubModId subModId1,
                                 cPhysModel * pModel2, tPhysSubModId subModId2,
                                 mxs_vector & normal)
{
   if (subModId2 == 0)
      return GetOBBVsSphereNormal(pModel1, subModId1, pModel2, subModId2, normal);
   else
   {
      CriticalMsg("GetOBBVsSphereHatNormal: this portion not implemented\n");
      return 0;
   }
}

////////////////////////////////////////

mxs_real GetSphereHatVsSphereNormal(cPhysModel * pModel1, tPhysSubModId subModId1,
                                    cPhysModel * pModel2, tPhysSubModId subModId2,
                                    mxs_vector & normal)
{
   if (subModId1 == 0)
      return GetSphereVsSphereNormal((cPhysSphereModel *)pModel1, subModId1, (cPhysSphereModel *)pModel2, subModId2, normal);
   else
   {
      cFaceContact faceContact((cPhysSphereModel *)pModel1, ((cPhysSphereModel *)pModel1)->GetRadius(0));

      if (faceContact.GetPoly() != NULL)
      {
         mx_copy_vec(&normal, &faceContact.GetNormal());
         mx_scaleeq_vec(&normal, -1.0);

         float radius = ((cPhysSphereModel *)pModel2)->GetRadius(subModId2); 
         
         if (pModel2->IsFancyGhost() && (radius > 1.0))
            radius = 1.0;

         return mx_dot_vec(&pModel2->GetLocationVec(subModId2), &faceContact.GetNormal()) - 
            faceContact.GetPlaneConst() - radius;
      }
      else
      {
         return GetSphereVsSphereNormal((cPhysSphereModel *)pModel1, subModId1, (cPhysSphereModel*)pModel2, subModId2, normal);
      }
   }
}

///////////////////////////////////////

mxs_real GetSphereHatVsBSPNormal(cPhysModel * pModel1, tPhysSubModId subModId1,
                                 cPhysModel * pModel2, tPhysSubModId subModId2,
                                 mxs_vector & normal)
{
   CriticalMsg("GetSphereHatVsBSPNormal: not implemented yet\n");
   return 0;
}

///////////////////////////////////////

mxs_real GetSphereHatVsOBBNormal(cPhysModel * pModel1, tPhysSubModId subModId1,
                                 cPhysModel * pModel2, tPhysSubModId subModId2,
                                 mxs_vector & normal)
{
   if (subModId1 == 0)
      return GetSphereVsOBBNormal(pModel1, subModId1, pModel2, subModId2, normal);
   else
   {
      CriticalMsg("GetSphereHatVsOBBNormal: this part not implemented\n");
      return 0;
   }
}

////////////////////////////////////////

mxs_real GetSphereHatVsSphereHatNormal(cPhysModel * pModel1, tPhysSubModId subModId1,
                           cPhysModel * pModel2, tPhysSubModId subModId2,
                           mxs_vector & normal)
{
   if ((subModId1 == 0) && (subModId2 == 0))
      return GetSphereVsSphereNormal((cPhysSphereModel *)pModel1, subModId1, (cPhysSphereModel *)pModel2, subModId2, normal);
   else
   {
      if (subModId1 == 1)
         return GetSphereHatVsSphereNormal(pModel1, subModId1, pModel2, subModId2, normal);
      if (subModId2 == 1)
         return GetSphereVsSphereHatNormal(pModel1, subModId1, pModel2, subModId2, normal);

      CriticalMsg("GetSphereHatVsSphereHatNormal: this part not implemented\n");
      return 0;
   }
}

////////////////////////////////////////

static GetObjectsNormalFn getObjectsNormalFns[kPMT_NumTypes][kPMT_NumTypes] =
{
   {
      (GetObjectsNormalFn) GetSphereVsSphereNormal,
      (GetObjectsNormalFn) GetSphereVsBSPNormal,
      (GetObjectsNormalFn) GetSphereVsSphereNormal,
      (GetObjectsNormalFn) GetSphereVsOBBNormal,
      (GetObjectsNormalFn) GetSphereVsSphereHatNormal
   },
   {
      (GetObjectsNormalFn) GetBSPVsSphereNormal,
      (GetObjectsNormalFn) GetBSPVsBSPNormal,
      (GetObjectsNormalFn) GetBSPVsSphereNormal,
      (GetObjectsNormalFn) GetBSPVsOBBNormal,
      (GetObjectsNormalFn) GetBSPVsSphereHatNormal
   },
   {
      (GetObjectsNormalFn) GetSphereVsSphereNormal,
      (GetObjectsNormalFn) GetSphereVsBSPNormal,
      (GetObjectsNormalFn) GetSphereVsSphereNormal,
      (GetObjectsNormalFn) GetSphereVsOBBNormal,
      (GetObjectsNormalFn) GetSphereVsSphereHatNormal
   },
   {
      (GetObjectsNormalFn) GetOBBVsSphereNormal,
      (GetObjectsNormalFn) GetOBBVsBSPNormal,
      (GetObjectsNormalFn) GetOBBVsSphereNormal,
      (GetObjectsNormalFn) GetOBBVsOBBNormal,
      (GetObjectsNormalFn) GetOBBVsSphereHatNormal
   },
   {
      (GetObjectsNormalFn) GetSphereHatVsSphereNormal,
      (GetObjectsNormalFn) GetSphereHatVsBSPNormal,
      (GetObjectsNormalFn) GetSphereHatVsSphereNormal,
      (GetObjectsNormalFn) GetSphereHatVsOBBNormal,
      (GetObjectsNormalFn) GetSphereHatVsSphereHatNormal
   }
};

///////////////////////////////////////
//
// Return distance between two objects (closest surfaces) and set the normal of the closest
// vector from one to the other
//

mxs_real PhysGetObjsNorm(cPhysModel * pModel, tPhysSubModId subModId,
                         cPhysModel * pModel2, tPhysSubModId subModId2,
                         mxs_vector & normal)
{
   return getObjectsNormalFns[pModel->GetType(subModId)][pModel2->GetType(subModId2)] 
          (pModel, subModId, pModel2, subModId2, normal);
}

///////////////////////////////////////////////////////////////////////////////





