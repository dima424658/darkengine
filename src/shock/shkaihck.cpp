/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkaihck.cpp,v 1.7 2000/02/19 12:36:27 toml Exp $

#include <shkaihck.h>

#include <propface.h>
#include <propbase.h>

#include <sdesc.h>
#include <sdesbase.h>

#ifdef AI_RIP
//include <ai.h>
//include <aicbllm.h>
//include <aipool.h>
#endif
#include <schema.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

IIntProperty* gPropShockAIType;

static sPropertyDesc ShockAITypeDesc =
{
  PROP_SHOCK_AI_TYPE_NAME, 0, 
  NULL, 0, 0, // constraints, versions
  { "AI", "Shock Type" },
};

void ShockAITypePropInit()
{
   gPropShockAIType = CreateIntProperty(&ShockAITypeDesc,kPropertyImplDense);
}

int GetShockAIType(ObjID obj)
{
   int ShockAIType = kShockAINone; // default value
   gPropShockAIType->Get(obj,&ShockAIType);
   return ShockAIType;
}

void ShockAIDie(cAI *pAI, int type)
{
   // death sounds don't seem to work?!?
   switch (type)
   {
   case kShockAIDroid:
#ifdef AI_RIP
      SchemaPlayObj((const Label*)"tdroid_die", pAI->GetObjID());
      pAI->GetCerebellum()->Die();
#endif
      break;
   case kShockAITurret:
#ifdef AI_RIP
      SchemaPlayObj((const Label*)"turret_die", pAI->GetObjID());
      g_pAIPool->DestroyAI(pAI->GetID());
#endif
      break;
   case kShockAIGrunt:
#ifdef AI_RIP
      SchemaPlayObj((const Label*)"grunt_di", pAI->GetObjID());
      g_pAIPool->DestroyAI(pAI->GetID());
#endif
      break;
   }
}
