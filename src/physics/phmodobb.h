/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/physics/phmodobb.h,v 1.15 2000/01/29 13:32:54 adurant Exp $
//
//
//
#pragma once

#ifndef __PHMODOBB_H
#define __PHMODOBB_H

#include <phmod.h>
#include <phcontct.h>

#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cPhysOBBModel
//
// A OBB model
//

class cPhysOBBModel : public cPhysModel
{
public:
   ////////////////////////////////////
   //
   // Constructors
   //
   cPhysOBBModel(ObjID objID, tPhysSubModId numSubModels, unsigned flags);
   cPhysOBBModel(PhysReadWrite func);

   ~cPhysOBBModel();

   ePhysModelType GetType(tPhysSubModId subModId) const;

   ////////////////////////////////////
   //
   // I/O
   //
   void Write(PhysReadWrite func) const;

   ////////////////////////////////////
   //
   // Check for a terrain collision in the specified time
   //
   // this just warns, since we can't move OBB models yet
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
   // OBB size
   //
   void              SetEdgeLengths(const mxs_vector &lengths);
   const mxs_vector &GetEdgeLengths() const;

   ////////////////////////////////////
   //
   // Offset from objpos
   //
   void              SetOffset(const mxs_vector &offset);
   const mxs_vector &GetOffset() const;

   ////////////////////////////////////
   //
   // Edge triggering
   //
   BOOL IsEdgeTrigger() const;
   void SetEdgeTrigger(BOOL state);

   ////////////////////////////////////
   //
   // Normals/constants for each side
   //
   void GetNormals(mxs_vector *start_list, mxs_vector *end_list = NULL);
   void GetConstants(mxs_real *start_list, mxs_vector *start_norm_list, mxs_real *end_list = NULL, mxs_vector *end_norm_list = NULL);

   void      GetNormal(int side, mxs_vector *normal, BOOL end_of_frame = FALSE);
   mxs_real  GetConstant(int side, mxs_vector  &normal, BOOL end_of_frame = FALSE);

   ////////////////////////////////////
   //
   // Climbable sides
   //
   void SetClimbableSides(int sides);
   int  GetClimbableSides() const;



   ////////////////////////////////////
   //
   // Pore size
   //
   void SetPoreSize(mxs_real poresize);
   mxs_real  GetPoreSize() const;



#ifndef SHIP

   virtual long InternalSize() const;

#endif

private:

   mxs_vector m_Offset;
   mxs_vector m_EdgeLen;

   BOOL m_EdgeTrigger;

   int m_ClimbableSides;
   mxs_real m_PoreSize;
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cPhysOBBModel, inline functions
//

inline ePhysModelType cPhysOBBModel::GetType(tPhysSubModId /*subModId*/ ) const
{
   return kPMT_OBB;
}

////////////////////////////////////////////////////////////////////////////////

inline mxs_real cPhysOBBModel::GetSize() const
{
   mxs_vector abs_vec;

   for (int i=0; i<3; i++)
      abs_vec.el[i] = fabs(m_Offset.el[i]) + fabs(m_EdgeLen.el[i]);

   return mx_mag_vec(&abs_vec);
}

////////////////////////////////////////

inline void cPhysOBBModel::SetEdgeLengths(const mxs_vector &size)
{
   mx_copy_vec(&m_EdgeLen, &size);
}

////////////////////////////////////////

inline const mxs_vector &cPhysOBBModel::GetEdgeLengths() const
{
   return m_EdgeLen;
}

////////////////////////////////////////////////////////////////////////////////

inline void cPhysOBBModel::SetOffset(const mxs_vector &offset)
{
   mx_copy_vec(&m_Offset, &offset);
}

////////////////////////////////////////

inline const mxs_vector  &cPhysOBBModel::GetOffset() const
{
   return m_Offset;
}

////////////////////////////////////////////////////////////////////////////////

inline BOOL cPhysOBBModel::IsEdgeTrigger() const
{
   return m_EdgeTrigger;
}

////////////////////////////////////////

inline void cPhysOBBModel::SetEdgeTrigger(BOOL state)
{
   m_EdgeTrigger = state;
}

////////////////////////////////////////////////////////////////////////////////

inline void cPhysOBBModel::SetClimbableSides(int sides)
{
   m_ClimbableSides = sides;
}

////////////////////////////////////////

inline int cPhysOBBModel::GetClimbableSides() const
{
   return m_ClimbableSides;
}

////////////////////////////////////////////////////////////////////////////////


inline void cPhysOBBModel::SetPoreSize(mxs_real poresize)
{
   m_PoreSize = poresize;
}

////////////////////////////////////////

inline mxs_real cPhysOBBModel::GetPoreSize() const
{
   return m_PoreSize;
}

////////////////////////////////////////////////////////////////////////////////


#pragma pack()

#endif /* defined(_MSC_VER) */




