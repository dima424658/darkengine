/*
 * $Source: s:/prj/tech/libsrc/g2/RCS/blend.c $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 17:33:03 $
 *
 * Support for creation and maintenance of blend tables
 *
 * This file is part of the g2 library.
 */

#include <dev2d.h>
#include <blncon.h>

// points to blend_tabs-1 tables, each 64k
uchar *grd_blend=NULL;
uchar *grd_half_blend=NULL;
int grd_log_blend_levels=0;

// blend fac is 0-256, where 0 is all 0, 256 is all 1
void gri_build_blend(uchar *base_addr, int blend_fac)
{
   uchar *c=grd_ipal, *cur_addr=base_addr, cols[2][3];
   int offs, i, j, k;                  /* offset from ipal for data, loop controls */
   int blend_bar=GR_BLEND_TABLE_RES-blend_fac;        /* remaining blend frac */

   for (i=0; i<256; i++)
   {
      gr_split_rgb(grd_bpal[i],&cols[0][0],&cols[0][1],&cols[0][2]);
      for (j=0; j<256; j++)
      {
         if ((i==0)||(i==j)||(cols[0][0]+cols[0][1]+cols[0][2]==0))
            *cur_addr++=i;                    // transparency and self and black are themselves, for zaniness w/shifts
         else
         {
		      gr_split_rgb(grd_bpal[j],&cols[1][0],&cols[1][1],&cols[1][2]);
            if ((j==0)||(cols[1][0]+cols[1][1]+cols[1][2]==0))
               *cur_addr++=j;
            else
            {
			      for (offs=0, k=2; k>=0; k--)      // go do the blends
		            offs=(offs<<5)+((((cols[0][k]*blend_bar)+(cols[1][k]*blend_fac))>>GR_BLEND_TABLE_RES_LOG)>>3);
		         *cur_addr++=*(c+offs);
            }
         }
      }
   }
}

/* frees the blending table. returns 0 if ok, nonzero if error. */
int gr_free_blend(void)
{
   if (grd_blend==NULL)
      return 1;
   gr_free(grd_blend);
   grd_blend=NULL;
   grd_log_blend_levels=0;
   return 0;
}

// returns 0 if ok, nonzero if error.
// at the moment, log_blend_levels = 0 deallocates the blend, ie. runs free_blend
bool gr_init_blend(int log_blend_levels)
{
   if (log_blend_levels>0)
   {
      int fac=GR_BLEND_TABLE_RES>>log_blend_levels;   /* base blend factor*/
      int tab_cnt=(1<<log_blend_levels)-1, i;         /* number of tables, loop control */ 

      if (grd_blend!=NULL) if (gr_free_blend()) return TRUE; /* something went horribly wrong */
	   if ((grd_blend=gr_malloc(tab_cnt*GR_BLEND_TABLE_SIZE))==NULL) /* x 64k tables */
         return TRUE; /* not enough memory */
	   for (i=0; i<tab_cnt; i++)
	      gri_build_blend(grd_blend+(i*GR_BLEND_TABLE_SIZE),fac*(i+1));
      grd_log_blend_levels=log_blend_levels;
      grd_half_blend=grd_blend+(tab_cnt>>1)*GR_BLEND_TABLE_SIZE;
      return FALSE;
   }
   else return gr_free_blend();
}
