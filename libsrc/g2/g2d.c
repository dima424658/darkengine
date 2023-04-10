// $Header: x:/prj/tech/libsrc/g2/RCS/g2d.c 1.4 1996/10/31 15:33:09 KEVIN Exp $
//
// Global data

int flat8_per_ltol=10;

bool g2d_error_dither=FALSE;

long g2d_dryness_mask_list[4];    // 0 if just a palette, otherwise 0xffffffff
uchar *g2d_dryness_table_list[4];
int g2d_dryness_table_size=16;      // nuber of rows

uchar *g2d_haze_table;
int g2d_haze_table_size=16;         // nuber of rows

void (**g2d_canvas_table)();

void (**g2d_flat8_canvas_table)();
void (**g2d_flat16_canvas_table)();
void (**g2d_flat24_canvas_table)();
void (**g2d_gen_canvas_table)();
