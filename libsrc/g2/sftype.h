/*
 * $Source: s:/prj/tech/libsrc/g2/RCS/sftype.h $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/11 09:59:51 $
 *
 * This file is part of the g2 library.
 *
 */

#ifndef __SFTYPE_H
#define __SFTYPE_H

#include <dev2d.h>
typedef void (g2us_func)(grs_bitmap *bm, int x, int y, int w, int h);
typedef g2us_func *g2us_func_p;
typedef g2us_func_p (g2us_expose_func)(grs_bitmap *bm, int x, int y, int w, int h);

typedef int (g2s_func)(grs_bitmap *bm, int x, int y, int w, int h);
typedef g2s_func *g2s_func_p;
typedef g2s_func_p (g2s_expose_func)(grs_bitmap *bm, int x, int y, int w, int h);

#endif

