/*
 * $Source: x:/prj/tech/libsrc/font/RCS/fontf.h $
 * $Revision: 1.6 $
 * $Author: PATMAC $
 * $Date: 1998/07/04 13:16:27 $
 *
 * Prototypes for font functions.
 *
 * This file is part of the font library.
 *
 */

#ifndef __FONTF_H
#define __FONTF_H
#include <dev2d.h>
#include <chrsiz.h>
#include <strsiz.h>

/* font set/get macros */
#define gr_set_font(font_parm) (grd_canvas->gc.font = (void *)(font_parm))
#define gr_get_font()     ((grs_font *)(grd_canvas->gc.font))

/* prototypes for non-table driven string handling routines. */
EXTERN void gr_font_string_uclear (grs_font *f, char *s, short x, short y);
EXTERN int gr_font_string_clear (grs_font *f, char *s, short x, short y);
EXTERN void gr_font_uchar (grs_font *f, char c, short x, short y);
EXTERN int gr_font_char (grs_font *f, char c, short x, short y);
EXTERN void gr_font_char_uclear (grs_font *f, char c, short x, short y);
EXTERN int gr_font_char_clear (grs_font *f, char c, short x, short y);
EXTERN int gr_font_string_wrap (grs_font *pfont, char *ps, short width);
EXTERN void gr_font_string_unwrap (char *s);
EXTERN void gr_font_ustring(grs_font *f, char *s, int x, int y);
EXTERN int gr_font_string(grs_font *f, char *s, int x, int y);
EXTERN void gr_font_scale_ustring(grs_font *f, char *s, int x, int y, int w, int h);
EXTERN int gr_font_scale_string(grs_font *f, char *s, int x, int y, int w, int h);

/* use the current font */

#define gr_char_width(c) \
    gr_font_char_width (gr_get_font(), c)

#define gr_char_size(c, w, h) \
    gr_font_char_size (gr_get_font(), c, w, h)

#define gr_string_size(s, w, h) \
   gr_font_string_size (gr_get_font(), s, w, h)

#define gr_string_nsize(s, n, w, h) \
   gr_font_string_nsize (gr_get_font(), s, n, w, h)

#define gr_string_width(s) \
   gr_font_string_width (gr_get_font(), s)

#define gr_string_nwidth(s, n) \
   gr_font_string_nwidth (gr_get_font(), s, n)

#define gr_string_height(s) \
   gr_font_string_height (gr_get_font(), s)

#define gr_string_nheight(s, n) \
   gr_font_string_nheight (gr_get_font(), s, n)

#define gr_string_uclear(s, x, y) \
   gr_font_string_uclear (gr_get_font, s, x, y)

#define gr_string_clear(s, x, y) \
    gr_font_string_clear (gr_get_font(), s, x, y)

#define gr_char(c, x, y) \
    gr_font_char (gr_get_font(), c, x, y)

#define gr_char_uclear(c, x, y) \
    gr_font_char_uclear (gr_get_font(), c, x, y)

#define gr_char_clear(c, x, y) \
    gr_font_char_clear (gr_get_font(), c, x, y)

#define gr_string_warp(s) \
    gr_font_string_wrap (gr_get_font(), s)

#define gr_string_unwarp(s) \
    gr_font_string_unwrap (gr_get_font(), s)

#define gr_ustring(s, x, y) \
    gr_font_ustring (gr_get_font(), s, x, y)

#define gr_string(s, x, y) \
    gr_font_string (gr_get_font(), s, x, y)

#define gr_scale_ustring(s, x, y, w, h) \
    gr_font_scale_ustring (gr_get_font(), s, x, y, w, h)

#define gr_scale_string(s, x, y, w, h) \
    gr_font_scale_string (gr_get_font(), s, x, y, w, h)

#endif
