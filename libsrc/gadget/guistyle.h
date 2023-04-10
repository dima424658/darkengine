/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/
// $Header: x:/prj/tech/libsrc/gadget/RCS/guistyle.h 1.3 1998/09/18 19:19:37 mahk Exp $

#ifndef __GUISTYLE_H
#define __GUISTYLE_H
#pragma once

#include <datasrc.h>

////////////////////////////////////////////////////////////
// Kinds of colors
////////////////////////////////////////////////////////////

typedef enum _StyleColorKind
{
   StyleColorFG,                // foreground 
   StyleColorBG,                // background,
   StyleColorText,              // text color
   StyleColorHilite,            // hilight color
   StyleColorBright,            // bright color
   StyleColorDim,               // dim color
   StyleColorFG2,               // alternate foreground 
   StyleColorBG2,               // alternate background
   StyleColorBorder,            // border color
   StyleColorWhite,             // white color
   StyleColorBlack,             // black color
   StyleColorXOR,               // color for xor-ing
   StyleColorBevelLight,        // light bevel color
   StyleColorBevelDark,         // dark bevel color
   StyleNumColors
} StyleColorKind;

typedef ulong StyleColor;

////////////////////////////////////////////////////////////
// MACROS FOR MAKING/UNMAKING RGB COLORS
////////////////////////////////////////////////////////////

//
// StyleGetColor will turn these colors into screen colors 
// automatically, or you can pre-compile them.
//

#define GUI_RGB_MASK 0x1000000
#define guiRGB(r,g,b)   (GUI_RGB_MASK|((b) << 16)|((g) << 8)|(r))
#define guiR(rgb)     (((rgb) >> 0) & 0xFF)
#define guiG(rgb)     (((rgb) >> 8) & 0xFF)
#define guiB(rgb)     (((rgb) >> 16) & 0xFF)
#define guiIsRGB(rgb)  ((rgb) & GUI_RGB_MASK)

//
// Pre-convert a single color to screen.  Idempotent.
//
EXTERN int guiScreenColor(int color); 

//
// Pre-convert an entire style
//
EXTERN void guiCompileStyleColors(struct guiStyle* targ, const struct guiStyle* src); 

////////////////////////////////////////////////////////////
// Kinds of fonts
////////////////////////////////////////////////////////////

typedef enum _StyleFontKind
{
   StyleFontNormal,             // for most text
   StyleFontTitle,              // for titles
   StyleNumFonts
} StyleFontKind;

typedef IDataSource * StyleFont;

////////////////////////////////////////////////////////////
// Kinds of sounds
////////////////////////////////////////////////////////////

typedef enum _StyleSoundKind
{
   StyleSoundTick,              // for common or passive events 
   StyleSoundSelect,            // when an item is selected
   StyleSoundAttention,         // grab user's attention
   StyleNumSounds
} StyleSoundKind;

typedef ulong StyleSound;

////////////////////////////////////////////////////////////
// Style struct
////////////////////////////////////////////////////////////

typedef ulong StylePalette;

typedef struct guiStyle
{
   StylePalette pal;
   StyleColor   colors[StyleNumColors];
   StyleFont    fonts[StyleNumFonts];
   StyleSound   sounds[StyleNumSounds];
} guiStyle;

////////////////////////////////////////////////////////////
// API
////////////////////////////////////////////////////////////

EXTERN void SetCurrentStyle(guiStyle* style);
EXTERN guiStyle* GetCurrentStyle(void);

//
// all of these accept NULL as a first argument meaning use the 
// current style.
//

EXTERN StylePalette guiStyleGetPalette(guiStyle* style);
EXTERN StyleColor guiStyleGetColor(guiStyle* style, StyleColorKind kind);
EXTERN StyleFont  guiStyleGetFont(guiStyle* style, StyleFontKind kind);
EXTERN StyleSound guiStyleGetSound(guiStyle* style, StyleSoundKind kind);

#define guiStyleAvail(s)  ( ((s) != NULL) || GetCurrentStyle() != NULL)

//
// directly setup a font for drawing
// return false if no such font
//

EXTERN bool guiStyleSetupFont(guiStyle* style, StyleFontKind font);
// finished using font for now
EXTERN bool guiStyleCleanupFont(guiStyle* style, StyleFontKind font);

//
// directly setup a palette
//
EXTERN bool guiStyleSetupPalette(guiStyle* style);

//
// play/stop a sound
//

EXTERN bool guiStylePlaySound(guiStyle* style, StyleSoundKind kind);
EXTERN bool guiStyleStopSound(guiStyle* style, StyleSoundKind kind);

////////////////////////////////////////////////////////////
// App-installable methods
//
// the app can install these callbacks to specify how various
// style entries should be interpreted.

typedef struct guiStyleMethods 
{
   // palette function: the default function treats the argument 
   // as a resource id (if < 0x10000) or an imgref id (if >= 0x10000)
   bool (*setpal_func)(StylePalette palette);   

   // font functions. default treats the argument as a 
   // res id
   bool (*setfont_func)(StyleFont font);
   bool (*unsetfont_func)(StyleFont font);

   // sound functions. no default
   bool (*playsound_func)(StyleSound sound);
   bool (*stopsound_func)(StyleSound sound);
} guiStyleMethods;


EXTERN void guiStyleMethodsSet(guiStyleMethods* methods); // sets the current methods
EXTERN guiStyleMethods* guiStyleMethodsGet(void); // gets them

EXTERN guiStyleMethods DefaultStyleMethods; 

#endif // __GUISTYLE_H
