/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/slitprop.h,v 1.3 2000/01/31 09:53:23 adurant Exp $
#pragma once

#ifndef SLITPROP_H
#define SLITPROP_H
#include <property.h>

// SELF-ILLUMINATION LEVEL PROPERTY NAME
#define PROP_SELF_LIT_NAME "SelfLit"

// SELF-ILLUMINATION PROPERTY API
EXTERN void SelfLitPropInit(void);


#define PROP_SHADOW_NAME "Shadow"
EXTERN void ShadowPropInit(void);

#endif // SLITPROP_H
