/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/tagdbt.h,v 1.3 2000/01/31 09:48:51 adurant Exp $
#pragma once

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\
   tagdbt.h

   This is an odd one.  It has all the basic typedefs of tagdb--both
   predeclarations and aliases for basic types.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */


#ifndef _TAGDBT_H_
#define _TAGDBT_H_


#include <dynarray.h>


// the main key, like "flavor" or "distance from player"
typedef uint tTagDBKeyType;

// an enum value, like "chocolate" or "pretty close by": when we
// make a query we're asking for keys ST the key types are equal
// and the key values are between the mins and maxes
typedef int tTagDBKeyValue;

typedef uchar tTagDBKeyEnum;
// If we have fewer enums to match against than we have room for,
// this value terminates the list.
#define kTagDBKeyEnumUnused 255

// position within a key set
typedef uint tTagDBKeySetIter;


// Each of these is in a different file.
class ITagDBDatabase;   // tagdbdat
class ITagDBKeySet;     // tagdbkst
class cTagDBKey;        // tagdbkey
class cTagDBInput;      // tagdbin
class cTagDBOutput;     // tagdbout

// This isn't worth its own header, so we'll drop it here for now.
struct sTagDBData
{
   int m_iData;
   float m_fWeight;

   sTagDBData(int iData, float fWeight)
      : m_iData(iData), m_fWeight(fWeight)
   {
   }

   sTagDBData()
   {
   }
};

#endif // ~_TAGDBT_H_
