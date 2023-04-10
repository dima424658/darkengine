/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/tagdbkst.h,v 1.3 2000/01/31 09:48:49 adurant Exp $
#pragma once

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\
   tagdbkst.h

   An ITagDBKeySet is a container for cTagDBKeys, and is used both for
   adding data to a tag database and generating queries from it.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */


#ifndef _TAGDBKST_H_
#define _TAGDBKST_H_

#include <tagfile.h>

#include <tagdbt.h>
#include <tagdbkey.h>


class ITagDBKeySet
{
public:
   virtual ~ITagDBKeySet() {}

   // The object is clear upon initialization.  This is in case you
   // want to clear it again to use it for another query.
   virtual void Clear() = 0;

   virtual int Size() = 0;

   // This does not maintain the sort order, so it invalidates the
   // internal flag exposed by IsSorted().  Keys should be unique.  In
   // debug bulds, expect an assertion if you try to use the same key
   // twice.
   virtual void AddKey(cTagDBKey *pKey) = 0;

   // If necessary, you can use this to keep you from entering
   // duplicate keys.  But please, try to find some more efficient
   // way.
   virtual BOOL HasKey(cTagDBKey *pKey) = 0;

   // Once all the keys have been inserted, this sets us up for
   // actually using the set for a query or whatever.
   virtual void Sort() = 0;

   virtual BOOL IsSorted() = 0;

   // Iteration involves a little iteration thingbob.  The client is
   // expected to call this once for a given iteration, then use
   // IterNext in a while loop.  The set should be sorted before it
   // gets iterated over; we check for this in debug builds.  Adding
   // keys or clearing the database can screw up the iteration.
   virtual tTagDBKeySetIter IterStart() = 0;

   // If there is no next element we return FALSE.
   virtual BOOL IterNext(tTagDBKeySetIter *Iter, cTagDBKey *pKey) = 0;
};


// factory function
ITagDBKeySet *NewITagDBKeySet();


#endif // ~_TAGDBKST_H_
