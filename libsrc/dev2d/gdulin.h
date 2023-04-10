/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/gdulin.h $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 16:38:00 $
 *
 * Line drawing stuff.
 *
 * This file is part of the dev2d library.
 */

#ifndef __GDUBM_H
#define __GDUBM_H

#include <linftype.h>
#include <grd.h>
#include <icanvas.h>

#ifdef __cplusplus
extern "C" {
#endif

#define gd_uhlin \
   ((gdulin_func *)(gdd_canvas_table[GDC_UHLIN]))

#define gd_uhlin_expose \
   ((gdulin_expose_func *)(gdd_canvas_table[GDC_UHLIN_EXPOSE]))

#define gd_uvlin \
   ((gdulin_func *)(gdd_canvas_table[GDC_UHLIN]))

#define gd_uvlin_expose \
   ((gdulin_expose_func *)(gdd_canvas_table[GDC_UHLIN_EXPOSE]))

#ifdef __cplusplus
};
#endif
#endif
