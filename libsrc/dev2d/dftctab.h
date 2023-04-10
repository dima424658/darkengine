/*
 * $Source: x:/prj/tech/libsrc/dev2d/RCS/dftctab.h $
 * $Revision: 1.2 $
 * $Author: TOML $
 * $Date: 1996/12/13 16:01:46 $
 *
 * Prototypes for detection routines.
 *
 * This file is part of the dev2d library.
 *
 */

#ifndef __DFTCTAB_H
#define __DFTCTAB_H

#ifdef __cplusplus
extern "C" {
#endif

extern void (*gdd_default_flat8_canvas_table[])();
extern void (*gdd_default_flat16_canvas_table[])();
extern void (*gdd_default_flat24_canvas_table[])();
extern void (*gdd_default_bank8_canvas_table[])();
extern void (*gdd_default_bank16_canvas_table[])();
extern void (*gdd_default_bank24_canvas_table[])();
extern void (*gdd_default_modex_canvas_table[])();
extern void (*gdd_default_dispdev_canvas_table[])();
extern void (*gdd_null_canvas_table[])();
#ifdef __cplusplus
};
#endif
#endif
