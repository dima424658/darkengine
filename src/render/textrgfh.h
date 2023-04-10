/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/textrgfh.h,v 1.3 2000/01/31 09:53:38 adurant Exp $
#pragma once

#ifndef __TEXTRGFH_H
#define __TEXTRGFH_H
#include <swappnp.h>
#include <gadget.h>

// create the texture gfh
EXTERN void CreateTextureGFH(LGadRoot* root, Rect* bounds, editBrush* brush);
EXTERN void DestroyTextureGFH(void);

#endif // __TEXTRGFH_H
