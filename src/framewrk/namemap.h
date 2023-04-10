/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/namemap.h,v 1.6 2000/01/29 13:21:21 adurant Exp $
#pragma once

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\
   namemap.h

   So this here's a weird little setup for two-way mappings between
   Labels and integers.  We can generate the integers ourselves, which
   lets them come out contiguous and starting from 0.  Or we can force
   a given Label to map to a given integer, provided there are no
   collisions.  Once we've got negative integers, the indices we
   expose the outside world are different from the physical positions
   in our internal array.  In this module we refer to the exposed
   indices as "apparent," and the internal, physical indices as
   "real."

   The actual storage is always contiguous, from the lowest integer
   we're using to the highest.  So this would be a bad structure for
   sparse mappings.

   Each Label is unique.  If someone tries to add one we've already
   got, our debug build will assert and hate them forever.

   Each index is also unique.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */


#ifndef _NAMEMAP_H_
#define _NAMEMAP_H_


#include <lgassert.h>
#include <mprintf.h>

#include <dynarray.h>
#include <hashset.h>
#include <hshsttem.h>
#include <label.h>
#include <tagfile.h>
#include <vernum.h>


// This is what you get if you ask for a handle for a name which is not
// in your cNameMap.
#define kNameMapIndexNotFound -10000000
#define kNameMapNameNotFound 0

class cNameMapEntry
{
public:
   Label m_Name;
   // this is used as a handle when we map from strings to ints (this
   // is the real position, not the apparent one).
   int m_iApparentIndex;

   cNameMapEntry();
   cNameMapEntry(const Label *pName);
};


class cNameMapHash : public cStrIHashSet<cNameMapEntry *>
{
public:
   tHashSetKey GetKey(tHashSetNode p) const;
};


class cNameMap
{
public:
   cNameMap();
   ~cNameMap();

   // The name is _copied_ in.  The return value is the ID.
   // There is no way to remove elements.
   int AddName(const Label *pName);

   // The index may be positive or negative; the mapping will be
   // adjusted internally if any indices fall below 0.
   void AddNameAtIndex(const Label *pName, int iIndex);

   Label *NameFromID(int id);
   int IDFromName(const Label *pName);

   void Load(ITagFile *pFile);
   void Save(ITagFile *pFile);
   void Clear();

   int Size()
   {
      return m_IntToName.Size();
   }

   // Both of these are apparent values, inclusive.
   int m_iUpperBound;
   int m_iLowerBound;

protected:
   // The mapping from ints to strings is an array, with a layer of
   // indirection so we can move the array without screwing up our
   // hash table.  We always generate this part first, since it gives
   // us a place to stash the actual string, which the hash table
   // only has a pointer to.
   cDynArray<cNameMapEntry *> m_IntToName;

   // The mapping from strings to ints is a hash table.
   cNameMapHash m_NameToInt;

   int RealIndex(int iApparentIndex)
   {
      return iApparentIndex - m_iLowerBound;
   }

   int ApparentIndex(int iRealIndex)
   {
      return iRealIndex + m_iLowerBound;
   }

   // This keeps the bounds and array size reasonable.
   void UpdateBounds(int iNewIndex);

   // This hashes the given entry and sets its iArrayIndex field.
   void CrossIndex(int iRealIndex);
};


#endif // ~_NAMEMAP_H_
