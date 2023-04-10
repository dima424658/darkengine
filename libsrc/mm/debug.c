// $Header: x:/prj/tech/libsrc/mm/RCS/debug.c 1.5 1998/05/05 15:48:48 kate Exp $

#include <stdlib.h>
#include <mm.h>
#include <mm_.h>
#include <matrixs.h>
#include <r3d.h>
#include <fix.h>
#include <memall.h>
#include <g2.h>
#include <dev2d.h>

static void draw_mark(mxs_trans *trans)
{
   r3s_point point;
   float w=0.15,h=0.15;
   fix sw,sh;
   fix maxw=fix_make(grd_bm.w,0),maxh=fix_make(grd_bm.h,0);
   grs_vertex v1,v2;
   int ocol;
   mxs_vector vec;

   r3_start_object_trans(trans);
   r3_start_block();
   mx_zero_vec(&vec);
   r3_transform_point(&point,&vec);
   if(point.ccodes)
   {
      r3_end_block();
      r3_end_object();
      return;
   }
   
   sw=fix_from_float(r3_get_hsize(point.p.z,w));
   sh=fix_from_float(r3_get_vsize(point.p.z,h));

   ocol=gr_get_fcolor();
   gr_set_fcolor(255);
   v1.x=max(0,point.grp.sx-sw);
   v1.y=max(0,point.grp.sy-sh);
   v2.x=min(maxw,point.grp.sx+sw);
   v2.y=min(maxh,point.grp.sy+sh);
   gr_line(&v1,&v2);

   v1.x=max(0,point.grp.sx-sw);
   v1.y=min(maxh,point.grp.sy+sh);
   v2.x=min(maxw,point.grp.sx+sw);
   v2.y=max(0,point.grp.sy-sh);
   gr_line(&v1,&v2);

   gr_set_fcolor(ocol);
   r3_end_block();
   r3_end_object();
}

void mm_dbg_draw_joints(mms_model *m)
{
   int i;
   mxs_trans trans;

   for(i=0;i<mmd_model->segs;i++)
   {
      if(mmd_segs[i].flags&MMSEG_FLAG_STRETCHY)
         continue;
      joint_pos_cback(mmd_model,mmd_segs[i].joint_id,&trans);
      draw_mark(&trans);
   }
}

static void draw_seg_verts(mms_model *m, mms_segment *pseg, fix r)
{
   int j,k;
   mms_smatseg *pss;
   r3s_point *pptr;

   for(j=0;j<pseg->smatsegs;j++)
   {
      pss=&mmd_smatsegs[mmd_mappings[pseg->map_start+j]];
      for(k=pss->data.vert_start;k<pss->data.vert_start+pss->data.verts;k++)
      {
         pptr=mmd_buff_point(k);
         g2_disk(pptr->grp.sx,pptr->grp.sy,r);
      }
   }
}

// this only really useful for 16-bit render mode
void mm_dbg_draw_seg_colored_verts(mms_model *m,int index,fix r)
{
   // choose colors evenly distributed in color space, one color
   // for each segment.  hope transparency isn't an issue.
   int bdepth=grd_bpp;
   ulong max,col,ocol;
   float inc;
   int i;
   mms_segment *pseg;

   void *buff;

   ocol=gr_get_fcolor();

   // allocate space for the buffer
   buff = (void *)Malloc(mm_buffsize(m, NULL));

   mm_set_buff(m, NULL, buff);

   // transform points per segment, as well as normals and lights
   mm_transform_only(m, NULL);

   if(bdepth>=32)
      bdepth=24;
   max=(0x1<<bdepth)-0x1;
   if(m->segs>1)
      inc=((float)max)/(m->segs-1);
   else
      inc=0;
   if(index>0&&index<m->segs) // only draw that seg
   {
      col=inc*index;
      gr_set_fcolor(col);
      draw_seg_verts(m,&mmd_segs[index],r);
   } else
   {
      pseg=mmd_segs;
      for(i=0;i<m->segs;i++,pseg++)
      {
         col=inc*i;
         gr_set_fcolor(col);
         draw_seg_verts(m,pseg,r);
      }
   }

   gr_set_fcolor(ocol);

   Free(buff);
}

void mm_dbg_draw_verts(mms_model *m,uchar c1, uchar c2,fix r)
{
   int i;
   r3s_point *pptr;
   uchar ocol,c;

   void *buff;

   // allocate space for the buffer
   buff = (void *)Malloc(mm_buffsize(m, NULL));

   mm_set_buff(m, NULL, buff);

   // transform points per segment, as well as normals and lights
   mm_transform_only(m, NULL);

   ocol=gr_get_fcolor();

   gr_set_fcolor(c1);
   for(i=0;i<mmd_model->verts;i++)
   {
      pptr=mmd_buff_point(i);
      
      if(!pptr->ccodes)
      {
         c=gr_get_pixel(fix_int(pptr->grp.sx),fix_int(pptr->grp.sy));
         if(c!=c1 && c!=c2)
         { // then not already drawn to
            gr_set_fcolor(c1);
            g2_disk(pptr->grp.sx,pptr->grp.sy,r);
         } else
         { // draw as dup vert
            gr_set_fcolor(c2);
            g2_disk(pptr->grp.sx,pptr->grp.sy,r);
         }
      }
   }
   gr_set_fcolor(ocol);

   Free(buff);
}
