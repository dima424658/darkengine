/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkuitul.h,v 1.2 2000/01/31 09:59:31 adurant Exp $
#pragma once

#ifndef __SHKUITUL_H
#define __SHKUITUL_H
#include <rect.h>
#include <guistyle.h>

// Flags for DrawString
enum eDrawStringFlags
{
   kLeftAlign  = 1 << 0, 
};

   // Draw an arbitrary string in a rect, centered and top aligned.
   // Return the height drawn, that is to say the y coord of the bottom of text  
extern int ShockDrawString(const char* str, const Rect& r, ulong flags = 0, guiStyle* style = NULL, StyleFontKind font = StyleFontNormal); 



#endif // __SHKUITUL_H
