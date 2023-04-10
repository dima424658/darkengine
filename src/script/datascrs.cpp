/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/script/datascrs.cpp,v 1.7 1998/10/05 17:22:53 mahk Exp $

#include <scrptapi.h>
#include <scrptbas.h>
#include <scrptsrv.h>

#include <datascrs.h>
#include <appagg.h>
#include <resapi.h>
#include <respaths.h>
#include <resistr.h>
#include <gamestr.h>

#include <rand.h>

// must be last header
#include <dbmem.h>


DECLARE_SCRIPT_SERVICE_IMPL(cDataSrv,Data)
{
   STDMETHOD_(string,GetString)(const char* table, const char* name, const char* def_val, const char* relpath)
   {
      AutoAppIPtr(GameStrings); 
      return (const char*)pGameStrings->FetchString(table,name,def_val,relpath); 
   }

   STDMETHOD_(string,GetObjString)(ObjID obj, const char* table)
   {
      AutoAppIPtr(GameStrings); 
      return (const char*)pGameStrings->FetchObjString(obj,table); 
   }

   // returns an int between low and high (inclusive)
   STDMETHOD_(integer,RandInt)(int low, int high)
   {
      if (low>=high) return low;
      int range=high-low+1;
      return low+(Rand()%range);
   }

   // returns an int between 0 and 2^15-1 - ie. just calls Rand
   STDMETHOD_(integer,DirectRand)(void)
   {
      return Rand();
   }

   // returns random 0-1 float
   STDMETHOD_(real,RandFlt0to1)(void)
   {
      float tmp=(float)((Rand()&0x7fff));
      tmp/=(1<<15);
      return tmp;
   }

   STDMETHOD_(real,RandFltNeg1to1)(void)
   {
      float tmp=(float)((Rand()&0x7fff)-(1<<14));
      tmp/=(1<<14);
      return tmp;
   }
}; 

IMPLEMENT_SCRIPT_SERVICE_IMPL(cDataSrv,Data); 



