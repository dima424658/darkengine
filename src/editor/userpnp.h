/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/userpnp.h,v 1.2 2000/01/29 13:13:21 adurant Exp $
#pragma once
#ifndef __USERPNP_H
#define __USERPNP_H

#include <swappnp.h>

EXTERN void CreateUserPNP(LGadRoot* root, Rect* bounds, editBrush* brush);
EXTERN void UpdateUserPNP(GFHUpdateOp op, editBrush* brush);
EXTERN void DestroyUserPNP(void);

#endif // __USERPNP_H
