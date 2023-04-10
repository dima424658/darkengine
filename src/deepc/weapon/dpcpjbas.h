/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef __DPCPJBAS_H
#define __DPCPJBAS_H

#ifndef __MXANG_H
#include <mxang.h>
#endif // !__MXANG_H

// info structure attached to a projectile archetype object
typedef struct sProjectile
{
   int     m_spray;         // number of projectiles/trigger pull (instantaneous)
   mxs_ang m_spread;        // random angular error per shot
} sProjectile;

#endif  // !__DPCPJBAS_H