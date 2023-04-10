/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkqbind.h,v 1.2 2000/01/31 09:58:55 adurant Exp $
#pragma once

#ifndef __SHKQBIND_H
#define __SHKQBIND_H

#define MAX_BIND  12

typedef enum ePsiPowers;

EXTERN int ShockBindVal(ePsiPowers power);
EXTERN void ShockQuickbindInit(void);

#endif