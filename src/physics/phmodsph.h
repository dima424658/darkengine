/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/physics/phmodsph.h,v 1.17 2000/01/29 13:32:56 adurant Exp $
//
// Sphere model
//
#pragma once

#ifndef __PHMODSPH_H
#define __PHMODSPH_H

#include <physapi.h>
#include <phmod.h>

#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cPhysSphereModel
//
// A Sphere
//

class cPhysSphereModel : public cPhysModel
{
public:
   ////////////////////////////////////
   //
   // Constructors/Destructor
   //
   cPhysSphereModel(ObjID objID, tPhysSubModId numSubModels, BOOL hat, unsigned flags, mxs_real radius,
                   mxs_real elasticity);
   cPhysSphereModel(PhysReadWrite func);
   ~cPhysSphereModel();

   ////////////////////////////////////
   //
   // I/O
   //   
   void Write(PhysReadWrite func) const;

   ////////////////////////////////////
   //
   // Check for a terrain collision in the specified time
   //
   // Calls the sphere caster
   //
   BOOL CheckTerrainCollision(tPhysSubModId subModId, mxs_real t0, mxs_real dt, cPhysClsn ** ppClsn);

   ////////////////////////////////////
   //
   // Call the sphere intersection checkers
   //
   ePhysIntersectResult TestLocationVec(int subModId, const mxs_vector & LocationVec) const;
   ePhysIntersectResult TestRotation(int subModId, const mxs_vector & Rotation) const;

   ////////////////////////////////////
   //
   // Distance from given polygon
   //
   mxs_real TerrainDistance(tPhysSubModId subModId, cFaceContact *pFaceContact) const;
   mxs_real TerrainDistance(tPhysSubModId subModId, cEdgeContact *pEdgeContact) const;
   mxs_real TerrainDistance(tPhysSubModId subModId, cVertexContact *pVertexContact) const;

   ////////////////////////////////////
   //
   // Force-applying function;
   //
   BOOL ApplyForces(tPhysSubModId subModId, sPhysForce *flist, int nforces, mxs_real dt, 
                    mxs_vector &start_loc, mxs_vector *end_loc, mxs_vector *force, 
                    mxs_real *ctime, BOOL terrain_check, BOOL object_check);

   ////////////////////////////////////
   //
   // Update physics refs from stationary position
   //
   void UpdateRefs();

   ////////////////////////////////////
   //
   // Get the radius of the sphere
   //
   mxs_real GetRadius(tPhysSubModId subModId) const;
   void     SetRadius(tPhysSubModId subModId, mxs_real size);

   ////////////////////////////////////
   //
   // Get overall model size
   //
   mxs_real GetSize() const;

#ifndef SHIP

   virtual long InternalSize() const;

#endif

private:
   // @TBD (toml 08-05-97): why only one radius for multiple submodels.  Probably wants to be cDynArray<mxs_real>?
   cDynArray<mxs_real> m_radius;

};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cPhysSphereModel, inline functions
//

inline mxs_real cPhysSphereModel::GetRadius(tPhysSubModId subModId) const
{
   return m_radius[subModId];
}

inline void cPhysSphereModel::SetRadius(tPhysSubModId subModId, mxs_real size)
{
   if (GetType(subModId) == kPMT_SphereHat)
      SetType(subModId, (size < 0.0001) ? kPMT_Point : kPMT_SphereHat);
   else
      SetType(subModId, (size < 0.0001) ? kPMT_Point : kPMT_Sphere);
 
   m_radius[subModId] = size;
}

inline mxs_real cPhysSphereModel::GetSize() const
{
   mxs_real   cur_size;
   mxs_real   big_size;

   for (int i=0; i<NumSubModels(); i++)
   {
      cur_size = mx_mag_vec(&GetSubModOffset(i)) + m_radius[i];
      if ((i == 0) || (cur_size > big_size))
         big_size = cur_size;
   }

   return big_size;
}

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__PHMODSPH_H */

