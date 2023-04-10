/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

////////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/engfeat/flinder.h,v 1.2 2000/01/29 13:19:32 adurant Exp $
#pragma once

#ifndef __FLINDER_H
#define __FLINDER_H

#include <objtype.h>
#include <matrixs.h>

EXTERN void CreateFlinders(ObjID victim, ObjID flinder, int count, BOOL scatter, float impulse, const mxs_vector &offset);

#endif
