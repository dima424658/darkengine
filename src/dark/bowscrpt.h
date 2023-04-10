/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/bowscrpt.h,v 1.2 1998/10/22 00:10:51 dc Exp $
#pragma once  
#ifndef __BOWSCRPT_H
#define __BOWSCRPT_H

#include <objscrpt.h>

///////////////////////////////////////
//
// Weapon system service 
//

DECLARE_SCRIPT_SERVICE(Bow, 0x115)
{
   // Equip the specified weapon
   STDMETHOD(Equip)() PURE;
   // Put away the specified weapon
   STDMETHOD(UnEquip)() PURE;

   // Has the specified weapon finished being equipped?
   STDMETHOD_(BOOL, IsEquipped)() PURE;

   // Start an attack (frob start)
   STDMETHOD(StartAttack)() PURE;
   // Finish an attack (frob end)
   STDMETHOD(FinishAttack)() PURE;
   // Abort an attack (frob abort)
   STDMETHOD(AbortAttack)() PURE;

   // Set the arrow type
   STDMETHOD_(BOOL, SetArrow)(object arrow) PURE;
};

#endif  // __BOWSCRPT_H





