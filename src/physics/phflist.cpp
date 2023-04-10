/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/physics/phflist.cpp,v 1.11 2000/02/19 12:32:15 toml Exp $
//
// Force list functions (C-callable)
//

#include <lg.h>

#include <matrix.h>
#include <matrixs.h>

#include <phystyp2.h>
#include <phflist.h>
#include <phcore.h>
#include <phmod.h>
#include <phmods.h>
#include <phmodsph.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

eForceListResult PhysApplyForceList(ObjID objID, tPhysSubModId subModId, sPhysForce *flist, int nforces, 
                    mxs_real dt, mxs_vector *start_loc, mxs_vector *end_loc, 
                    mxs_vector *force, mxs_real *ctime, BOOL terrain_check, BOOL object_check)
{
   cPhysModel *pModel;

   if ((pModel = g_PhysModels.GetActive(objID)) == NULL)
   {
      Warning(("PhysClearForceList: No active physics models for obj %d\n", objID));
      return FALSE;
   }

   // This is stupid, but should stop us from crashing.
   if (subModId >= pModel->NumSubModels())
   {
      AssertMsg3(FALSE, "PhysApplyForceList: Obj %d has %d submodels (asking for %d)\n", objID, pModel->NumSubModels(), subModId);   
      return kFLR_Success;
   }

   Assert_((pModel->GetType(0) == kPMT_Sphere) || (pModel->GetType(0) == kPMT_Point));

   return ((cPhysSphereModel *)pModel)->ApplyForces(subModId, flist, nforces, dt, 
                                                    *start_loc, end_loc, force, ctime, 
                                                    terrain_check, object_check);
}


