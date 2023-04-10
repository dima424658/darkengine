/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef __SUSPROP_H
#define __SUSPROP_H

#include <objtype.h>
#include <property.h>
#include <prcniter.h>

//
// Suspicious Info Properties
// 

#define PROP_SUSPICIOUS_NAME "SuspObj"

/////////////////////////////////////////////////////////////////////////////

BOOL InitSuspiciousProp();
BOOL TermSuspiciousProp();


BOOL IsSuspicious(ObjID object);
BOOL GetSuspiciousType(ObjID object, char *sustype, int typelen);
BOOL GetSuspiciousLightLevel(ObjID object, float *lightlevel);

void StartSuspiciousIter(sPropertyObjIter *iter);
BOOL NextSuspiciousIter(sPropertyObjIter *iter, ObjID *obj, BOOL *setting);
void StopSuspiciousIter(sPropertyObjIter *iter);

#endif // __SUSPROP_H
