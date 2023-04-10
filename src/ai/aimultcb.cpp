/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aimultcb.cpp,v 1.16 2000/02/16 21:08:08 bfarquha Exp $
//
//
//

#include <objpos.h>
#include <physapi.h>
#include <playrobj.h>
#include <property.h>
#include <propface.h>
#include <propbase.h>

#include <aiactprx.h>
#include <aiapiact.h>
#include <aiapipth.h>
#include <aiapisns.h>
#include <aiaware.h>
#include <aidebug.h>
#include <aimultcb.h>
#include <aiprops.h>
#include <aipthloc.h>
#include <aitrginf.h>

#include <aipathdb.h>

// Added for player climbing check
#include <phclimb.h>


#ifndef SHIP
#include <config.h>
#endif

#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////
//
// Tactical ranges
// @TBD (toml 03-03-99): need to replace hand-to-hand dependence
//

// Older whay to deal -- replace this (toml 03-03-99)
static IVectorProperty *g_pAIHtoHCombatDists = NULL;
static sPropertyDesc HtoHCombatDists = { "HTHCombatDist", 0, NULL, 0, 0, { AI_ABILITY_CAT, "HtoHCombat: Distances" }, kPropertyChangeLocally };

///////////////////////////////////////
//
// This function converts older property to projected new one
// @TBD (toml 03-03-99): convert this. Establish if there needs to be more than just Huge
//

sAICombatTacticalRanges * AIGetTacticalRanges(ObjID id)
{
   static sAICombatTacticalRanges ranges;
   static mxs_vector default_dists;

   if (default_dists.el[0] == 0.0)
   {
      default_dists.el[0]  = 4.8;
      default_dists.el[1]  = 7.4;
      default_dists.el[2]  = 10.2;
   }

   mxs_vector *dists=&default_dists;
   g_pAIHtoHCombatDists->Get(id,&dists);

   ranges.Near = dists->el[0];
   ranges.Ideal = dists->el[1];
   ranges.Huge = dists->el[2];
#ifndef SHIP
   if (!config_get_float("HugeZ", &ranges.HugeZ))
#endif
   ranges.HugeZ = HUGE_Z; // was 2.5, but after some testing, this seems like a reasonable value. Value stored in aipathdb.h

   return &ranges;
}

///////////////////////////////////////////////////////////////////////////////

BOOL AIInitMultiCombat(IAIManager *)
{
   g_pAIHtoHCombatDists = CreateVectorProperty(&HtoHCombatDists, kPropertyImplVerySparse);
   return TRUE;
}

///////////////////////////////////////

BOOL AITermMultiCombat()
{
   SafeRelease(g_pAIHtoHCombatDists);
   return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIMultiCombat
//

cAIMultiCombat::cAIMultiCombat()
 : m_pActiveSubcombat(NULL),
   m_pTargetInfo(new sAITargetInfo)
{
   memset(m_pTargetInfo, 0, sizeof(*m_pTargetInfo));
}

///////////////////////////////////////

cAIMultiCombat::~cAIMultiCombat()
{
   delete m_pTargetInfo;
}

///////////////////////////////////////

void cAIMultiCombat::SetSubcombat(cAISubcombat * pSubcombat)
{
   SafeRelease(m_pActiveSubcombat);
   m_pActiveSubcombat = pSubcombat;
   if (m_pActiveSubcombat != NULL)
      m_pActiveSubcombat->AddRef();
}

///////////////////////////////////////

STDMETHODIMP cAIMultiCombat::SuggestActions(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew)
{
   if (!BeginSuggestActions())
   {
      AIWatch(Combat, GetID(), "Combat cannot act -- probably no target");

      SignalGoal();
      return E_FAIL;
   }

   cAISubcombat* pActiveSubcombat = SelectSubcombat(pGoal, previous);
   if (pActiveSubcombat != m_pActiveSubcombat)
      SetSubcombat(pActiveSubcombat);

   if (!m_pActiveSubcombat)
      return S_OK;

   int        i;
   cAIActions innerActions;
   cAIActions innerPrevious;

   if (previous.Size() && previous[0]->GetInnerAction()->Access()->pOwner == m_pActiveSubcombat)
   {
      for (i = 0; i < previous.Size(); i++)
         innerPrevious.Append(previous[i]->GetInnerAction());
   }

   HRESULT result = m_pActiveSubcombat->SuggestActions(pGoal, innerPrevious, &innerActions);

   for (i = 0; i < innerActions.Size(); i++)
   {
      pNew->Append(new cAIProxAction(innerActions[i], this));
      innerActions[i]->Release();
   }

   return result;
}

///////////////////////////////////////
//
// Called at start of SuggestActions()
// Default just gathers target info
//

BOOL cAIMultiCombat::BeginSuggestActions()
{
   return GatherTargetInfo();
}

///////////////////////////////////////

BOOL cAIMultiCombat::GatherTargetInfo()
{
   const cMxsVector & loc = *m_pAIState->GetLocation();

   memset(m_pTargetInfo, 0, sizeof(*m_pTargetInfo));

   m_pTargetInfo->id = GetTarget();

   if (!m_pTargetInfo->id)
      return FALSE;

   IAISenses * pSenses = m_pAI->AccessSenses();

   if (pSenses && IsAPlayer(m_pTargetInfo->id))
      pSenses->KeepFresh(m_pTargetInfo->id, 0);

   m_pTargetInfo->pAwareness = m_pAI->GetAwareness(m_pTargetInfo->id);

   if (m_pTargetInfo->pAwareness->flags & kAIAF_CanRaycast)
   {
      m_pTargetInfo->flags |= kAITF_CanRaycast;
      GetObjLocation(m_pTargetInfo->id, &m_pTargetInfo->loc);
   }
   else
   {
      m_pTargetInfo->flags &= ~kAITF_CanRaycast;

      if (m_pTargetInfo->pAwareness->ValidLastPos())
         m_pTargetInfo->loc = m_pTargetInfo->pAwareness->lastPos;
      else
         m_pTargetInfo->loc = kInvalidLoc;
   }

   if (memcmp(&m_pTargetInfo->loc, &kInvalidLoc, sizeof(mxs_vector)) == 0)
   {
      Warning(("AI %d has an attack link to object %d, but doesn't know where the object is.\n", GetID(), m_pTargetInfo->id));
      return FALSE;
   }

#ifdef TBD
#ifdef NET_TODO // NEW_NETWORK_ENABLED
   if (m_pAI->IsNetworkProxy())
      fCanRaycast = TRUE;   // The proxy can always see the target.
#endif
#endif

   m_pTargetInfo->distSq = m_pAIState->DistSq(m_pTargetInfo->loc);

   m_pTargetInfo->targetHeading = floatang(loc.x, loc.y, m_pTargetInfo->loc.x, m_pTargetInfo->loc.y);
   m_pTargetInfo->targetHeadingDelta = Delta(m_pAIState->GetFacingAng(), m_pTargetInfo->targetHeading);

   if (m_pTargetInfo->targetHeadingDelta > PI/2)
      m_pTargetInfo->flags |= kAITF_Behind;
   else
      m_pTargetInfo->flags &= ~kAITF_Behind;

   // Compute tactical range
   if (!(m_pTargetInfo->flags & kAITF_CanRaycast))
      m_pTargetInfo->range = kAICR_Unseen;
   else
   {
      BOOL bTargetIsClimbing = FALSE;
      const sAICombatTacticalRanges * pRanges = AIGetTacticalRanges(GetID());

      // Using z height of cells, compute z difference
      float zdist;
      tAIPathCellID targetCell = AIFindClosestCell(m_pTargetInfo->loc, 0, m_pTargetInfo->id);

      if (targetCell && m_pAIState->GetPathCell())
      {
         float zTarg;
         float zAI = AIGetZAtXYOnCell(*m_pAIState->GetPathLocation(), m_pAIState->GetPathCell());

         zTarg = AIGetZAtXYOnCell(m_pTargetInfo->loc, targetCell);
         m_pTargetInfo->zLocNearFloor = min(zTarg + 3.0, m_pTargetInfo->loc.z);

         // wsf: added so AI's will get frustrated when player is too high on rope or ladder.
         // We're restricting this to *just* the player, and *only* when climbing, though
         // possibly we could make a more general solution. I just want to keep the possible
         // side effects local, as we're close to shipping...
         //
         if (IsAPlayer(m_pTargetInfo->id) && PhysObjIsClimbing(m_pTargetInfo->id))
         {
            float fMinZ, fMaxZ; // Oh, we really don't need to do this every time!
            zTarg = m_pTargetInfo->loc.z;

            // Assumed to be sphere model!
            AIGetPhysSphereZMinMax(m_pTargetInfo->id, &fMinZ, &fMaxZ);

            // If target is above ai, the subtract from z so feet are targetable,
            // else add z, so head is targetable:
            if (m_pTargetInfo->loc.z > ObjPosGet(GetID())->loc.vec.z)
               zTarg += fMinZ; // fMinZ will be negative value.
            else
               zTarg += fMaxZ; // fMaxZ is positive.
            bTargetIsClimbing = TRUE;
         }

         zdist = ffabsf(zAI - zTarg);
      }
      else
      {
         m_pTargetInfo->zLocNearFloor = m_pTargetInfo->loc.z;
         zdist = 0.0;
      }

      m_pTargetInfo->zdist = zdist;

      if (zdist > pRanges->HugeZ)
      {
         m_pTargetInfo->range = kAICR_HugeZ;
         if (bTargetIsClimbing) // Special way we talk to aicbhtoh.cpp about getting frustrated when player is climbing and out of range.
            m_pTargetInfo->fStrikeRange = zdist;
         else
            m_pTargetInfo->fStrikeRange = 0;
      }
      else
      {
         mxs_vector targ_vel, delta_pos;

         // Adjust ranges based on closing speed of target
         float scaleVal;
         PhysGetVelocity(m_pTargetInfo->id, &targ_vel);
         mx_sub_vec(&delta_pos, &ObjPosGet(GetID())->loc.vec, &ObjPosGet(m_pTargetInfo->id)->loc.vec);
         mx_normeq_vec(&delta_pos);

         scaleVal = 1.0 + 0.09 * mx_dot_vec(&delta_pos, &targ_vel);

         if (m_pTargetInfo->distSq < sq(pRanges->Near * scaleVal))
            m_pTargetInfo->range = kAICR_Near;
         else if (m_pTargetInfo->distSq < sq(pRanges->Ideal * scaleVal))
            m_pTargetInfo->range = kAICR_Norm;
         else if (m_pTargetInfo->distSq < sq(pRanges->Huge * scaleVal))
         {
            if (m_pAI->AccessPathfinder()->Pathcast(m_pTargetInfo->loc))
            {
               if (m_pTargetInfo->distSq < sq((pRanges->Ideal * scaleVal) + 1.2))
                  m_pTargetInfo->range = kAICR_JustFar;
               else
                  m_pTargetInfo->range = kAICR_Far;
            }
            else
            {
// wsf: changed to this, so we can try to hit player, if we don't care where on cell he is.
//               m_pTargetInfo->range = kAICR_JustFar;
               // delta_pos is normalized vec from target to AI
               mxs_vector ZVec=  {0,0,1};
               scaleVal = 1-fabs(mx_dot_vec(&delta_pos, &ZVec));
               float fUseDist = pRanges->Ideal*scaleVal+0.1;
               if (fUseDist < 1) // if we have to be this close in order to strike, just forget it.
               {
                  if (bTargetIsClimbing) // Special way we talk to aicbhtoh.cpp about getting frustrated when player is climbing and out of range.
                     m_pTargetInfo->fStrikeRange = kAICR_HugeZ;
                  else
                     m_pTargetInfo->fStrikeRange = 0;

                  m_pTargetInfo->range = kAICR_HugeZ;
               }
               else
               {
                  m_pTargetInfo->range = kAICR_Huge;
                  m_pTargetInfo->fStrikeRange = fUseDist;
               }
            }
         }
         else
         {
            mxs_vector ZVec=  {0,0,1};
            scaleVal = 1-fabs(mx_dot_vec(&delta_pos, &ZVec));
            float fUseDist = pRanges->Ideal*scaleVal+0.1;
            if (fUseDist < 1) // if we have to be this close in order to strike, just forget it.
            {
               if (bTargetIsClimbing) // Special way we talk to aicbhtoh.cpp about getting frustrated when player is climbing and out of range.
                  m_pTargetInfo->fStrikeRange = kAICR_HugeZ;
               else
                  m_pTargetInfo->fStrikeRange = 0;

               m_pTargetInfo->range = kAICR_HugeZ;
            }
            else
            {
               m_pTargetInfo->range = kAICR_Huge;
               m_pTargetInfo->fStrikeRange = fUseDist;
            }
         }
      }
   }

   return TRUE;
}

///////////////////////////////////////

const sAICombatTacticalRanges* cAIMultiCombat::GetRanges(void) const
{
   return AIGetTacticalRanges(GetID());
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAISimpleMultiCombat
//

cAISimpleMultiCombat::cAISimpleMultiCombat()
{
}

///////////////////////////////////////

cAISimpleMultiCombat::~cAISimpleMultiCombat()
{
   SafeRelease(m_pActiveSubcombat);
}

///////////////////////////////////////

cAISimpleMultiCombat::cAISimpleMultiCombat(cAISubcombat * pSubcombat)
{
   SetSubcombat(pSubcombat);
}

///////////////////////////////////////

void cAISimpleMultiCombat::SetSubcombat(cAISubcombat * pSubcombat)
{
   m_pActiveSubcombat = pSubcombat;
   m_pActiveSubcombat->AddRef();
   m_pActiveSubcombat->InitSubability(this);
}

///////////////////////////////////////

STDMETHODIMP_(void) cAISimpleMultiCombat::GetSubComponents(cAIComponentPtrs * pResult)
{
   m_pActiveSubcombat->AddRef();
   pResult->Append(m_pActiveSubcombat);
}

///////////////////////////////////////

cAISubcombat * cAISimpleMultiCombat::SelectSubcombat(cAIGoal * pGoal, const cAIActions & previous)
{
   return m_pActiveSubcombat;
}

///////////////////////////////////////////////////////////////////////////////
