/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/plyscrpt.h,v 1.1 1998/05/13 10:38:39 kate Exp $
#pragma once  
#ifndef __PLYSCRPT_H
#define __PLYSCRPT_H

#include <objscrpt.h>

///////////////////////////////////////
//
// Player item service 
//

DECLARE_SCRIPT_SERVICE(PlayerLimbs, 0x15d)
{
   // Equip the specified item
   STDMETHOD(Equip)(object item) PURE;
   // Put away the specified item
   STDMETHOD(UnEquip)(object item) PURE;
   // Start using (frob start)
   STDMETHOD(StartUse)(object item) PURE;
   // Finish using (frob end)
   STDMETHOD(FinishUse)(object item) PURE;
};

#endif  // __PLYSCRPT_H





