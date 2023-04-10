/*
 * $Source: x:/prj/tech/libsrc/star/RCS/star.c $
 * $Revision: 1.9 $
 * $Author: KEVIN $
 * $Date: 1998/04/01 14:49:38 $
 *
 * Main star library source
 *
 * $Log: star.c $
 * Revision 1.9  1998/04/01  14:49:38  KEVIN
 * more 16 bit support.
 * 
 * Revision 1.8  1998/02/18  15:57:28  KEVIN
 * added function to set pixel rendering function for next call to StarRender().
 * 
 * Revision 1.7  1997/11/04  16:22:14  KEVIN
 * 16 bit support.
 * 
 * Revision 1.6  1997/11/02  20:05:37  buzzard
 * anti-aliased stars cleanup
 * 
 * Revision 1.5  1997/10/26  21:01:54  dc
 * reform to make debugging easier
 * add check before free
 * 
 * Revision 1.4  1997/01/31  23:25:27  JAEMZ
 * Updated star library for the nineties
 * 
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

#include <stdlib.h>
#include <math.h>

#include <lg.h>
#include <dev2d.h>
#include <r3d.h>
#include <mprintf.h>

#include <star.h>

//#define  STAR_SPEW
// #define STARS_ANTI_ALIAS

// globals for state
// pointer to list of stars
// and color list of stars, and number
// of them
#define STAR_MAX_Z 1.0e16

mxs_vector  *std_vec;
uchar    *std_col;
int      std_num;
int   std_size = 1;

// The canvas must be more than <std_alias_size> pixels wide
// for us to anti-alias the stars (which makes them bigger,
// hence the size restriction)
int   std_alias_size = 16000;     
// This size is chosen so anti-aliasing starts happening
// in full screen 640x400 modes.  Won't happen in demo mode
// if demo mode uses a subcanvas

// We must record the meaning of the colors of stars so
// we can anti-alias them
int   std_color_dark, std_color_bright;

// gamma-correct star colors
uchar std_alias_color_table[256];

static float std_min_z;
static float std_max_rad2; // maximum radius squared

static BOOL check_bkgnd = TRUE;

// gpix_func is NULL when check_bkgnd is FALSE;
// i.e., when we want to skip the check.
static gdgpix_func *gpix_func;

// This is our unclipped pixel drawing function
static gdupix_func *pix_func = NULL;

// This is here because the 2d don't support this mode.
static void flat16_clut8_upix8(int c8, int x, int y)
{
   uchar *clut = (uchar *)(grd_gc.fill_parm);
   ushort *pal16 = grd_pal16_list[0];
   ushort *dst = (ushort *)(grd_bm.bits + 2*x + grd_bm.row * y);
   AssertMsg (pal16 != NULL, "NULL pal.");
   *dst = pal16[clut[c8]];
}

void StarSetPixFunc(gdupix_func *f)
{
   pix_func = f;
}

void StarSetCheckBkgnd(BOOL check)
{
   check_bkgnd = check;
}


// sets global pointers in the star library
// to the number of stars, their positions, their colors
void StarSet(int n,mxs_vector *vlist,uchar *clist)
{
   std_num = n;
   std_vec = vlist;
   std_col = clist;
}

// allocates the necessary space for stars using alloc
// returns -1 if problem.
int StarMalloc(int n)
{
   std_vec = (mxs_vector *)Malloc(n*sizeof(mxs_vector)+n);
   if (std_vec == NULL) return -1;
   std_col = (uchar *)(std_vec + n);
   std_num = n;
   return n;
}

// frees star space using free, only if you've used
// alloc to allocate it
void StarFree()
{
   if (std_vec) Free(std_vec);
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

#ifdef STAR_SPEW
extern   int   star_num_behind;
extern   int   star_num_projected;
#endif


// Note this messes up your context
void StarPoly(int n,r3s_phandle *vp)
{
   int i;
   fix m;
   r3s_phandle *p;

   // draw star poly in color zero.  This part very
   // important, if not zero, won't work.
   // vbuf is clipped poly.  So we just want to clip ourselves
   n = r3_clip_polygon(n,vp,&p);

   // Draw the black poly
   r3_set_polygon_context(R3_PL_POLYGON);
   r3_set_color(0);
   r3_draw_poly(n,p);

   // snag points that have been fully clipped and projected
   // out of the depths of insanity of the 3d
   for (i=0;i<n;++i)
   {
      if (p[i]->p.z < std_min_z) std_min_z = p[i]->p.z;
      m = r3_camera_mag2(p[i]);
      if (m > std_max_rad2) std_max_rad2 = m;
   }
}


// Just enters the list, you have to render the 
// background
void StarEmpty(int n,r3s_phandle *vp)
{
   int i;
   float m;
   r3s_phandle *dest;   // assume max clip 10

   // clip it just like you would when you render
   // then run through it
   n = r3_clip_polygon(n,vp,&dest);

   for (i=0;i<n;++i) {
      if (dest[i]->p.z < std_min_z) std_min_z = dest[i]->p.z;
      m = r3_camera_mag2(dest[i]);
      if (m > std_max_rad2) std_max_rad2 = m;
   }
}

// render stars to a sky-like thing,
// no viewports, clips to half sphere
void StarSky(void)
{
   std_min_z     = 0;
   std_max_rad2  = 1.0;
}

// render a single pixel of an anti-aliased star
void do_aa_star_pixel(int x, int y, int fx, int fy, int c)
{
  int q;
  
  // gpix_func is NULL when check_bkgnd is FALSE;
  // i.e., when we want to skip the check.
  if ((gpix_func == NULL) || (gd_gpix_opt(x,y,gpix_func) == 0))
  {
    q = fx * fy * c;

    // fx is % 0..256; fy is % 0..256; c is % 0..255.

    q = q >> 16;

    // q is now 0..255, which we rescale back into color range

    q = std_alias_color_table[q];
    if (q)
      gd_pix_opt(q,x,y,pix_func);
  }
}

static unsigned char effective_color[256];

// render an anti-aliased star.
// this is a prime candidate for being made table driven
void do_aa_star(fix fx, fix fy, int c)
{
  int color = effective_color[c];
  // ok, now compute the weightings for each pixel

  if (color > 4) {
     // isolate the fractions and the integers
     int x_frac = fix_frac(fx) >> 8;
     int y_frac = fix_frac(fy) >> 8;
     int x = fix_int(fx);
     int y = fix_int(fy);

     do_aa_star_pixel(x,y, 256-x_frac, 256-y_frac, color);
     do_aa_star_pixel(x+1,y, x_frac, 256-y_frac, color);
     do_aa_star_pixel(x,y+1,256-x_frac, y_frac, color);
     do_aa_star_pixel(x+1,y+1, x_frac, y_frac, color);
  } else {
     int x = fix_rint(fx);
     int y = fix_rint(fy);
     if ((gpix_func == NULL) || (gd_gpix_opt(x,y,gpix_func) == 0))
       gd_pix_opt(c,x,y,pix_func);
  }
}

double star_gamma = .45776;

static int compute_brightness(int n)
{
   int i;
   for (i=255; i > 0; --i)
      if (std_alias_color_table[i] == n)
         return i;
   return 0;
}

void star_init_alias_table(void)
{
   // init gamma corrected table
   float b;
   int a,i;

   if (std_color_dark < std_color_bright) {
          // set a to the "black" color
      a = std_color_dark-1; 
          // set b to the number of colors in the bank + black
      b = (std_color_bright - std_color_dark) + 2;
   } else {
      a = std_color_dark+1;   // "black"
      b = (std_color_bright - std_color_dark) - 2;
   }

   for (i=0; i < 256; ++i) {
      int c = floor(b * pow((float)i/256.0,star_gamma));
      if (c != 0)
         std_alias_color_table[i] = a + c;
      else
         std_alias_color_table[i] = 0;
   }

  // remap input colors into the value 0..255

  i = std_color_dark;
  goto middle;
  do {
     i += std_color_dark < std_color_bright ? 1 : -1;
    middle:
     effective_color[i] = compute_brightness(i);
  } while (i != std_color_bright);
}


// In this case, the user is responsible for setting the position
// and orientation of the stars.  At the very least, you NEED to put
// them at the viewer position, at least until we have our optimized
// star transformer.  But that's easily gotten from the r3d
// Must be in a block
void StarRender(void)
{
   int i;
   r3s_point s;
   int x,y;
   int x1,y1;
   float old_near_z;
   int anti_alias = grd_bm.w >= std_alias_size;

   if (check_bkgnd)
      gpix_func = gd_ugpix8_expose(0,0);
   else
      gpix_func = NULL;

   if (pix_func == NULL) {
      if ((grd_bpp>8)&&(gr_get_fill_type() == FILL_CLUT))
         pix_func = flat16_clut8_upix8;
      else
         pix_func = gd_upix8_expose(0,0,0);
   }

   if (grd_bpp>8)
      pixpal = (void *)grd_pal16_list[0];

   #ifdef STAR_SPEW
   star_num_behind = 0;
   star_num_projected = 0;
   #endif

   // exit if no one ever drew a star field anywhere visible
   // This is biggest possible number
   if (std_min_z == STAR_MAX_Z) {
      #ifdef STAR_SPEW
      mprintf("ignored\n");
      #endif
      return;
   }

   #ifdef STAR_SPEW
   mprintf("max_rad = %f min_z = %f\n",sqrt(star_max_rad2),star_min_z);
   #endif

   // Can this ever happen?
   if (std_min_z < 0) std_min_z = 0;

   // in actuality, this is identical to pushing forward plane
   // forward to this value and going from there... hmmmm.
   // hmmm... move plane forward

   std_min_z /= sqrt(std_max_rad2);

   old_near_z = r3_get_near_plane();
   r3_set_near_plane(std_min_z);

   for (i=0;i<std_num;++i) {
      // in theory if codes aren't set it's on the screen

      // unpack star vec to a normal vec
      // sometimes you need to transform to clip code...
      r3_transform_point(&s,&std_vec[i]);
 
      // If on screen and z is big enough go for it
      if ((s.p.z > std_min_z) && (s.ccodes == 0)) {
         if (std_size <= 1)
         {
            if (anti_alias)
               do_aa_star(s.grp.sx,s.grp.sy,std_col[i]);
            else {
               int x = fix_rint(s.grp.sx);
               int y = fix_rint(s.grp.sy);
               if ((gpix_func == NULL) || (gd_gpix_opt(x,y,gpix_func) == 0))
                  gd_pix_opt(std_col[i],x,y,pix_func);
            }
         }
         else
         {
            x = fix_rint(s.grp.sx);
            y = fix_rint(s.grp.sy);
            for (x1=x;x1<x+std_size;++x1)
               for (y1=y;y1<y+std_size;++y1)
                  if ((gpix_func == NULL) || (gd_gpix_opt(x,y,gpix_func) == 0))
                     gd_pix_opt(std_col[i],x,y,pix_func);
         }

      }
   }

   #ifdef STAR_SPEW
   mprintf("stars = %d behind = %d proj = %d\n",st_num,star_num_behind,star_num_projected);
   #endif

   // reset min z and max rad
   // make this very big
   // very very big
   std_min_z     = STAR_MAX_Z;
   std_max_rad2  = 0;

   r3_set_near_plane(old_near_z);

   pix_func = NULL;
}


// Not be in a block
void StarRenderSimple()
{
   mxs_vector view;
   mxs_vector zero = {0,0,0};

   view = *r3_get_view_pos();

   r3_set_view(&zero);

   r3_start_block();
   StarRender();
   r3_end_block();

   r3_set_view(&view);
}

// Base is the black color, range includes
// the white color
// Also, set the canvas width size you need to be
// to use anti-aliasing
void StarSetAntiAlias(int dark_col,int bright_col,int wsize)
{
   std_color_dark = dark_col; // col;
   std_color_bright = bright_col; // range;
   std_alias_size = wsize; // size of canvas to exceed
   star_init_alias_table();
}


// stuffs random vectors and colors into the set areas
// randomly assigning a color range to them
// feel free to seed
void StarRand(uchar col,uchar range)
{
   int i;
   mxs_vector v;
   mxs_vector *s;

   for (i=0;i<std_num;++i) {
      s = &std_vec[i];

      v.x = (rand()%4000 - 2000);
      v.y = (rand()%4000 - 2000);
      v.z = (rand()%4000 - 2000);

      mx_norm_vec(s,&v);

      // assign color
      std_col[i] = rand()%range + col;

   }
}
