/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/ui/uigame.h,v 1.7 1998/11/01 05:17:33 dc Exp $
#pragma once  
#ifndef __UIGAME_H
#define __UIGAME_H
#include <guistyle.h>

#ifndef DEFAULT_TO
#ifdef __cplusplus
#define DEFAULT_TO(x) = (x)
#else
#define DEFAULT_TO(x)
#endif 
#endif 

// One-time app init/term
EXTERN void GameInitGUI(void); 
EXTERN void GameTermGUI(void); 

// Game mode init/term
EXTERN void GameCreateGUI(void);
EXTERN void GameDestroyGUI(void); 

//EXTERN int find_palette_color(ulong rgb);
EXTERN void uiGameStyleSetup(void);
EXTERN void uiGameStyleCleanup(void); 
EXTERN guiStyle* uiGameStyle(void); 

// for attaching metasnds
EXTERN void SetMetaSndGUI(void);
EXTERN void RestoreMetaSndGUI(void);

// Load a style from config
EXTERN void uiGameLoadStyle(const char* prefix, guiStyle* style, const char* relpath DEFAULT_TO(NULL)); 
EXTERN void uiGameUnloadStyle(guiStyle* style); 
EXTERN void uiGameLoadStyleColors(const char* prefix, guiStyle* style); 

// contrast-protect all colors in the style 
EXTERN void uiGameProtectStyle(const guiStyle* style);

#define uiRGB(r,g,b)    guiRGB(r,g,b)
#define uiR(rgb)        guiR(rgb)
#define uiG(rgb)        guiG(rgb)
#define uiB(rgb)        guiB(rgb)


#endif // __UIGAME_H
