/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/traitq.cpp,v 1.6 1998/07/16 13:15:11 mahk Exp $
#include <traitbas.h>
#include <traitma_.h>
#include <traisrch.h>
#include <donorq_.h>
#include <traitln_.h>
#include <propface.h>

// Must be last header
#include <dbmem.h>

////////////////////////////////////////////////////////////
// TRANSITIVE VERSION OF A STANDARD LINK MANAGER QUERY
//

class cClosureQuery : public cDepthFirstObjectQuery
{
   eTraitQueryType Type;
   ITraitManager* TraitMan;

protected:
   IObjectQuery* Successors(ObjID obj)
   {
      return TraitMan->Query(obj,Type);
   }

public: 
   cClosureQuery(ObjID obj, eTraitQueryType type, ITraitManager* traitman)
      : cDepthFirstObjectQuery(obj), Type(type), TraitMan(traitman)
   {
      traitman->AddRef();
   }

   ~cClosureQuery() { SafeRelease(TraitMan);};
   
};

////////////////////////////////////////////////////////////
// TRAIT MANAGER QUERIES
//

STDMETHODIMP_(IObjectQuery*) cTraitManager::Query(ObjID obj, eTraitQueryType type)
{
   if (type & kTraitQueryTransitive)
   {
      IObjectQuery* query = new cClosureQuery(obj,type & ~kTraitQueryTransitive,this);

      // Some query types skip the first object (that being the argument)
      switch (type & ~kTraitQueryTransitive)
      {
         case kTraitQueryArchetype:
         case kTraitQueryDescendents:
            query->Next();
            break;
      }
      return query;
   }


   switch (type)
   {
      case kTraitQueryDonors:
      {
         if (GetObjectMetaPropLinks(obj) == NULL)
            break; 
         ILinkQuery* links = MetaProps->Query(obj,LINKOBJ_WILDCARD);
         IDonorQuery* out = new cPriLinkDonorQuery(links);
         SafeRelease(links);
         return out;
      }
      break;

      case kTraitQueryArchetype:
      {
         ObjID arch = GetArchetype(obj); 
         return new cSingleDonorQuery(arch); 
      }

      case kTraitQueryDescendents:
      {
         IRelation* inv = MetaProps->Inverse(); 
         ILinkQuery* links = inv->Query(obj,LINKOBJ_WILDCARD);
         IObjectQuery* out = new cPriLinkDonorQuery(links);

         SafeRelease(inv);
         SafeRelease(links);
         return out;
      }
         
      default:
         Warning(("Unsupported Trait Manager Query %d\n"));
   }
   return CreateEmptyObjectQuery();
}



