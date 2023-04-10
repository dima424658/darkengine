/*
 * $Source: s:/prj/tech/libsrc/g2/RCS/setdrv.c $
 * $Revision: 1.2 $
 * $Author: KEVIN $
 * $Date: 1996/05/20 10:43:51 $
 * 
 * This file is part of the g2 library.
 *
 */

#include <g2d.h>

void g2_use_default_drivers()
{
   g2d_flat8_canvas_table = g2d_default_flat8_canvas_table;
   g2d_flat16_canvas_table = g2d_default_flat16_canvas_table;
//   g2d_flat24_canvas_table = g2d_default_flat24_canvas_table;
   g2d_flat24_canvas_table = g2d_default_gen_canvas_table;
   g2d_gen_canvas_table = g2d_default_gen_canvas_table;
}

void g2_use_generic_drivers()
{
   g2d_flat8_canvas_table = g2d_gen_canvas_table;
   g2d_flat16_canvas_table = g2d_gen_canvas_table;
   g2d_flat24_canvas_table = g2d_gen_canvas_table;
}
