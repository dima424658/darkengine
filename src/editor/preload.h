/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/preload.h,v 1.2 2000/01/29 13:12:56 adurant Exp $
// stupid preloading system
#pragma once

#ifndef __PRELOAD_H
#define __PRELOAD_H

EXTERN BOOL preload_data(int type_code, char *name, char *aux);
EXTERN void preload_free_all(void);

// type codes
#define PRELOAD_SOUND 0x01
#define PRELOAD_BIN   0x02

#endif  // __PRELOAD_H
