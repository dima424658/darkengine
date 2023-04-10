/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef __DPCCAM_H
#define __DPCCAM_H

#ifndef __MATRIXS_H
#include <matrixs.h>
#endif // !__MATRIXS_H

#ifndef _OBJTYPE_H
#include <objtype.h>
#endif // !_OBJTYPE_H

#ifndef __OSYSTYPE_H
#include <osystype.h>
#endif // !__OSYSTYPE_H

void PlayerCameraSetLocation(const mxs_vector *pPos, const mxs_angvec *pAng);
BOOL PlayerCameraAttach(ObjID objID);
BOOL PlayerCameraAttach(const Label *pName);
BOOL PlayerCameraAttach(const char *pName);

void ZoomTarget(float target, float rate);
void ZoomIntegrate(void);

#endif  // !__DPCCAM_H
