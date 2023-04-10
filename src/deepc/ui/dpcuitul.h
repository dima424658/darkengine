/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef __DPCUITUL_H
#define __DPCUITUL_H

#ifndef RECT_H
#include <rect.h>
#endif // !RECT_H

#ifndef __GUISTYLE_H
#include <guistyle.h>
#endif // !__GUISTYLE_H

// Flags for DrawString
enum eDrawStringFlags
{
   kLeftAlign  = 1 << 0, 
};

   // Draw an arbitrary string in a rect, centered and top aligned.
   // Return the height drawn, that is to say the y coord of the bottom of text  
extern int DPCDrawString(const char* str, const Rect& r, ulong flags = 0, guiStyle* style = NULL, StyleFontKind font = StyleFontNormal); 

#endif // __DPCUITUL_H
