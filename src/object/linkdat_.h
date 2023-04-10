/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/linkdat_.h,v 1.3 2000/01/29 13:23:16 adurant Exp $
#pragma once

#ifndef __LINKDAT__H
#define __LINKDAT__H
#include <linksto_.h>
#include <lnkdtabl.h>
#include <linkdb_.h>

//
// The standard data stores 
//


class cLargeLinkDataStore : public cBaseLinkDataStore
{
   ulong Size; 
   LinkDataTable& Table;

public:
   cLargeLinkDataStore (ulong sz, LinkDataTable* t = NULL) 
      : Size(sz),Table((t==NULL) ? LinkDatabases::DBs()->DataTable() : *t) {};
   ~cLargeLinkDataStore() {}; 

   STDMETHOD(Remove)(LinkID id);
   STDMETHOD(Set)(LinkID id, void* data);
   STDMETHOD_(void*,Get)(LinkID id);
   STDMETHOD_(ulong,DataSize)(void);
};
#endif // __LINKDAT__H
