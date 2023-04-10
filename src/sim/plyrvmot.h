/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/plyrvmot.h,v 1.4 2000/01/31 10:00:28 adurant Exp $
#pragma once

#ifndef __PLYRVMOT_H
#define __PLYRVMOT_H

#include <matrixs.h>

// registers player virtual motions
EXTERN void PlyrVMotRegisterMotions();

EXTERN int PlyrVMotNumMotions();

EXTERN void PlyrVMotSetOffsets(mxs_vector *pPos, mxs_angvec *pAng);
EXTERN void PlyrVMotSetStartOffsets(mxs_vector *pPos, mxs_angvec *pAng);
EXTERN void PlyrVMotSetEndOffsets(mxs_vector *pPos, mxs_angvec *pAng);

EXTERN void PlyrVMotSetDuration(float seconds);

typedef void (*PlayerArmFilterFunc)(mxs_vector *loc, mxs_matrix *orient);

EXTERN void SetPlayerArmFilter(PlayerArmFilterFunc filterFunc);
EXTERN void ClearPlayerArmFilter();

#endif
