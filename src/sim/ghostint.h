/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/ghostint.h,v 1.2 2000/01/29 13:41:08 adurant Exp $
#pragma once

#ifndef __GHOSTINT_H
#define __GHOSTINT_H

#include <hashset.h>
#include <ghosttyp.h>

//////////////////
// class declarations for the actual hashes we will us

// we want 2 hashes, one for local ghosts we control, one for remote ghosts
// the current model is to simply delete from one and recreate in the other
// rather than some sneaky shared hash thing

// Local Models
typedef cHashSet<sGhostLocal *, ObjID, cHashFunctions> cGhostLocalTableBase;

class cGhostLocalHash : public cGhostLocalTableBase
{
   virtual tHashSetKey GetKey(tHashSetNode node) const
   {
      return (tHashSetKey)(((sGhostLocal *)(node))->obj);
   }
};

// Remote Models
typedef cHashSet<sGhostRemote *, ObjID, cHashFunctions> cGhostRemoteTableBase;

class cGhostRemoteHash : public cGhostRemoteTableBase
{
   virtual tHashSetKey GetKey(tHashSetNode node) const
   {
      return (tHashSetKey)(((sGhostRemote *)(node))->obj);
   }
};

#endif  // __GHOSTINT_H
