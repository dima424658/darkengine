/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/physics/phsubmod.cpp,v 1.7 2000/02/19 12:32:22 toml Exp $
//
//
//

#include <lg.h>

#include <phsubmod.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cPhysSubModelList
//

cPhysSubModelList::~cPhysSubModelList(void)
{
   cPhysSubModelInst *pSubModel = GetFirst();

   while (pSubModel != NULL)
   {
      Remove(pSubModel);
      delete pSubModel;
      pSubModel = GetFirst();
   }
}

///////////////////////////////////////
//
// Find a sub model in a list, given sub-model in that list
//

BOOL cPhysSubModelInst::Find(ObjID objID, tPhysSubModId subModId, cPhysSubModelInst ** ppSubModel)
{
   cPhysSubModelInst *pSubModel = this;

   while (pSubModel != NULL)
   {
      if ((pSubModel->GetObjID() == objID) && ((pSubModel->GetSubModId() == subModId) || (subModId == -1)))
      {
         *ppSubModel = pSubModel;
         return TRUE;
      }
      pSubModel = pSubModel->GetNext();
   }

   return FALSE;
}

///////////////////////////////////////
//
// Find a sub model in a list
//

BOOL cPhysSubModelList::Find(ObjID objID, tPhysSubModId subModId, cPhysSubModelInst ** ppSubModel) const
{
   cPhysSubModelInst *pSubModel = GetFirst();

   if (pSubModel != NULL)
      return pSubModel->Find(objID, subModId, ppSubModel);

   return FALSE;
}

///////////////////////////////////////////////////////////////////////////////

