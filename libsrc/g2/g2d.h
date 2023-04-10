// $Header: x:/prj/tech/libsrc/g2/RCS/g2d.h 1.4 1998/07/04 13:13:02 PATMAC Exp $

#ifndef __G2D_H
#define __G2D_H

EXTERN bool g2d_error_dither;
EXTERN long g2d_dryness_mask_list[];    // 0 if just a palette, otherwise 0xffffffff
EXTERN uchar *g2d_dryness_table_list[];
EXTERN int g2d_dryness_table_size;     // nuber of rows
EXTERN uchar *g2d_haze_table;
EXTERN int g2d_haze_table_size;        // nuber of rows

EXTERN void (**g2d_canvas_table)();

EXTERN void (**g2d_flat8_canvas_table)();
EXTERN void (**g2d_flat16_canvas_table)();
EXTERN void (**g2d_flat24_canvas_table)();
EXTERN void (**g2d_gen_canvas_table)();

EXTERN void (*g2d_default_flat8_canvas_table[])();
EXTERN void (*g2d_default_flat16_canvas_table[])();
EXTERN void (*g2d_default_flat24_canvas_table[])();
EXTERN void (*g2d_default_gen_canvas_table[])();

#define gr_set_dither(_d) (g2d_error_dither = (_d))

#endif
