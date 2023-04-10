/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/skyrend.h,v 1.4 2000/02/05 18:20:13 adurant Exp $
#pragma once

#ifndef _SKYREND_H_
#define _SKYREND_H_

struct mxs_vector;

EXTERN void SkyRendererAppInit();
EXTERN void SkyRendererInit();
EXTERN void SkyRendererTerm();
EXTERN void SkyRendererRender();
EXTERN BOOL SkyRendererTryEnhanced();
EXTERN void SkyRendererSetEnhanced(BOOL enhance);
EXTERN BOOL SkyRendererUseEnhanced();
EXTERN float SkyRendererGetHorizonDip();
EXTERN float SkyRendererGetSkyIntensity(mxs_vector *pDir);
EXTERN float SkyRendererGetSkyDist(mxs_vector *pDir);
EXTERN float SkyRendererGetClipLat();

#endif

