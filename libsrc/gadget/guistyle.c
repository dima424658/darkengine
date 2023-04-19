/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/libsrc/gadget/guistyle.c,v 1.1 1996/07/30 19:27:26 mahk Exp $

#include <2d.h>
#include <res.h>
#include <gfile.h>
#include <2dres.h>

#include <guistyle.h>

////////////////////////////////////////////////////////////
// Global style and methods
////////////////////////////////////////////////////////////

static guiStyle* curr_style = NULL;

void SetCurrentStyle(guiStyle* style)
{
   curr_style = style;
}

guiStyle* GetCurrentStyle(void)
{
   return curr_style;
}

static guiStyleMethods* curr_methods = &DefaultStyleMethods;

void guiStyleMethodsSet(guiStyleMethods* methods)
{
   curr_methods = methods;
}

guiStyleMethods* guiStyleMethodsGet(void)
{
   return curr_methods;
}

////////////////////////////////////////////////////////////
// default style methods
////////////////////////////////////////////////////////////

static bool default_setpal_func(StylePalette palette)
{
   if (palette == 0) 
      return FALSE;
   if (REFID(palette) == 0) // must be a resid
   {
      PallInfo* pal = ResLock(palette);
      gr_set_pal(pal->index,pal->numcols,pal->rgb);
      ResUnlock(palette);
   }
   else
   {
      gr_set_pal_imgref(palette);
   }
   return TRUE;
}


static bool default_setfont_func(StyleFont font)
{
   if (font == NULL)
      return FALSE;

   gr_set_font((grs_font*)IDataSource_Lock(font));
   return TRUE;
}

static bool default_unsetfont_func(StyleFont font)
{
    if (font == NULL)
        return FALSE;

    IDataSource_Unlock(font);
    return TRUE;
}


guiStyleMethods DefaultStyleMethods = 
{
   default_setpal_func,

   default_setfont_func,
   default_unsetfont_func,
   NULL, // no sound defaults
   NULL, 
};

////////////////////////////////////////////////////////////
// Accessor methods
////////////////////////////////////////////////////////////

#define SET_STYLE(s) (s = (((s) == NULL) ? curr_style : (s)))

StylePalette guiStyleGetPalette(guiStyle* style)
{
   SET_STYLE(style); 
   if (style == NULL)
      return 0; 
   return style->pal;
}


// Yum, kill three accessors with one macro

#define IMPLEMENT_ACCESSOR(type,array) \
Style##type guiStyleGet##type(guiStyle* style, Style##type##Kind idx) \
{ \
     SET_STYLE(style); \
     if (style == NULL) return 0; \
     return style->array[idx];\
} 


IMPLEMENT_ACCESSOR(Color,colors)

IMPLEMENT_ACCESSOR(Font,fonts)

IMPLEMENT_ACCESSOR(Sound,sounds)

#define METHOD_CALL(func,arg) \
   (curr_methods->func != NULL) ? curr_methods->func(arg) : FALSE

bool guiStyleSetupPalette(guiStyle* style)
{
   SET_STYLE(style);
   if (style == NULL) return FALSE;
   return METHOD_CALL(setpal_func,style->pal);
}

bool guiStyleSetupFont(guiStyle* style, StyleFontKind font)
{
   SET_STYLE(style);
   if (style == NULL) return FALSE;
   return METHOD_CALL(setfont_func,style->fonts[font]);
}


bool guiStyleCleanupFont(guiStyle* style, StyleFontKind font)
{
   SET_STYLE(style);
   if (style == NULL) return FALSE;
   return METHOD_CALL(unsetfont_func,style->fonts[font]);
}


bool guiStylePlaySound(guiStyle* style, StyleSoundKind sound)
{
   SET_STYLE(style);
   if (style == NULL) return FALSE;
   return METHOD_CALL(playsound_func,style->sounds[sound]);
}

bool guiStyleStopSound(guiStyle* style, StyleSoundKind sound)
{
   SET_STYLE(style);
   if (style == NULL) return FALSE;
   return METHOD_CALL(stopsound_func,style->sounds[sound]);
}

