/*
 * $Source: x:/prj/tech/libsrc/font/RCS/chrsiz.h $
 * $Revision: 1.2 $
 * $Author: PATMAC $
 * $Date: 1998/07/04 13:16:18 $
 *
 * character size routine prototypes.
 *
 * This file is part of the font library.
 *
 */

#ifndef __CHRSIZ_H
#define __CHRSIZ_H
#include <fonts.h>

EXTERN short gr_font_char_width (grs_font *f, char c);
EXTERN void gr_font_char_size (grs_font *f, char c, short *w, short *h);

#endif
