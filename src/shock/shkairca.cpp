/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkairca.cpp,v 1.1 1999/03/25 17:39:11 JON Exp $
//
// AI Combat - ranged
//

#include <shkairca.h>

#include <appagg.h>
#include <comtools.h>

#include <shkaircp.h>

#include <aiactrng.h>
#include <aiaware.h>
#include <aibasabl.h>
#include <aibasact.h>
#include <aiprcore.h>
#include <aitagtyp.h>
#include <aitrginf.h>

#include <iobjsys.h>
#include <linkbase.h>
#include <lnkquery.h>
#include <objdef.h>
#include <objpos.h>
#include <phconst.h>
#include <phoprop.h>
#include <phprop.h>
#include <port.h>
#include <rendprop.h>
#include <traitman.h>

// Must be last header
#include <dbmem.h>

enum eAIShockRangedRange_ {kAIShockRangedTooClose, kAIShockRangedClose, kAIShockRangedOK, kAIShockRangedFar, kAIShockRangedTooFar};

enum eAIShockRangedState_ {kAIShockRangedNothing, kAIShockRangedFiring, kAIShockRangedClosing, kAIShockRangedRetreating};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIShockRangedSubcombat
//

cAIShockRangedSubcombat::cAIShockRangedSubcombat():
   m_pShockRangedProp(NULL)
{
}

///////////////////////////////////////

cAIShockRangedSubcombat::~cAIShockRangedSubcombat()
{
}

///////////////////////////////////////

void cAIShockRangedSubcombat::Reset()
{
   m_pShockRangedProp = ShockAIGetRangedProp(m_pAIState->GetID());
}

///////////////////////////////////////

void cAIShockRangedSubcombat::SetState(eAIShockRangedState state)
{
   m_state = state;
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIShockRangedSubcombat::Init()
{
   cAISubcombat::Init();

   SetNotifications(kAICN_ActionProgress |
                    kAICN_Damage |
                    kAICN_GoalChange |
                    kAICN_ModeChange);

   Reset();
}

///////////////////////////////////////

STDMETHODIMP_(const char *) cAIShockRangedSubcombat::GetName()
{
   return "Shock Ranged";
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAIShockRangedSubcombat::Save(ITagFile * pTagFile)
{
   if (cAISubcombat::Save(pTagFile))
   {
      if (AIOpenTagBlock(GetID(), kAISL_ShockRanged, 0, 1, pTagFile))
      {
         AITagMove(pTagFile, &m_state);
         AICloseTagBlock(pTagFile);
      }
   }
   return TRUE;
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAIShockRangedSubcombat::Load(ITagFile * pTagFile)
{
   if (cAISubcombat::Load(pTagFile))
   {
      if (AIOpenTagBlock(GetID(), kAISL_ShockRanged, 0, 1, pTagFile))
      {
         AITagMove(pTagFile, &m_state);
         AICloseTagBlock(pTagFile);
      }
   }
   return TRUE;
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIShockRangedSubcombat::OnGoalChange(const cAIGoal * pPrevious, const cAIGoal * pGoal)
{
   cAISubcombat::OnGoalChange(pPrevious, pGoal);
// @TBD (toml 03-04-99): check this
   if (IsOwnerLosingControl(pPrevious, pGoal))
      Reset();
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIShockRangedSubcombat::OnDamage(const sDamageMsg *pMsg)
{
   cAISubcombat::OnDamage(pMsg);
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIShockRangedSubcombat::OnActionProgress(IAIAction * pAction)
{
   cAISubcombat::OnActionProgress(pAction);

   pAction = pAction->GetTrueAction(); // get our action in case proxied

   if (!IsOwn(pAction))
      return;

   switch (pAction->GetResult())
   {
      // We've succeeded and are done
      case kAIR_Success:
      {
         SetState(kAIShockRangedNothing);
         SignalAction();
         break;
      }

      // Failure!
      case kAIR_Fail:
      {
         SetState(kAIShockRangedNothing);
         SignalAction();
         break;
      }

      case kAIR_NoResultSwitch:
      {
         break;
      }
   }
}

////////////////////////////////////////

cAIAction * cAIShockRangedSubcombat::SuggestRetreatAction(void)
{
   cAIAction * pAction = NULL;
   return pAction;
}

////////////////////////////////////////

cAIAction * cAIShockRangedSubcombat::SuggestCloseAction(void)
{
   cAIAction * pAction = NULL;
   return pAction;
}

////////////////////////////////////////

cAIAction* cAIShockRangedSubcombat::SuggestFireAction(void)
{
   cAIAttackRangedAction* pAttackAction = NULL;

   // Start an attack
   if (m_selectedProjID != OBJ_NULL)
   {
      // Clone it
      AutoAppIPtr(ObjectSystem);
      ObjID projID = pObjectSystem->Create(m_selectedProjID, kObjectConcrete);
      
      // Remove its physics and make it invisible
      g_pPhysTypeProp->Delete(projID);
      ObjSetHasRefs(projID, FALSE);

      pAttackAction = new cAIAttackRangedAction(this);
      
      pAttackAction->Set(GetTargetInfo()->id, projID, m_pSelectedProjData->leads_target,
         m_pSelectedProjData->accuracy, 1, m_pSelectedProjData->launch_joint);
   }
   return pAttackAction;
}


////////////////////////////////////////

STDMETHODIMP cAIShockRangedSubcombat::SuggestActions(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew)
{
   cAIAction *pAction = NULL;

   EvaluatePosition();

   switch (m_range)
   {
   case kAIShockRangedTooClose:
      pAction = SuggestRetreatAction();
      break;
   case kAIShockRangedOK:
      pAction = SuggestFireAction();
      break;
   case kAIShockRangedTooFar:
      pAction = SuggestCloseAction();
      break;
   }

   if (pAction != NULL)
      pNew->Append(pAction);

   return S_OK;
}

///////////////////////////////////////////

void cAIShockRangedSubcombat::EvaluatePosition(void)
{
   EvaluateRange();
   SelectProjectile();
}

///////////////////////////////////////////
//
// This should be based on the projectile and folded into SelectProjectile
//

void cAIShockRangedSubcombat::EvaluateRange(void)
{
   m_dist = sqrt(GetTargetInfo()->distSq);
   if (m_dist<m_pShockRangedProp->m_minimumDist)
      m_range = kAIShockRangedTooClose;
   else if (m_dist>m_pShockRangedProp->m_maximumDist)
      m_range = kAIShockRangedTooFar;
   else
      m_range = kAIShockRangedOK;
}

///////////////////////////////////////////

struct sSelection
{
   sSelection(ObjID _projectile, sAIProjectileRel *_projectile_data, mxs_vector _targ_loc)
    : projectile(_projectile),
      projectile_data(_projectile_data),
      targ_loc(_targ_loc)
   {};

   ObjID projectile;
   sAIProjectileRel *projectile_data;
   mxs_vector targ_loc;
};

////////////////////////////////////////////////////////

BOOL cAIShockRangedSubcombat::SelectProjectile(void)
{
   cDynArray<sSelection> selection_list;

   AutoAppIPtr(TraitManager);
   ILinkQuery *query = g_pAIProjectileRelation->Query(pTraitManager->GetArchetype(m_pAIState->GetID()), LINKOBJ_WILDCARD);
   for (; !query->Done(); query->Next())
   {
      sAIProjectileRel *pProjData = (sAIProjectileRel *)query->Data();
      sLink link;

      query->Link(&link);

      // Check if the target is hittable
      mxs_vector targ_loc;
      if (!ProjectileHittable(pProjData->targeting_method, link.dest, m_pAIState->GetID(),
                              GetTargetInfo()->id))
         continue;

      sSelection selection(link.dest, pProjData, targ_loc);

      // Bias selection
      for (int i=0; i<pProjData->selection_desire+1; i++)
         selection_list.Append(selection);
   }
   SafeRelease(query);

   if (selection_list.Size() > 0)
   {
      int index = AIRandom(0, selection_list.Size() - 1);
      m_selectedProjID = selection_list[index].projectile;
      m_pSelectedProjData = selection_list[index].projectile_data;
      return TRUE;
   }
   m_selectedProjID = OBJ_NULL;
   m_pSelectedProjData = NULL;
   return FALSE;
}

////////////////////////////////////////

BOOL cAIShockRangedSubcombat::ProjectileHittable(int targ_method, ObjID projectile, ObjID source, ObjID target) 
{
   if ((target == OBJ_NULL) || (source == OBJ_NULL))
      return FALSE;

   mxs_vector *source_loc = &ObjPosGet(source)->loc.vec;
   const mxs_vector *target_loc = &GetTargetLoc();

   Location start, end, hit;

   switch (targ_method)
   {
      case kTM_StraightLine:
      {
         MakeLocationFromVector(&start, source_loc);
         MakeLocationFromVector(&end, target_loc);

         ComputeCellForLocation(&start);
         if (PortalRaycast(&start, &end, &hit, FALSE))
            return TRUE;
         else
            return FALSE;
      }

      case kTM_Arcing:
      {
         mxs_vector    *pInitVel;
         mxs_real       height;
         mxs_vector     midpt;

         if (!g_pPhysInitVelProp->Get(projectile, &pInitVel))
         {
            Warning(("Projectile %d has no initial velocity\n", projectile));
            return FALSE;
         }

         for (float mult = 0.2; mult <= 1.0; mult += 0.2)
         {
            height = (0.5 * sq(pInitVel->x * mult)) / (2 * kGravityAmt);

            mx_sub_vec(&midpt, target_loc, source_loc);
            mx_scaleeq_vec(&midpt, 0.5);
            midpt.z += height;
            mx_addeq_vec(&midpt, source_loc);

            MakeLocationFromVector(&start, &midpt);
            ComputeCellForLocation(&start);

            if (start.cell == CELL_INVALID)
               continue;

            MakeLocationFromVector(&end, source_loc);

            if (!PortalRaycast(&start, &end, &hit, TRUE))
               continue;

            MakeLocationFromVector(&end, target_loc);

            if (!PortalRaycast(&start, &end, &hit, TRUE))
               continue;

            return TRUE;
         }

         return FALSE;
      }
      default:
      {
         Warning(("Unknown targeting method: %d\n", targ_method));
         return FALSE;
      }
   }
}

////////////////////////////////////////

BOOL cAIShockRangedSubcombat::HasTargetLOS(void)
{
   if (GetTarget() == OBJ_NULL)
      return FALSE;

   const sAIAwareness *pAwareness = m_pAI->GetAwareness(GetTarget());

   return !!(pAwareness->flags & kAIAF_HaveLOS);
}
