/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/linkqdb_.h,v 1.4 2000/01/29 13:23:32 adurant Exp $
#pragma once

#ifndef __LINKQDB__H
#define __LINKQDB__H
#include <linkqdb.h>



class cBaseLinkQueryDatabase : public ILinkQueryDatabase
{
public:
   virtual ~cBaseLinkQueryDatabase() {};
   DECLARE_UNAGGREGATABLE();

   STDMETHOD(AddRelation)(RelationID ) { return S_OK; }
   STDMETHOD(RemoveRelation)(RelationID ) { return S_OK; }; 
   STDMETHOD_(ILinkQuery*,Query)(ObjID source, ObjID dest, RelationID rel) 
   { return QueryComplex(source,dest,rel,QUERY_DATE_NONE); }; 

};


#endif // __LINKQDB__H
