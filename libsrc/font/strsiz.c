/*
 * $Source: s:/prj/tech/libsrc/font/RCS/strsiz.c $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/11 10:25:44 $
 *
 * String width and height calculators.
 *
 * This file is part of the font library.
 *
 */

#include <chr.h>
#include <fonts.h>

/* returns the width of string s in pixels for the specified font */

int gr_font_string_width (grs_font *f, char *s)
{
   short *offset_tab;         /* table of character offsets */
   short offset;              /* offset of current character */
   short w_lin=0;             /* current line's width so far */
   short w=0;                 /* width of widest line */
   uchar c;                    /* current character */

   offset_tab = f->off_tab;
   while ((c= (uchar) (*s++)) != '\0') {
      if (c == CHAR_SOFTSP)
         continue;
      if (c=='\n' || c==CHAR_SOFTCR) {
         if (w_lin>w) w=w_lin;
         w_lin = 0;
         continue;
      }
      offset = offset_tab[c-f->min];
      w_lin += offset_tab[c-f->min+1]-offset;
   }
   return (w_lin>w) ? w_lin : w;
}

/* returns the height of string s in pixels for the specified font
   has the same behavior as string size for 0 length and newline
   terminated strings.
*/

int gr_font_string_height (grs_font *f, char *s)
{
   short h;                   /* height */
   char c;                    /* current character */

   h = f->h;
   while ((c=*s++) != '\0') {
     if (c=='\n' || c==CHAR_SOFTCR) {
       h += f->h;
     }
   }
   return h;
}

/* calculate the width and height of a string in the specified font, and
   return in the given pointers. */

void gr_font_string_size (grs_font *f, char *s, short *w, short *h)
{
   short *offset_tab;         /* table of character offsets */
   short offset;              /* offset of current character */
   short w_lin=0;             /* current line's width so far */
   short w_str=0;             /* width of widest line */
   short h_str;               /* height of string */
   uchar c;                   /* current character */

   offset_tab = f->off_tab;
   h_str = f->h;
   while ((c = (uchar) (*s++)) != '\0') {
      if (c == CHAR_SOFTSP)
         continue;
      if (c=='\n' || c==CHAR_SOFTCR) {
         if (w_lin>w_str) w_str=w_lin;
         w_lin = 0;
         h_str += f->h;
         continue;
      }
      offset = offset_tab[c-f->min];
      w_lin += offset_tab[c-f->min+1]-offset;
   }
   *w = (w_lin>w_str) ? w_lin : w_str;
   *h = h_str;
}

/* as above, but only for first n characters */

int gr_font_string_nwidth (grs_font *f, char *s, int n)
{
   short *offset_tab;         /* table of character offsets */
   short offset;              /* offset of current character */
   short w_lin=0;             /* current line's width so far */
   short w=0;                 /* width of widest line */
   uchar c;                   /* current character */

   offset_tab = f->off_tab;
   while ((c = (uchar)(*s++))!='\0' && n--) {
      if (c == CHAR_SOFTSP)
         continue;
      if (c=='\n' || c==CHAR_SOFTCR) {
         if (w_lin>w) w=w_lin;
         w_lin = 0;
         continue;
      }
      offset = offset_tab[c-f->min];
      w_lin += offset_tab[c-f->min+1]-offset;
   }
   return (w_lin>w) ? w_lin : w;
}

int gr_font_string_nheight (grs_font *f, char *s, int n)
{
   short h;                 /* height */
   char c;                  /* current character */

   h = f->h;
   while ((c=*s++) != '\0' && n--) {
     if (c=='\n' || c==CHAR_SOFTCR) {
       h += f->h;
     }
   }
   return h;
}

void gr_font_string_nsize (grs_font *f, char *s, short *w, short *h, int n)
{
   short *offset_tab;         /* table of character offsets */
   short offset;              /* offset of current character */
   short w_lin=0;             /* current line's width so far */
   short w_str=0;             /* width of widest line */
   short h_str;               /* height of string */
   uchar c;                   /* current character */

   offset_tab = f->off_tab;
   h_str = f->h;
   while ((c = (uchar)(*s++)) != '\0' && n--) {
      if (c == CHAR_SOFTSP)
         continue;
      if (c=='\n' || c==CHAR_SOFTCR) {
         if (w_lin>w_str) w_str=w_lin;
         w_lin = 0;
         h_str += f->h;
         continue;
      }
      offset = offset_tab[c-f->min];
      w_lin += offset_tab[c-f->min+1]-offset;
   }
   *w = (w_lin>w_str) ? w_lin : w_str;
   *h = h_str;
}
