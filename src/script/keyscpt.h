/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/script/keyscpt.h,v 1.5 1998/06/25 22:51:05 XEMU Exp $
#pragma once  
#ifndef __KEYSCPT_H
#define __KEYSCPT_H

#include <objscrpt.h>
#include <keytype.h>

///////////////////////////////////////
//
// Property system service 
//

DECLARE_SCRIPT_SERVICE(Key, 0x10d)
{
   // try to unlock the lock with the key
   STDMETHOD_(BOOL, TryToUseKey)(const object &key_obj, const object &lock_obj, eKeyUse how) PURE;
};

#endif  // __KEYSCPT_H
