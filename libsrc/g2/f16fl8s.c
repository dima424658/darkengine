/*
 * $Source: s:/prj/tech/libsrc/g2/RCS/f16fl8s.c $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/05/20 10:38:50 $
 *
 * flat8 canvas, flat8 bitmap scalers.
 *
 * This file is part of the g2 library.
 *
 */

#include <tmapd.h>
#include <ilfunc.h>
#include <scale.h>

extern void flat16_flat8_trans_np2_nowrap_il();
void flat16_flat8_norm_trans_uscale(grs_bitmap *bm, int x, int y, int w, int h)
{
   g2s_tmap_info *ti;

   ti = &g2d_tmap_info;
   ti->bm = bm;
   ti->drow = grd_bm.row;
   ti->il_func = flat16_flat8_trans_np2_nowrap_il;
   uscale_shell(x, y, w, h);
}

int flat16_flat8_norm_trans_scale(grs_bitmap *bm, int x, int y, int w, int h)
{
   g2s_tmap_info *ti;

   ti = &g2d_tmap_info;
   ti->bm = bm;
   ti->drow = grd_bm.row;
   ti->il_func = flat16_flat8_trans_np2_nowrap_il;
   return scale_shell(x, y, w, h);
}

extern void flat16_flat8_opaque_np2_nowrap_il();
void flat16_flat8_norm_opaque_uscale(grs_bitmap *bm, int x, int y, int w, int h)
{
   g2s_tmap_info *ti;

   ti = &g2d_tmap_info;
   ti->bm = bm;
   ti->drow = grd_bm.row;
   ti->il_func = flat16_flat8_opaque_np2_nowrap_il;
   uscale_shell(x, y, w, h);
}

int flat16_flat8_norm_opaque_scale(grs_bitmap *bm, int x, int y, int w, int h)
{
   g2s_tmap_info *ti;

   ti = &g2d_tmap_info;
   ti->bm = bm;
   ti->drow = grd_bm.row;
   ti->il_func = flat16_flat8_opaque_np2_nowrap_il;
   return scale_shell(x, y, w, h);
}

extern uchar *g2d_tnni_clut[];
extern void flat16_flat8_clut_trans_np2_nowrap_il();
void flat16_flat8_clut_trans_uscale(grs_bitmap *bm, int x, int y, int w, int h)
{
   g2s_tmap_info *ti;

   ti = &g2d_tmap_info;
   ti->bm = bm;
   ti->drow = grd_bm.row;
   g2d_tnni_clut[-1] = (uchar *)grd_gc.fill_parm;
   ti->il_func = flat16_flat8_clut_trans_np2_nowrap_il;
   uscale_shell(x, y, w, h);
}

int flat16_flat8_clut_trans_scale(grs_bitmap *bm, int x, int y, int w, int h)
{
   g2s_tmap_info *ti;

   ti = &g2d_tmap_info;
   ti->bm = bm;
   ti->drow = grd_bm.row;
   g2d_tnni_clut[-1] = (uchar *)grd_gc.fill_parm;
   ti->il_func = flat16_flat8_clut_trans_np2_nowrap_il;
   return scale_shell(x, y, w, h);
}

extern uchar *g2d_onni_clut[];
extern void flat16_flat8_clut_opaque_np2_nowrap_il();
void flat16_flat8_clut_opaque_uscale(grs_bitmap *bm, int x, int y, int w, int h)
{
   g2s_tmap_info *ti;

   ti = &g2d_tmap_info;
   ti->bm = bm;
   ti->drow = grd_bm.row;
   g2d_onni_clut[-1] = (uchar *)grd_gc.fill_parm;
   ti->il_func = flat16_flat8_clut_opaque_np2_nowrap_il;
   uscale_shell(x, y, w, h);
}

int flat16_flat8_clut_opaque_scale(grs_bitmap *bm, int x, int y, int w, int h)
{
   g2s_tmap_info *ti;

   ti = &g2d_tmap_info;
   ti->bm = bm;
   ti->drow = grd_bm.row;
   g2d_onni_clut[-1] = (uchar *)grd_gc.fill_parm;
   ti->il_func = flat16_flat8_clut_opaque_np2_nowrap_il;
   return scale_shell(x, y, w, h);
}