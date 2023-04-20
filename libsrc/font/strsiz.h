/*
 * $Source: x:/prj/tech/libsrc/font/RCS/strsiz.h $
 * $Revision: 1.2 $
 * $Author: PATMAC $
 * $Date: 1998/07/04 13:16:30 $
 *
 * string size routine prototypes.
 *
 * This file is part of the font library.
 *
 */

#ifndef __STRSIZ_H
#define __STRSIZ_H
#include <fonts.h>

EXTERN short gr_font_string_width (grs_font *f, const char *s);
EXTERN short gr_font_string_height (grs_font *f, const char *s);
EXTERN void gr_font_string_size (grs_font *f, const char *s, short *w, short *h);

EXTERN short gr_font_string_nwidth (grs_font *f, char *s, int n);
EXTERN short gr_font_string_nheight (grs_font *f, char *s, int n);
EXTERN void gr_font_string_nsize (grs_font *f, const char *s, short *w, short *h, int n);

#endif

