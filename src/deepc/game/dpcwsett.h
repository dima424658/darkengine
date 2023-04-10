/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef __DPCWSETT_H
#define __DPCWSETT_H

extern "C"
{
#include <event.h>
}

#ifndef _OBJTYPE_H
#include <objtype.h>
#endif // !_OBJTYPE_H

EXTERN void DPCSettingInit(int which);
EXTERN void DPCSettingTerm(void);
EXTERN void DPCSettingDraw(unsigned long inDeltaTicks);
EXTERN bool DPCSettingHandleMouse(Point pt);
EXTERN void DPCSettingStateChange(int which);

#endif  // !__DPCWSETT_H