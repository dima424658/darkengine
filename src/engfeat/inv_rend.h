/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/engfeat/inv_rend.h,v 1.9 1998/08/25 10:27:09 KEVIN Exp $
// inventory object rendering/tracking

#pragma once
#ifndef __INV_REND_H
#define __INV_REND_H

#include <dev2d.h>  // for grs_canvas, sadly
#include <rect.h>

struct sInvRendView
{
   float cam_dist;          // distance to camera - or scale factor for bitmaps
   mxs_angvec off_ang;      // ang from obj to cam
   float lighting;          // lighting level 0.0..1.0
};

typedef struct sInvRendView sInvRendView;

typedef struct _invRendState invRendState;

// user flag bits - internal are just at top of inv_rend.c
#define INVREND_SET          (1<<0)  // set this if you have meaningful flags set
#define INVREND_CACHEPICTURE (1<<1)  // unsupported
#define INVREND_ROTATE       (1<<2)
#define INVREND_DIMMED       (1<<3)
#define INVREND_DISTANT      (1<<4)
#define INVREND_HARDWARE_IMMEDIATE (1<<5)  // break pipeline to render immediately
#define INVREND_HARDWARE_QUEUE     (1<<6)  // queue and render efficiently on next frame
#define INVREND_CLEAR        (1<<7)        // clear canvas before rendering
#define INVREND_USERFLAGS    (0xff)

EXTERN invRendState *invRendBuildState(int flags, ObjID o_id, const Rect *r, grs_canvas *draw_cnv);
EXTERN invRendState *invRendBuildStateFromType(int flags, struct sInvRenderType*, const Rect *r, grs_canvas* canv); 

EXTERN void invRendDrawUpdate(invRendState *us);
EXTERN void invRendFreeState(invRendState *us);

EXTERN void invRendSetRect(invRendState *us, const Rect *r);
EXTERN void invRendSetCanvas(invRendState* us, grs_canvas* canv);
EXTERN void invRendSetType(invRendState* us, const struct sInvRenderType* type);


// this is the icky one
// NOTE: if the initial state was built CACHEPICTURE, you cannot flip that flag here...
// ...not that that flag does anything yet anyway...
EXTERN void invRendUpdateState(invRendState *us, int flags, ObjID o_id, const Rect *r, grs_canvas *draw_cnv);

EXTERN void invRendGetView(const invRendState* us, sInvRendView * view); 
EXTERN void invRendSetView(invRendState* us, const sInvRendView * view);

// Flush list of hardware rendered objects queued by calls to invRendDrawUpdate()
EXTERN void invRendFlushQueue(void);

// Free queue on mode changes
EXTERN void invRendFreeQueue(void);

// init/term
EXTERN void invRendInit(void);
EXTERN void invRendTerm(void);

#endif  // __INV_REND_H




