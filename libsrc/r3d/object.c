/*
 * $Source: x:/prj/tech/libsrc/r3d/RCS/object.c $
 * $Revision: 1.14 $
 * $Author: KEVIN $
 * $Date: 1997/06/30 16:34:08 $
 *
 * Transform and projection definitions
 *
 */

#include <lg.h>
#include <_xfrm.h>
#include <matrix.h>
#include <object.h>

// stack_base points to first first place to write to, result of malloc, basically
// stack_cur points to next place to write to, starts at stack_base
// stack_size is current depth == elements on stack, starts at zero
// stack_max is max depth == max elements that can fit on stack

static void push_stack(void)
{
   // need to realloc, if already full
   if (cx.stack_size == cx.stack_max) {
      r3s_trans *n;

      #ifdef DBG_ON
      Warning(("r3d: push_stack: needs to realloc, may want to increase xform stack\n"));
      #endif

      cx.stack_max *= 2;
      n = (r3s_trans *)Realloc(cx.stack_base,cx.stack_max);
 
      #ifdef DBG_ON
      if (cx.stack_base == NULL) {
         Warning(("r3d: start_object: depth %d out of memory\n",cx.stack_max));
         cx.stack_max /= 2;
         return;
      }
      #endif

      cx.stack_base = n;
      cx.stack_cur = cx.stack_base + cx.stack_size;
   }

   // copy it up
   *cx.stack_cur = cx.o2w;

   cx.stack_cur++;
   cx.stack_size++;
}


static void pop_stack(void) {
   // need to realloc, if already full
   #ifdef DBG_ON
   if (cx.stack_size == 0) {
      Warning(("r3d: end_object: too many end_objects\n"));
      return;
   }
   #endif

   // copy it down
   cx.stack_cur--;
   cx.o2w = *cx.stack_cur;
   cx.stack_size--;
}


// Starts an object coordinate system, with no orientation.  What happens
// is the object stack is incremented, and the new transform is
// calculated.  Sets o2c_invalid.  All of them do. 

void r3_start_object(mxs_vector *v)
{
   mxs_vector vtmp;

   TEST_STATE("r3_start_object");

   push_stack();

   // M1m2 + m1
   mx_mat_mul_vec(&vtmp,X2MAT(&cx.o2w),v);
   mx_addeq_vec(X2VEC(&cx.o2w),&vtmp);
   cx.o2c_invalid=TRUE;
}

void r3_start_object_angles(mxs_vector *v, mxs_angvec *a,r3e_order o)
{
   mxs_trans tmp;

   TEST_STATE("r3_start_object_angles");

   push_stack();

   // build the matrix from the angles
   _r3_concat_matrix(&tmp.mat,a,o);
   tmp.vec = *v;

   mx_mul_trans(X2TRANS(&cx.o2w),X2TRANS(cx.stack_cur-1),&tmp);
   cx.o2c_invalid=TRUE;
}

// push onto stack, transform
void r3_start_object_trans(mxs_trans *t)
{
   TEST_STATE("r3_start_object_trans");

   push_stack();

   mx_mul_trans(X2TRANS(&cx.o2w),X2TRANS(cx.stack_cur-1),t);
   cx.o2c_invalid=TRUE;
}

// These next three are useful for retardo-vision objects, and internally
// for bsp objects that are hinged.
// note that are really all the same and perhaps should be one function that
// has several macro calls, and just passes in an index

typedef void (*rot_fun)(mxs_matrix *,mxs_matrix *,mxs_ang);
static rot_fun rot_table[3] = {
   mx_rot_x_mat,
   mx_rot_y_mat,
   mx_rot_z_mat
};


//n = 0,1,2 for x,y,z, uses a macro to expand it.
// but you can call it directly too
void r3_start_object_n(mxs_vector *v,mxs_ang ang,int n)
{
   mxs_vector vtmp;

   TEST_STATE("r3_start_object_n");

   #ifdef DBG_ON
   if (n<0 || n>2) {
      Warning(("r3_start_object_n: angle arg outside of 0-2\n"));
      return;
   }
   #endif

   push_stack();

   // first rotate vector through old transform
   // then rotate matrix
   mx_mat_mul_vec(&vtmp,X2MAT(&cx.o2w),v);
   mx_addeq_vec(X2VEC(&cx.o2w),&vtmp);
   rot_table[n](X2MAT(&cx.o2w),X2MAT(cx.stack_cur-1),ang);

   cx.o2c_invalid=TRUE;
}

// Ends the object transform scope

void r3_end_object()
{
   TEST_STATE("r3_end_object");
   
   pop_stack();
   cx.o2c_invalid = TRUE;
}

// These are considered pathological, and so, aren't
// implemented as block calls or anything like that.


// Rotate a vector from object space to camera space,
// meaning this is non-orthogonal, beware
void r3_rotate_o2c(mxs_vector *dst,mxs_vector *src)
{
   TEST_IN_CONTEXT("r3_rotate_o2c");
   mx_mat_mul_vec(dst,X2MAT(&cx.o2c),src);
}

// transform from object to world space 
// could be matrix
void r3_transform_o2w(mxs_vector *dst, mxs_vector *src)
{
   TEST_IN_CONTEXT("r3_transform_o2w");
   mx_trans_mul_vec(dst,X2TRANS(&cx.o2w),src);
}

// Rotate a vector from object to world space
void r3_rotate_o2w(mxs_vector *dst, mxs_vector *src)
{
   TEST_IN_CONTEXT("r3_rotate_o2w");
   mx_mat_mul_vec(dst,X2MAT(&cx.o2w),src);
}


// transform from world to object space
// could be matrix
void r3_transform_w2o(mxs_vector *dst, mxs_vector *src)
{
   TEST_IN_CONTEXT("r3_transform_w2o");
   mx_trans_tmul_vec(dst,X2TRANS(&cx.o2w),src);
}

// rotate vector from world to object space
// could be matrix
void r3_rotate_w2o(mxs_vector *dst, mxs_vector *src)
{
   TEST_IN_CONTEXT("r3_rotate_w2o");
   mx_mat_tmul_vec(dst,X2MAT(&cx.o2w),src);
}
