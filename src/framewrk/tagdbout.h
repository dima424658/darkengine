/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/tagdbout.h,v 1.4 2000/01/31 09:48:50 adurant Exp $
#pragma once

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\
   tagdbres.h

   A cTagDBOutput is the container for the result of a query on
   a tag database.  While it's an extension of a cDynArray, in
   practice we only expect to create it, clear it, read from it
   using Size() and the [] operator, and choose results from it.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */


#ifndef _TAGDBOUT_H_
#define _TAGDBOUT_H_


#include <lgassert.h>
#include <dynarray.h>
#include <tagdbt.h>


class cTagDBOutput : public cDynArray<sTagDBData>
{
public:
   void Clear()
   {
      SetSize(0);
   }

   // pick a datum, randomly, using the weight in the sTagDBData
   int Choose();

   // This incremental selection sort finds the element with the highest
   // weighting for the array, from iIndex to the end, and swaps that
   // element with the one at iIndex.  The client must keep track of the
   // outer loop of the sort; that is, the value of iIndex.
   void NextByWeight(int iIndex);
};

#endif // ~_TAGDBOUT_H_
