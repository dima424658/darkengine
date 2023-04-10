/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/ai/aistnapi.h,v 1.3 1999/02/25 18:37:17 JON Exp $
// 

#pragma once

#ifndef __AISTNAPI_H
#define __AISTNAPI_H

#include <objtype.h>

EXTERN BOOL DoAISetStun(ObjID obj, char *begin, char *loop, int ms);
EXTERN BOOL AIGetStun(ObjID obj); 

EXTERN BOOL DoAIUnsetStun(ObjID obj);

#endif  // __AISTNAPI_H   
