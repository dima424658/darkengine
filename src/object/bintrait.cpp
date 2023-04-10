/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/bintrait.cpp,v 1.3 1998/10/05 17:21:41 mahk Exp $
#include <objtype.h>
#include <appagg.h>
#include <bintrait.h>

#include <linkbase.h>
#include <relation.h>
#include <lnkquery.h>
#include <osetlnkq.h>

#include <trait.h>
#include <traitman.h>
#include <traitbas.h>

#include <iobjsys.h>
#include <objdef.h>

// must be last header
#include <dbmem.h>

// Query factory for donors 

class cDestDonorQueryFactory : public cLinkQueryFactory
{
   ITraitManager* m_pTraitMan; 
   ObjID m_dest; 
   IRelation* m_pRel;

public:
   cDestDonorQueryFactory(IRelation* pRel, ObjID dest, ITraitManager* pTM)
      : m_pRel(pRel),
        m_dest(dest),
        m_pTraitMan(pTM)
   {
      m_pRel->AddRef();
      m_pTraitMan->AddRef(); 
   }

   ~cDestDonorQueryFactory() 
   { 
      SafeRelease(m_pRel);
      SafeRelease(m_pTraitMan);
   };

   ILinkQuery* Query(ObjID obj) 
   { 
      cLinkQueryFactory* pFact = CreateDestSetQueryFactory(m_pRel,obj);
      cAutoIPtr<IObjectQuery> ObjSet (m_pTraitMan->Query(m_dest, kTraitQueryAllDonors));
      
      return CreateObjSetLinkQuery(ObjSet, pFact);
   }; 
   
};


static BOOL LnkPred(ObjID obj, TraitPredicateData data)
{
   IRelation* pRel = (IRelation*)data; 
   return pRel->AnyLinks(obj,LINKOBJ_WILDCARD); 
}


ITrait* MakeTraitFromRelation(IRelation* pRel)
{
   AutoAppIPtr_(TraitManager,pTraitMan); 
   sTraitDesc desc = { "", 0 };
   strcpy(desc.name,pRel->Describe()->name); 
   sTraitPredicate pred = { LnkPred, pRel }; 
   return pTraitMan->CreateTrait(&desc,&pred); 
}

ILinkQuery* QueryInheritedLinks(ITrait* pT, IRelation* pR, ObjID src, ObjID dest)
{
   AutoAppIPtr(TraitManager);
   cLinkQueryFactory* factory = new cDestDonorQueryFactory(pR, dest, pTraitManager); 
   IObjectQuery* donors = pT->GetAllDonors(src); 
   ILinkQuery* query = CreateObjSetLinkQuery(donors,factory);
   SafeRelease(donors); 
   return query; 
}

ILinkQuery* QueryInheritedLinksSingle(ITrait* pT, IRelation* pR, ObjID src, ObjID dest)
{
   AutoAppIPtr(TraitManager);
   cLinkQueryFactory* factory = CreateSourceSetQueryFactory(pR,dest);
   IObjectQuery* donors = pT->GetAllDonors(src); 
   ILinkQuery* query = CreateObjSetLinkQuery(donors,factory);
   SafeRelease(donors); 
   return query; 
}



ILinkQuery* QueryInheritedLinksUncached(IRelation* pR, ObjID src, ObjID dest)
{
   AutoAppIPtr(TraitManager);
   cLinkQueryFactory* factory = new cDestDonorQueryFactory(pR, dest, pTraitManager); 
   IObjectQuery* donors = pTraitManager->Query(src, kTraitQueryAllDonors); 
   ILinkQuery* query = CreateObjSetLinkQuery(donors,factory);
   SafeRelease(donors); 
   return query; 
}

ILinkQuery* QueryInheritedLinksSingleUncached(IRelation* pR, ObjID src, ObjID dest)
{
   AutoAppIPtr(TraitManager);
   cLinkQueryFactory* factory = CreateSourceSetQueryFactory(pR,dest);
   IObjectQuery* donors = pTraitManager->Query(src, kTraitQueryAllDonors); 
   ILinkQuery* query = CreateObjSetLinkQuery(donors,factory);
   SafeRelease(donors); 
   return query; 
}


