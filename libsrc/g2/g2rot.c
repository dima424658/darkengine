#include <grtm.h>

// Note that these describe clockwise rotations.
int gr_rotate_bitmap(grs_bitmap *bm, fixang theta, fix x, fix y)
{
   grs_vertex vl[4];
   grs_vertex *vpl[4];
   fix wct, hct, wst, hst;
   {
      int h=bm->h, w=bm->w;
      wct = fix_cos(theta);
      wst = fix_sin(theta);
      hct = (h*wct)>>1;
      hst = (h*wst)>>1;
      wct = (w*wct)>>1;
      wst = (w*wst)>>1;
   }
   vpl[0]=vl, vpl[1]=vl+1, vpl[2]=vl+2, vpl[3]=vl+3;
   vl[0].x = x - wct + hst;         vl[0].y = y - hct - wst;
   vl[1].x = x + wct + hst;         vl[1].y = y - hct + wst;
   vl[2].x = x + wct - hst;         vl[2].y = y + hct + wst;
   vl[3].x = x - wct - hst;         vl[3].y = y + hct - wst;
   vl[0].u = fix_make(0, 0);        vl[0].v = fix_make(0, 0);
   vl[1].u = fix_make(bm->w, 0)-1;  vl[1].v = fix_make(0, 0);
   vl[2].u = fix_make(bm->w, 0)-1;  vl[2].v = fix_make(bm->h, 0)-1;
   vl[3].u = fix_make(0, 0);        vl[3].v = fix_make(bm->h, 0)-1;
   return gr_lin_map(bm, 4, vpl);
}


int gr_rotate_bitmap_90(grs_bitmap *bm, int x, int y)
{
   grs_vertex vl[4];
   grs_vertex *vpl[4];
   vpl[0]=vl, vpl[1]=vl+1, vpl[2]=vl+2, vpl[3]=vl+3;
   vl[0].x = fix_make(x,0);         vl[0].y = fix_make(y,0);
   vl[1].x = fix_make(x+bm->h,0)-100; vl[1].y = fix_make(y,0);
   vl[2].x = fix_make(x+bm->h,0)-100; vl[2].y = fix_make(y+bm->w,0)-100;
   vl[3].x = fix_make(x,0);         vl[3].y = fix_make(y+bm->w,0)-100;
   vl[0].u = fix_make(0, 0);        vl[0].v = fix_make(bm->h, 0)-1;
   vl[1].u = fix_make(0, 0);        vl[1].v = fix_make(0, 0);
   vl[2].u = fix_make(bm->w, 0)-1;  vl[2].v = fix_make(0, 0);
   vl[3].u = fix_make(bm->w, 0)-1;  vl[3].v = fix_make(bm->h, 0)-1;
   return gr_lin_map(bm, 4, vpl);
}

int gr_rotate_bitmap_180(grs_bitmap *bm, int x, int y)
{
   grs_vertex vl[4];
   grs_vertex *vpl[4];
   vpl[0]=vl, vpl[1]=vl+1, vpl[2]=vl+2, vpl[3]=vl+3;
   vl[0].x = fix_make(x,0);         vl[0].y = fix_make(y,0);
   vl[1].x = fix_make(x+bm->w,0)-100;   vl[1].y = fix_make(y,0);
   vl[2].x = fix_make(x+bm->w,0)-100;   vl[2].y = fix_make(y+bm->h,0)-100;
   vl[3].x = fix_make(x,0);         vl[3].y = fix_make(y+bm->h,0)-100;
   vl[0].u = fix_make(bm->w, 0)-1;  vl[0].v = fix_make(bm->h, 0)-1;
   vl[1].u = fix_make(0, 0);        vl[1].v = fix_make(bm->h, 0)-1;
   vl[2].u = fix_make(0, 0);        vl[2].v = fix_make(0, 0);
   vl[3].u = fix_make(bm->w, 0)-1;  vl[3].v = fix_make(0, 0);
   return gr_lin_map(bm, 4, vpl);
}

int gr_rotate_bitmap_270(grs_bitmap *bm, int x, int y)
{
   grs_vertex vl[4];
   grs_vertex *vpl[4];
   vpl[0]=vl, vpl[1]=vl+1, vpl[2]=vl+2, vpl[3]=vl+3;
   vl[0].x = fix_make(x,0);         vl[0].y = fix_make(y,0);
   vl[1].x = fix_make(x+bm->h,0)-100; vl[1].y = fix_make(y,0);
   vl[2].x = fix_make(x+bm->h,0)-100; vl[2].y = fix_make(y+bm->w,0)-100;
   vl[3].x = fix_make(x,0);         vl[3].y = fix_make(y+bm->w,0)-100;
   vl[0].u = fix_make(bm->w, 0)-1;  vl[0].v = fix_make(0, 0);
   vl[1].u = fix_make(bm->w, 0)-1;  vl[1].v = fix_make(bm->h, 0)-1;
   vl[2].u = fix_make(0, 0);        vl[2].v = fix_make(bm->h, 0)-1;
   vl[3].u = fix_make(0, 0);        vl[3].v = fix_make(0, 0);
   return gr_lin_map(bm, 4, vpl);
}
