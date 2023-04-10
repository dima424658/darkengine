/*
 * $Source: x:/prj/tech/libsrc/g2/RCS/cominit.c $
 * $Revision: 1.2 $
 * $Author: TOML $
 * $Date: 1996/05/18 13:16:13 $
 *
 * Routine to initizalize g2 (and dev2d) library using
 * common object model displayt device interface.
 *
 * This file is part of the g2 library.
 *
 */

#include <dev2d.h>
extern int g2_init();

int g2_com_init()
{
   gd_use_com();
   return g2_init();
}
