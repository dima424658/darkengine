/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/physics/phmodbsp.cpp,v 1.7 2000/02/19 12:32:19 toml Exp $
//
//
//

#include <lg.h>
#include <matrixs.h>
#include <matrix.h>

#include <phcore.h>
#include <phmodbsp.h>
#include <phclsn.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cPhysBSPModel
//

cPhysBSPModel::cPhysBSPModel(ObjID objID, tPhysSubModId numSubModels, unsigned flags)
 : cPhysModel(objID, kPMT_BSP, numSubModels, flags)
{
}

///////////////////////////////////////

cPhysBSPModel::cPhysBSPModel(PhysReadWrite func) : cPhysModel(func)
{
   // Load BSP-model specific stuff
}

///////////////////////////////////////

void cPhysBSPModel::Write(PhysReadWrite func) const
{
   // Write model-common data
   cPhysModel::Write(func);

   // Write BSP-model specific stuff 
}

///////////////////////////////////////

BOOL cPhysBSPModel::CheckTerrainCollision(tPhysSubModId subModId, mxs_real t0, mxs_real dt, cPhysClsn ** ppClsn)
{
   CriticalMsg("cPhysBSPModel::CheckTerrainCollision: not yet implemented\n");
   return FALSE;
}

///////////////////////////////////////

mxs_real cPhysBSPModel::TerrainDistance(tPhysSubModId subModId, cFaceContact *pFactContact) const
{
   CriticalMsg("cPhysBSPModel::TerrainDistance: not yet implmented\n");
   return 0.0;
}

mxs_real cPhysBSPModel::TerrainDistance(tPhysSubModId subModId, cEdgeContact *pEdgeContact) const
{
   CriticalMsg("cPhysBSPModel::TerrainDistance: not yet implmented\n");
   return 0.0;
}

mxs_real cPhysBSPModel::TerrainDistance(tPhysSubModId subModId, cVertexContact *pVertexContact) const
{
   CriticalMsg("cPhysBSPModel::TerrainDistance: not yet implmented\n");
   return 0.0;
}

///////////////////////////////////////

ePhysIntersectResult cPhysBSPModel::TestLocationVec(int subModId, const mxs_vector & Location) const
{
   CriticalMsg("cPhysBSPModel::TestLocation: not yet implemented\n");
   return kPCR_Nothing;
}

///////////////////////////////////////

ePhysIntersectResult cPhysBSPModel::TestRotation(int subModId, const mxs_vector & Location) const
{
   CriticalMsg("cPhysBSPModel::TestRotation: not yet implemented\n");
   return kPCR_Nothing;
}

///////////////////////////////////////

void cPhysBSPModel::UpdateRefs(void)
{
   CriticalMsg("cPhysBSPModel::UpdateRefs: not yet implemented\n");
}

