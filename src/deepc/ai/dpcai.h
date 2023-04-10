/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef __DPCAI_H
#define __DPCAI_H

#ifndef __RELATION_H
#include <relation.h>
#endif // !__RELATION_H

typedef int eAIVisionType;
// note: must keep this in synch with struct desc in dpcai.cpp
enum eAIVisionType_ 
{
   kAIVisionGeneric   = 0, 
   kAIVisionCamera    = 1, 
   kAIVisionRobot     = 2, 
   kAIVisionHuman     = 3, 
   kAIVisionHybrid    = 4, 
   kAIVisionAnnelid   = 5, 
   kAIVisionDog       = 6,
   kAIVisionTypeNum
};

EXTERN IRelation *g_pAIGunFlashLinks;

EXTERN void DPCAIInit();
EXTERN void DPCAITerm();

#endif // __DPCAI_H