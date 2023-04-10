/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/script/pupscrpt.h,v 1.1 1998/01/05 16:12:31 kate Exp $
#pragma once  
#ifndef __PUPSCRPT_H
#define __PUPSCRPT_H

#include <scrptmsg.h>
#include <objtype.h>
#include <objscrt.h>

///////////////////////////////////////
//
// Puppet Services
//

DECLARE_SCRIPT_SERVICE(Puppet, 0xfd)
{
   // Play motion with name "name"
   STDMETHOD_(boolean,PlayMotion)(const object obj, const char *name) PURE;
};

#endif // __PUPSCRPT_H

