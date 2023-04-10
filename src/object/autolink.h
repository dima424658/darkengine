/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/autolink.h,v 1.2 1998/09/25 23:33:57 mahk Exp $
#pragma once  
#ifndef __AUTOLINK_H
#define __AUTOLINK_H

#include <comtools.h>
#include <lnkquery.h>
#include <relation.h>
#include <linkman.h>
#include <linktype.h>
#include <appagg.h>
#include <linkbase.h>
////////////////////////////////////////////////////////////
// LINK QUERY AUTO POINTER CLASS
//


class cAutoLinkQuery : public cAutoIPtr<ILinkQuery>
{
   typedef cAutoIPtr<ILinkQuery> cParent; 
   static inline cAutoIPtr<ILinkManager> LinkMan() 
      { return AppGetObj(ILinkManager); }; 
   static inline cAutoIPtr<IRelation> RelNamed(const char* name) 
   { return LinkMan()->GetRelationNamed(name); };

public: 
   // Constructors 
   cAutoLinkQuery(ILinkQuery* q = NULL) : cParent(q) {}; 
   cAutoLinkQuery(REFIID iid, IUnknown* unk) : cParent(iid,unk) {}; 

   // Query given a rel and some objs
   cAutoLinkQuery(IRelation* pRel, ObjID src = LINKOBJ_WILDCARD, ObjID dst = LINKOBJ_WILDCARD) 
      : cParent(pRel->Query(src,dst)) 
   {
   }
   
   // Query given a relid and some objs
   cAutoLinkQuery(RelationID id, ObjID src = LINKOBJ_WILDCARD, ObjID dst = LINKOBJ_WILDCARD) 
      : cParent(LinkMan()->Query(src,dst,id))
   {
   }

   // Query given a relation name 
   cAutoLinkQuery(const char* relname, ObjID src = LINKOBJ_WILDCARD, ObjID dst = LINKOBJ_WILDCARD) 
      : cParent(RelNamed(relname)->Query(src,dst))
   {
   }

   // 
   // Convenient Link Accessors
   // 
   // Be aware that if you're using both GetSource()
   // and GetDest() that you probably want to just
   // call GetLink() once; 
   //

   inline sLink GetLink()
   {
      sLink link = { 0, 0, 0, }; 
      (*this)->Link(&link); 
      return link; 
   }

   inline ObjID GetSource()
   {
      return GetLink().source; 
   }

   inline ObjID GetDest()
   {
      return GetLink().dest; 
   }




};


#endif // __AUTOLINK_H
