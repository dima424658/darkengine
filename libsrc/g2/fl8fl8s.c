#include <dev2d.h>

extern void gen_flat8_uscale(grs_bitmap *bm, int x, int y, int w, int h);
extern int gen_flat8_scale(grs_bitmap *bm, int x, int y, int w, int h);

void opaque_8to8_uscale(grs_bitmap *bm, int x, int y, int w, int h)
{
   gen_flat8_uscale(bm, x, y, w, h);
}

int opaque_8to8_scale(grs_bitmap *bm, int x, int y, int w, int h)
{
   return gen_flat8_scale(bm, x, y, w, h);
}

void trans_8to8_uscale(grs_bitmap *bm, int x, int y, int w, int h)
{
   gen_flat8_uscale(bm, x, y, w, h);
}

int trans_8to8_scale(grs_bitmap *bm, int x, int y, int w, int h)
{
   return gen_flat8_scale(bm, x, y, w, h);
}

void opaque_clut_8to8_uscale(grs_bitmap *bm, int x, int y, int w, int h)
{
   gen_flat8_uscale(bm, x, y, w, h);
}

int opaque_clut_8to8_scale(grs_bitmap *bm, int x, int y, int w, int h)
{
   return gen_flat8_scale(bm, x, y, w, h);
}

void trans_clut_8to8_uscale(grs_bitmap *bm, int x, int y, int w, int h)
{
   gen_flat8_uscale(bm, x, y, w, h);
}

int trans_clut_8to8_scale(grs_bitmap *bm, int x, int y, int w, int h)
{
   return gen_flat8_scale(bm, x, y, w, h);
}
