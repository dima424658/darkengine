/*
 * $Source: x:/prj/tech/libsrc/star/RCS/star.c $
 * $Revision: 1.3 $
 * $Author: JAEMZ $
 * $Date: 1995/12/29 13:55:07 $
 *
 * Main star library source
 *
 * $Log: star.c $
 * Revision 1.3  1995/12/29  13:55:07  JAEMZ
 * Stars obey fix mode
 * 
 * Revision 1.2  1994/11/11  19:51:04  buzzard
 * Anti/aliasing stars in high resolutions.
 * Ugly hacked for system shock rather than changing interface.
 * 
 * Revision 1.1  1994/10/24  23:27:31  jaemz
 * Initial revision
 * 
 * Revision 1.2  1994/10/21  16:45:47  jaemz
 * *** empty log message ***
 * 
 * Revision 1.1  1994/09/07  17:41:29  jaemz
 * Initial revision
 * 
*/

#include <2d.h>
#include <3d.h>
#include <stdlib.h>
#include <mprintf.h>
#include "star.h"

//#define  STAR_SPEW
#define STARS_ANTI_ALIAS

#ifdef STEREO_ON
extern bool g3d_stereo;
extern fix g3d_eyesep_raw;
extern uchar *g3d_rt_canv_bits;
extern uchar *g3d_lt_canv_bits;
#endif

typedef struct sts_vec
{
    ushort x, y, z;
} sts_vec;

// globals for state
sts_vec  *std_vec;
uchar    *std_col;
int      std_num;


int   std_size = 1;

#ifdef STARS_ANTI_ALIAS
// The canvas must be more than <std_alias_size> pixels wide
// for us to anti-alias the stars (which makes them bigger,
// hence the size restriction)
int   std_alias_size = 640;     
// This size is chosen so anti-aliasing starts happening
// in full screen 640x400 modes.  Won't happen in demo mode
// if demo mode uses a subcanvas

// We must record the meaning of the colors of stars so
// we can anti-alias them
int   std_color_base, std_color_range;

// gamma-correct star colors
uchar std_alias_color_table[256];
#endif

extern   fix   std_min_z;
extern   fix   std_max_rad;

extern   g3s_vector matrix_scale;
extern   g3s_phandle vbuf2;
extern   int   n_verts;

// sets global pointers in the star library
// to the number of stars, their positions, their colors
void star_set(int n,sts_vec *vlist,uchar *clist)
{
   std_num = n;
   std_vec = vlist;
   std_col = clist;
}

// allocates the necessary space for stars using alloc
int star_alloc(int n)
{
   std_vec = (sts_vec *)malloc(n*sizeof(sts_vec)+n);
   if (std_vec == NULL) return -1;
   std_col = (uchar *)(std_vec + n);
   std_num = n;
   return n;
}

// frees star space using free, only if you've used
// alloc to allocate it
void star_free()
{
   free(std_vec);
}


// renders star field in the polygon defined by the vertex list
// uses your 3d context, so make sure that's been set
// what we do is render a zero polygon, which is black,
// then render stars on that field.  Beware of CLUT modes or
// other FILL modes.  In general, will hurt this and we'll
// have to code around it.
// Then we rotate star field around viewer and draw them
// wherever there is black.   Not dissimilar to Kevins
// per tmap hack

extern g3s_vector view_position;

#ifdef STAR_SPEW
extern   int   star_num_behind;
extern   int   star_num_projected;
#endif

// for the love of god, I hate 3d scaling.
fix mag2_point(g3s_phandle p)
{
   fix a,f;

   a = fix_div(p->x,matrix_scale.x);
   f = fix_mul(a,a);

   a = fix_div(p->y,matrix_scale.y);
   f += fix_mul(a,a);

   a = fix_div(p->z,matrix_scale.z);
   f += fix_mul(a,a);

   return f;
}

// in stereo mode, we can leave this normal
// and it should work, assuming the two
// polygons are similar enough
void star_poly(int n,g3s_phandle *vp)
{
   int i;
   fix m;
   g3s_phandle *p;
   int f;

   // draw star poly in color zero.  This part very
   // important, if not zero, won't work.
   f = g3_draw_poly(0,n,vp);

   // snag points that have been fully clipped and projected
   // out of the depths of insanity of the 3d
   if (f!=CLIP_ALL)
   {
      p = &vbuf2;
      for (i=0;i<n_verts;++i)
      {
         if (p[i]->z < std_min_z) std_min_z = p[i]->z;
         m = mag2_point(p[i]);
         if (m > std_max_rad) std_max_rad = m;
      }
   }
}


void star_empty(int n,g3s_phandle *vp)
{
   int i;
   fix m;
   int n1;
   g3s_phandle dest[10];   // assume max clip 10

   // clip it just like you would when you render
   // then run through it
   // set max out the maximum it could be, which would be eyesep raw
   n1 = g3_clip_polygon(n,vp,dest);
   for (i=0;i<n1;++i) {
      if (dest[i]->z < std_min_z) std_min_z = dest[i]->z;
      m = mag2_point(dest[i]);
      if (m > std_max_rad) std_max_rad = m;
   }
}

// render stars to a sky-like thing,
// no viewports, clips to half sphere
void star_sky(void)
{
   std_min_z     = 0;
   std_max_rad   = FIX_UNIT;
}

#ifdef STARS_ANTI_ALIAS
// render a single pixel of an anti-aliased star
void do_aa_star_pixel(int x, int y, int fx, int fy, int c)
{
  int q; 
  
  if (gr_get_pixel(x,y) == 0) {
    q = fx * fy * c;

    // fx is % 0..256; fy is % 0..256; c is % 0..255.

    q = q >> 16;

    // q is now 0..255, which we rescale back into color range

    gr_fill_pixel(std_alias_color_table[q], x, y);
  }
}

// render an anti-aliased star.
// this is a prime candidate for being made table driven
void do_aa_star(fix fx, fix fy, int c)
{
  // isolate the fractions and the integers
  int x_frac = fix_frac(fx) >> 8;
  int y_frac = fix_frac(fy) >> 8;
  int x = fix_int(fx);
  int y = fix_int(fy);

  int color = (std_color_base + std_color_range - 1 - c);

  // rescale the color so that it's 0..255, 0 = dark, 255 = light
  color = (255 * color) / (std_color_range+1);

  // ok, now compute the weightings for each pixel

  do_aa_star_pixel(x,y, 256-x_frac, 256-y_frac, color);
  do_aa_star_pixel(x+1,y, x_frac, 256-y_frac, color);
  do_aa_star_pixel(x,y+1,256-x_frac, y_frac, color);
  do_aa_star_pixel(x+1,y+1, x_frac, y_frac, color);
}

void star_init_alias_table(void)
{
  // init gamma corrected table
  int i,a;
  fix b,gamma;

  gamma = fix_make(0, 30000);

  a = std_color_base + std_color_range - 1;
  b = fix_make(-(std_color_range-1),0);

  for (i=0; i < 256; ++i)
    std_alias_color_table[i] = 
      a + fix_int(fix_mul(b, fix_pow(fix_make(i,0)/255, gamma)));
}

#endif

void star_render()
{
   int i;
   g3s_phandle s;
   int x,y;
   int x1,y1;
   g3s_vector v;
   #ifdef STEREO_ON
   bool  old_stereo;
   #endif
   #ifdef STARS_ANTI_ALIAS
   int anti_alias = grd_bm.w >= std_alias_size;
   #endif

   #ifdef STAR_SPEW
   star_num_behind = 0;
   star_num_projected = 0;
   #endif

   #if defined(STARS_ANTI_ALIAS) && defined(STEREO_ON)
   if (g3d_stereo) anti_alias = 0;
   #endif

   // exit if no one every drew a star field anywhere visible
   if (std_min_z == 0x7fffffff) {
      #ifdef STAR_SPEW
      mprintf("ignored\n");
      #endif
      return;
   }

   #ifdef STAR_SPEW
   mprintf("max_rad = %x min_z = %x\n",fix_sqrt(star_max_rad),star_min_z);
   #endif
   if (std_min_z < 0) std_min_z = 0;

   // scale by max radius
   #ifndef STEREO_ON
   g3_scale_object(fix_sqrt(std_max_rad));
   #else
   // add in eyesep raw cause that's as much bigger it could be
   g3_scale_object(fix_sqrt(std_max_rad) + (g3d_stereo?g3d_eyesep_raw:0));
   #endif

   #ifdef STEREO_ON
   old_stereo = g3d_stereo;
   g3d_stereo = 0;
   #endif

   for (i=0;i<std_num;++i) {
      // in theory if codes aren't set it's on the screen

      // unpack star vec to a normal vec
      v.x = ((fix)std_vec[i].x)<<1;
      v.y = ((fix)std_vec[i].y)<<1;
      v.z = ((fix)std_vec[i].z)<<1;

      s = star_transform_point(&v);
 
      if (s->codes == 0) {
         x = fix_rint(s->sx);
         y = fix_rint(s->sy);
         if (std_size <= 1) {
         #ifdef STARS_ANTI_ALIAS
            if (anti_alias) {
              do_aa_star(s->sx,s->sy,std_col[i]);
            } else
         #endif
            if (gr_get_pixel(x,y) == 0) gr_fill_pixel(std_col[i],x,y);
         } else {
            for (x1=x;x1<x+std_size;++x1) {
               for (y1=y;y1<y+std_size;++y1) {
                  if (gr_get_pixel(x1,y1) == 0) gr_fill_pixel(std_col[i],x1,y1);
               }
            }
         }

         #ifdef STEREO_ON
         if (old_stereo) {
            // switch canvases quickly
            grd_bm.bits = g3d_rt_canv_bits;
            if (std_size <= 1) {
               if (gr_get_pixel(x,y) == 0) gr_fill_pixel(std_col[i],x,y);
            } else {
               for (x1=x;x1<x+std_size;++x1) {
                  for (y1=y;y1<y+std_size;++y1) {
                     if (gr_get_pixel(x1,y1) == 0) gr_fill_pixel(std_col[i],x1,y1);
                  }
               }
            }
            // switch back
            grd_bm.bits = g3d_lt_canv_bits;
         }
         #endif

      }
      g3_free_point(s);
   }

   #ifdef STEREO_ON
   g3d_stereo = old_stereo;
   #endif

   #ifdef STAR_SPEW
   mprintf("stars = %d behind = %d proj = %d\n",st_num,star_num_behind,star_num_projected);
   #endif

   // reset min z and max rad
   std_min_z     = 0x7fffffff;
   std_max_rad   = 0;
                        
}




// stuffs random vectors and colors into the set areas
// randomly assigning a color range to them
// feel free to seed
void star_rand(uchar col,uchar range)
{
   int i;
   g3s_vector v;
   sts_vec *s;
   fix m;

   #ifdef STARS_ANTI_ALIAS
     // SYSTEM SHOCK HACK!
   std_color_base = 208; // col;
   std_color_range = 16; // range;

   star_init_alias_table();
   #endif

   for (i=0;i<std_num;++i) {
      s = &std_vec[i];

      v.x = (rand()%4000 - 2000) << 8;
      v.y = (rand()%4000 - 2000) << 8;
      v.z = (rand()%4000 - 2000) << 8;

      m = fix_mul(v.x,v.x)
         + fix_mul(v.y,v.y)
         + fix_mul(v.z,v.z);

      if (m<FIX_UNIT/100) {
         i = i-1;
         continue;
      }

      m = fix_sqrt(m);

      // normalize for fun hack
      v.x = fix_div(v.x,m);
      v.y = fix_div(v.y,m);
      v.z = fix_div(v.z,m);

      // put into star vec
      s->x = (v.x>>1);
      s->y = (v.y>>1);
      s->z = (v.z>>1);

      // assign color
      std_col[i] = rand()%range + col;

   }
}








              
