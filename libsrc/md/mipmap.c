/*
 * $Source: x:/prj/tech/libsrc/md/RCS/mipmap.c $
 * $Revision: 1.13 $
 * $Author: JAEMZ $
 * $Date: 1998/06/16 13:42:35 $
 *
 * Mip mapping routines for models 
 *
 */

#include <string.h>

#include <lg.h>
#include <grspoint.h>
#include <dev2d.h>
#include <r3d.h>
#include <mdutil.h>
#include <math.h>

static uchar closest_col(float r,float g,float b,uchar *pal)
{
   int j;
   double best_dist = 256*256*3; // bigger than biggest possible
   double dist;
   int best=0;
   for (j=0; j<256; ++j)
   {
      dist = (double)(r-pal[j*3])*(r-pal[j*3]) + (double)(g-pal[j*3+1])*(g-pal[j*3+1]) + (double)(b-pal[j*3+2])*(b-pal[j*3+2]);
      if (dist<best_dist)
      {
         // return if perfect match
         if (dist==0) return j;
         best=j;
         best_dist=dist;
      }
   }
   return best;
}


// Returns size of a mipmap
int md_sizeof_mipmap(r3s_texture mmap)
{
   int size = 0;

   while(1) {
      size += sizeof(grs_bitmap) + mmap->row * mmap->h;
      if ((mmap->w==1) && (mmap->h==1)) return size;
      mmap++;
   }
}

//----- (009174DD) --------------------------------------------------------
int md_sizeof_mipmap_bits(grs_bitmap *mmap)
{
   int size = 0;
   while (TRUE)
   {
      size += mmap->h * mmap->row;
      if ( mmap->w == 1 && mmap->h == 1 )
         break;
      ++mmap;
   }

   return size;
}

// Down samples a source bitmap image into a collection of them.
// Pass it a single bitmap
// it passes you back a multimap from it.

// Down samples an 8 or 16 bit texture into another one,
// brute force each time
// source and destinations better be multiples of each other
// for 8 bit it does a diffusion dither across each scan line
// which helps prevent big blocks of color
// Not that 16 bit is assumed to be 565 and doesn't check

// Use structure to make this easier
typedef struct _rgb565
{
   ushort r :5;
   ushort g :6;
   ushort b :5;
} _rgb565;


void md_mipmap_downsamp(uchar *dstbit,int dw,int dh,uchar *srcbit,int sw,int sh,uchar *pal,int type)
{
   int i,j; // dest coords
   int x,y; // src coords
   float r,g,b;  // guess
   int col;
   uchar *srcptr;
   int area,hscale,wscale;
   int area_shift,tmp;
   bool use_ipal;

   // Optimize the slowest case AND prevent accidental
   // remapping of the source
   if (sh==dh && sw==dw) {
      memcpy(dstbit,srcbit,(type==BMT_FLAT8)?dh*dw:dh*dw*2);
      return;
   }

   if (pal==NULL) {
      use_ipal=TRUE;
      pal = grd_pal;
   } else {
      use_ipal=FALSE;
   }

   hscale = sh/dh;
   wscale = sw/dw;
   area = hscale*wscale;

   // Area is almost always power of two
   tmp = area;
   area_shift = 0;
   while((tmp>>=1)) area_shift++;
   
   // Iterate through the destination
   for (j=0;j<sh;j+=hscale) {
      r=g=b=0;
      for (i=0;i<sw;i+=wscale) {

         if (type==BMT_FLAT8) {
            // Iterate through the source, accumulating
            for (y=j;y<j+hscale;++y) {
               srcptr = srcbit + i + y*sw;
               x = wscale;
               while(x--) {
                  col = *srcptr;
                  r += pal[col*3];
                  g += pal[col*3+1];
                  b += pal[col*3+2];
                  srcptr++;
               }
            }

            r/=area;
            g/=area;
            b/=area;

            // clamp so we don't get silly
            if (r<0) r = 0; else if (r>255) r = 255;
            if (g<0) g = 0; else if (g>255) g = 255;
            if (b<0) b = 0; else if (b>255) b = 255;
            
            if (!use_ipal) {
               *dstbit = closest_col(r,g,b,pal);
            } else {
               fix fr,fg,fb;
               // make into fixed point
               fr = fix_from_float(r);
               fg = fix_from_float(g);
               fb = fix_from_float(b);
               *dstbit = grd_ipal[gr_index_rgb(fr,fg,fb)];
            } 

            // now subtract the actual color out and scale by area
            // to get diffusion term
            r = area*(r-pal[*dstbit*3]);
            g = area*(g-pal[*dstbit*3+1]);
            b = area*(b-pal[*dstbit*3+2]);

            dstbit++;
         } else {
            // 16 bit
            // Iterate through the source, accumulating
            ulong r,g,b;
            r = g = b = 0;
            for (y=j;y<j+hscale;++y) {
               srcptr = (uchar *)((ushort*)srcbit + i + y*sw);
               x = wscale;
               while(x--) {     
                  r += ((_rgb565 *)srcptr)->r;
                  g += ((_rgb565 *)srcptr)->g;
                  b += ((_rgb565 *)srcptr)->b;
                  srcptr+=2;
               }
            }

            // Now map through the ipal
            ((_rgb565 *)dstbit)->r = (r>>area_shift);
            ((_rgb565 *)dstbit)->g = (g>>area_shift);
            ((_rgb565 *)dstbit)->b = (b>>area_shift);

            dstbit+=2;
         }

      }
   }
}


// Render mip map levels upper left going down
void md_mipmap_render(r3s_texture tmap,int x,int y)
{
   while(1) {
      gr_bitmap(tmap,x,y);
      if ((tmap->w==1) && (tmap->h==1)) return;
      x += tmap->w + 1;
      tmap++;
   }
}

// Like a bitmap alloc, but with mip maps, free the whole at once
// I'm going to migrate this to r3d utils
// I hate writing the same damn code over and over again...
// Creates it all in one big block, copies srcbits
r3s_texture md_mipmap_alloc(uchar *srcbits,int type,int flags,int w,int h,uchar *pal)
{
   int size;
   int num_levs;
   int hlog,wlog;
   uchar *bits;
   grs_bitmap *bm;
   r3s_texture tmap;
   int scale;

   scale = (type==BMT_FLAT16)?2:1;

   // First figure out the total size we need
   // wlog is number of additional levels beyond 1 that we need
   wlog = 0;
   while((1<<wlog) < w) ++wlog;

   // ditto
   hlog = 0;
   while((1<<hlog) < h) ++hlog;
   
   num_levs = (hlog>wlog)?hlog:wlog;
   num_levs++;


   size = num_levs * sizeof(grs_bitmap);

   // Figure out texture space
   wlog=w;
   hlog=h;
   while(1) {
      size += wlog*hlog*scale;

      if (wlog==1 && hlog==1) break;

      if (wlog>1) wlog>>=1;
      if (hlog>1) hlog>>=1;
   }

   tmap = bm = (grs_bitmap *)Malloc(size);

   bits = (uchar *)(bm+num_levs);

   // Generate all the levels including the top one
   wlog=w;
   hlog=h;
   while(1) {
      
      // generate the lower level
      if (srcbits)
         md_mipmap_downsamp(bits,wlog,hlog,srcbits,w,h,pal,type);

      gr_init_bitmap(bm,bits,type,flags,wlog,hlog);

      if (wlog==1 && hlog==1) break;

      bm++;
      bits+= wlog*hlog*scale;

      if (wlog>1) wlog>>=1;
      if (hlog>1) hlog>>=1;
   }

   return tmap;
}

// This looks pretty darn good, you can override it if you wish
static float mipmap_detail = 1.8;

void md_mipmap_set_detail(float detail)
{
   mipmap_detail = detail;
}

float md_mipmap_get_detail(void)
{
   return mipmap_detail;
}



// Used for the callback when you set
// md_set_render_pgon_callback
void md_mipmap_render_pgon(mds_pgon *p, r3s_phandle *v,grs_bitmap *bm, ulong color, ulong type)
{
   // from mds
   extern ulong mdd_tmap_mode;
   float pix_area,tex_area,t_area;
   int flag=0;
   int i,j;

   // Now render like normal
   if (type&MD_PGON_LIGHT_ON) {
      flag |= R3_PL_GOURAUD;
   }

   if (bm) {
      int off, n, i;

      r3s_phandle v0, v1, v2, *vlist;

      r3_set_clip_flags(R3_CLIP_ALL);
      n = r3_clip_polygon(p->num, v, &vlist);
      if (n < 3) return;

      if (n > 3) {
         int x0,x1,x2,x3,x4, nearest;
         double near_z, max1,max2,max3;
         near_z = 1e40;
         for (i=0; i < n; ++i) {
            if (vlist[i]->p.z < near_z) {
               near_z = vlist[i]->p.z;
               nearest = i;
            }
         }

         // now decide which of the adjacent ones to use
         // either near-2..near
         //        near-1..near+1
         //        near..near+2
         // whichever of these is nearest

         x0 = nearest-2; if (x0 < 0) x0 += n;
         x1 = nearest-1; if (x1 < 0) x1 += n;
         x2 = nearest;
         x3 = nearest+1; if (x3 >= n) x3 -= n;
         x4 = nearest+2; if (x4 >= n) x4 -= n;

         // determine the max for each case
         max1 = vlist[x0]->p.z>vlist[x1]->p.z ? vlist[x0]->p.z:vlist[x1]->p.z;
         max2 = vlist[x1]->p.z>vlist[x3]->p.z ? vlist[x1]->p.z:vlist[x3]->p.z;
         max3 = vlist[x3]->p.z>vlist[x4]->p.z ? vlist[x3]->p.z:vlist[x4]->p.z;

         // determine the min
         if (max1 < max2 && max1 < max3) {
             v0 = vlist[x0];
             v1 = vlist[x1];
             v2 = vlist[x2];
         } else if (max2 < max3) {
             v0 = vlist[x1];
             v1 = vlist[x2];
             v2 = vlist[x3];
         } else {
             v0 = vlist[x2];
             v1 = vlist[x3];
             v2 = vlist[x4];
         }
      } else {
         v0 = vlist[0];
         v1 = vlist[1];
         v2 = vlist[2];
      }

      // assume polygon on screen.
      // calculate texel area from UV coords
  
      // Calculate pixel area from screen coords
      pix_area = (float)(v1->grp.sx - v0->grp.sx)*(float)(v2->grp.sy - v1->grp.sy);
      pix_area -= (float)(v2->grp.sx - v1->grp.sx)*(float)(v1->grp.sy - v0->grp.sy);
      pix_area *= mipmap_detail / (65536.0*65536.0);

      t_area = (v1->grp.u - v0->grp.u)*(v2->grp.v - v1->grp.v);
      t_area -= (v2->grp.u - v1->grp.u)*(v1->grp.v - v0->grp.v);
      t_area *= bm->w * bm->h;

      tex_area = fabs(t_area/pix_area);

      // In truth, does this work anyway?  Isn't it the wrong size now...
      // if the other one goes to one...
      // we should generate bigger textures next time...
      // wlog has an incorrect meaning in 16 bit...  I'm mad...
      if (bm->type==BMT_FLAT16) {
         off = (bm->wlog-1)>bm->hlog?(bm->wlog-1):bm->hlog;
      } else {
         off = bm->wlog>bm->hlog?bm->wlog:bm->hlog;
      }

      // This could be done with a float and a mask
      while(tex_area > 1.0 && off>0) {
         tex_area *= (1.0/4.0);
         off--;
         bm++;
      }

#if 0
      // This don't work with hware because the texture
      // is defunct!  Can't depend on it!
//    // If tiny, use solid color, cause it is!
      if (off==0) {
  //       mprintf("MAKING SOLID!\n");
         type = MD_PGON_PRIM_SOLID;
         color = bm->bits[0];
      } else {
#endif
         // Set the requested texture map and context
         r3_set_texture(bm);
         r3_set_polygon_context(flag | R3_PL_POLYGON | mdd_tmap_mode);
         r3_draw_poly(n,vlist);
         return;
         //      }
   }

   switch(type&MD_PGON_PRIM_MASK) {
      case MD_PGON_PRIM_SOLID:
      {
         r3_set_color(color);
         r3_set_polygon_context(flag | R3_PL_POLYGON);
         r3_draw_poly(p->num,v);
         return;
      }
      case MD_PGON_PRIM_WIRE:
      {
         r3_set_color(color);
         j = p->num-1;
         for (i=0;i<p->num;++i) {
            r3_draw_line(v[j],v[i]);
            j = i;
         }
         return;
      }
   }
}




