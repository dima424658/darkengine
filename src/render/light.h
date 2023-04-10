/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/light.h,v 1.3 2000/01/29 13:39:01 adurant Exp $
#pragma once

#ifndef __LIGHT_H
#define __LIGHT_H

#include <wr.h>

extern void reset_dynamic_lighting(void);
extern void add_dynamic_light(Location *loc, float bright);

#endif
