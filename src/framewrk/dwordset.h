/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/dwordset.h,v 1.2 2000/01/29 13:20:46 adurant Exp $
//
// DWORDSet.h
//
// Defines a simple little concept of a "set" of DWORDs. Doesn't track
// anything except existence of each. Currently a tad inefficient; this
// should be improved and templatized when we get a spare minute. We can
// probably remove the spurious BOOLs by using cHashSet as the
// base instead of cHashTable, but that's more work than I have time
// for at the moment...
//
#pragma once

#ifndef __DWORDSET_H
#define __DWORDSET_H

#include <hashpp.h>
#include <hshpptem.h>

class cDWORDSet {
public:
   cDWORDSet() {}
   ~cDWORDSet() {}

   void Add(DWORD val) { Set.Set(val, TRUE); }
   BOOL InSet(DWORD val) { BOOL b; return Set.Lookup(val, &b); }
   void Delete(DWORD val) { Set.Delete(val); }
   int Size() { return Set.nElems(); }

private:
   typedef cScalarHashFunctions<DWORD> cSetHashFns;
   typedef cHashTable<DWORD, BOOL, cSetHashFns> cSetTable;
   cSetTable Set;
};

#endif // !__DWORDSET_H
