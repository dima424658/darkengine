/* Portable forms of the legacy power-of-two texture inner loops. */

#include <tmapd.h>

void flat8_flat8_opaque_p2_wrap_il(int x_left, int x_right, fix u, fix v)
{
   g2s_tmap_info *info = &g2d_tmap_info;
   uchar *dest = info->p_dest + x_left;
   int count = x_right - x_left;
   fix du = info->dux & (fix)0xffffff00;
   int i;

   info->p_dest += info->drow;
   for (i = 0; i < count; ++i) {
      unsigned offset = ((unsigned)u >> 23) & (unsigned)info->mask;
      offset |= (((unsigned)v & (unsigned)info->mask) >> 15);
      dest[i] = info->p_src[offset];
      u += du;
      v += info->dvx;
   }
}

void flat8_flat8_opaque_p2_wrap_il_chain_from_c(int negative_count,
                                                 fix u, fix v)
{
   g2s_tmap_info *info = &g2d_tmap_info;
   grs_bitmap *bm = info->bm;
   int count = -negative_count;
   uchar *dest = g2d_tmap_buffer + G2C_TMAP_BUFFER_SIZE - count;
   int i;

   for (i = 0; i < count; ++i) {
      int x = fix_int(u) & (bm->w - 1);
      int y = fix_int(v) & (bm->h - 1);
      dest[i] = bm->bits[y * bm->row + x];
      u += info->dux;
      v += info->dvx;
   }
}
