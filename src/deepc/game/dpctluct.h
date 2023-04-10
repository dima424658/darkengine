/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef __DPCTLUCT_H
#define __DPCTLUCT_H

EXTERN void DPCTlucTextDraw(unsigned long inDeltaTicks);
EXTERN void DPCTlucTextInit(int which);
EXTERN void DPCTlucTextTerm(void);

EXTERN void DPCTlucTextAdd(char *name, char *table, int offset);

#endif  // !__DPCTLUCT_H