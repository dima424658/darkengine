/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aitagtyp.h,v 1.5 2000/01/31 09:34:10 adurant Exp $
//
// Save/load tag management
//
// @Note (toml 10-05-98): It is gross to have a completely parallel way to 
// refer to component types...
// 
#pragma once

#ifndef __AITAGTYP_H
#define __AITAGTYP_H

#include <aitype.h>

class cMultiParm;

///////////////////////////////////////////////////////////////////////////////

typedef unsigned tAISaveLoadSubTag;

enum eAISaveLoadSubTagsEnum
{
   kAISL_CoreBase = 1,
   
   kAISL_AI       = kAISL_CoreBase,
   kAISL_Goals,
   kAISL_Actions,
   
   kAISL_CoreCompBase = 100,
   
   kAISL_FlowDebugger = kAISL_CoreCompBase,
   kAISL_Test,
   kAISL_MoveEnactor,
   kAISL_SoundEnactor,
   kAISL_Pathfinder,
   kAISL_ObjectsMovReg,
   kAISL_WallsCliffsMovReg,
   kAISL_DoorMovReg,
   kAISL_Senses,
   kAISL_Defend,
   kAISL_Network,
   kAISL_Death,
   kAISL_BasicScript,
   kAISL_Stun,
   kAISL_Detect,
   kAISL_Investigate,
   kAISL_Patrol,
   kAISL_Idle,
   kAISL_Flee,
   kAISL_NonCombatDamageResponse,
   kAISL_Follow,
   kAISL_SignalResponse,
   kAISL_AlertResponse,
   kAISL_ThreatResponse,
   kAISL_Watch,
   kAISL_Converse,
   kAISL_Inform,
   kAISL_SenseCombatResponse,
   kAISL_Combat,
   kAISL_CombatHtoH,
   kAISL_CombatRanged,
   kAISL_ShockRanged,
   kAISL_CombatNewRanged,
   
   kAISL_GameBase = 3000,
};


///////////////////////////////////////////////////////////////////////////////

typedef struct _TagFileTag TagFileTag; 

BOOL AICreateTag(ObjID obj, tAISaveLoadSubTag subTag, unsigned addId, TagFileTag * pResult);

///////////////////////////////////////

BOOL AIOpenTagBlock(ObjID obj, tAISaveLoadSubTag subTag, unsigned addId, unsigned subTagVer, ITagFile *);

///////////////////////////////////////

BOOL AITagModeWrite(ITagFile *);

///////////////////////////////////////

void AICloseTagBlock(ITagFile *);

///////////////////////////////////////

void AITagMoveRaw(ITagFile * pTagFile, void * pData, unsigned nBytes);

///////////////////////////////////////

template <class T>
inline
void AITagMove(ITagFile * pTagFile, T * pItem)
{
   AITagMoveRaw(pTagFile, pItem, sizeof(T));
}

///////////////////////////////////////

template <class T>
inline
void AITagMoveDynarray(ITagFile * pTagFile, T * pDynarray)
{
   unsigned nItems;
   if (AITagModeWrite(pTagFile))
   {
      nItems = pDynarray->Size();
      AITagMoveRaw(pTagFile, &nItems, sizeof(unsigned));
      if (nItems)
         AITagMoveRaw(pTagFile, pDynarray->AsPointer(), nItems * pDynarray->GetElementSize());
   }
   else
   {
      pDynarray->SetSize(0);
      AITagMoveRaw(pTagFile, &nItems, sizeof(unsigned));
      if (nItems)
      {
         pDynarray->SetSize(nItems);
         AITagMoveRaw(pTagFile, pDynarray->AsPointer(), nItems * pDynarray->GetElementSize());
      }
   }
}


///////////////////////////////////////

#ifdef __STR_H
inline
void AITagMoveString(ITagFile * pTagFile, cStr * pStr)
{
   unsigned len;
   if (AITagModeWrite(pTagFile))
   {
      len = pStr->GetLength();
      AITagMoveRaw(pTagFile, &len, sizeof(unsigned));
      if (len)
         AITagMoveRaw(pTagFile, (void *)pStr->operator const char *(), len);
   }
   else
   {
      pStr->Empty();
      AITagMoveRaw(pTagFile, &len, sizeof(unsigned));
      if (len)
      {
         char * buf = pStr->GetBuffer(len + 1);
         AITagMoveRaw(pTagFile, buf, len);
         buf[len] = 0;
         pStr->ReleaseBuffer();
      }
   }
}
#endif

///////////////////////////////////////

void AITagMoveMultiParm(ITagFile * pTagFile, cMultiParm * pParm);

///////////////////////////////////////////////////////////////////////////////

#endif /* !__AITAGTYP_H */
