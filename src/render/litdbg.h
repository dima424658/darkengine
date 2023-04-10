/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/litdbg.h,v 1.1 1999/06/07 22:58:27 dc Exp $
// is this idiotic, or what...

#ifndef __LITDBG_H
#pragma once
#define __LITDBG_H

#include <config.h>

#ifdef EDITOR
EXTERN int _watch_light;
#define is_watch_light(o)       (o==_watch_light)
#define watch_light_mprint(o,x) do { if (is_watch_light(o)) mprintf x; } while (0)
#define WATCH_LIGHTS
#else
#define is_watch_light(o)       (FALSE)
#define watch_light_mprint(o,x)
#endif

// uncomment this if you want wicked loud debugging of lights
//#define LOUD_LIGHT_DEBUG

#ifdef LOUD_LIGHT_DEBUG
#define loud_lights()        config_is_defined("loud_lighting")
#define loud_light_mprint(x) do { if (loud_lights()) mprintf x; } while (0)
#else
#define loud_lights()
#define loud_light_mprint(x)
#endif

#endif  __LITDBG_H
