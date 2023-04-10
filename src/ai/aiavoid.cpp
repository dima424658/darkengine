/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiavoid.cpp,v 1.6 2000/03/25 22:13:00 adurant Exp $
//
//
//

#include <lg.h>
#include <dynarray.h>

#include <objpos.h>
#include <prcniter.h>
#include <propbase.h>
#include <property.h>
#include <propert_.h>
#include <prophash.h>
#include <sdesbase.h>
#include <sdesc.h>

#include <aiutils.h>
#include <aiavoid.h>
#include <aiprops.h>

#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////

static cDynArray<ObjID> m_AIRepelObjects;

///////////////////////////////////////////////////////////////////////////////
//
// PROPERTY: IAIObjectAvoidProperty
//

#undef INTERFACE
#define INTERFACE IAIObjectAvoidProperty
DECLARE_PROPERTY_INTERFACE(IAIObjectAvoidProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sAIObjectAvoid *);
};

///////////////////////////////////////

static sPropertyDesc _g_ObjectAvoidPropertyDesc = 
{
   "AI_ObjAvoid",
   0,
   NULL, 0, 0,  // constraints, version
   { AI_UTILITY_CAT, "Path avoid" },
   kPropertyChangeLocally, // net_flags
};

///////////////////////////////////////

typedef cSpecificProperty<IAIObjectAvoidProperty, &IID_IAIObjectAvoidProperty, sAIObjectAvoid *, cHashPropertyStore< cClassDataOps<sAIObjectAvoid> > > cAIObjectAvoidPropertyBase;

class cAIObjectAvoidProperty : public cAIObjectAvoidPropertyBase
{
public:
   cAIObjectAvoidProperty()
      : cAIObjectAvoidPropertyBase(&_g_ObjectAvoidPropertyDesc)
   {
      SetRebuildConcretes(TRUE);
   }

   STANDARD_DESCRIBE_TYPE(sAIObjectAvoid);

   virtual void RebuildConcrete(ObjID obj, BOOL fIsRelevant, uPropListenerValue, ObjID donor);
};

///////////////////////////////////////

void cAIObjectAvoidProperty::RebuildConcrete(ObjID obj, BOOL fIsRelevant, uPropListenerValue val, ObjID donor)
{
   sAIObjectAvoid * pObjectAvoid = (sAIObjectAvoid *) val.ptrval;
   
   BOOL fHad;
   int iPrev;
   
   for (iPrev = 0; iPrev < m_AIRepelObjects.Size(); iPrev++)
   {
      if (m_AIRepelObjects[iPrev] == obj)
         break;
   }
   
   fHad = (iPrev != m_AIRepelObjects.Size());

   if (fIsRelevant && (pObjectAvoid->flags & kAIOA_Repel))
   {
      if (!fHad)
         m_AIRepelObjects.Append(obj);
   }
   else if (fHad)
   {
      m_AIRepelObjects.FastDeleteItem(iPrev);
   }
   
   cAIObjectAvoidPropertyBase::RebuildConcrete(obj, fIsRelevant, val, donor);
}

///////////////////////////////////////

static const char * g_pszAIObjectAvoidFlags[] = 
{ 
   "Block pathfind",
   "Repel",
};
 
static sFieldDesc _g_AIObjectAvoidFieldDesc[] =
{
   { "Flags",              kFieldTypeBits,   FieldLocation(sAIObjectAvoid, flags), kFieldFlagUnsigned, 0, FieldNames(g_pszAIObjectAvoidFlags)},
};

static sStructDesc _g_AIObjectAvoidStructDesc = StructDescBuild(sAIObjectAvoid, kStructFlagNone, _g_AIObjectAvoidFieldDesc);

////////////////////////////////////////

static IAIObjectAvoidProperty * g_pAIObjectAvoidProperty;

#define _AIGetObjAvoid(obj) AIGetProperty(g_pAIObjectAvoidProperty, (obj), (sAIObjectAvoid *)NULL)

///////////////////////////////////////////////////////////////////////////////

BOOL AIInitObjectAvoid(IAIManager *)
{
   StructDescRegister(&_g_AIObjectAvoidStructDesc);
   
   g_pAIObjectAvoidProperty = new cAIObjectAvoidProperty;

   return TRUE;
}

///////////////////////////////////////

BOOL AITermObjectAvoid()
{
   SafeRelease(g_pAIObjectAvoidProperty);
   return TRUE;
}

///////////////////////////////////////

BOOL AIDBResetObjectAvoid()
{
   m_AIRepelObjects.SetSize(0);
   return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
//
// Get avoidance for one object
//

const sAIObjectAvoid * AIGetObjAvoid(ObjID obj)
{
   sAIObjectAvoid * pResult = _AIGetObjAvoid(obj);
   if (pResult)
      pResult->object = obj;
   return pResult;
}

///////////////////////////////////////
//
// Get all objects to repel from
//

void AIGetAllObjRepel(const ObjID ** ppResult, int * pCount)
{
   *pCount = m_AIRepelObjects.Size();
   *ppResult = m_AIRepelObjects;
}

///////////////////////////////////////

void AIGetAllObjMarkDB(cDynArray<sAIObjectAvoidMark> * pResult)
{
   cConcreteIter    iter(g_pAIObjectAvoidProperty);
   ObjID            object;
   sAIObjectAvoid * pObjectAvoid;
   ObjPos *         pPos;
   
   pResult->SetSize(0);
   
   iter.Start();
   while (iter.Next(&object))
   {
      if ((pPos = ObjPosGet(object)) == NULL || pPos->loc.cell == CELL_INVALID)
          continue;
   
      if (g_pAIObjectAvoidProperty->Get(object, &pObjectAvoid) && 
          (pObjectAvoid->flags & kAIOA_MarkDB))
      {
         sAIObjectAvoidMark & mark = (*pResult)[pResult->Grow()];
         mark.object = object;
         AIGetObjFloorBBox(object, &mark.vertices);
      }
   }
   iter.Stop();
}

///////////////////////////////////////////////////////////////////////////////
