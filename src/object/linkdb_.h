/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/linkdb_.h,v 1.4 2000/01/29 13:23:17 adurant Exp $
#pragma once

#ifndef __LINKDB__H
#define __LINKDB__H
#include <linktype.h>

////////////////////////////////////////////////////////////
// LINK DATABASES
// 
// This is a repository for global databases for use by built-in
// relations and such. 
//
// It is an abstract base class so that header depends might be sane. 
//

class LinkDataTable;

class LinkDatabases
{
public:
   static LinkDatabases* DBs();
   static void SetDBs(LinkDatabases* );

   virtual LinkDataTable& DataTable() = 0;
   virtual ILinkQueryDatabase* ByObjQDB() = 0;
   virtual ILinkQueryDatabase* ToObjQDB() = 0;
   virtual ILinkQueryDatabase* BothObjQDB() = 0;
  
   virtual ~LinkDatabases() {};
};

EXTERN LinkDatabases* MakeLinkDatabases(void);


#endif // __LINKDB__H
