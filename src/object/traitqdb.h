/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/traitqdb.h,v 1.3 2000/02/24 23:41:11 mahk Exp $
#pragma once

#ifndef __TRAITQDB_H
#define __TRAITQDB_H

#include <traittyp.h>
#include <comtools.h>
#include <traitln_.h>

#include <linktype.h>
#include <linkqdb.h>
#include <ObjArray.h>

//////////////////////////////////////////////////////////// 
// CUSTOM LINK QUERY DATABASE FOR "METAPROP" LINKS
// 
// The point here is to make hierarchy searches fast by giving 
// the trait manager a fast, "back door" access to outgoing metaprop links. 
// 
// Our database is implemented as a big array mapping ObjIDs into lists of links.
// Our link list has ALL of the link data (id, dest obj, priority),
// not just the link id.   
//

//------------------------------------------------------------
// Our special "list of metaprop links" 
//


class cMetaPropLinks: public MetaPropLinks
{

public: 
   void AddLink(const sMetaPropLink& link); 
   void RemoveLink(const sMetaPropLink& link); 

}; 

//------------------------------------------------------------
// The Query Database 
//

class cMetaPropQDB: public cCTUnaggregated<ILinkQueryDatabase,&IID_ILinkQueryDatabase,kCTU_Default>
{
protected: 
   cObjArray<cMetaPropLinks*> mObjArray; 
   IRelation* mpRel; 

public:
   cMetaPropQDB(); 
   virtual ~cMetaPropQDB(); 

   STDMETHOD(AddRelation)(RelationID id) ;
   STDMETHOD(RemoveRelation)(RelationID id) ;

   STDMETHOD(Add)(LinkID id, sLink* link);
   STDMETHOD(Remove)(LinkID id, sLink* link);
   STDMETHOD(Reset)(RelationID);
   STDMETHOD_(ILinkQuery*,Query)(ObjID source, ObjID desc, RelationID rel);
   STDMETHOD_(ILinkQuery*,QueryComplex)(ObjID source, ObjID desc, RelationID rel,tQueryDate birthday);

   cMetaPropLinks* Get(ObjID obj)
   {
      return mObjArray[obj]; 
   }
   
}; 


#endif // __TRAITQDB_H



