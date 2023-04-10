/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aidbdraw.cpp,v 1.6 2000/02/28 17:27:52 toml Exp $
//
//
//

extern "C"
{
#include <r3d.h>
}
#include <matrix.h>
#include <aidbdraw.h>
#include <aidebug.h>
#include <aiutils.h>
#include <guistyle.h>
#include <aiapiiai.h>
#include <ai.h>

// Must be last header
#include <dbmem.h>

#ifndef SHIP
static void AIDrawOneSuggestion(const mxs_vector & loc, const sAIMoveSuggestion &suggestion)
{
   r3s_point   pt[4];
   mxs_vector  translate;
   mxs_vector  temp;
   int         good = guiScreenColor(guiRGB(255,255,255));
   int         bad = guiScreenColor(guiRGB(255,0,0));
   float       drawSize;

   translate.x = 0;
   translate.y = 0;
   translate.z = 0.2;

//   r3_start_object(&translate);
//   r3_start_block();

   if (suggestion.bias > 0)
   {
      drawSize = suggestion.bias / 10.0;
      r3_set_color(good);
   }
   else
   {
      drawSize = -suggestion.bias / 10.0;
      r3_set_color(bad);
   }

   temp.x = loc.x;
   temp.y = loc.y;
   temp.z = loc.z;
   r3_transform_point(&pt[0], &temp);

   if (drawSize < 1.0) drawSize = 1.0;
   if (drawSize > 10.0) drawSize = 10.0;

   ProjectFromLocationOnZPlane(loc, drawSize, suggestion.dirArc.ccw, &temp);
   r3_transform_point(&pt[1], &temp);

   ProjectFromLocationOnZPlane(loc, drawSize, suggestion.dirArc.center(), &temp);
   r3_transform_point(&pt[2], &temp);

   ProjectFromLocationOnZPlane(loc, drawSize, suggestion.dirArc.cw, &temp);
   r3_transform_point(&pt[3], &temp);

   r3_draw_line(&pt[0], &pt[1]);
   r3_draw_line(&pt[1], &pt[2]);
   r3_draw_line(&pt[2], &pt[3]);
   r3_draw_line(&pt[3], &pt[0]);

//   r3_end_block();
//   r3_end_object();
}

STDMETHODIMP_(void) cAI::DebugDraw()
{
   r3s_point   pt[5];
   mxs_vector  translate;
   int         combatAIColor = guiScreenColor(guiRGB(255,0,0));
   const int   aiColor = 207;
   int         destColor = guiScreenColor(guiRGB(255,150,150));
   int         moveColor = guiScreenColor(guiRGB(240,145,70));
   const float drawSize = 1.0;
   cMxsVector  atLocation, tempPt;
   sAIMoveGoal  moveGoal;

   floatang    facing = m_state.GetFacingAng();

   translate.x = 0;
   translate.y = 0;
   translate.z = 0.2;

   m_state.GetLocation(&atLocation);

   // draw the ai...

   r3_start_object(&translate);
   r3_start_block();

   if (m_state.GetMode() == kAIM_Combat)
      r3_set_color(combatAIColor);
   else
      r3_set_color(aiColor);

   ProjectFromLocationOnZPlane(atLocation, drawSize, facing, &tempPt);
   r3_transform_point(&pt[0], &tempPt);

   ProjectFromLocationOnZPlane(atLocation, drawSize, facing + floatang(2*PI*0.4), &tempPt);
   r3_transform_point(&pt[1], &tempPt);

   ProjectFromLocationOnZPlane(atLocation, drawSize, facing - floatang(2*PI*0.4), &tempPt);
   r3_transform_point(&pt[2], &tempPt);

   r3_draw_line(&pt[0], &pt[1]);
   r3_draw_line(&pt[1], &pt[2]);
   r3_draw_line(&pt[2], &pt[0]);

   if (g_AIDebugDrawMoveGoal && m_pDebugMoveGoal && m_pDebugMoveGoal->speed)
   {
      r3_set_color(destColor);

      tempPt = atLocation;
      r3_transform_point(&pt[0], &tempPt);

      tempPt = m_pDebugMoveGoal->dest;
      r3_transform_point(&pt[1], &tempPt);

      r3_draw_line(&pt[0], &pt[1]);
   }

   // draw line of current suggestion

   if (g_AIDebugDrawSuggs && m_pDebugMoveGoal && m_pDebugMoveGoal->speed)
   {
      float      x, y;

      int speeds[] = 
      {
         0,                                      // kAIS_Stopped
         20,                                     // kAIS_VerySlow
         40,                                     // kAIS_Slow
         60,                                     // kAIS_Normal
         80,                                     // kAIS_Fast
         100,                                    // kAIS_VeryFast
      };

      x = cos(m_pDebugMoveGoal->dir.value) * speeds[m_pDebugMoveGoal->speed] / 10;
      y = sin(m_pDebugMoveGoal->dir.value) * speeds[m_pDebugMoveGoal->speed] / 10;

      r3_set_color(moveColor);

      tempPt.x = atLocation.x + x;
      tempPt.y = atLocation.y + y;
      tempPt.z = atLocation.z;
      r3_transform_point(&pt[0], &tempPt);

      tempPt = atLocation;
      r3_transform_point(&pt[1], &tempPt);

      r3_draw_line(&pt[0], &pt[1]);
      
      if (m_pDebugMoveSuggestions)
      {
         for (int i = 0; i < m_pDebugMoveSuggestions->Size(); i++)
         {
            AIDrawOneSuggestion(atLocation, *((*m_pDebugMoveSuggestions)[i]));
         }
      }
   }

   r3_end_block();
   r3_end_object();
}


STDMETHODIMP_(void) cAI::DebugSetPrimitiveMove(const cAIMoveSuggestions * pSuggs, const sAIMoveGoal * pGoal)
{
   Assert_((pSuggs && pGoal && !m_pDebugMoveSuggestions && !m_pDebugMoveGoal) ||
           (!pSuggs && !pGoal && m_pDebugMoveSuggestions && m_pDebugMoveGoal));
   m_pDebugMoveSuggestions = pSuggs;
   m_pDebugMoveGoal = pGoal;
}

STDMETHODIMP_(void) cAI::DebugGetPrimitiveMove(const cAIMoveSuggestions ** ppSugg, const sAIMoveGoal **ppGoal)
{
   *ppSugg = m_pDebugMoveSuggestions;
   *ppGoal = m_pDebugMoveGoal;
}

void AIPathFindDrawPath(cAIPath &path)
{
}

///////////////////////////////////////////////////////////////////////////////

#endif
