/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/render.h,v 1.4 1998/09/16 00:12:45 mahk Exp $
#pragma once  
#ifndef __RENDER_H
#define __RENDER_H
#include <comtools.h>

EXTERN void RendererInit(void);
EXTERN void RendererTerm(void); 

EXTERN void RendererReset(void); 
EXTERN void RendererPalChange(void); 

EXTERN void RendererDBMessage(ulong msg, IUnknown* file) ;
EXTERN void SetZNearFar(double z_near, double z_far);

enum eSkyModes 
{
   kSkyModeTextures,
   kSkyModeStars, 
}; 

EXTERN int GetSkyMode(void); 
EXTERN void SetSkyMode(int mode); 

EXTERN int GetWaterBankClut(int bank); 

#endif // __RENDER_H

