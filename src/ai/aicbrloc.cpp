/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/ai/aicbrloc.cpp,v 1.7 2000/02/19 12:17:26 toml Exp $

#include <aicbrloc.h>

#include <autolink.h>
#include <objpos.h>

#include <aicbrmpr.h>
#include <aicbrnew.h> 
#include <aicbrsht.h>
#include <aitrginf.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

   float m_LOFPlus;
   float m_rangePlus[kAIRC_NumRanges];
   int m_damageMax;
   float m_damageMinus;
   float m_coverPlus;

sAIRangedLocationScorecard g_defaultScorecard = 
{
   1000.,                  // LOF
   500,                    // partial LOF (to head)
   {-50, 0, 50, 0, -100},  // range 
   100.,                   // maximum damage penalty
   1.,                     // damage penalty/point
   0.,                     // cover bonus 
};

///////////////////////////////////////////////////////////////////////////////

cAIRangedLocation::cAIRangedLocation(cAINewRangedSubcombat* pOwner): 
   m_pOwner(pOwner)
{
   m_pOwner->AddRef();
   m_ownerID = m_pOwner->GetID();
}

///////////////////////////////////////////////////////////////////////////////

cAIRangedLocation::cAIRangedLocation(cAINewRangedSubcombat* pOwner, const Location& loc): 
   m_pOwner(pOwner)
{
   m_pOwner->AddRef();
   m_ownerID = m_pOwner->GetID();
   SetLocation(loc);
}

///////////////////////////////////////////////////////////////////////////////

void cAIRangedLocation::SetLocation(const Location& loc)  
{
   m_loc = loc;
   m_damageTaken = 0;
}

///////////////////////////////////////////////////////////////////////////////

BOOL cAIRangedLocation::TestPathcast(void)
{
   m_pathcast = m_pOwner->Pathcast(m_loc.vec, &m_cellID);
   return m_pathcast;
}

///////////////////////////////////////////////////////////////////////////////

BOOL cAIRangedLocation::TestLOF(void)
{
   ObjID targetID = m_pOwner->GetTargetInfo()->id;
   ObjID projID = m_pOwner->GetDefaultProjectile();

   if (projID != OBJ_NULL)
   {
      cAutoLinkQuery query(g_pAIProjectileRelation, m_ownerID, projID);

      if (!query->Done())
      {
         mxs_vector ignore;
         sAIProjectileRel* pProjData = (sAIProjectileRel *)query->Data();
         if (pProjData)
         {
            // Check for launch offset
            Location launchLoc;
            if (AIGetLaunchOffset(m_ownerID, &launchLoc))
               m_targetSubModel = m_pOwner->ProjectileHittable(pProjData->targeting_method, projID, launchLoc, targetID, &ignore);
            else
               m_targetSubModel = m_pOwner->ProjectileHittable(pProjData->targeting_method, projID, m_loc, targetID, &ignore);
            if (m_targetSubModel<0)
               m_LOFFlags = kAIRC_ProjBlocked;
            else if (m_targetSubModel>0)
               m_LOFFlags = kAIRC_ProjPartial;
            else
               m_LOFFlags = kAIRC_ProjClear;
         }
         else
            m_LOFFlags = kAIRC_ProjBlocked; // why not?
      }
      else
      {
         Warning(("cAIRangedLocation::ForceUpdate - projectile %d has no link\n", projID));
         m_LOFFlags = kAIRC_ProjBlocked;
      }
   }
   else
      m_LOFFlags = kAIRC_ProjBlocked;
   return !(m_LOFFlags&kAIRC_ProjBlocked);
}

///////////////////////////////////////////////////////////////////////////////

eAIRangedCombatRange cAIRangedLocation::TestRangeClass(void)  
{
   ObjID targetID = m_pOwner->GetTargetInfo()->id;
   mxs_vector *pTargetLoc = &ObjPosGet(targetID)->loc.vec;
   m_rangeSq = AIDistanceSq(m_loc.vec, *pTargetLoc);
   m_rangeClass = m_pOwner->ClassifyRange(m_rangeSq);
   return m_rangeClass;
}

///////////////////////////////////////////////////////////////////////////////

void cAIRangedLocation::Score(void)
{
   // @TODO: propertize, select based on state
   sAIRangedLocationScorecard* pScorecard = &g_defaultScorecard;
   const sAIRangedRanges* pRangesSq = m_pOwner->GetRangesSq();

   m_score = 0;
   if (!(m_LOFFlags&kAIRC_ProjBlocked))
      if (m_LOFFlags&kAIRC_ProjPartial)
         m_score -= pScorecard->m_LOFPartialPlus;
      else
         m_score += pScorecard->m_LOFPlus;
   // range score is linearly interpolated
   if (m_rangeClass<kAIRC_RangeVeryLong)
      m_score += pScorecard->m_rangePlus[m_rangeClass]+
         ((m_rangeSq-pRangesSq->m_ranges[m_rangeClass])/(pRangesSq->m_ranges[m_rangeClass+1]-pRangesSq->m_ranges[m_rangeClass]))*
         (pScorecard->m_rangePlus[m_rangeClass+1]-pScorecard->m_rangePlus[m_rangeClass]);
   else
      m_score = pScorecard->m_rangePlus[kAIRC_RangeVeryLong];
   m_score -= m_damageTaken*pScorecard->m_damageMinus;
   m_score += pScorecard->m_coverPlus*m_cover;
}

///////////////////////////////////////////////////////////////////////////////
