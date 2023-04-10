/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkblood.h,v 1.2 2000/01/29 13:40:30 adurant Exp $
#pragma once

#ifndef __SHKBLOOD_H
#define __SHKBLOOD_H

#include <dmgmodel.h>

extern void ShockBloodInit(void);
extern void ShockBloodTerm(void);
extern void ShockReleaseBlood(const sDamageMsg* msg);

#endif // __SHKBLOOD_H
