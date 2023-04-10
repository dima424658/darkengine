/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/gamma.h,v 1.3 2000/03/16 17:16:54 patmac Exp $
#pragma once

// Implementations are in ctrast.c

#ifndef __GAMMA_H
#define __GAMMA_H

// global variable just because
EXTERN float g_gamma;

// force screen to use the current gamma
EXTERN void gamma_display(void);

// call if g_gamma changes (redisplays and saves to config file)
EXTERN void gamma_update(void);

// call to initialize g_gamma at the start
EXTERN void gamma_load(void);

// reasonable limits for options panel to use
#define MIN_GAMMA 0.5
#define DEFAULT_GAMMA 1.0
#define MAX_GAMMA 1.5

#endif
