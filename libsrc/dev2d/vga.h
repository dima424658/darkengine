/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/vga.h $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 17:02:15 $
 *
 * Prototypes and macros for general purpose vga code.
 *
 * This file is part of the dev2d library.
 */

#ifndef __VGA_H
#define __VGA_H

#ifdef __cplusplus
extern "C" {
#endif

extern void vga_save_mode (void);
extern void vga_rest_mode (void);
extern void vga_wait_vsync (void);
extern void vga_wait_display (void);
extern int  vga_set_mode (int mode, int flags);
extern void vga_set_pal (int start, int n, uchar *pal_data);
extern void vga_get_pal (int start, int n, uchar *pal_data);
extern int vga_save_state();
extern int vga_restore_state();
extern void vga_stat_htrace();
extern void vga_stat_vtrace();
extern void vga_set_width();
extern void vga_set_focus();
extern void vga_get_focus();

#ifdef __cplusplus
};
#endif
#endif /* !__VGA_H */

