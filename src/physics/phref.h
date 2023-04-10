/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

////////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/physics/phref.h,v 1.9 2000/01/31 09:52:06 adurant Exp $
//
// Physics ref system header
//
#pragma once

#ifndef __PHREF_H
#define __PHREF_H

#include <refsys.h>
#include <objtype.h>

#ifdef __cplusplus
class cPhysModel;
#else
typedef int cPhysModel;
#endif


typedef ObjRefID tPhysRef;

EXTERN int  PhysRefSystemInit();
EXTERN void PhysRefSystemClear(); 
EXTERN void PhysRefSystemResize();
EXTERN void PhysRefSystemRebuild(); 
EXTERN void PhysRefSystemTerm();

EXTERN int PhysUpdateRefs(cPhysModel *pModel);
EXTERN int PhysUpdateRefsObj(ObjID objID);

EXTERN int PhysRefSystemID;

#endif // __PHREF_H
