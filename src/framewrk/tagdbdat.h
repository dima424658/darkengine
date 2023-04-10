/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/tagdbdat.h,v 1.6 2000/01/31 09:48:46 adurant Exp $
#pragma once

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\
   tagdbdat.h

   A tag database maintains a simple search space.  This is the ABC.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */


#ifndef _TAGDBDAT_H_
#define _TAGDBDAT_H_


#include <osystype.h>
#include <tagdbt.h>
#include <tagfile.h>


class ITagDBDatabase
{
public:
   virtual ~ITagDBDatabase() {}

   // associate a piece of data with an unordered set of keys
   virtual void Insert(ITagDBKeySet *pSet, const sTagDBData Data) = 0;

   // find all data which matches any subset of the input set, but
   // have no keys outside the input set--FALSE means none found
   virtual BOOL MatchSubset(cTagDBInput *pInput,
                            cTagDBOutput *pOutput) = 0;

   // find all data which exactly matches the input set--FALSE if none
   // found
   virtual BOOL MatchSet(cTagDBInput *Input,
                         cTagDBOutput *Output) = 0;

   // the in-between case: the client specifies which members of the
   // input set are required
   virtual BOOL MatchPartialSet(cTagDBInput *Input,
                                cTagDBOutput *Output) = 0;

   // This applies the given function to all data in the tree.
   // Swizzle, swizzle, swizzle.
   virtual void TransformData(void Func(sTagDBData *)) = 0;

   // This applies the given function to all cTagDBKeys.
   virtual void TransformKeys(void Func(cTagDBKey *)) = 0;

   // Since our searches depend on canonical orderings, the only way
   // we can swizzle a database from one set of tokenizations to
   // another is to create a new tree from scratch.
   virtual ITagDBDatabase *CopyWithFilter(void Func(cTagDBKey *)) = 0;

   virtual void Load(ITagFile *pFile) = 0;
   virtual void Save(ITagFile *pFile) = 0;

   // Our clear function destroys all of the rest of the tree, but leaves
   // the top node intact, if empty.
   virtual void Clear() = 0;

#ifndef SHIP
   // mono spew
   virtual void Dump() = 0;
#endif // ~SHIP
};


// factory function
ITagDBDatabase *NewITagDBDatabase();

#endif // ~_TAGDBDAT_H_
