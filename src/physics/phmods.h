/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/physics/phmods.h,v 1.14 2000/01/29 13:32:55 adurant Exp $
//
// Physics model lists
//
// We maintain three lists of models: active, inactive and moving
// Moving models are always also active, so the moving list is actually part of the active list
// The stationary models are the part of the active list that is not moving
//
#pragma once

#ifndef __PHMODS_H
#define __PHMODS_H

#include <phystyp2.h>
#include <phmod.h>
#include <cfgdbg.h>

#pragma pack(4)

// alignment on this sucks, I know
struct sDeferredVelocity
{
   ObjID objID;
   int   submod;
   mxs_vector velocity;
};

typedef cDynArray<sDeferredVelocity> cDeferredVelocityList;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cPhysModels
//

class cPhysModels
{
public:

   cPhysModels();
   ~cPhysModels();

   ////////////////////////////////////

   int  NumModels();
   int  NumActiveModels();
   int  NumInactiveModels();
   int  NumMovingModels();
   int  NumStationaryModels();

   void DestroyAll();

   void Read(PhysReadWrite func, ulong obj_partition);
   void Write(PhysReadWrite func, ulong obj_partition);

   ////////////////////////////////////
   //
   // Remove a model from the lists
   //
   void Remove(cPhysModel * pModel);

   ////////////////////////////////////
   //
   // Hashed lookups
   //
   cPhysModel * Get(ObjID);
   cPhysModel * GetActive(ObjID);
   cPhysModel * GetInactive(ObjID);

   ////////////////////////////////////
   //
   // Get model lists
   //
   cPhysModel * GetFirstMoving();
   cPhysModel * GetFirstActive();
   cPhysModel * GetFirstStationary();
   cPhysModel * GetFirstInactive();

   ////////////////////////////////////
   //
   // Get next model (assuming it's in a particular list)
   //
   cPhysModel * GetNextMoving(const cPhysModel * pModel) const;
   cPhysModel * GetNextActive(const cPhysModel * pModel) const;
   cPhysModel * GetNextStationary(const cPhysModel * pModel) const;
   cPhysModel * GetNextInactive(const cPhysModel * pModel) const;

   cPhysModel * GetPreviousMoving(const cPhysModel * pModel) const;

   ////////////////////////////////////
   //
   // Add to inactive list
   //
   void AddToInactive(cPhysModel * pModel);

   ////////////////////////////////////
   //
   // Add to active, moving list
   //
   void AddToMoving(cPhysModel * pModel);

   ////////////////////////////////////
   //
   // Add to active, stationary list
   //
   void AddToStationary(cPhysModel * pModel);

   ////////////////////////////////////
   //
   // Move from active to inactive list
   //
   void Deactivate(cPhysModel * pModel);

   ////////////////////////////////////
   //
   // Move from inactive to active, moving list
   //
   void ActivateToMoving(cPhysModel * pModel);

   ////////////////////////////////////
   //
   // Move from active, moving to active, stationary
   //
   void StopMoving(cPhysModel * pModel);

   ////////////////////////////////////
   //
   // Move from active, stationary to active, moving
   //
   void StartMoving(cPhysModel * pModel);

   ////////////////////////////////////
   //
   // Deferred velocity applications
   //
   void AddDeferredVelocity(ObjID objID, const mxs_vector &velocity);
   void AddDeferredVelocity(ObjID objID, int subModId, const mxs_vector &velocity);

   void ApplyDeferredVelocities();

#ifndef SHIP
   ////////////////////////////////////
   //
   // Storage spew
   //
   void SpewStats();

   // this will only remove from the list -- don't call this unless you know
   // what you're doing
   void HackListRemove(cPhysModel *pModel); 
#endif

private:
   void DestroyAllInList(cPhysModelList * pList);
   void DecNum(cPhysModel * pModel);

   int m_nMovingModels;
   int m_nStationaryModels;
   int m_nInactiveModels;

   // We implement with two lists
   cPhysModelList  m_activeList;
   cPhysModelList  m_inactiveList;

   cPhysModelTable m_activeTable;
   cPhysModelTable m_inactiveTable;

   // First node in the active list that isn't moving
   cPhysModel *   m_pFirstStationary;

   // Deferred velocity list
   cDeferredVelocityList m_deferredVelocityList;
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cPhysModels, inline functions
//

inline cPhysModels::cPhysModels()
 : m_pFirstStationary(0),
   m_nMovingModels(0),
   m_nStationaryModels(0),
   m_nInactiveModels(0)
{
   m_deferredVelocityList.SetSize(0);
}

inline cPhysModels::~cPhysModels()
{
   m_deferredVelocityList.SetSize(0);
}

///////////////////////////////////////

inline int cPhysModels::NumModels()
{
   return m_nStationaryModels + m_nMovingModels + m_nInactiveModels;
}

inline int cPhysModels::NumActiveModels()
{
   return m_nStationaryModels + m_nMovingModels;
}

inline int cPhysModels::NumInactiveModels()
{
   return m_nInactiveModels;
}

inline int cPhysModels::NumMovingModels()
{
   return m_nMovingModels;
}

inline int cPhysModels::NumStationaryModels()
{
   return m_nStationaryModels;
}

///////////////////////////////////////

inline cPhysModel *cPhysModels::Get(ObjID id)
{
   cPhysModel * pModel = m_activeTable.Search(id);
   if (pModel)
      return pModel;
   return m_inactiveTable.Search(id);
}

///////////////////////////////////////

inline cPhysModel *cPhysModels::GetActive(ObjID id)
{
   return m_activeTable.Search(id);
}

///////////////////////////////////////

inline cPhysModel *cPhysModels::GetInactive(ObjID id)
{
   return m_inactiveTable.Search(id);
}

///////////////////////////////////////

inline void cPhysModels::AddToInactive(cPhysModel * pModel)
{
   ConfigSpew("ModelState", ("Object 0 becomes inactive\n", pModel->GetObjID()));
   m_inactiveList.Prepend(pModel);
   m_inactiveTable.Insert(pModel);
   m_nInactiveModels++;
}

///////////////////////////////////////

inline void cPhysModels::AddToMoving(cPhysModel * pModel)
{
   ConfigSpew("ModelState", ("Object 0 becomes moving\n", pModel->GetObjID()));
   m_activeList.Prepend(pModel);
   m_activeTable.Insert(pModel);
   pModel->SetStationary(FALSE);
   m_nMovingModels++;
}

///////////////////////////////////////

inline cPhysModel *cPhysModels::GetFirstMoving(void)
{
   cPhysModel *pFirstActive = m_activeList.GetFirst();
   if (pFirstActive == m_pFirstStationary)
      return NULL;
   return pFirstActive;
}

///////////////////////////////////////

inline cPhysModel *cPhysModels::GetFirstActive()
{
   return m_activeList.GetFirst();
}

///////////////////////////////////////

inline cPhysModel *cPhysModels::GetFirstStationary()
{
   return m_pFirstStationary;
}

///////////////////////////////////////

inline cPhysModel *cPhysModels::GetFirstInactive()
{
   return m_inactiveList.GetFirst();
}

///////////////////////////////////////

inline cPhysModel *cPhysModels::GetNextMoving(const cPhysModel * pModel) const
{
   cPhysModel *pNext = pModel->GetNext();
   if (pNext == m_pFirstStationary)
      return NULL;
   return pNext;
}

///////////////////////////////////////

inline cPhysModel *cPhysModels::GetNextActive(const cPhysModel * pModel) const
{
   return pModel->GetNext();
}

///////////////////////////////////////

inline cPhysModel *cPhysModels::GetNextStationary(const cPhysModel * pModel) const
{
   return pModel->GetNext();
}

///////////////////////////////////////

inline cPhysModel *cPhysModels::GetNextInactive(const cPhysModel * pModel) const
{
   return pModel->GetNext();
}

///////////////////////////////////////

inline cPhysModel *cPhysModels::GetPreviousMoving(const cPhysModel * pModel) const
{
   return pModel->GetPrevious();
}

///////////////////////////////////////////////////////////////////////////////

inline void cPhysModels::DecNum(cPhysModel *pModel)
{
   if (pModel->IsActive()) {
      if (pModel->IsStationary())
         m_nStationaryModels--;
      else
         m_nMovingModels--;
   } else
      m_nInactiveModels--;
}

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__PHMODS_H */
