/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/script/datascrs.h,v 1.4 1998/09/07 20:47:15 mahk Exp $
#pragma once  
#ifndef __DATASCRS_H
#define __DATASCRS_H

#include <scrptbas.h>

////////////////////////////////////////////////////////////
// "DATA" SCRIPT SERVICE
//
// Service for accessing various kinds of game data.
//

DECLARE_SCRIPT_SERVICE(Data,0x1a0)
{
   //
   // Fetch a string, given a string table and a string name.
   // The string table comes from the .str file in finals\strings
   // The third argument is the default string value to use if it isn't found 
   // The fourth arg is a path relative to art\finals.  
   //
   STDMETHOD_(string,GetString)(const char* table, const char* name, const char* def = "", const char* relpath = "strings") PURE; 

   //
   // Fetch an object string, using the property that corresponds to the table
   //
   STDMETHOD_(string,GetObjString)(ObjID obj, const char* table) PURE;

   //////////////////
   // RANDOM NUMBER GENERATION
   // NOTE: these all have 15 bits of actual data

   // just calls Rand() directly, ie. retuns 0 to 2^15-1
   STDMETHOD_(integer,DirectRand)(void) PURE;
   // return integer between low and high inclusive, if high<=low, returns low
   STDMETHOD_(integer,RandInt)(int low, int high) PURE;
   // returns a float from 0.0 to 1.0-(1/(1<<16)) (ie. just short of 1)
   STDMETHOD_(real,RandFlt0to1)(void) PURE;
   // returns a float from -1.0 to 1.0-(1/(1<<15)) (ie. just short of 1)
   STDMETHOD_(real,RandFltNeg1to1)(void) PURE;
}; 



#endif // __DATASCRS_H
