/*
 * $Source: s:/prj/tech/libsrc/g2/RCS/svgainit.c $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 17:36:17 $
 * 
 * This file is part of the g2 library.
 *
 */

#include <dev2d.h>
extern int g2_init();

int g2_svga_init()
{
   gd_use_svga();
   return g2_init();
}
