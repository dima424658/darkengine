/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkuitul.cpp,v 1.2 2000/02/19 13:26:31 toml Exp $
#include <shkuitul.h>
#include <gcompose.h>
#include <dev2d.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

//
// DrawString
//
// Boy I hope this goes in a library soon 
//

int ShockDrawString(const char* str, const Rect& r, ulong flags, guiStyle* style, StyleFontKind font)
{
   // cast away from const, because we are sneaky
   char* s = (char*)str; 

   GUIcompose c; 
   GUIsetup(&c,(Rect*)&r,GUIcomposeFlags(ComposeFlagClear|ComposeFlagRead),GUI_CANV_ANY); 

   guiStyleSetupFont(style,font); 
   gr_set_fcolor(guiStyleGetColor(style,StyleColorText)); 

   gr_font_string_wrap(gr_get_font(),s,RectWidth(&r)); 

   short w,h; 
   gr_string_size(s,&w,&h); 
   int x = (flags & kLeftAlign) ? 0 : (RectWidth(&r) - w)/2;

   gr_string(s,x,0); 

   gr_font_string_unwrap(s);

   guiStyleCleanupFont(style,font);

   GUIdone(&c); 
   return h; 
}



