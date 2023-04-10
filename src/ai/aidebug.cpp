/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aidebug.cpp,v 1.18 1999/04/09 11:30:53 TOML Exp $
//
//
//

#include <aitype.h>
#include <aidebug.h>
#include <aidbdraw.h>
#include <aiapiiai.h>
#include <comtools.h>
#include <appagg.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////

#ifndef SHIP
BOOL g_AIEfficiencyDebug;
#endif

///////////////////////////////////////////////////////////////////////////////

ObjID g_AIAlertWatchObj;
ObjID g_AIFlowWatchObj;
ObjID g_AISoundWatchObj;
ObjID g_AIHearWatchObj;
ObjID g_AICbtWatchObj;
ObjID g_AIPathWatchObj;
ObjID g_AIDeathWatchObj;
ObjID g_AISignalWatchObj;
ObjID g_AIRangedWatchObj;
ObjID g_AIInformWatchObj;
ObjID g_AISightWatchObj;
ObjID g_AISensePulseWatchObj;
ObjID g_AIRecoverPathWatchObj;
ObjID g_AICombatWatchObj;
ObjID g_AIRangedModeWatchObj;

BOOL g_AIScheduleTrace;

///////////////////////////////////////////////////////////////////////////////

ObjID g_AICurrentAI = (unsigned) -1; // for debugging purposes.

cStr          g_AIDumpStr;
IInternalAI * g_pAIDumpAI;

///////////////////////////////////////////////////////////////////////////////
//
// Rendered debug info
//

BOOL g_AIDrawAIs = FALSE;
BOOL gAIDebugDrawInGame = TRUE;

BOOL g_AIDebugDrawMoveGoal = TRUE;
BOOL g_AIDebugDrawSuggs = TRUE;
BOOL g_AIDebugDrawPaths = TRUE;

void AIDebugDrawObjAIInfo(ObjID obj)
{
#ifndef SHIP
   if (!g_AIDrawAIs)
      return;

   AutoAppIPtr(AIManager);

   // @TODO: what is right way to get internal ai from objID? KATE 5/98
   IInternalAI * pAI=(IInternalAI *)pAIManager->GetAI(obj);

   if (pAI)
   {
      pAI->DebugDraw();
      pAI->Release();
   }
#endif
}

///////////////////////////////////////////////////////////////////////////////
