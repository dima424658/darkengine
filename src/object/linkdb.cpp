/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/linkdb.cpp,v 1.6 1997/11/06 18:03:52 mahk Exp $
#include <linkdb_.h>
#include <lnkdtabl.h>
#include <lnkbyobj.h>
#include <linkboth.h>
#include <hshsttem.h>

// Must be last header
#include <dbmem.h>

////////////////////////////////////////////////////////////
// LINK DATABASE ACCESSORS


static LinkDatabases* TheDatabases = NULL;


LinkDatabases* LinkDatabases::DBs()
{
   return TheDatabases;
}

void LinkDatabases::SetDBs(LinkDatabases* dbs)
{
   TheDatabases = dbs;
}

////////////////////////////////////////////////////////////
//
// LinkDatabases instantiation
//

#define PAD(x,y)  char pad_##y[sizeof(int)-sizeof(x)%sizeof(int)]

class RealLinkDatabases : public LinkDatabases
{
   LinksByObj byObj;                      PAD(LinksByObj,byObj); 
   LinksByObj toObj;                      PAD(LinksByObj,toObj); 
   cBothObjTable bothObj;                 PAD(cBothObjTable,bothObj);
   LinkDataTable dataTable;               PAD(LinkDataTable,dataTable); 
   cFromObjQueryDatabase* byObjQDB;
   cToObjQueryDatabase* toObjQDB;
   cBothKnownQueryDatabase* bothObjQDB; 

public:
   RealLinkDatabases() 
      : byObjQDB(new cFromObjQueryDatabase(&byObj)),
        toObjQDB(new cToObjQueryDatabase(&toObj)),
        bothObjQDB(new cBothKnownQueryDatabase(&bothObj))
   {}; 
   ~RealLinkDatabases() { delete byObjQDB; delete toObjQDB; delete bothObjQDB;};

   LinkDataTable& DataTable() { return dataTable;};
   ILinkQueryDatabase* ByObjQDB() { byObjQDB->AddRef(); return byObjQDB;};
   ILinkQueryDatabase* ToObjQDB() { toObjQDB->AddRef(); return toObjQDB;};
   ILinkQueryDatabase* BothObjQDB() { bothObjQDB->AddRef(); return bothObjQDB;};
};


LinkDatabases* MakeLinkDatabases(void)
{
   return new RealLinkDatabases();
}

////////////////////////////////////////////////////////////

ILinkQueryDatabase* CreateKnownSourceLinkQueryDatabase(void)
{
   return new cFromObjQueryDatabase;
}

ILinkQueryDatabase* CreateKnownDestLinkQueryDatabase(void)
{
   return new cToObjQueryDatabase; 
}

ILinkQueryDatabase* SharedKnownSourceLinkQueryDatabase(void)
{
   return LinkDatabases::DBs()->ByObjQDB();
}

ILinkQueryDatabase* SharedKnownDestLinkQueryDatabase(void)
{
   return LinkDatabases::DBs()->ToObjQDB();
}

ILinkQueryDatabase* SharedBothKnownLinkQueryDatabase(void)
{
   return LinkDatabases::DBs()->BothObjQDB(); 
}






