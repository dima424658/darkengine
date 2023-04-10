// $Header: x:/prj/tech/libsrc/r3d/RCS/init.c 1.9 1997/10/03 13:02:13 KEVIN Exp $
// this has the r3d initializers, as well as global state setup

#include <string.h>
#include <ctxts.h>
#include <r3dctxt.h>
#include <_ctxt.h>
#include <_xfrm.h>

r3s_global_context r3d_glob;     // global context
r3s_global_state   r3d_state;    // global state

// more complex call that let's the user set stuff
// trans depth is expected object transform depth
// clip_planes is the number of expected clip planes
void r3_init_defaults(int trans_depth,int clip_planes,r3e_space space,int ctxt_cnt, int stride)
{
   r3_init_context( (ctxt_cnt==-1)?R3_DEF_CTXT_STACK:ctxt_cnt );

   r3d_state.trans_depth = (trans_depth==-1)?8:trans_depth;
   r3d_state.clip_planes = (clip_planes==-1)?4:clip_planes;
   r3d_state.space = (space==-1)?R3_CLIPPING_SPACE:space;
   r3d_state.stride = (stride==-1)?(sizeof (r3s_point )):stride;

   // indicate that there is no current space
   r3d_glob.cur_space = -1;

   // set world default to turn x into z, y into -x, z into -y
   memset(&r3d_state.world,0,sizeof(r3d_state.world));
   X2MAT(&r3d_state.world)->m2 = 1.0;
   X2MAT(&r3d_state.world)->m3 = -1.0;
   X2MAT(&r3d_state.world)->m7 = -1.0;

   r3_use_g2(); // set the default driver
}

// simple init call that most users will use, that just uses all the default
void r3_init(void)
{
   r3_init_defaults(-1,-1,-1,-1,-1);    // need to do this for real
}

void r3_close(void)
{
   r3_close_context();
}

// Sets a world coord system inherited by all subsequent context creations.  
// The default is the identity transform.  As well, the handedness bit is set
// in the transform.  If you have a current context.

void r3_set_default_world(mxs_trans *w)
{
   // wow, copying structures!
   *X2TRANS(&r3d_state.world) = *w;
}

mxs_trans *r3_get_default_world()
{
   return X2TRANS(&r3d_state.world);
}

// Sets the default camera space that all new contexts inherit.  The default
// is R3_CLIPPING.

void r3_set_default_space(r3e_space s)
{
   r3d_state.space = s;
}

r3e_space r3_get_default_space()
{
   return r3d_state.space;
}

void r3_set_default_stride(int stride)
{
   r3d_state.stride = stride;
}

int r3_get_default_stride()
{
   return r3d_state.stride;
}
