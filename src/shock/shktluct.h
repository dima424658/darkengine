/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shktluct.h,v 1.2 2000/01/31 09:59:24 adurant Exp $
#pragma once

#ifndef __SHKTLUCT_H
#define __SHKTLUCT_H

EXTERN void ShockTlucTextDraw(void);
EXTERN void ShockTlucTextInit(int which);
EXTERN void ShockTlucTextTerm(void);

EXTERN void ShockTlucTextAdd(char *name, char *table, int offset);

#endif