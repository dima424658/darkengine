/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/mipmap.c,v 1.10 2000/02/19 12:35:28 toml Exp $

// dynamically create mipmap levels

#include <string.h>

#include <lg.h>
#include <dev2d.h>
#include <config.h>

#ifdef USE_MEM_RECT
#include <portal.h>
#endif

#include <mipmap.h>
#include <palmgr.h>

#include <mprintf.h>

static uchar *tmap_ipal;
static uchar *tmap_pal;

bool mipmap_fix_phase = FALSE;
bool mipmap_dither = TRUE;
bool mipmap_enable = TRUE;

#define LIGHT_MAP

void mm_copy_bitmap_to_texture(uchar *dest, grs_bitmap *src)
{
   int i,drow;

   drow = src->w;

   for (i=0; i < src->h; ++i)
      memcpy(dest + i * drow, src->bits + i * src->row, src->w);
}

void mm_init_bitmap(grs_bitmap *dest, grs_bitmap *src, int w, int h)
{
   gr_init_bitmap(dest, dest->bits, src->type, src->flags, w, h);
   dest->align=src->align;
}

#define mm_index(x,y,row)  ((y)*(row) + (x))


//  compute weighted rgb values, and turn into palettes

#define init_rgb_sum()     int r_=0, g_=0, b_=0, n_=0

#define add_element_raw(bm, x, y, wt)                     \
           { int temp_ = (bm)->bits[mm_index((x),(y),bm->row)]*3; \
             r_ += tmap_pal[temp_+0] * (wt);               \
             g_ += tmap_pal[temp_+1] * (wt);               \
             b_ += tmap_pal[temp_+2] * (wt);               \
             n_ += (wt);                                  \
           }

#define add_element(bm, x, y)            add_element_raw(bm, x, y, 1)
#define add_element_weight(bm, x, y, w)  add_element_raw(bm, x, y, w)

#define compute_rgb_average() compute_rgb_average_raw(r_,g_,b_,n_)

int r_err, g_err, b_err;

unsigned char compute_rgb_average_raw(int r, int g, int b, int n)
{
   int c;
   
   r /= n; r += r_err; if (r < 0) r = 0; else if (r > 255) r = 255;
   g /= n; g += g_err; if (g < 0) g = 0; else if (g > 255) g = 255;
   b /= n; b += b_err; if (b < 0) b = 0; else if (b > 255) b = 255;

   c = tmap_ipal[gr_index_rgb(fix_make(r,0),fix_make(g,0),fix_make(b,0))];
   
   if (mipmap_dither) {
      r_err = r - tmap_pal[c*3+0];
      g_err = g - tmap_pal[c*3+1];
      b_err = b - tmap_pal[c*3+2];
   }

   return c;
}

//  note that compute_rgb_average doesn't actually clear the values,
//  which means we can incrementally update a box filter

  // downsample a bitmap by a factor of 2
void mm_downsample(grs_bitmap *dest, grs_bitmap *src)
{
   //int w = src->w, h = src->h,x,y;
   int w = src->w/2, h = src->h/2,x,y;

   mm_init_bitmap(dest, src, w, h);

   // iterate over the destination pixels
   for (y=0; y < h; ++y) {
      r_err = g_err = b_err = 0;
      for (x=0; x < w; ++x) {
         init_rgb_sum();
         add_element(src, x*2, y*2);
         add_element(src, x*2+1, y*2);
         add_element(src, x*2, y*2+1);
         add_element(src, x*2+1, y*2+1);
         dest->bits[mm_index(x,y,dest->row)] = compute_rgb_average();
      }
   }
}

  // filter a texture
void mm_filter_downsample(grs_bitmap *dest, grs_bitmap *src,int width,int skip)
{
   int w = src->w/skip, h = src->h/skip;
   int w_mask = (src->w) - 1;
   int h_mask = (src->h) - 1;

   r_err = g_err = b_err = 0;

   mm_init_bitmap(dest, src, w, h);

   if (!mipmap_fix_phase) {
      int x,y;
      for (y=0; y < h; ++y) {
         int x2,y2;
         init_rgb_sum();

         // go total up our rectangle
         for (y2=y*skip; y2 < y*skip+width; ++y2)
            for (x2 = 0; x2 < width; ++x2)
               add_element(src, x2, y2 & h_mask);
                               // total executions: width*width*dest_h

         for (x=0; x < w; ++x) {
#if 0
            init_rgb_sum();

            // go total up our rectangle
            for (y2=y*skip; y2 < y*skip+width; ++y2)
               for (x2 = x*skip; x2 < x*skip+width; ++x2)
                  add_element(src, x2 & w_mask, y2 & h_mask);
#endif
            dest->bits[mm_index(x,y,dest->row)] = compute_rgb_average();
#if 1

               // remove the trailing edge
            for (x2 = x*skip; x2 < x*skip+skip; ++x2)
               for (y2 = y*skip; y2 < y*skip+width; ++y2)
                  add_element_weight(src, x2 & w_mask, y2 & h_mask,-1);
                               // width*w*dest_h, w = dest_w * skip

               // add the leading edge
            for (x2 = (x)*skip+width; x2 < (x+1)*skip+width; ++x2)
               for (y2 = y*skip; y2 < y*skip+width; ++y2)
                  add_element_weight(src, x2 & w_mask, y2 & h_mask, 1);
                               // width*w*dest_h, w = dest_w * skip
#endif
         }
      }
   } else {
      // if fix phase is true, then we want a half pixel from the edges,
      // and a quarter pixel from the corners.  Yuck!

      // In other words, output pixel 0,0 normally comes from
      // (0..width-1, 0..width-1)

      // with fix phase, then it uses the following weightings:
      // 1/4 * ([-1,-1] + [-1,width-1] + [width-1, -1] + [width-1, width-1])
      // 1/2 * ([-1,0..width-2] + [0..width-2,-1] + [width-1,0..width-2] +
      //                    [0..width-2,width-1]
      //   1 * ([0..width-2,0..width-2])

      // This makes a total of:  1/4 * 4,
      //                         1/2 * (width-1) * 4
      //                           1 * (width-1) * (width-1)
      //           or  1 + 2 * (width-1) + (width-1 * width-1)
      //           or  1 + 2*width - 2 + width*width - 2*width + 1
      //           or  1+1-2 + 2*width-2*width + width*width
   }
}

// used by family_cvrt in the texture family loading code
// takes a grs_bitmap sort of texture and puts it into portals mem mapping
r3s_texture create_mipmapped_texture(grs_bitmap *src_orig)
{
   // compute # of mipmap levels
   r3s_texture tmap;
   int n, mn;
   int w,h,total,i;
   uchar *bits;
   grs_bitmap temp, *src = &temp;

   *src = *src_orig;

   n = 0;

   mn = src->w / 16;

   tmap_pal  = palmgr_get_pal(src->align);
   tmap_ipal = palmgr_get_ipal(src->align);

   if (mipmap_enable && tmap_ipal && !(src->flags & BMF_TRANS)) {
#ifdef LIGHT_MAP
      while (src->w > (4 << n) && src->h > (4 << n))
#else
      while (src->w > (1 << n) && src->h > (1 << n))
#endif
         ++n;
   }

   // n is max (0 means 1 level)

   ++n;

   // n is number of levels

   // allocate enough memory for n+1 levels

   total = 0;

   // iterate through all levels and count up storage
   w = src->w;
   h = src->h;
   for (i=0; i < n; ++i) {
      total += w*h;
      w >>= 1;
      h >>= 1;
   }

   tmap = Malloc(sizeof(grs_bitmap) * (n+1) + total);

   // distribute the memory for all the levels
   bits = ((uchar *) tmap + sizeof(grs_bitmap) * (n+1));
   w = src->w;
   h = src->h;
   for (i=0; i < n; ++i) {
      tmap[i].bits = bits;
      bits += w*h;
      w >>= 1;
      h >>= 1;
   }
   tmap[n].bits = NULL;

   // set up the first level

   gr_init_bitmap(&tmap[0], tmap[0].bits, src->type, src->flags, src->w, src->h);
   mm_copy_bitmap_to_texture(tmap[0].bits, src);

   tmap[0].align=src->align;

   // set up the last level

   tmap[n].w = 0;

   // now compute the mipmaps

   for (i=1; i < n; ++i) {
      //mm_filter_downsample(&tmap[i], &tmap[0], 1 << i, 1 << i);
      mm_downsample(&tmap[i], &tmap[i-1]);
   }

   return tmap;
}

extern BOOL g_lgd3d;
#include <lgd3d.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

void free_mipmapped_texture(r3s_texture dest, bool free_rect)
{
   r3s_texture temp;

   if (g_lgd3d) {
      temp = dest;
      while (temp->w) {
         lgd3d_unload_texture(temp);
         ++temp;
      }
   }

   Free(dest);
}
