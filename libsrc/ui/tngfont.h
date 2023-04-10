// $Header: x:/prj/tech/libsrc/ui/RCS/tngfont.h 1.3 1998/06/18 13:33:16 JAEMZ Exp $

#ifndef __TNGFONT_H
#define __TNGFONT_H
#pragma once

EXTERN int fnt_c_wid(char c);
EXTERN int fnt_s_wid(char *s);
EXTERN int fnt_c_draw(char c, int xpos, int ypos);
EXTERN int fnt_s_draw(char *s, int xpos, int ypos);
EXTERN int fnt_s_clr(char *s, int x, int y);
EXTERN int fnt_l_clr(int len, int x, int y);
EXTERN int fnt_c_clr(char c, int x, int y);
EXTERN int fnt_cursor(int x, int y);
EXTERN int anal_s_draw(char *s, int xpos, int ypos);
#ifdef nOAF
EXTERN TTFontInfo *build_fnt_TTFI(TextToolFontInfo *mfont);
#endif

#endif  // __TNGFONT
