/*
 * $Source: x:/prj/tech/libsrc/dev2d/RCS/dtabfcn.h $
 * $Revision: 1.4 $
 * $Author: KEVIN $
 * $Date: 1998/03/12 10:40:46 $
 * 
 * device table function prototypes.
 *
 * This file is part of the dev2d library.
 *
 */

#ifndef __DTABFCN_H
#define __DTABFCN_H
#include <vga.h>
#include <vesa.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void flat8_set_mode();

extern void save_gen_state();
extern void restore_gen_state();
extern void vbe_get_mode();
extern void vbe_stat_htrace();
extern void vbe_stat_vtrace();

extern void win32_set_mode();
extern void win32_set_pal();
extern void win32_get_pal();

extern void com_init();
extern void com_close();
extern void com_set_mode();
extern void com_get_mode();
extern void com_save_state();
extern void com_restore_state();
extern void com_stat_htrace();
extern void com_stat_vtrace();
extern void com_set_pal();
extern void com_get_pal();
extern void com_set_width();
extern void com_set_focus();
extern void com_get_focus();
extern void com_get_rgb_bitmask();

extern void null_device();
extern void null_set_mode();
extern void null_get_mode();
extern void null_state();
extern void null_trace();
extern void null_width();
extern void null_get_focus();
extern void null_get_rgb_bitmask();

#ifdef __cplusplus
};
#endif
#endif
