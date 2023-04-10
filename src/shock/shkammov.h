/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkammov.h,v 1.7 2000/01/29 13:40:25 adurant Exp $
#pragma once

#ifndef __SHKAMMO_H
#define __SHKAMMO_H

extern "C" 
{
#include <event.h>
}

EXTERN void ShockAmmoDraw(void);
EXTERN void ShockAmmoInit(int which);
EXTERN void ShockAmmoTerm(void);
EXTERN bool ShockAmmoHandleMouse(Point pt);
EXTERN bool ShockAmmoCheckTransp(Point pt);
EXTERN void ShockAmmoRefreshButtons(void);
EXTERN void ShockAmmoStateChange(int which);

EXTERN void ShockAmmoReload(void);
EXTERN void ShockAmmoChangeTypes(void);
EXTERN void ShockAmmoPsiLevel(int button);

#endif