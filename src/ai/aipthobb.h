/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aipthobb.h,v 1.5 2000/03/06 17:19:24 toml Exp $
//
// AI OBB pathfinding header
//
#pragma once

#ifndef __AIPTHOBB_H
#define __AIPTHOBB_H

#include <comtools.h>
#include <dynarray.h>
#include <aipathdb.h>

F_DECLARE_INTERFACE(IAIManager);
F_DECLARE_INTERFACE(IBoolProperty);

struct sAIObjCell // (toml 03-06-00) : public sAIExternCell
{
   sAIObjCell()
   {
      memset(this, 0, sizeof(*this));
   }
   
   ~sAIObjCell()
   {
      if (pAIExternCell)
         delete pAIExternCell;
   }
   
   sAIExternCell *pAIExternCell;
   ObjID          objID;
   void  *data;
};

////////////////////////////////////////

typedef cDynClassArray<sAIObjCell> cAIObjCells;

////////////////////////////////////////

EXTERN IBoolProperty *g_pObjPathableProperty;

////////////////////////////////////////

EXTERN void AIInitObjectPathable(IAIManager *);
EXTERN void AITermObjectPathable();

EXTERN void AIGetAllMovingTerrainCells(cAIObjCells *pCellList);
EXTERN void AIGetAllPathableOBBCells(cAIObjCells *pCellList);

EXTERN void AIGetAllMovingTerrainObjs(cDynArray<ObjID> *pObjList);

////////////////////////////////////////

#endif // __AIPTHOBB_H
