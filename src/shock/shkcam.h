/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkcam.h,v 1.2 2000/01/29 13:40:32 adurant Exp $
#pragma once

#ifndef __SHKCAM_H
#define __SHKCAM_H

#include <matrixs.h>
#include <objtype.h>
#include <osystype.h>

void PlayerCameraSetLocation(const mxs_vector *pPos, const mxs_angvec *pAng);
BOOL PlayerCameraAttach(ObjID objID);
BOOL PlayerCameraAttach(const Label *pName);
BOOL PlayerCameraAttach(const char *pName);

#endif