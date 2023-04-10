/*
 * $Source: x:/prj/tech/libsrc/r3d/RCS/r3ds.h $
 * $Revision: 1.11 $
 * $Author: PATMAC $
 * $Date: 1998/07/02 12:04:51 $
 *
 * Base structure and type definitions
 *
 */

#ifndef __R3DS_H
#define __R3DS_H
#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <grspoint.h>
#include <matrixs.h>

// The big guy himself.  This may change over time
typedef struct _r3s_point {
   mxs_vector p;
   ulong ccodes;
   grs_point grp;
} r3s_point;

typedef r3s_point * r3s_phandle;

#ifdef __cplusplus
}
#endif
#endif // __R3DS_H
