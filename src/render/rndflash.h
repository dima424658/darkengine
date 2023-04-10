/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/rndflash.h,v 1.2 2000/01/31 09:53:14 adurant Exp $
#pragma once

#ifndef __RNDFLASH_H
#define __RNDFLASH_H

#include <property.h>
#include <propface.h>

// to use the flashbomb stuff: instantiate an object
// which has an already setup RendFlash property on it
// (i.e. instantiate an archetype).  Make that object
// be in the location you want the flashbomb to be.
// Then call the following function, and pass in how
// intense you want the effect to be on the player;
// 1.0 is the full effect, and 0.0 is no effect (the
// world is always effected as determined by the property
// settings).
EXTERN void startFlashbombRender(ObjID obj, float intensity);

// used by the renderer to determine overlay effect and
// to update the worldrep lighting
EXTERN float compute_flashbomb_effects(uchar *rgb);


EXTERN BOOL RendFlashPropInit(void);
EXTERN void RendFlashPropShutdown(void);


typedef struct FlashbombRender FlashbombRender;

#define PROP_REND_FLASH_NAME "RenderFlash"

#undef INTERFACE
#define INTERFACE IRendFlashProperty
DECLARE_PROPERTY_INTERFACE(IRendFlashProperty)
{
   DECLARE_UNKNOWN_PURE(); 
   DECLARE_PROPERTY_PURE(); 
   DECLARE_PROPERTY_ACCESSORS(FlashbombRender*); 
}; 

#endif
