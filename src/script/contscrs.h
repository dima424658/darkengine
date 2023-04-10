/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/script/contscrs.h,v 1.4 1998/12/14 22:02:19 XEMU Exp $
#pragma once  
#ifndef __CONTSCRS_H
#define __CONTSCRS_H

#include <objscrpt.h>
#include <contain.h>

DECLARE_SCRIPT_SERVICE(Container,0x17d)
{
   //
   // Add an object to a container
   //
   STDMETHOD(Add)(object obj, object container, int type = 0, int flags = CTF_COMBINE) PURE; 

   //
   // Remove an object from a container, null container implies whatever container 
   // it happens to be in 
   //
   STDMETHOD(Remove)(object obj, object container = 0) PURE; 

   //
   // Move all contents of a container
   // 
   STDMETHOD(MoveAllContents)(object src, object targ, int flags = CTF_COMBINE) PURE; 

   // Add to the stack count of an object
   STDMETHOD(StackAdd)(object src, int quantity) PURE;

   // Is the object in question contained?
   STDMETHOD_(eContainType,IsHeld)(object container, object containee) PURE;
};

#endif // __CONTSCRS_H





