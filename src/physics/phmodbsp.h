/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/physics/phmodbsp.h,v 1.9 2000/01/29 13:32:53 adurant Exp $
//
//
//
#pragma once

#ifndef __PHMODBSP_H
#define __PHMODBSP_H

#include <phmod.h>

#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cPhysBSPModel
//
// A BSP model
//

class cPhysBSPModel : public cPhysModel
{
public:
   ////////////////////////////////////
   //
   // Constructors
   //
   cPhysBSPModel(ObjID objID, tPhysSubModId numSubModels, unsigned flags);
   cPhysBSPModel(PhysReadWrite func);

   ////////////////////////////////////
   //
   // I/O
   //
   void Write(PhysReadWrite func) const;

   ////////////////////////////////////
   //
   // Check for a terrain collision in the specified time
   //
   // this just warns, since we can't move BSP models yet
   //
   BOOL CheckTerrainCollision(tPhysSubModId subModId, mxs_real t0, mxs_real dt, cPhysClsn ** ppClsn);

   ////////////////////////////////////
   //
   // Distance from given polygon
   //
   // TODO: actually write this, it just warns currently
   //
   mxs_real TerrainDistance(tPhysSubModId subModId, cFaceContact *pFaceContact) const;
   mxs_real TerrainDistance(tPhysSubModId subModId, cEdgeContact *pEdgeContact) const;
   mxs_real TerrainDistance(tPhysSubModId subModId, cVertexContact *pVertexContact) const;

   ////////////////////////////////////
   //
   // It would be nice to have these actually test for intersection, etc.
   //
   ePhysIntersectResult TestLocationVec(int subModId, const mxs_vector & LocationVec) const;
   ePhysIntersectResult TestRotation(int subModId, const mxs_vector & Rotation) const;

   ////////////////////////////////////
   //
   // Update physics refs from stationary position
   //
   void UpdateRefs();

   ////////////////////////////////////
   //
   // Get overall model size
   //
   mxs_real GetSize() const;

   ////////////////////////////////////
   //
   // Access BSP model
   //
   mds_model * GetMDSModel();

};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cPhysBSPModel, inline functions
//
inline mxs_real cPhysBSPModel::GetSize() const
{
   CriticalMsg("cPhysBSPModel::GetSize: not yet implemented\n");
   return 0;
}

#pragma pack()

#endif /* defined(_MSC_VER) */



