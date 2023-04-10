/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkrend.h,v 1.6 2000/01/31 09:59:00 adurant Exp $
#pragma once

#ifndef __SHOCKREND_H
#define __SHOCKREND_H

EXTERN ObjID g_distPickObj;

EXTERN void shock_init_object_rend(void);
EXTERN void shock_term_object_rend(void);
EXTERN void shock_init_renderer(void);
EXTERN void shock_term_renderer(void);
EXTERN void shock_pick_reset(void);

#endif // __SHOCKREND_H
