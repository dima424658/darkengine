/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/dark/daielem.cpp,v 1.3 1998/11/07 21:58:33 CCAROLLO Exp $
//
//
//


#include <lg.h>

#include <comtools.h>
#include <appagg.h>
#include <property.h>
#include <propman.h>
#include <propface.h>

#include <ai.h>
#include <aiutils.h>
#include <daielem.h>

#include <objslit.h>
#include <slitprop.h>

#include <linkman.h>
#include <relation.h>
#include <autolink.h>

#include <pgrpprop.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIElemental
//

STDMETHODIMP_(float) cAIElemental::GetGroundOffset()
{
   return cAI::GetGroundOffset() + sin(((AIGetTime() % 4000) / 2000.0) * PI) * 0.5;
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIElementalLightAbility
//

cAIElementalLightAbility::cAIElementalLightAbility()
   : m_lastLightProp(0)
{
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIElementalLightAbility::Init()
{
   SetNotifications(kAICN_ModeChange);
}

///////////////////////////////////////

STDMETHODIMP_(const char *) cAIElementalLightAbility::GetName()
{
   return "Elemental light ability";
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIElementalLightAbility::OnModeChange(eAIMode previous, eAIMode mode)
{
   // Waking up
   if ((previous < kAIM_Normal) && (mode >= kAIM_Normal))
   {
      // turn on dynamic light
      ObjSetSelfLit(m_pAIState->GetID(), m_lastLightProp);

      // turn on particle groups
      AutoAppIPtr_(LinkManager, pLinkMan);
      IRelation *particle_attach = pLinkMan->GetRelationNamed(LINK_PARTICLE_ATTACHMENT_NAME);
      cAutoLinkQuery query(particle_attach, LINKOBJ_WILDCARD, m_pAIState->GetID());

      for (; !query->Done(); query->Next())
         ObjParticleSetActive(query.GetSource(), TRUE);

      SafeRelease(particle_attach);
   }
   
   // Going to sleep
   if ((previous >= kAIM_Normal) && (mode < kAIM_Normal))
   {
      AutoAppIPtr_(PropertyManager, pPropMan);
      IProperty *pSelfLitProp;

      // turn off dynamic light
      if ((pSelfLitProp = pPropMan->GetPropertyNamed(PROP_SELF_LIT_NAME)) != NULL)
      {
         ((IIntProperty *)pSelfLitProp)->Get(m_pAIState->GetID(), &m_lastLightProp);
         pSelfLitProp->Delete(m_pAIState->GetID());
         SafeRelease(pSelfLitProp);
      }

      // turn off particle groups
      AutoAppIPtr_(LinkManager, pLinkMan);
      IRelation *particle_attach = pLinkMan->GetRelationNamed(LINK_PARTICLE_ATTACHMENT_NAME);
      cAutoLinkQuery query(particle_attach, LINKOBJ_WILDCARD, m_pAIState->GetID());

      for (; !query->Done(); query->Next())
         ObjParticleSetActive(query.GetSource(), FALSE);

      SafeRelease(particle_attach);
   }
}

///////////////////////////////////////////////////////////////////////////////
