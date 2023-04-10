
/*
 * $Source: x:/prj/tech/libsrc/skel/RCS/render.c $
 * $Revision: 1.37 $
 * $Author: kate $
 * $Date: 1997/12/10 11:10:29 $
 *
 * Oi, skeleton libary renderer
 * 
 */

#include <r3d.h>
#include <md.h>
#include <mdutil.h>
#include <res.h>
#include <mprintf.h>
#include <string.h>
#include <2dres.h>
#include <_skel.h>
#include <tmpalloc.h>
#include <math.h>
#include <matrix.h>

void ff_cyl(float x0,float y0,float z0,float x1,float y1,float z1,float rad,long c);

extern uchar skd_dumb_clut[256];

bool skd_debug=FALSE;
#define DEBUG_PBA 1
// #define DEBUG_BMAP
//#define DEBUG_VIEW
#define USE_POLYS
bool skd_scale_polys=TRUE;

uchar *compute_skel_clut(uchar *clut_first, uchar *clut_second);

uchar skd_light_detail=0;  // 0 = max

// Provide it the segment info and the endpoints, orientation vector, transformed endpoints, and clut or not
void sk_render_bmap_piece(sks_seginfo *cseg,mxs_vector *j1,mxs_vector *j2,mxs_vector *or,r3s_phandle p1,r3s_phandle p2)
{
   grs_vertex ver[4];
   grs_vertex *v[4];
   CylBMFrame *f;
   CylBMObj *co;
   fix scale;     // bmap scaling factor
   fix frame;     // frame number
   fix dx1,dy1,dx2,dy2,du1,dv1,du2,dv2;   //bmap stuff
   fix dx,dy,tsqr,tsquare;
   bool sym;
   mxs_vector bone,view,tang; // bone along joint, view to joint, perp to tangent
   int old_ftype;
   uchar *clut = cseg->clut;

   #ifdef DEBUG_BMAP
   int duh;
   #endif

   if (cseg->flag&SKF_DONTRENDER) return;

#if 0 // this should be seen to by app
   // clipping if too far off screen.  If we had true 3d perspective prims, we 
   // wouldn't need this, and the parts would be full perspective.  Something
   // to think about.  
   if(p1->ccodes&p2->ccodes)
      return;

#endif

   if ( (p1->p.z <= 0) || (p2->p.z <= 0)) return;

   co = cseg->co;

//   mprintf("nviews %x ppu %x len %x bisym %x\n",co->nviews,co->ppu,co->len,co->bisym);

   // initialize points
   v[0] = &ver[0];
   v[1] = &ver[1];
   v[2] = &ver[2];
   v[3] = &ver[3];

   // this gives width of bmap seg in screen pix when multiplied by source pix width
   // basically just r3_get_vsize with v=(what you want to scale)/ppu. and
   // z = average depth of bmap piece.
   scale = fix_from_float(grd_bm.w*r3_get_zoom()/(co->ppu*(p1->p.z+p2->p.z)));

   // compute orientation of seg to viewer
   mx_sub_vec(&view,j1,r3_get_view_pos());
   mx_sub_vec(&bone,j2,j1);
   mx_cross_vec(&tang,&view,&bone);
   mx_normeq_vec(&tang);

   // take the dot prod of the orientation vector with the view vector (v1)
   frame = fix_from_float(mx_dot_vec(&tang,or));

   // take the asin
   if (frame>FIX_UNIT) frame = FIX_UNIT;

   else if (frame<-FIX_UNIT) frame = -FIX_UNIT;
   frame = ((long)(short)fix_fast_asin(frame))<<2;

   // dot prod with raw view vector (away or towards viewer)
   if (mx_dot_vec(&view,or) > 0)
      frame = (FIX_UNIT - frame);
   else
      frame = (3*FIX_UNIT + frame);

   // This all assumes 16 frames per, duh!
   frame = (frame + FIX_UNIT/8) >> 14;
   frame = frame & 15;

   // mirror frame if set is mirrored
   if (cseg->flag & SKF_MIRROR) {
      frame = (16-frame)&15;
   }

   if ((co->bisym==TRUE) && (frame>8) ) {
      frame=(16-frame)&15;
      sym = 1;
   } else
      sym = 0;

   // we know the frame, lock the bits
   f = (CylBMFrame *)((ubyte *)co + co->off[frame]);
   f->bm.bits = (uchar *)(f+1);

   // real pixel screen distance
   dx = p2->grp.sx - p1->grp.sx;
   dy = p2->grp.sy - p1->grp.sy;

   // These are all based on the real distance, should not be compensated for
   dx1 = fix_mul(dx,f->vper1);
   dy1 = fix_mul(dy,f->vper1);
   dx2 = fix_mul(dx,f->vper2);
   dy2 = fix_mul(dy,f->vper2);

   tsquare = fix_safe_pyth_dist(dx,dy);
   tsqr = fix_div(f->u1 * scale,tsquare);

   // These probably *do* have to be compensated for
   // I haven't actually tested this with non-identity aspect ratio.

   if ( ((cseg->flag & SKF_MIRROR) != 0) ^ sym ) {
      dv2 = -fix_mul_div(dx,tsqr,grd_cap->aspect);      // and aspect, maybe;
      dv1 = fix_mul(dv2,f->uper);  
      du2 = fix_mul(dy,tsqr);
      du1 = fix_mul(du2,f->uper);
   } else {
      dv1 = -fix_mul_div(dx,tsqr,grd_cap->aspect);      // and aspect, maybe;
      dv2 = fix_mul(dv1,f->uper);  
      du1 = fix_mul(dy,tsqr);
      du2 = fix_mul(du1,f->uper);
   }

   ver[0].x = p1->grp.sx - dx1 - du1;
   ver[0].y = p1->grp.sy - dy1 - dv1;
   ver[1].x = p1->grp.sx - dx1 + du2;
   ver[1].y = p1->grp.sy - dy1 + dv2;
   ver[2].x = p2->grp.sx + dx2 + du2;
   ver[2].y = p2->grp.sy + dy2 + dv2;
   ver[3].x = p2->grp.sx + dx2 - du1;
   ver[3].y = p2->grp.sy + dy2 - dv1;

   ver[0].v = 0;
   ver[1].v = 0;
   ver[2].v = fix_make(f->bm.h,0) - 1;
   ver[3].v = fix_make(f->bm.h,0) - 1;

   if ( ((cseg->flag & SKF_MIRROR) != 0) ^ sym ) {
      ver[0].u = fix_make(f->bm.w,0) - 1;
      ver[1].u = 0;
      ver[2].u = 0;
      ver[3].u = fix_make(f->bm.w,0) - 1;
   } else {
      ver[0].u = 0;
      ver[1].u = fix_make(f->bm.w,0) - 1;
      ver[2].u = fix_make(f->bm.w,0) - 1;
      ver[3].u = 0;
   }

   old_ftype = gr_get_fill_type();

#if 0 // not ported to new 3d yet
   if (g3d_light_type != 0) {
      g3s_vector up,front;
      fix lfront;
      // fix ltop,lleft,lright;

      // note the front left and up vectors are defined now
      g3_vec_sub(&up,j1,j2);
      g3_vec_normalize(&up);
      g3_vec_cross(&front,&up,&tang);

      lfront = g3_light(&front,j1);
      gr_set_fill_type(FILL_CLUT);
      gr_set_fill_parm(g3d_light_tab + ((lfront >> g3d_ltab_shift) & ~0xff));
   }
#endif

   // I think this can probably go away now
   if (grd_canvas->gc.fill_type == FILL_NORM) {
      if (clut== NULL) { 
         gr_lin_map(&(f->bm),4,v);
         if(cseg->decfunc)
            (cseg->decfunc)(cseg->decdata,ver,(uchar)frame);
      } else {
         gr_set_fill_type(FILL_CLUT);
         gr_set_fill_parm(clut);
         gr_lin_map(&(f->bm),4,v);
         if(cseg->decfunc)
            (cseg->decfunc)(cseg->decdata,ver,(uchar)frame);
      }
   } else {
      if (clut==NULL) {
         gr_lin_map(&(f->bm),4,v);
      } else {
         uchar *old = (uchar *) grd_canvas->gc.fill_parm;
         gr_set_fill_parm(compute_skel_clut(clut, old));
         gr_lin_map(&(f->bm),4,v);
         if(cseg->decfunc)
            (cseg->decfunc)(cseg->decdata,ver,(uchar)frame);
         gr_set_fill_parm(old);
      }
   }

   gr_set_fill_type(old_ftype);

#ifdef DEBUG_BMAP
   gr_bitmap(&(f->bm),0,0);
   duh = gr_get_fill_type();
   gr_set_fill_type(FILL_NORM);
   gr_set_fcolor(255);
   gr_fix_line(ver[0].x,ver[0].y,ver[1].x,ver[1].y);
   gr_fix_line(ver[3].x,ver[3].y,ver[0].x,ver[0].y);

//   gr_fix_line(ver[1].x,ver[1].y,ver[2].x,ver[2].y);
//   gr_fix_line(ver[2].x,ver[2].y,ver[3].x,ver[3].y);
//   gr_point(fix_rint(p1->sx),fix_rint(p1->sy));
//   gr_point(fix_rint(p2->sx),fix_rint(p2->sy));
   gr_set_fill_type(duh);
#endif 

}

// The fear is that orientation vector isn't perp to j1 and j2 so we renormalize
// but assign orientation vec is unitary
void sk_set_poly_frame(mxs_vector *j1,mxs_vector *j2,mxs_vector *or,mds_model *m,float *scalefac)
{
   mxs_vector v1,v2,v3;
   mxs_vector *a1,*a2;
   mds_vhot *vh;
//   mxs_matrix mt;
   mxs_trans tr;
   mxs_real a;
   int i;

   // render the freakin' polygon
   // it's column based
   // orientation vector
   v2.x = or->x;
   v2.y = or->y;
   v2.z = or->z;

   // normalized seg vector
   mx_sub_vec(&v1,j2,j1);
   mx_normeq_vec(&v1);

   a = mx_dot_vec(&v1,&v2);

   if (a!=0) {
      v2.x -= a*v1.x;
      v2.y -= a*v1.y;
      v2.z -= a*v1.z;
      mx_normeq_vec(&v2);
   }

   // cross product (m1 x m2)
   mx_cross_vec(&v3,&v1,&v2);

   tr.mat.vec[0]=v3;
   tr.mat.vec[1]=v2;
   mx_neg_vec(&tr.mat.vec[2],&v1);

   a1=a2=0;
   // assume first two vhots are anchor points    
   vh=(mds_vhot *)(((uchar *)m)+m->vhot_off);
   for(i=0;i<m->vhots;i++,vh++)
   {
      if(vh->id==0)
         a1=&vh->v;
      if(vh->id==1)
         a2=&vh->v;
   }
   if(!a1||!a2) // model doesn't use vhots for anchor points
   {
      tr.vec=*j1;
      *scalefac=1.0;
   } else
   {
      *scalefac=sqrt(mx_dist2_vec(j1,j2)/mx_dist2_vec(a1,a2));
      mx_mat_mul_vec(&v1,&tr.mat,a2);
      mx_scaleeq_vec(&v1,*scalefac);
      mx_subeq_vec(&v1,j2);
      mx_negeq_vec(&v1);
      tr.vec=v1;
   }
   r3_start_object_trans(&tr);
}

#ifdef USE_POLYS

// Provide it the segment, endpoints, and orientation vector
void sk_render_poly_piece(sks_seginfo *cseg,mxs_vector *j1,mxs_vector *j2,mxs_vector *or)
{
   mds_model *m;
   mxs_vector pos;
   float scalefac;
   mxs_vector scalevec;

   // set pos = bone center
   mx_add_vec(&pos,j1,j2);
   mx_scaleeq_vec(&pos,0.5);
   if(!(m=skel_poly_prep(cseg,&pos)))
      return;
   sk_set_poly_frame(j1,j2,or,m,&scalefac);
   if(scalefac!=1.0 && skd_scale_polys)
   {
      mds_model *m2;

      scalevec.x=1.0;
      scalevec.y=1.0;
      scalevec.z=scalefac;
      m2=md_scale_model(NULL,m,&scalevec,TRUE);
      if (m2!=NULL)  // if null, just go home
      {
         md_fancy_render_model(m2,cseg->parm);
         if (m2!=m)
            Free(m2);  // dont free ourselves if we are ourselves
      }
   } else
      md_fancy_render_model(m,cseg->parm);
   r3_end_object();
}

// XXX if we do want scaling to work there must be some better way to 
// deal than scaling the entire model AGAIN for vhots
void sk_vhot_poly_piece(sks_seginfo *cseg,mxs_vector *j1,mxs_vector *j2,mxs_vector *or)
{
   mds_model *m;
   mxs_vector pos;
   float scalefac;
   mxs_vector scalevec;

   if(!(m=skel_poly_prep(cseg,&pos)))
      return;
   sk_set_poly_frame(j1,j2,or,m,&scalefac);
   if(scalefac!=1.0)
   {
      mds_model *m2;

      scalevec.x=1.0;
      scalevec.y=1.0;
      scalevec.z=scalefac;
      m2=md_scale_model(NULL,m,&scalevec,TRUE);
      md_eval_vhots(m2,cseg->parm);
      Free(m2);
   } else
      md_eval_vhots(m,cseg->parm);
   r3_end_object();
}

#endif

// For right now uses wacky non 3d
// stuff will change shortly to use
// all 3d sytem.  Just need to make
// sure it works like this first

// XXX this is not really ideal.
// every time skeleton is rendered, it checks to see if skd_debug is true.
// really want separate "draw debug skel" function that draws line segments
// and orientation vectors that gets called explicity by app _after_
// sk_render_skel.  This will have to project points again, but hey, it's
// just for debugging.
void sk_render_skel(sks_skel *sk,ske_sorttype st)
{
   mxs_vector *j1,*j2,*or; // joints and orientation vector
   r3s_phandle p1,p2;      // transformed joints
   r3s_point *tmp;
   sks_seginfo *cseg;   // current seg
   int seg,i;  // segment number, iterator
   uchar *rl;  // render list
   mxs_vector seg_vec;
   r3s_point seg_point;

   // Can we have any more stack variables, hmmm?
   // in fact,...
   #ifdef DEBUG_PBA
   mxs_vector *tmp2;
   r3s_point *tmp3;
   #endif
   #ifdef DEBUG_VIEW
   mxs_vector *tmp4;
   r3s_point *tmp5;
   #endif

   #ifdef MONO_LOG
   static log = FALSE;
   #endif

   #ifdef MONO_LOG
   if (!log) {
      log = TRUE;
      mono_logon("duh.log",MONO_LOG_DEF,0);
   }
   #endif

   //allocate temporary points as phandles then allocate 3d 
   // use the 2d buffer
   tmp = (r3s_point *)gr_alloc_temp(sk->pointn * sizeof(r3s_point));
   rl = (ubyte *)gr_alloc_temp(sk->segn);

   // transform all the points, so you can get depths for sorting segments
   r3_start_block();
   r3_transform_block(sk->pointn,tmp,sk->pn);
   r3_end_block();

   #ifdef DEBUG_PBA
   // Generate a list of orientation vectors, one per segment
   // transform them
   if(skd_debug) {
      tmp2 = Malloc(sk->segn * sizeof(mxs_vector));
      tmp3 = Malloc(sk->segn * sizeof(r3s_point));
      for (i=0;i<sk->segn;++i) {
         mxs_vector v1;
         // copy over
         mx_copy_vec(&v1,&(sk->or[sk->sg[i].or]));
         // normalize vector
         mx_normeq_vec(&v1);
         mx_scale_vec(&v1,&v1,0.25);
         mx_add_vec(&(tmp2[i]),&(sk->pn[sk->sg[i].p1]),&v1);            
      }
      r3_start_block();
      r3_transform_block(sk->segn,tmp3,tmp2);
      r3_end_block();
   }
   #endif
   #ifdef DEBUG_VIEW
   // Generate a list of orientation vectors, one per segment
   // transform them
   tmp4 = Malloc(m->segn * sizeof(mxs_vector));
   tmp5 = Malloc(m->segn * sizeof(r3s_point));
   for (i=0;i<m->segn;++i) {
      mxs_vector tang,dum,dum2;

// equiv to g3_compute_normal(&tang,r3_get_view_pos(),&(s->vh[m->s[i].p1]),&(s->vh[m->s[i].p2]));
      mx_sub_vec(&dum,&(s->vh[m->s[i].p2]),&(s->vh[m->s[i].p1]));
      mx_sub_vec(&dum2,&r3_get_view_pos(),&(s->vh[m->s[i].p1]));
      mx_cross_norm_vec(&tang,&dum,&dum2);

      mx_scale_vec(&tang,&tang,0.25);
      mx_add_vec(&(tmp4[i]),&(s->vh[m->s[i].p1]),&tang);            
   }
   r3_start_block();
   r3_transform_block(s->segn,tmp5,tmp4);
   r3_end_block();
   #endif

   // create the render list
   // QUICK uses specific model, medium uses
   switch(st) {
      case NONE: sk_no_sort(sk,rl);
      break;
      case FAST: sk->qsort(sk,rl,tmp);
      break;
      case MEDIUM: sk_midpoint_sort(sk,rl,tmp);
      break;
      case SLOW: sk_midpoint_sort(sk,rl,tmp);
      break;
   }

   // Go through the render list
   switch(sk->type) {
      case LINE:
      r3_start_block();
      for (i=0;i<sk->segn;++i) {
         seg = rl[i];
         cseg = &(sk->sg[seg]);
         if (cseg->flag&SKF_DONTRENDER) continue;
         p1 = &tmp[cseg->p1];
         p2 = &tmp[cseg->p2];
         r3_set_color(cseg->col);
         r3_draw_line(p1,p2);
      }
      r3_end_block();
      break;
      case CYL:
      for (i=0;i<sk->segn;++i) {
         seg = rl[i];
         cseg = &(sk->sg[seg]);
         if (cseg->flag&SKF_DONTRENDER) continue;
         p1 = &tmp[cseg->p1];
         p2 = &tmp[cseg->p2];
         ff_cyl(p1->p.x,p1->p.y,p1->p.z,p2->p.x,p2->p.y,p2->p.z,cseg->rad,cseg->col);
      }
      break;
#ifdef USE_POLYS
      case POLY:
         // assumes points unrotated or transformed, p1
         // and them must be in world 3d coords like .loc
      for (i=0;i<sk->segn;++i) {
         seg = rl[i];
         cseg = &(sk->sg[seg]);
         if (cseg->flag&SKF_DONTRENDER) continue;
         j1 =   &(sk->pn[cseg->p1]);
         j2 =   &(sk->pn[cseg->p2]);
         or =   &(sk->or[cseg->or]);

         sk_render_poly_piece(cseg,j1,j2,or);
      }
      break;
#endif
      case BMAP:

      for (i=0;i<sk->segn;++i) {
         seg = rl[i];
         cseg = &(sk->sg[seg]);

         j1 = &(sk->pn[cseg->p1]);
         j2 = &(sk->pn[cseg->p2]);
         or = &(sk->or[cseg->or]);
         p1 = &tmp[cseg->p1];
         p2 = &tmp[cseg->p2];

         // Do joint callback if possible
         if (skd_joint_call_tab[seg]) {
            if ((p1->p.z > p2->p.z) ^ (skd_joint_call_joint[seg]==0)) {
               if (p1->p.z > p2->p.z) {
                  skd_joint_call_tab[seg](p1,&sk->pn[cseg->p1]);
               } else {
                  skd_joint_call_tab[seg](p2,&sk->pn[cseg->p2]);
               }
               skd_joint_call_tab[seg] = NULL;
            }
         }

         // Do seg callback, duh
         if (skd_seg_call_tab[seg]) {
            mxs_vector s1;
            mxs_vector bone;
            mxs_vector off; // offset from center of line
            mxs_vector line; // projection onto line
            mxs_vector voff; // view to center of line

            mx_sub_vec(&bone,j2,j1);

            // get normal from p1 to p2
            mx_normeq_vec(&bone);
            mx_cross_vec(&s1,&bone,or);

            // coordinate system is bone,or,s1

            // get offset from line to callback
            mx_scale_vec(&off,or,skd_seg_call_vecs[seg].y);
            mx_scale_vec(&line,&s1,skd_seg_call_vecs[seg].z);
            mx_add_vec(&off,&line,&off);

            // get projection onto line
            mx_scale_vec(&line,&bone,skd_seg_call_vecs[seg].x);
            mx_add_vec(&line,&line,j1);

            // get view vector
            mx_sub_vec(&voff,&line,r3_get_view_pos());

            // get actual location and project
            mx_add_vec(&seg_vec,&line,&off);
            r3_start_block();
            r3_transform_point(&seg_point,&seg_vec);
            r3_end_block();

            if ( mx_dot_vec(&off,&voff)  > 0) {
               skd_seg_call_tab[seg](&seg_point,&seg_vec);
               skd_seg_call_tab[seg] = NULL;
            }
         }

         sk_render_bmap_piece(cseg,j1,j2,or,p1,p2);

         // Do joint callback if possible
         if (skd_joint_call_tab[seg]) {
            if (p1->p.z > p2->p.z) {
               skd_joint_call_tab[seg](p2,j2);
            } else {
               skd_joint_call_tab[seg](p1,j1);
            }
            skd_joint_call_tab[seg] = NULL;
         }
         if (skd_seg_call_tab[seg]) {
            skd_seg_call_tab[seg](&seg_point,&seg_vec);
            skd_seg_call_tab[seg] = NULL;
         }
      }

      break;   
   }

 #ifdef DEBUG_PBA
    // draw line of body superimposed on body
   if(skd_debug)
   {
      r3_start_block();
      for (seg=0;seg<sk->segn;++seg) {
         p1 = &tmp[sk->sg[seg].p1];
         p2 = &tmp[sk->sg[seg].p2];
   //      r3_set_color((seg<4)?53:67);
         r3_set_color(32);
         r3_draw_line(p1,p2);

         // draw the orientation vector
         r3_set_color(32);
         r3_draw_line(p1,&tmp3[seg]);
      }
      r3_end_block();
   }
 #endif

 #ifdef DEBUG_VIEW
    // draw line of body superimposed on body
   for (seg=0;seg<s->segn;++seg) {
      p1 = &tmp[m->s[seg].p1];
      r3_set_color(8*16+10);
      r3_draw_line(p1,&tmp5[seg]);
   }
 #endif

   gr_free_temp(rl);
   gr_free_temp(tmp);

   #ifdef DEBUG_PBA
   if(skd_debug)
   {
      Free(tmp2);
      Free(tmp3);
   }
   #endif

   #ifdef DEBUG_VIEW
   Free(tmp4);
   Free(tmp5);
   #endif
}

uchar *compute_skel_clut(uchar *clut_first, uchar *clut_second)
{
   static uchar skel_clut[256];
   
   static uchar *old_first, *old_second;
   int i;

   if (clut_first != old_first || clut_second != old_second) {
      old_first = clut_first;
      old_second = clut_second;
      for (i=0; i < 256; ++i)
         skel_clut[i] = clut_second[clut_first[i]];
   } 
   return skel_clut;
}

// Evaluate vhots
void sk_vhot_skel(sks_skel *sk)
{
   int seg;
   mxs_vector v3;
   sks_seginfo *cseg;   // current seg

   for (seg=0;seg<sk->segn;++seg) {
      cseg = &(sk->sg[seg]);

      // Do joint callback if possible
      if (skd_joint_call_tab[seg]) {
         if (skd_joint_call_joint[seg]==0) {
            memcpy(&skd_vhot_vecs[((int)(skd_joint_call_tab[seg]))-1]
               ,&sk->pn[cseg->p1]
               ,sizeof(mxs_vector));
         } else {
            memcpy(&skd_vhot_vecs[((int)(skd_joint_call_tab[seg]))-1]
               ,&sk->pn[cseg->p2]
               ,sizeof(mxs_vector));
         }
         skd_joint_call_tab[seg] = NULL;
      }

#ifdef USE_POLYS
      if (sk->type == POLY) {
         sk_vhot_poly_piece(cseg,&(sk->pn[cseg->p1]),&(sk->pn[cseg->p2]),&(sk->or[cseg->or]));
      }
#endif

      // Do seg vhot
      if (skd_seg_call_tab[seg]!=NULL) {
         mxs_vector s1;
         mxs_vector *or = &(sk->or[cseg->or]);
         mxs_vector off; // offset from center of line
         mxs_vector line; // projection onto line

         mx_sub_vec(&v3,&(sk->pn[cseg->p2]),&(sk->pn[cseg->p1]));
         // get normal from p1 to p2
         mx_normeq_vec(&v3);
         mx_cross_vec(&s1,&v3,or);

         // coordinate system is [v3,or,s1]

         // get offset from line to callback
         mx_scale_vec(&off,or,skd_seg_call_vecs[seg].y);
         mx_scale_vec(&line,&s1,skd_seg_call_vecs[seg].z);
         mx_add_vec(&off,&line,&off);

         // get projection onto line
         mx_scale_vec(&line,&v3,skd_seg_call_vecs[seg].x);
         mx_add_vec(&line,&line,&(sk->pn[cseg->p1]));

         // Put vector into table
         mx_add_vec(&skd_vhot_vecs[((int)(skd_seg_call_tab[seg]))-1],&line,&off);

         skd_seg_call_tab[seg] = NULL;
      }
   }
}
