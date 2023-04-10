/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkver.h,v 1.2 2000/01/31 09:59:33 adurant Exp $
#pragma once

#ifndef __SHKVER_H
#define __SHKVER_H

#include <resapi.h>

EXTERN void ShockVersionInit(int which);
EXTERN void ShockVersionTerm(void);
EXTERN IRes *ShockVersionBitmap(void);

#endif