/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/portal/animlit.h,v 1.3 2000/01/29 13:37:11 adurant Exp $
#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

extern void LightTurnOff(int t);
extern void LightTurnOn(int t);
extern void LightTurnOnSet(int t, int b);
extern void LightSet(int t, int b);
extern bool IsLightOn(int t);
extern void LightToggle(int t);

// internal function, doesn't change default brightness
//   could be useful for flickering to partial bright
extern void LightSetBrightness(int t, int b);

// animation system
extern void LightAnimSetScript(int t, uchar *script);
extern void LightAnimPlayScripts(int ticks);

//  Defining the meaning of lights

extern void LightDefineStart(void);
extern int LightDefineEnd(void);

extern void LightCellStart(int cell_id);
extern void LightCellEnd(void);

extern void LightAtVertex(int vertex, uchar lighting);

// clear all existing definitions of lights
extern void LightClear(void);

//// INTERFACE BACK INTO PORTAL ////
// not defined by lighting.h, but I'm making sure to
// keep the interface as narrow as possible and this
// is one strange way to do it.  The function is defined
// in portal.c
extern unsigned char *portal_get_lighting_data(int cell);


#ifdef __cplusplus
};
#endif
