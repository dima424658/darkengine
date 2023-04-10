/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/ghost.h,v 1.2 2000/01/29 13:41:05 adurant Exp $
// misc controller included by all ghost internals
#pragma once

#ifndef __GHOST_H
#define __GHOST_H

// no local ghost things can get away w/o the types
#include <ghosttyp.h>

// for config vars, inc in ship since it locally #def's them to NULL
#include <ghostcfg.h>

// misc debugging usefulness
#ifndef SHIP
#include <config.h>
#include <mprintf.h>
#include <objedit.h>
#endif

#endif  // __GHOST_H
