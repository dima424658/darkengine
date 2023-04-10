/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/starhack.h,v 1.5 2000/01/31 09:53:30 adurant Exp $
// starfield setup/initializers/so on
#pragma once

// NOTE: this is all a hack
// it really should track polys and all be inside the portal library, id bet


#ifndef __STARHACK_H
#define __STARHACK_H

EXTERN BOOL any_stars;

// tell starfield you are about to start rendering (ie. about to call portal)
EXTERN void starfield_startframe(Position *campos, float zoom);
// tell starfield you are done with portal, it should go fill in
EXTERN void starfield_endframe(Position *campos, float zoom);

// set current star count, call on level load/editor command
EXTERN void starfield_set_cnt(int new_cnt);

// call on mode change
EXTERN void starfield_enter_mode(void);

// call on startup/shutdown of app
EXTERN void starfield_init(void);
EXTERN void starfield_term(void);

EXTERN void starfield_set_z(double z);
#endif
