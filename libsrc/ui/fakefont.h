// $Header: x:/prj/tech/libsrc/ui/RCS/fakefont.h 1.5 1998/06/18 13:28:22 JAEMZ Exp $

#ifndef __FAKEFONT_H
#define __FAKEFONT_H
#pragma once

// prototypes
// drawing - return the width drawn
EXTERN int fnt_c_draw(char  c, int xpos, int ypos);
EXTERN int fnt_s_draw(char *s, int xpos, int ypos);
EXTERN int fnt_c_width(char  c);
EXTERN int fnt_s_width(char *s);
EXTERN int fnt_l_clr(int len, int xpos, int ypos);
EXTERN int fnt_s_clr(char *s, int xpos, int ypos);
EXTERN int fnt_c_clr(char c, int xpos, int ypos);
EXTERN int fnt_no_cursor(int x, int y);

// load/system
EXTERN int fnt_init(char *def_fname);      // ret handle of def_fname, -1 fail,
                                    //  0 if def_fname NULL + succesful
EXTERN int fnt_init_from_style(void *style_ptr);
EXTERN bool fnt_free(void);                // free the font system
EXTERN int fnt_load(char *fnt_fname);      // returns the handle
EXTERN int fnt_load_from_style(void *style_ptr);
EXTERN bool fnt_unload(int fnt_handle);    // these both
EXTERN bool fnt_select(int fnt_handle);    //   return success or not

// for texttools...
#ifdef  NOT
EXTERN TTFontInfo *build_font_TTFI(TextToolFontInfo *tfont);
#endif

// contstants
#define FNT_MAX_CHARS 128

#endif
