/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/litnet.h,v 1.3 2000/01/29 13:39:02 adurant Exp $
//
// Networking for lights, specifically AnimLights
//
#pragma once

#ifndef _LITNET_H
#define _LITNET_H

#include <objtype.h>

//
// Tell the rest of the world that an animated light has changed modes.
// The owner of the light should call this whenever it does a major mode
// change, so the clients know about it. (We can't simply network the
// AnimLight property, because that gets set on a frame-by-frame basis.)
//
// mode should be an ANIM_LIGHT_MODE, as defined in animlgte.h
//
EXTERN void BroadcastAnimLightMode(ObjID obj, short mode);

//
// Setup and shutdown animated light networking.
//
EXTERN void AnimLightNetInit();
EXTERN void AnimLightNetTerm();

#endif // !_LITNET_H
