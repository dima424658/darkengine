/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkradar.cpp,v 1.9 2000/02/19 13:26:02 toml Exp $

#include <shkradar.h>

#include <matrix.h>

#if AI_RIP
//include <aiext.h>
//include <aipool.h>
//include <ai.h>
#endif

#include <wrtype.h>
#include <objpos.h>
#include <osysbase.h>
#include <playrobj.h>
#include <property.h>
#include <propbase.h>
#include <prcniter.h>
#include <rendprop.h>

#include <shkrdrbs.h>
#include <shkovrly.h>
#include <shkovcst.h>
#include <shkrdrov.h>
#include <shkpsipr.h>

#include <mprintf.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

cRadar g_radar;

#define kRadarRange (80.0F)
#define kRadarRangeSquared (kRadarRange*kRadarRange)

cRadar::cRadar(void)
{
   int i;

   m_status = kRadarOff;
   // we ignore z deltas for now
   for (i=0; i<kMaxRadarTargets; i++)
      m_targetVecs[i].z = 0;
}

void cRadar::On(void) 
{
   m_status = kRadarOn; 
   Scan();
   //ShockOverlayChange(kOverlayRadar, kOverlayModeOn);
}

void cRadar::Off(void) 
{
   m_status = kRadarOff;
   //ShockOverlayChange(kOverlayRadar, kOverlayModeOff);
}

// Re-scan for targets
// Currently just looks for all AIs in range
// Store off the target deltas 
void cRadar::Scan(void)
{
   cConcreteIter iter(g_PsiRadarProperty);
   // PropertyObjIter iter;
   ObjID obj;
   mxs_vector *pMyLoc = &(ObjPosGet(PlayerObject())->loc.vec);
   mxs_vector delta;
   mxs_vector *targLoc;
   float dist;

   m_numTargets = 0;
   //g_PsiRadarProperty->IterStart(&iter);
   iter.Start();   
//   while (g_PsiRadarProperty->IterNext(&iter,&obj))
   while (iter.Next(&obj))
   {
      if (!OBJ_IS_CONCRETE(obj) || !ObjHasRefs(obj))
         continue;

      int targtype;
      g_PsiRadarProperty->Get(obj, &targtype);

      targLoc = &(ObjPosGet(obj)->loc.vec);
      mx_sub_vec(&delta, targLoc, pMyLoc);
      // ignore z for the moment
      delta.z = 0;
      dist = mx_mag2_vec(&delta);
      if (dist<=kRadarRangeSquared)
      {
         // reverse x coord system because we are going to translate to screen units
         m_targetVecs[m_numTargets].x = -int((delta.x/kRadarRange)*kRadarHalfWidth);
         m_targetVecs[m_numTargets].y = int((delta.y/kRadarRange)*kRadarHalfHeight);
         m_targetData[m_numTargets] = targtype;
         m_numTargets++;
         if (m_numTargets == kMaxRadarTargets)
            break;
      }
   }
//   g_PsiRadarProperty->IterStop(&iter);
   iter.Stop();
   

   /*
   cAI *pAI;
   mxs_vector *pMyLoc = &(ObjPosGet(PlayerObject())->loc.vec);
   mxs_vector aiLoc;
   mxs_vector delta;
   float dist;

   m_numTargets = 0;
   if (g_pAIPool->GetFirstAI(pAI))
   {
      do
      {
         if (!pAI->IsPlayer()) 
         {
            AIGetAILocation(pAI, &aiLoc);
            mx_sub_vec(&delta, &aiLoc, pMyLoc);
            // ignore z for the moment
            delta.z = 0;
            if ((dist = mx_mag2_vec(&delta))<=kRadarRangeSquared)
            {
               // reverse x coord system because we are going to translate to screen units
               m_targetVecs[m_numTargets].x = -int((delta.x/kRadarRange)*kRadarHalfWidth);
               m_targetVecs[m_numTargets].y = int((delta.y/kRadarRange)*kRadarHalfHeight);
               m_targetData[m_numTargets] = kTargetEnemy;
               m_numTargets++;
            }
         }
      }
      while (g_pAIPool->GetNextAI(pAI));
   }
   */
}

// Update target points without re-scanning.
void cRadar::Status(int *pTargetNum, Point **ppTargetPts, int **ppTargetData)
{
//   if (m_status == kRadarOn)
   {
      mxs_vector relVec;
      mxs_ang heading = ObjPosGet(PlayerObject())->fac.tz+0x4000;
      int i;

      for (i=0; i<m_numTargets; i++)
      {
         mx_rot_z_vec(&relVec, &m_targetVecs[i], heading);
         m_targetPts[i].x = int(relVec.x);
         m_targetPts[i].y = int(relVec.y);
      }
      *pTargetNum = m_numTargets;
      *ppTargetPts = m_targetPts;
      *ppTargetData = m_targetData;
   }
//   else 
//      *pTargetNum = 0;
}

void RadarOn(void)
{
   g_radar.On();
}

void RadarOff(void)
{
   g_radar.Off();
}

void RadarScan(void)
{
   g_radar.Scan();
}

void RadarStatus(int *pTargetsNum, Point **ppTargetPts, int **ppTargetData)
{
   g_radar.Status(pTargetsNum, ppTargetPts, ppTargetData);
}