/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/linkqdb.h,v 1.5 2000/01/29 13:23:31 adurant Exp $
#pragma once

#ifndef __LINKQDB_H
#define __LINKQDB_H
#include <lnkquery.h>

////////////////////////////////////////////////////////////
// IQueryDatabase
//
//

#undef INTERFACE
#define INTERFACE ILinkQueryDatabase

DECLARE_INTERFACE_(ILinkQueryDatabase,IUnknown)
{
   DECLARE_UNKNOWN_PURE(); 

   //
   // Inform the query database that a new relation will be being used
   // 
   STDMETHOD(AddRelation)(THIS_ RelationID id) PURE;
   STDMETHOD(RemoveRelation)(THIS_ RelationID id) PURE;

   //
   // Add/remove a link from the database
   // 
   STDMETHOD(Add)(THIS_ LinkID id, sLink* link) PURE;
   STDMETHOD(Remove)(THIS_ LinkID id, sLink* link) PURE;

   // Reset the database
   STDMETHOD(Reset)(THIS_ RelationID) PURE;

   STDMETHOD_(ILinkQuery*,Query)(THIS_ ObjID source, ObjID desc, RelationID rel) PURE;
   STDMETHOD_(ILinkQuery*,QueryComplex)(THIS_ ObjID source, ObjID desc, RelationID rel,tQueryDate birthday) PURE;
};

////////////////////////////////////////////////////////////

//------------------------------------------------------------
// Create a database optimized explicitly for a certain set of wildcards
//
EXTERN ILinkQueryDatabase* CreateKnownSourceLinkQueryDatabase(void);
EXTERN ILinkQueryDatabase* CreateKnownDestLinkQueryDatabase(void);
EXTERN ILinkQueryDatabase* CreateBothKnownLinkQueryDatabase(void);

//
// Create a "shared" database that is used by others as well
//

EXTERN ILinkQueryDatabase* SharedKnownSourceLinkQueryDatabase(void);
EXTERN ILinkQueryDatabase* SharedKnownDestLinkQueryDatabase(void);
EXTERN ILinkQueryDatabase* SharedBothKnownLinkQueryDatabase(void);

#endif // __LINKQDB_H








