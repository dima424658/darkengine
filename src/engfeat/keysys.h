/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/engfeat/keysys.h,v 1.2 2000/01/29 13:19:44 adurant Exp $
#pragma once

#ifndef __KEYSYS_H
#define __KEYSYS_H

#include <keytype.h>
#include <comtools.h>
#include <objtype.h>

////////////////////////////////////////////////////////////
// Contain System Interface
// 
// 

F_DECLARE_INTERFACE(IKeySys); 

#undef INTERFACE
#define INTERFACE IKeySys

DECLARE_INTERFACE_(IKeySys,IUnknown)
{
   DECLARE_UNKNOWN_PURE(); 

   STDMETHOD_(BOOL, TryToUseKey)(ObjID key_object, ObjID locked_object, eKeyUse how) PURE;
};

#undef INTERFACE

EXTERN void KeySysCreate(void); 

#endif