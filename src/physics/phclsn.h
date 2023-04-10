/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/physics/phclsn.h,v 1.18 2000/01/29 13:32:38 adurant Exp $
//
// Physics collisions
//
#pragma once

#ifndef __PHCLSN_H
#define __PHCLSN_H

#include <objtype.h>

#include <phystyp2.h>
#include <phmod.h>
#include <phsubmod.h>
#include <phterr.h>

typedef struct sSphrContactData sSphrContactData;

class cPhysSphereModel;
class cPhysOBBModel;

///////////////////////////////////////////////////////////////////////////////

//
// Result of a collision
//
enum ePhysClsnResultEnum
{
   kPCR_Nothing,
   kPCR_Bounce,
   kPCR_Die1,
   kPCR_Die2,
   kPCR_DieBoth,
   kPCR_NonPhys1,
   kPCR_NonPhys2,
   kPCR_NonPhysBoth,
   kPCR_Stop,
   kPCR_NewControl,
};

//#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////

enum ePhysClsnType
{
   kPC_StationaryObject = 0x0001,
   kPC_MoveableObject   = 0x0002,
   kPC_OBBObject        = 0x0004,
   kPC_SphereHatObject  = 0x0008,

   kPC_Object           = kPC_StationaryObject | kPC_MoveableObject | kPC_OBBObject | kPC_SphereHatObject,

   kPC_TerrainFace      = 0x0010,
   kPC_TerrainEdge      = 0x0020,
   kPC_TerrainVertex    = 0x0040,

   kPC_Terrain          = kPC_TerrainFace | kPC_TerrainEdge | kPC_TerrainVertex,

   kPC_Invalid          = 0xffffffffL
};

///////////////////////////////////////////////////////////////////////////////

class cPhysClsn;

typedef cDList<cPhysClsn, 0>     cPhysClsnsBase;
typedef cDListNode<cPhysClsn, 0> cPhysClsnNodeBase;

///////////////////////////////////////////////////////////////////////////////
//
// STRUCT: sPhysClsnTerr
//
// describes terrain for a collision
//

struct sPhysClsnTerr
{
   cPhysTerrPolyList m_polyList;
   mxs_vector        m_normal;                    // computed normal of polys
};

///////////////////////////////////////////////////////////////////////////////
//
// STRUCT: sPhysClsn
//
// Instance of a collision, implemented as a struct so we can (eventually)
// pass it to C
//

struct sPhysClsn
{
   ePhysClsnType   type;
   ePhysClsnResult result;
   mxs_real        t0;
   mxs_real        dt;
   mxs_vector      clsn_pt;
   sPhysSubModInst instance;
   void *          pData;           // either a pointer to another sub-model instance or collision terrain struct
};

///////////////////////////////////////////////////////////////////////////////

class cPhysClsn : public sPhysClsn, public cPhysClsnNodeBase
{
public:
   ////////////////////////////////////
   //
   // Constructors/Destructor
   //

   // Make a copy of the clsn
   cPhysClsn(cPhysClsn *pClsn);

   // A sphere-sphere collision
   cPhysClsn(ePhysClsnType type, mxs_real t0, mxs_real dt, 
             const cPhysSphereModel * pSphereModel1, tPhysSubModId subModId1, const mxs_vector & locVec1,
             const cPhysSphereModel * pSphereModel2, tPhysSubModId subModId2, const mxs_vector & locVec2);

   // A sphere-obb collision
   cPhysClsn(ePhysClsnType type, mxs_real t0, mxs_real dt,
             const cPhysSphereModel * pSphereModel, tPhysSubModId subModId1, const mxs_vector & locVec1,
             const cPhysOBBModel * pOBBModel, int side, const mxs_vector & locVec2);

   // A sphere object-terrain collision
   cPhysClsn(ePhysClsnType type, mxs_real t0, mxs_real dt, const mxs_vector & clsn_pt,
             const cPhysModel * pModel, tPhysSubModId subModId, const mxs_vector & locVec,
             sSphrContactData aContactData[], int contactID);

   // A point object-terrain collision
   cPhysClsn(ePhysClsnType type, mxs_real t0, mxs_real dt, const mxs_vector & clsn_pt, 
             const cPhysModel * pModel, tPhysSubModId subModId, const mxs_vector & locVec,
             int cell_id, int poly_id);

   ~cPhysClsn();

   ////////////////////////////////////
   //
   // Access functions
   //
   ePhysClsnType           GetType() const;
   mxs_real                GetTime() const;
   mxs_real                GetT0() const;
   mxs_real                GetDT() const;

   const mxs_vector &      GetLoc() const;
   const mxs_vector &      GetLoc2() const;

   void                    SetResult(ePhysClsnResult result);
   ePhysClsnResult         GetResult() const;

   const sPhysSubModInst & GetInstance() const;
   ObjID                   GetObjID() const;
   tPhysSubModId           GetSubModId() const;
   cPhysModel *            GetModel() const;
   int                     GetTexture() const; 

   const sPhysSubModInst & GetInstance2() const;
   ObjID                   GetObjID2() const;
   tPhysSubModId           GetSubModId2() const;
   cPhysModel *            GetModel2() const;

   cPhysTerrPoly *         GetTerrainPoly(int index = 0) const;
   int                     GetCellID(int index = 0) const;
   int                     GetPolyID(int index = 0) const;
   const mxs_vector        GetNormal(int index = -1) const;

   // Note that collision points for non-terrain collisions are NOT EXACT
   const mxs_vector &      GetClsnPt() const;

#ifdef DBG_ON
   void MonoPrint() const;
#endif

private:
   #define AssertObjectCollision() Assert_(type & kPC_Object)
   void CreateTerrainInfo(ePhysClsnType type,
                          sSphrContactData * pContactData,
                          cPhysTerrPolyList * pList);
   void GenerateClsnPt();
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cPhysClsns
//
// List of collisions
//

class cPhysClsns : public cPhysClsnsBase
{
public:
   void New(const cPhysModel * pModel,
            tPhysSubModId subModId,
            mxs_real time,
            const Location & hitLoc,
            const mxs_vector * pNormals);
   void Insert(cPhysClsn * pClsn);
   void Sort();
private:
};

///////////////////////////////////////////////////////////////////////////////
//
// Non-member inline functions
//

inline void InitSubModelInstance(sPhysSubModInst * pInstance, tPhysSubModId subModId, const cPhysModel * pModel,
                                  const mxs_vector & locVec)
{
   // This isn't a member function, because this is a structure we want to expose to c functions
   pInstance->objID    = pModel->GetObjID();
   pInstance->subModId = subModId;
   pInstance->pModel   = (cPhysModel *) pModel;
   pInstance->locVec   = locVec;
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cPhysClsn, inline functions
//

inline ePhysClsnType cPhysClsn::GetType() const
{
   return type;
}

///////////////////////////////////////

inline mxs_real cPhysClsn::GetTime() const
{
   return t0 + dt;
}

///////////////////////////////////////

inline mxs_real cPhysClsn::GetT0() const
{
   return t0;
}

///////////////////////////////////////

inline mxs_real cPhysClsn::GetDT() const
{
   return dt;
}

///////////////////////////////////////

inline const mxs_vector & cPhysClsn::GetLoc() const
{
   return instance.locVec;
}

///////////////////////////////////////

inline const mxs_vector & cPhysClsn::GetLoc2() const
{
   Assert_(type & kPC_Object);
   return ((sPhysSubModInst *) pData)->locVec;
}

///////////////////////////////////////

inline ePhysClsnResult cPhysClsn::GetResult() const
{
   return result;
}

///////////////////////////////////////

inline void cPhysClsn::SetResult(ePhysClsnResult _result)
{
   result = _result;
}

///////////////////////////////////////

inline const sPhysSubModInst & cPhysClsn::GetInstance() const
{
   return instance;
}

///////////////////////////////////////

inline ObjID cPhysClsn::GetObjID() const
{
   return instance.objID;
}

////////////////////////////////////////

inline tPhysSubModId cPhysClsn::GetSubModId() const
{
   return instance.subModId;
}

///////////////////////////////////////

inline cPhysModel *cPhysClsn::GetModel() const
{
   return instance.pModel;
}

///////////////////////////////////////

inline const sPhysSubModInst & cPhysClsn::GetInstance2() const
{
   AssertObjectCollision();
   return *(sPhysSubModInst *) pData;
}

///////////////////////////////////////

inline ObjID cPhysClsn::GetObjID2() const
{
   AssertObjectCollision();
   return ((sPhysSubModInst *) pData)->objID;
}

///////////////////////////////////////

inline tPhysSubModId cPhysClsn::GetSubModId2() const
{
   AssertObjectCollision();
   return ((sPhysSubModInst *) pData)->subModId;
}

///////////////////////////////////////

inline cPhysModel *cPhysClsn::GetModel2() const
{
   AssertObjectCollision();
   return ((sPhysSubModInst *) pData)->pModel;
}

////////////////////////////////////////

inline const mxs_vector &cPhysClsn::GetClsnPt() const
{
   return clsn_pt;
}

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__PHCLSN_H */



