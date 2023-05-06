/*
 * $Source: x:/prj/tech/libsrc/md/RCS/render.c $
 * $Revision: 1.28 $
 * $Date: 1998/06/30 18:56:08 $
 *
 * Model Library rendering routines
 *
 */

#include <string.h>

#include <md.h>
#include <md_.h>
#include <r3d.h>
#include <lg.h>
#include <mprintf.h>
#include <g2.h> // g2s_point

// These could be legitimately shared between objects recursing...
static int         def_vcolor_tab[MD_TAB_SIZE];
static r3s_texture def_vtext_tab[MD_TAB_SIZE];
static mdf_vcall   def_vcall_tab[MD_TAB_SIZE];
static mxs_vector  def_vhot_tab[MD_TAB_SIZE];

int *mdd_vcolor_tab = def_vcolor_tab;
r3s_texture *mdd_vtext_tab = def_vtext_tab;
mdf_vcall *mdd_vcall_tab = def_vcall_tab;
mxs_vector *mdd_vhot_tab = def_vhot_tab;


// Call if you want to reset the tables
// use only if you are sneaky
void md_reset_tables()
{
   mdd_vcolor_tab = def_vcolor_tab;
   mdd_vtext_tab = def_vtext_tab;
   mdd_vcall_tab = def_vcall_tab;
   mdd_vhot_tab = def_vhot_tab;
}

// All the context that's fit to save
mds_con mdd;

// These are all kind of global
// These are for controlling the lighting type, and the
// primitive type
mds_parm   *mdd_parms;     // current parms
ubyte      mdd_type_and=0xFF;  // mask for pgon type at render time
ubyte      mdd_type_or=0;   // mask for pgon type at render time

// Texture
ulong       mdd_tmap_mode=R3_PL_TEXTURE;  // default to perspective

// global
static bool render_b2f = TRUE; //render back to front?

bool        mdd_rgb_lighting = FALSE; // if true, use RGB lighting

// Pgon callback, global
static mdf_pgon_cback pgon_callback=md_render_pgon;
static mdf_render_pgon_cback render_pgon_callback=0;

// Subobject callback, global
static mdf_subobj_cback mdd_subobj_cback=NULL;

// Lighting callback, global
static mdf_light_setup_cback mdd_light_setup_cback=NULL;
static mdf_light_cback mdd_light_cback=NULL;
static mdf_light_obj_cback mdd_light_obj_cback=NULL;

// Vertex list.  Pretty darn big.
// Temporary state
static r3s_phandle vlist[64];

// temporary
static int buff_light_off; // where the light vals start
static int buff_norm_off;  // where the norm values are start

#ifndef SHIP
   #define TEST_VERSION(fname,model)   \
   do { \
      if (model->ver!=MD_CUR_VER) { \
         char safename[10]; \
         strncpy(safename,model->name,8); \
         safename[8]='\0'; \
         CriticalMsg3("%s: model %s is old version %d",fname,safename,model->ver); \
         return; \
      } \
   } while(0)
#else
   #define TEST_VERSION(fname,model) 
#endif

// This is more or less global and doesn't have to be saved each time
ulong *mdd_index_table;  // an array of offsets into the buffer

void update_stride(mds_model *m)
{
   static int max_points;
   static int last_stride;
   BOOL recompute_index = FALSE;

   if (m->verts > max_points) {
      max_points = m->verts;

      if (mdd_index_table)
         Free(mdd_index_table);
      mdd_index_table = Malloc(sizeof(ulong) * max_points);
      recompute_index = TRUE;
   }

   if (last_stride != r3d_glob.cur_stride) {
      last_stride = r3d_glob.cur_stride;
      recompute_index = TRUE;
   }

   if (recompute_index) {
      int i,n;
      ulong cur,step;
      n = max_points;
      cur = 0;
      step = last_stride;
      for (i=0; i < n; ++i, cur += step)
         mdd_index_table[i] = cur;
   }
}

// set all the globals based on the model
// Can be called from outside if you are sneaky
void md_set_globals(mds_model *m)
{
   mdd.model = m;
   mdd.subs = md_subobj_list(m);
   mdd.mats  = md_mat_list(m);
   mdd.uvs   = (mds_uv*)((uchar *)m+m->uv_off);
   mdd.points= md_point_list(m);
   mdd.lights= md_light_list(m);
   mdd.norms = md_norm_list(m);
   mdd.pgons = (uchar *)((uchar *)m+m->pgon_off);
   mdd.nodes = (uchar *)((uchar *)m+m->node_off);

   update_stride(m);
}


// Set the 3d stack for this subobj
// Both rotating and sliding subobjs need to transform into the plane of the
// axle, then rotate or slide about it
void md_start_subobj(int i)
{
   // this is the root node, already set
   switch(mdd.subs[i].type) {

      case MD_SUB_NONE:
         // nothing to see here, not really a subobject
         return;

      // This is a subobj on an axle
      case MD_SUB_ROT:
      {
         mxs_vector zero;
         mx_mk_vec(&zero,0,0,0);

         // first start object matrix
         r3_start_object_trans(&mdd.subs[i].trans);

         // then rotate about x
         // cast the float to an mxs_ang
         // the size the same
       	r3_start_object_x(&zero,mdd_parms[mdd.subs[i].parm].ang);

         if (mdd_light_obj_cback)
            mdd_light_obj_cback(MD_OBJ_PUSH);

         break;
      }

      // This is a subobj on a slider
      case MD_SUB_SLIDE:
      {
         mxs_vector v;

         // first start object matrix
         r3_start_object_trans(&mdd.subs[i].trans);

         mx_mk_vec(&v,mdd_parms[mdd.subs[i].parm].slide,0,0);
         r3_start_object(&v);

         if (mdd_light_obj_cback)
            mdd_light_obj_cback(MD_OBJ_PUSH);

         break;
      }
   }
}

// End the transform stack for this object
void md_end_subobj(int i)
{
   switch(mdd.subs[i].type) {

      case MD_SUB_NONE:
      {
         // nothing to see here, not really a subobject
         return;
      }

      // This is a subobj on an axle or 
      // slider.  Need two end_objects
      case MD_SUB_ROT:
      case MD_SUB_SLIDE:
      {
         // end the block
         r3_end_object();
         r3_end_object();

         if (mdd_light_obj_cback)
            mdd_light_obj_cback(MD_OBJ_POP);

      }
   }

}

// function type that gets recurred through
typedef void (* mdf_subrecur)(int i);

// Recurses through the subobject list starting
// with zero to allow point transform and others.
// It automatically sets up the 3d to be in the
// right coord system
// The caller is responsible for being in a block
void traverse_subobjs(mdf_subrecur cback,int i)
{
   // end of the rope, return
   if (i==-1) return;

   md_start_subobj(i);

   // call callback function
   cback(i);

   // go to child
   traverse_subobjs(cback,mdd.subs[i].child);

   md_end_subobj(i);
      
   // go to next
   traverse_subobjs(cback,mdd.subs[i].next);
}

#define STACK_SIZE 4
static mds_con stack[STACK_SIZE];
static int stack_index=0;
static void push_context()
{
   stack[stack_index++] = mdd;

   AssertMsg(stack_index<=STACK_SIZE,"Stack depth exceeded, raise this limit\n");
}

static void pop_context()
{
   mdd = stack[--stack_index];

   AssertMsg(stack_index>=0,"Stack depth negative, bad bug here\n");
}

// Render a node back to front, like usual.  There also exists a slightly
// modified version that goes front to back.

#define n_raw(c) ((mds_node_raw *)(c))
#define n_split(c) ((mds_node_split *)(c))
#define n_call(c) ((mds_node_call *)(c))
#define n_vcall(c) ((mds_node_vcall *)(c))
#define n_sub(c) ((mds_node_subobj *)(c))

// Renders a bsp tree from back to front, given the initial node
void render_node_b2f(uchar *n)
{
   static int i;  // so we need very little stack space

   switch (*n) {
      case MD_NODE_SUBOBJ:
      {
         // start a new 3d frame of reference
         r3_end_block();
         md_start_subobj(n_sub(n)->index);
         r3_start_block();

         // render the immediately following node
         render_node_b2f(n+sizeof(mds_node_subobj));

         // end the frame of reference
         r3_end_block();
         md_end_subobj(n_sub(n)->index);
         r3_start_block();

         return;
      }
      case MD_NODE_VCALL:
      {
         mdf_vcall cur_vcall;
         // Pass in the whole thing, including bounding
         // sphere and index and type, which you already 
         // know
         cur_vcall = mdd_vcall_tab[n_vcall(n)->index];

         if (cur_vcall) {
            cur_vcall(n_vcall(n));
         }

         break;
      }
      case MD_NODE_CALL:
      {
         // render polygons before
         for (i=0;i<n_call(n)->pgons_before;++i)
            pgon_callback((mds_pgon *)(mdd.pgons + n_call(n)->polys[i]));

         render_node_b2f((uchar *)mdd.nodes + n_call(n)->node_call);

         // render polygons after
         for (i=0;i<n_call(n)->pgons_after;++i)
            pgon_callback((mds_pgon *)(mdd.pgons + n_call(n)->polys[i + n_call(n)->pgons_before]));

         break;
      }
      case MD_NODE_SPLIT:
      {
         // render polygons before
         for (i=0;i<n_split(n)->pgons_before;++i)
            pgon_callback((mds_pgon *)(mdd.pgons + n_split(n)->polys[i]));

         if ((mdd.buff_norms[n_split(n)->norm] + n_split(n)->d) >= 0.0) {
            render_node_b2f((uchar *)mdd.nodes + n_split(n)->node_behind);
            render_node_b2f((uchar *)mdd.nodes + n_split(n)->node_front);
         } else {
            render_node_b2f((uchar *)mdd.nodes + n_split(n)->node_front);
            render_node_b2f((uchar *)mdd.nodes + n_split(n)->node_behind);
         }

         // render polygons after
         for (i=0;i<n_split(n)->pgons_after;++i)
            pgon_callback((mds_pgon *)(mdd.pgons + n_split(n)->polys[i+n_split(n)->pgons_before]));

         break;
      }
      case MD_NODE_RAW:
      {
         // Just render all the pgons in it
         for (i=0;i<n_raw(n)->num;++i)
            pgon_callback((mds_pgon *)(mdd.pgons + n_raw(n)->polys[i]));
         break;
      }
   }
}


void render_node_f2b(uchar *n)
{
   static int i;  // so we need very little stack space

   switch (*n) {
      case MD_NODE_SUBOBJ:
      {
         // start a new 3d frame of reference
         r3_end_block();
         md_start_subobj(n_sub(n)->index);
         r3_start_block();

         // render the immediately following node
         render_node_f2b(n+sizeof(mds_node_subobj));

         // end the frame of reference
         r3_end_block();
         md_end_subobj(n_sub(n)->index);
         r3_start_block();

         return;
      }
      case MD_NODE_VCALL:
      {
         mdf_vcall cur_vcall;

         // Pass in the whole thing, including bounding
         // sphere and index and type, which you already 
         // know
         cur_vcall = mdd_vcall_tab[n_vcall(n)->index];

         if (cur_vcall) {
            cur_vcall(n_vcall(n));
         }

         break;
      }
      case MD_NODE_CALL:
      {
         // render polygons after
         for (i=n_call(n)->pgons_after-1;i>=0;--i)
            pgon_callback((mds_pgon *)(mdd.pgons + n_call(n)->polys[i + n_call(n)->pgons_before]));

         render_node_f2b((uchar *)mdd.nodes + n_call(n)->node_call);

         // render polygons before
         for (i=n_call(n)->pgons_before-1;i>=0;--i)
            pgon_callback((mds_pgon *)(mdd.pgons + n_call(n)->polys[i]));

         break;
      }
      case MD_NODE_SPLIT:
      {
         // render polygons after
         for (i=n_split(n)->pgons_after-1;i>=0;--i)
            pgon_callback((mds_pgon *)(mdd.pgons + n_split(n)->polys[i+n_split(n)->pgons_before]));

         if ((mdd.buff_norms[n_split(n)->norm] + n_split(n)->d) >= 0.0) {
            render_node_f2b((uchar *)mdd.nodes + n_split(n)->node_front);
            render_node_f2b((uchar *)mdd.nodes + n_split(n)->node_behind);
         } else {
            render_node_f2b((uchar *)mdd.nodes + n_split(n)->node_behind);
            render_node_f2b((uchar *)mdd.nodes + n_split(n)->node_front);
         }

         // render polygons before
         for (i=n_split(n)->pgons_before-1;i>=0;--i)
            pgon_callback((mds_pgon *)(mdd.pgons + n_split(n)->polys[i]));

         break;
      }
      case MD_NODE_RAW:
      {
         // Just render all the pgons in it
         for (i=n_raw(n)->num-1;i>=0;--i)
            pgon_callback((mds_pgon *)(mdd.pgons + n_raw(n)->polys[i]));
         break;
      }
   }
}


// Reentrant call from a vcall...
// supposed to be used like a subobject
// pushes and pops context
void md_render_vcall(mds_model *m)
{
   void *buff;

   push_context();

   TEST_VERSION("md_render_vcall",m);

   // allocate space for the buffer
   buff = (void *)Malloc(md_buffsize(m));

   md_set_buff(m,buff);
   md_set_globals(m);

   r3_end_block();
   
   // Recurse through the subobjs, in the order of parents, then 
   // children, to get the transforms right.  
   traverse_subobjs(md_transform_subobj,0);

   r3_start_block();
   
   // render first node of first object
   if (render_b2f) 
      render_node_b2f((uchar *)mdd.nodes);
   else 
      render_node_f2b((uchar *)mdd.nodes);
   
   r3_end_block();

   Free(buff);

   pop_context();

   r3_start_block();
}

void md_use_lgd3d()
{
    // TODO
}

// Normal way to render a model.  Pass in pointer to the model and parms
// list
void md_render_model(mds_model *m,mds_parm parms[])
{
   void *buff;

   TEST_VERSION("md_render_model",m);

   // allocate space for the buffer
   buff = (void *)Malloc(md_buffsize(m));

   md_set_buff(m,buff);
   
   // Now transform points and normals and lights
   md_transform_only(m,parms);
   // Now render the bastard
   md_render_only(m,parms);
   
   Free(buff);
}

// Given a model, returns the needed size of the buffer for it
int md_buffsize(mds_model *m)
{
   int size;

   // need space for transformed points
   size = m->verts * r3d_glob.cur_stride;
   buff_light_off = size;

   // space for lighting values
   size += (mdd_rgb_lighting ? 3*sizeof(float) : sizeof(float))
                 * (m->norm_off - m->light_off)/(sizeof(mds_light));
   buff_norm_off = size;

   // space for normal dot prods
   size += sizeof(float) * (m->pgon_off - m->norm_off);
   return size;
}


// Use this buffer for the next model rendered
void md_set_buff(mds_model *m,void *buff)
{
   // set all the global pointers to point at it
   mdd.buff = buff;

   // set the offsets
   md_buffsize(m);

   // points are kept at the top of the buffer
   mdd.buff_points = (r3s_point *)buff;

   // next is light values
   mdd.buff_lights = (float *)((uchar *)buff + buff_light_off);

   // next is normal values
   mdd.buff_norms = (float *)((uchar *)buff + buff_norm_off);

   update_stride(m);
}





// this gets called back by the subobj traverser
void md_transform_subobj(int i)
{
   int j;
   mds_subobj *s;
   mxs_vector *viewer;
   
   s = &mdd.subs[i];

   // start the block
   r3_start_block();
   
   // transform into the buffer
   r3_transform_block(s->point_num, md_buff_point(s->point_start), &mdd.points[s->point_start]);

   // end your block
   r3_end_block();

   if (mdd_subobj_cback)
      mdd_subobj_cback(s);
   
   // transform the light values if set
   // passing in number of lights, i list, light list, and object space
   // and camera space points
   if (mdd_light_cback) {
      mdd_light_cback(s->light_num
         ,&mdd.buff_lights[s->light_start]
         ,&mdd.lights[s->light_start]
         ,&mdd.points[s->point_start]
         ,md_buff_point(s->point_start) );
   }

   // transform the viewer into object space
   // now we can do dot prods
   viewer = r3_get_view_in_obj();            
   
   // transform the normals, just need to do once
   for (j=s->norm_start;j<s->norm_start+s->norm_num;++j) 
   {
      mdd.buff_norms[j] = mx_dot_vec(viewer,&mdd.norms[j]);
   }
}



// Only transforms points, polygon normals, and lighting values
// into the buffer.  Does not render.
void md_transform_only(mds_model *m,mds_parm p[])
{
   TEST_VERSION("md_transform_only",m);

   // Per object lighting setup
   if (mdd_light_setup_cback)
      mdd_light_setup_cback(&m->bmin,&m->bmax);

   md_set_globals(m);
   mdd_parms=p;

   // notify the lighting system about
   // root subobj
   if (mdd_light_obj_cback)
      mdd_light_obj_cback(MD_OBJ_PUSH);


   // Recurse through the subobjs, in the order of parents, then 
   // children, to get the transforms right.  
   traverse_subobjs(md_transform_subobj,0);

   // notify the lighting system
   if (mdd_light_obj_cback)
      mdd_light_obj_cback(MD_OBJ_POP);

}


void md_render_pgon(mds_pgon *p)
{
   int i;
   int j;
   ulong flag=0;
   ulong type;

   // Bail if it isn't facing you
   // use the cached normal dot prod
   if ((mdd.buff_norms[p->norm] + p->d) <= 0.0) return;
   
   // Set vlist to contain the list of verts
   for (i=0;i<p->num;++i) {
      vlist[i] = md_buff_point(p->verts[i]);
   }

   // Set the type, accounting for rendering mode,
   // ie, no lighting, or a particular prim type
   type = (p->type&mdd_type_and)|mdd_type_or;

   // lighting on
   if (type&MD_PGON_LIGHT_ON) {
      // Set lighting values
      if (!mdd_rgb_lighting) {
         flag |= R3_PL_GOURAUD;
         for (i=0,j=p->num;i<p->num;++i,++j) {
            vlist[i]->grp.i = mdd.buff_lights[p->verts[j]];
         }
      } else {
         flag |= R3_PL_RGB_GOURAUD;
         for (i=0,j=p->num;i<p->num;++i,++j) {
            int offset = p->verts[j] * 3;
            g2s_point *g2p = (g2s_point *) &vlist[i]->grp;
            g2p->i = mdd.buff_lights[offset];
            g2p->h = mdd.buff_lights[offset+1];
            g2p->d = mdd.buff_lights[offset+2];
         }
      }
   }

   switch (type&MD_PGON_COLOR_MASK) {
      case MD_PGON_COLOR_PAL:
      {
         // punt stupid encoding
         r3_set_color(p->data);
         break;
      }   
      case MD_PGON_COLOR_VCOL:
      {
         r3_set_color(mdd_vcolor_tab[p->data]);
         break;
      }
   }

   switch(type&MD_PGON_PRIM_MASK) {
      case MD_PGON_PRIM_SOLID:
      {
         r3_set_polygon_context(flag | R3_PL_POLYGON);
         r3_draw_poly(p->num,vlist);
         return;
      }
      case MD_PGON_PRIM_WIRE:
      {
         j = p->num-1;
         for (i=0;i<p->num;++i) {
            r3_draw_line(vlist[j],vlist[i]);
            j = i;
         }
         return;
      }
      case MD_PGON_PRIM_TMAP: 
      {   
         for (i=0,j=2*p->num;i<p->num;++j,++i) {
            vlist[i]->grp.u = mdd.uvs[p->verts[j]].u;
            vlist[i]->grp.v = mdd.uvs[p->verts[j]].v;
         }
         // Set the requested texture map and context
         r3_set_texture(mdd_vtext_tab[p->data]);
         r3_set_polygon_context(flag | R3_PL_POLYGON | mdd_tmap_mode);
         r3_draw_poly(p->num,vlist);
         return;
      }
   }
}

void md_render_pgon_render_callback(mds_pgon *p)
{
   int i,j;
   grs_bitmap *bm;
   ulong color;
   ulong type;

   // Bail if it isn't facing you
   // use the cached normal dot prod
   if ((mdd.buff_norms[p->norm] + p->d) <= 0.0) return;
   
   // Set vlist to contain the list of verts
   for (i=0;i<p->num;++i) {
      vlist[i] = md_buff_point(p->verts[i]);
   }

   // Set the type, accounting for rendering mode,
   // ie, no lighting, or a particular prim type
   type = (p->type&mdd_type_and)|mdd_type_or;

   // lighting on
   if (type&MD_PGON_LIGHT_ON) {
      // Set lighting values
      if (!mdd_rgb_lighting) {
         for (i=0,j=p->num;i<p->num;++i,++j) {
            vlist[i]->grp.i = mdd.buff_lights[p->verts[j]];
         }
      } else {
         for (i=0,j=p->num;i<p->num;++i,++j) {
            int offset = p->verts[j] * 3;
            g2s_point *g2p = (g2s_point *) &vlist[i]->grp;
            g2p->i = mdd.buff_lights[offset];
            g2p->h = mdd.buff_lights[offset+1];
            g2p->d = mdd.buff_lights[offset+2];
         }
      }
   }

   if ((type & MD_PGON_COLOR_MASK) == MD_PGON_COLOR_VCOL)
      color = mdd_vcolor_tab[p->data];
   else
      color = p->data;

   if ((type & MD_PGON_PRIM_MASK) == MD_PGON_PRIM_TMAP) {
      for (i=0,j=2*p->num;i<p->num;++j,++i) {
         vlist[i]->grp.u = mdd.uvs[p->verts[j]].u;
         vlist[i]->grp.v = mdd.uvs[p->verts[j]].v;
      }
      bm = mdd_vtext_tab[p->data];
   } else
      bm = 0;

   render_pgon_callback(p, vlist, bm, color, type);
}


// Force all solid or wire, and turn off colorization
void md_set_render_prim(ubyte rmode)
{
   switch (rmode) {
      case MD_RMODE_NORMAL:
         mdd_type_and |= MD_PGON_PRIM_MASK;
         mdd_type_or &= ~MD_PGON_PRIM_MASK;
         mdd_type_and |= MD_PGON_COLOR_MASK;
         mdd_type_or &= ~MD_PGON_COLOR_MASK;
         break;
      case MD_RMODE_SOLID:
         mdd_type_and &= ~MD_PGON_PRIM_MASK;
         mdd_type_or  |= MD_PGON_PRIM_SOLID;
         mdd_type_and &= ~MD_PGON_COLOR_MASK;
         mdd_type_or &= ~MD_PGON_COLOR_MASK;
         break;
      case MD_RMODE_WIRE:         
         mdd_type_and &= ~MD_PGON_PRIM_MASK;
         mdd_type_or  |= MD_PGON_PRIM_WIRE;      
         mdd_type_and &= ~MD_PGON_COLOR_MASK;
         mdd_type_or &= ~MD_PGON_COLOR_MASK;
         break;
   }
}

// For disabling lighting.  Can't create
// it if not already there.
void md_set_render_light(bool l)
{
   if (l) {
      mdd_type_and |= MD_PGON_LIGHT_ON;
   } else {
      mdd_type_and &= ~MD_PGON_LIGHT_ON;
   }   
}





// Only render the model, assumes it has been transformed, and in fact,
// only works then.
void md_render_only(mds_model *m,mds_parm pm[])
{

   TEST_VERSION("md_render_only",m);

   md_set_globals(m);
   mdd_parms = pm;

   // notify the lighting system
   if (mdd_light_obj_cback)
      mdd_light_obj_cback(MD_OBJ_PUSH);

   r3_start_block();
   
   // render first node of first object
   if (render_b2f) 
      render_node_b2f((uchar *)mdd.nodes);
   else 
      render_node_f2b((uchar *)mdd.nodes);

   r3_end_block();

   // Reset the polygon callback, so
   // we don't use it again
   pgon_callback = md_render_pgon;
   render_pgon_callback = NULL;

   // notify the lighting system
   if (mdd_light_obj_cback)
      mdd_light_obj_cback(MD_OBJ_POP);

   // reset the tmap mode
   mdd_tmap_mode = R3_PL_TEXTURE;
}


void md_eval_vhot_subobj(int i)
{
   mds_vhot *v;
   int j;
   int e = mdd.subs[i].vhot_start+mdd.subs[i].vhot_num;

   // don't need to be in a block for this
   // since non rendering op
   for (j=mdd.subs[i].vhot_start;j<e;++j) {
      v = mdd.vhots+j;         
      r3_transform_o2w(&mdd_vhot_tab[v->id],&v->v);
   }
}


// Just evaluate the vhots, stuffing them into their positions.
void md_eval_vhots(mds_model *m,mds_parm p[])
{
   // set the parms global for md_start_subobj
   mdd_parms = p;
   // find the vhots
   mdd.vhots = md_vhot_list(m);
   mdd.subs = md_subobj_list(m);

   traverse_subobjs(md_eval_vhot_subobj,0);
}


// Set the polygon callback function.  Gets reset at the end of the
// next rendering.
mdf_pgon_cback md_set_pgon_callback(mdf_pgon_cback func)
{
   mdf_pgon_cback last;

   last = pgon_callback;
   pgon_callback = func;

   return last;
}

mdf_pgon_cback md_set_render_pgon_callback(mdf_render_pgon_cback func)
{
   mdf_pgon_cback last;

   last = md_set_pgon_callback(md_render_pgon_render_callback);
   render_pgon_callback = func;
   return last;
}

// By default is set true, you can set it false here, and change the render order for doing craziness like span 
// clipping, and potentially Z buffer.
void md_render_back_to_front(bool f)
{
   render_b2f = f;
}

// per subobject callback.  Use it for whatever you want; my personal reason is for hazing...
mdf_subobj_cback md_set_subobj_callback(mdf_subobj_cback c)
{
   mdf_subobj_cback old = mdd_subobj_cback;

   mdd_subobj_cback = c;

   return old;
}

// Set the per object setup callback, returns the
// last one
mdf_light_setup_cback md_set_light_setup_callback(mdf_light_setup_cback c)
{
   mdf_light_setup_cback last;

   last = mdd_light_setup_cback;
   mdd_light_setup_cback = c;
   return last;
}


mdf_light_cback md_set_light_callback(mdf_light_cback l)
{
   mdf_light_cback old = mdd_light_cback;

   mdd_light_cback = l;

   return old;
}


mdf_light_obj_cback md_set_light_obj_callback(mdf_light_obj_cback l)
{
   mdf_light_obj_cback old = mdd_light_obj_cback;

   mdd_light_obj_cback = l;

   return old;
}


void md_norm2light(ulong *l,mxs_vector *n)
{
   uint x,y,z;

   x = (((uint)(16384*n->x) & 0xFFC0) << 16);
   y = (((uint)(16384*n->y) & 0xFFC0) << 6);
   z = (((uint)(16384*n->z) & 0xFFC0) >> 4);

   *l = x | y | z;
}



// You should set to R3_PL_TEXTURE or R3_PL_LINEAR
void md_set_tmap_linear(bool l)
{
   mdd_tmap_mode = l?R3_PL_TEXTURE_LINEAR:R3_PL_TEXTURE;
}

