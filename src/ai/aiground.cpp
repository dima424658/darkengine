/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiground.cpp,v 1.3 2000/02/19 12:17:32 toml Exp $
//
//
//

#include <aitype.h>
#include <aidebug.h>
#include <ai.h>
#include <aiman.h>
#include <aipthloc.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

BOOL g_AINoGround;

float AIAdjustMvrGroundHeight(ObjID obj, const Location * pLoc, float height, ObjID hitObj)
{
   if (g_AINoGround)
      return height;

   cAI * pAI = g_pAIManager->AccessAI(obj);
   if (pAI)
      return pAI->AdjustMvrGroundHeight(pLoc, height, hitObj);
      
   return height;
}
