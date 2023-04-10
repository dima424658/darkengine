/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/physics/phsubmod.h,v 1.6 2000/01/31 09:52:09 adurant Exp $
//
// Accessory data structures used by contact tracking code
//
#pragma once

#ifndef __PHSUBMOD_H
#define __PHSUBMOD_H

#include <matrixs.h>
#include <dlist.h>
#include <dynarray.h>

#include <objtype.h>
#include <phystyp2.h>

class cPhysModel;

///////////////////////////////////////////////////////////////////////////////
//
// STRUCT: sPhysSubModInst
//
// Descriptor used to identify a sub-model quicly, but with the risk of invalid data should
// the model change
//

struct sPhysSubModInst
{
   ObjID         objID;
   tPhysSubModId subModId;
   cPhysModel *  pModel;                         // cache pointer to model
   mxs_vector    locVec;
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cPhysSubModelList
//
// List of submodels
//

typedef class cPhysSubModelInst          cPhysSubModelInst;
typedef cDList<cPhysSubModelInst, 0>     cPhysSubModelListBase;
typedef cDListNode<cPhysSubModelInst, 0> cPhysSubModelBase;

///////////////////////////////////////

class cPhysSubModelList : public cPhysSubModelListBase
{
public:
   ~cPhysSubModelList(void);
   BOOL Find(ObjID objID, tPhysSubModId subModId, cPhysSubModelInst ** ppSubModel) const;
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cPhysSubModelListArray
//
// Dynamic array of object contact lists
//

typedef cDynZeroArray <cPhysSubModelList *> cPhysSubModelListArrayBase;

class cPhysSubModelListArray : public cPhysSubModelListArrayBase
{
public:
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cPhysSubModelInst
//

class cPhysSubModelInst : private sPhysSubModInst, public cPhysSubModelBase
{
public:
   cPhysSubModelInst(ObjID objID, tPhysSubModId subModId, cPhysModel * pModel);

   ObjID         GetObjID(void) const;
   tPhysSubModId GetSubModId(void) const;
   cPhysModel *  GetPhysicsModel(void) const;

   BOOL          Find(ObjID objID, tPhysSubModId subModId, cPhysSubModelInst ** ppSubModel);

};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cPhysSubModelInst, inline functions
//

inline cPhysSubModelInst::cPhysSubModelInst(ObjID _objID, tPhysSubModId _subModId, cPhysModel * _pModel)
{
   Assert_(_pModel != NULL);

   objID    = _objID;
   subModId = _subModId;
   pModel   = _pModel;
}

///////////////////////////////////////

inline ObjID cPhysSubModelInst::GetObjID(void) const
{
   return objID;
}

///////////////////////////////////////

inline tPhysSubModId cPhysSubModelInst::GetSubModId(void) const
{
   return subModId;
}

///////////////////////////////////////

inline cPhysModel *cPhysSubModelInst::GetPhysicsModel(void) const
{
   return pModel;
}

///////////////////////////////////////////////////////////////////////////////

#endif /* !__PHSUBMOD_H */
