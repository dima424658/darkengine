/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef __DPCVER_H
#define __DPCVER_H

#ifndef __RESAPI_H
#include <resapi.h>
#endif // !__RESAPI_H

EXTERN void  DPCVersionInit(int which);
EXTERN void  DPCVersionTerm(void);
EXTERN IRes *DPCVersionBitmap(void);

#endif  // ! __DPCVER_H