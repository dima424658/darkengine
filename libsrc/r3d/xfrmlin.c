/*
 * $Source: x:/prj/tech/libsrc/r3d/RCS/xfrmlin.c $
 * $Revision: 1.7 $
 * $Author: KEVIN $
 * $Date: 1996/10/29 12:45:01 $
 *
 * Transform and projection definitions for linear space
 *
 */

#include <lg.h>
#include <xfrmtab.h>
#include <_xfrm.h>
#include <matrix.h>

#define FIX_HALF 32768

// just transforms the top 2 rows and sets the z to zero.
static void _top_2rows_trans(mxs_vector *dest,mxs_trans *t,mxs_vector *v)
{
   mxs_matrix *m;

   m = &t->mat;

   dest->x = m->el[0]*v->x + m->el[3]*v->y + m->el[6]*v->z;
   dest->y = m->el[1]*v->x + m->el[4]*v->y + m->el[7]*v->z;
   dest->z = 0;

   mx_addeq_vec(dest,&t->vec);
}

// what this does is transform the point using the whole
// matrix, note this isn't really faster because we do the
// full multiply which we don't need to do.
void r3_rotate_block_lin(int n,r3s_point *dst,mxs_vector *src)
{
   uchar *cur, *last;

   cur = (uchar *)(&(dst->p));
   last = cur + n*r3d_glob.cur_stride;

   TEST_IN_BLOCK("r3_rotate_block_lin");

   while (cur < last) {
      _top_2rows_trans((mxs_vector *)cur,X2TRANS(&cx.o2c),src++);
      cur += r3d_glob.cur_stride;
   }
}

void r3_project_block_lin(int n,r3s_point *p_list)
{
   uchar *cur, *last;
   double w;

   TEST_IN_BLOCK("r3_project_block_lin");

   cur = (uchar *)p_list;
   last = cur + n * r3d_glob.cur_stride;
   w = cx.lin_w; // w is 1/z of the center

   while (cur < last) {
      r3s_point *p = (r3s_point *)cur;
      cur += r3d_glob.cur_stride;

      // is this fast or what?
      p->grp.sx = (int)(p->p.x) - FIX_HALF;
      p->grp.sy = (int)(p->p.y) - FIX_HALF;
      p->grp.w = w;
   }
}

r3s_xform_tab r3d_xform_tab_lin = {
   r3_rotate_block_lin,
   r3_project_block_lin
};

