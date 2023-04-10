/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/partprop.h,v 1.2 2000/01/31 09:53:03 adurant Exp $
#pragma once

//
// Particle property header
//

#ifndef __PARTPROP_H
#define __PARTPROP_H

#include <objtype.h>

#define PROP_PARTICLE_NAME "Particle"

EXTERN void ParticlePropInit(void);
EXTERN BOOL ObjIsParticle(ObjID obj);
EXTERN void ObjSetParticle(ObjID obj);

#endif   // __COLLPROP_H
