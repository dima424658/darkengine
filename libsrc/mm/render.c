// $Header: x:/prj/tech/libsrc/mm/RCS/render.c 1.7 1998/06/30 18:59:26 buzzard Exp $
// XXX TO DO: go through all the loops and make them more efficient
#include <mm.h>
#include <lg.h>
#include <sortpoly.h>
#include <xformseg.h>
#include <r3d.h>
#include <mmd.h>

// global variables
int current_smatr_id=-1;
mmf_joint_pos_cback joint_pos_cback;   
mmf_stretchy_joint_cback stretchy_joint_cback;
mmf_block_render_cback block_render_cback;

// lighting callbacks
mmf_light_setup_cback mmd_light_setup_cback=NULL;
mmf_light_cback mmd_light_cback=NULL;

// custom callbacks (ones which depend on data separate from the model)
mmf_attachment_cback mmd_attachment_cback=NULL;

// globals set from the model
mms_model   *mmd_model;       // pointer to the model
mms_smatr   *mmd_smatrs;      // single material regions list
mms_segment *mmd_segs;        // segment list
mms_mapping *mmd_mappings;
mms_smatseg *mmd_smatsegs;
mms_uvn     *mmd_vert_uvns;   // uv and normal list
mxs_vector  *mmd_vert_vecs;   // vertex list
mms_pgon    *mmd_pgons;       // pgon list
mxs_vector  *mmd_norms;       // pgon normal list
mms_weight  *mmd_weights;     // weight list

// something like the above pointers, only passed in from the app rather
// than coming from the model
mms_custom_data *mmd_custom_data; // things to modify the model

// globals for object buffer
void        *mmd_buff;
r3s_point   *mmd_buff_points;
float       *mmd_buff_norms;   // a pointer to the normal dot product results
mms_pgon    *mmd_buff_sorted_pgons;  // a pointer to sorted polygons
mxs_trans   *mmd_buff_attach_trans;  // attachments are oriented by segment


// Texture
ulong       mmd_tmap_mode=R3_PL_TEXTURE;  // default to perspective

// internal for knowing where stuff is
static int buff_norm_off;  // where the norm values are start
static int buff_spgon_off; // where sorted polygons start
static int buff_attach_off;// transforms for attachments

// other misc globals
static BOOL use_debug=FALSE;
bool mmd_rgb_lighting;

// renderer pipeline stuff

// predeclarations are grouped by pipeline

// polygon pipeline
void _mm_poly_transform(mms_model *m, ulong flags);
void _mm_poly_sort(mms_model *m, ulong flags);
void _mm_poly_render(mms_model *m);
int _mm_poly_buffsize(mms_model *m, mms_custom_data *d);
void _mm_poly_set_buff(mms_model *m, mms_custom_data *d, void *buff);

// hardware pipeline (uses poly transform func)
void _mm_hardware_sort(mms_model *m, ulong flags);
void _mm_hardware_render(mms_model *m);
int _mm_hardware_buffsize(mms_model *m, mms_custom_data *d);
void _mm_hardware_set_buff(mms_model *m, mms_custom_data *d, void *buff);

typedef void(*mmf_transform_func)(mms_model *m, ulong flags);
typedef void(*mmf_sort_func)(mms_model *m, ulong flags);
typedef void(*mmf_render_func)(mms_model *m);
typedef int(*mmf_buffsize_func)(mms_model *m, mms_custom_data *d);
typedef void(*mmf_set_buff_func)(mms_model *m, mms_custom_data *d, 
                                 void *buff);

typedef struct mms_render_pipeline
{
   mmf_transform_func transform;
   mmf_sort_func sort;
   mmf_render_func render;
   mmf_buffsize_func buffsize;
   mmf_set_buff_func set_buff;
} mms_render_pipeline;


// If we have no model instance, we use this one.  It has the safe,
// do-nothing values for all fields.
static mms_custom_data _mm_dummy_custom_data
= { 0, 0, };

// array indexed by rendering pipeline mode

// Any mode we're supporting should have actual functions for all five
// fields, if only dummies.  We check for the presence of a transform
// function to test a pipeline mode setting for validity.
mms_render_pipeline mmd_rpipelines[] = {
   // MM_RPIPE_POLYSORT
   { _mm_poly_transform, _mm_poly_sort, _mm_poly_render,
     _mm_poly_buffsize, _mm_poly_set_buff },

   // MM_RPIPE_ZBUFFER
   { NULL, NULL, NULL, NULL, NULL },

   // MM_RPIPE_HARDWARE
   { _mm_poly_transform, _mm_hardware_sort, _mm_hardware_render,
     _mm_hardware_buffsize, _mm_hardware_set_buff },
};

int mmd_num_rpipes = sizeof(mmd_rpipelines) / sizeof(mms_render_pipeline);

mms_render_pipeline *render_pipeline = NULL;



// segment transform stuff

typedef void(*mmf_seg_transform)(mms_segment *seg, int segment_index);

// array indexed by model layout
// XXX may want to take into account lod when doing vertex weighting
mmf_seg_transform mmd_model_transforms[] = { \
   mm_transform_seg_any,
   mm_transform_seg_any,
};
mmf_seg_transform transform_seg = NULL;

#define SET_SEG_TRANSFORM(layout,flags) \
   (transform_seg=mmd_model_transforms[(layout)])

// XXX TO DO: make this better
extern void mm_transform_stretchy_normals(mms_segment *s);


// segment rotation

typedef void(*mmf_seg_o2w)(mms_segment *);

mmf_seg_o2w mmd_model_o2ws[] = {
   mm_o2w_seg_any,
   mm_o2w_seg_any,
};
mmf_seg_o2w o2w_seg=NULL;

#define SET_SEG_O2W(layout, flags) (o2w_seg = mmd_model_o2ws[(layout)])



// poly sorting stuff

typedef void(*mmf_polysort)(mms_model *);

// array indexed by model layout
// XXX may want to take into account lod when deciding how good sort needs to be
mmf_polysort mmd_polysorts[] = { \
   mm_polysort_global,
   mm_polysort_global, 
};
mmf_polysort polysort=NULL;

#define SET_POLYSORT(layout,flags) (polysort=mmd_polysorts[(layout)])

// segment render stuff


BOOL mm_punt_render;
BOOL mm_punt_sort;
BOOL mm_punt_xform;

#ifndef SHIP
   #define TEST_VERSION(fname,model)   \
   do { \
      if (model->version!=MM_CUR_VER) { \
         CriticalMsg2("%s: model is old version %d",fname,model->version); \
         return; \
      } \
   } while(0)
#else
   #define TEST_VERSION(fname,model) 
#endif


//// INITIALIZATION

quat dum_quat;
mxs_trans dum_trans;

void dummy_stretchy_joint_cback(mms_model *m,int joint_id, mxs_trans *t,quat *q)
{
   mx_identity_trans(t);
   quat_identity(q);
}

void dummy_joint_pos_cback(mms_model *m,int joint_id, mxs_trans *t)
{
   mx_identity_trans(t);
}

void dummy_light_cback(int num,void *tpts,mms_uvn *uvns,mxs_vector *pts)
{
   float *pptr=tpts;

   if (mmd_rgb_lighting) num *= 3;

   while(num--)
   {
      *pptr++ = 1.0;
   }
}

void dummy_attachment_cback(mxs_trans *trans, mms_attachment *attachment)
{
   Warning(("Attachment to mesh model not being rendered: no callback"));
}


void mm_init()
{
   // set defaults
   mm_set_render_pipeline(MM_RPIPE_POLYSORT);
   mm_set_joint_pos_cback(dummy_joint_pos_cback);
   mm_set_stretchy_joint_cback(dummy_stretchy_joint_cback);
   mm_set_light_callback(dummy_light_cback);
   mm_set_block_render_cback(mm_render_pgon_block);
   mm_set_attachment_cback(dummy_attachment_cback);
}

void mm_close()
{
}

// returns previous one
mmf_joint_pos_cback mm_set_joint_pos_cback(mmf_joint_pos_cback func)
{
   mmf_joint_pos_cback old;

   old=joint_pos_cback;
   joint_pos_cback=func;
   return old;
}

// returns previous one
mmf_stretchy_joint_cback mm_set_stretchy_joint_cback(mmf_stretchy_joint_cback func)
{
   mmf_stretchy_joint_cback old;

   old=stretchy_joint_cback;
   stretchy_joint_cback=func;
   return old;
}

// returns previous one
mmf_block_render_cback mm_set_block_render_cback(mmf_block_render_cback func)
{
   mmf_block_render_cback old;

   old=block_render_cback;
   block_render_cback=func;
   return old;
}

mmf_light_setup_cback mm_set_light_setup_callback(mmf_light_setup_cback func)
{
   mmf_light_setup_cback old = mmd_light_setup_cback;

   mmd_light_setup_cback = func;

   return old;
}

mmf_light_cback mm_set_light_callback(mmf_light_cback func)
{
   mmf_light_cback old = mmd_light_cback;

   mmd_light_cback = func;

   return old;
}

mmf_attachment_cback mm_set_attachment_cback(mmf_attachment_cback func)
{
   mmf_attachment_cback old = mmd_attachment_cback;

   mmd_attachment_cback = func;

   return old;
}

// only has an effect in debug build of mesh renderer
void mm_debug_toggle()
{
   use_debug=!use_debug;
}


static void _mm_set_globals(mms_model *m, mms_custom_data *d)
{
   mmd_model=m;
   mmd_smatrs=mm_smatr_list(m);
   mmd_segs=mm_segment_list(m);
   mmd_smatsegs=mm_smatseg_list(m);
   mmd_mappings=mm_mapping_list(m);
   mmd_vert_uvns=mm_vert_uvn_list(m);
   mmd_vert_vecs=mm_vert_vec_list(m);
   mmd_pgons=mm_pgon_list(m);
   mmd_norms=mm_norm_list(m);
   mmd_weights=mm_weight_list(m);
   if (d)
      mmd_custom_data = d;
   else 
      mmd_custom_data = &_mm_dummy_custom_data;
}


//// PIPELINES
void mm_set_render_pipeline(ubyte pipe)
{
   if (pipe >= mmd_num_rpipes || !mmd_rpipelines[pipe].transform)
   {
      CriticalMsg1("mm_set_render_pipeline: pipeline %d not supported\n",pipe);
      return;
   }
   render_pipeline = &mmd_rpipelines[pipe];
}


void mm_render_model(mms_model *m, mms_custom_data *d, ulong flags)
{
   void *buff;

   TEST_VERSION("mm_render_model", m);

   AssertMsg(render_pipeline,
             "must call mm_init before rendering mesh model\n");

   // The real work, as you can see, varies by pipeline.
   {
      // allocate space for the buffer
      buff = (void *)Malloc(render_pipeline->buffsize(m, d));
      render_pipeline->set_buff(m, d, buff);
      _mm_set_globals(m, d);

      // transform points per segment, as well as normals and lights
      render_pipeline->transform(m, flags);

      // sort--or not
      render_pipeline->sort(m, flags);

      // call appropriate render func
      render_pipeline->render(m);
   }

   Free(buff);
}


///// PIPELINE COMPONENTS WHICH ARE ALSO EXTERNALLY CALLABLE

///// WRAPPERS TO EXPORT THE CURRENT BUFFER MANAGEMENT, TRANSFORM,
///// SORT, AND RENDER FUNCTIONS

void mm_transform_only(mms_model *m, ulong flags)
{
   TEST_VERSION("mm_transform_only", m);
   _mm_set_globals(m, mmd_custom_data);
   render_pipeline->transform(m, flags);
}


void mm_sort_only(mms_model *m, ulong flags)
{
   TEST_VERSION("mm_sort_only", m);
   _mm_set_globals(m, mmd_custom_data);
   render_pipeline->sort(m, flags);
}


void mm_render_only(mms_model *m)
{
   TEST_VERSION("mm_render_only", m);
   _mm_set_globals(m, mmd_custom_data);
   render_pipeline->render(m);
}


int mm_buffsize(mms_model *m, mms_custom_data *d)
{
   return render_pipeline->buffsize(m, d);
}


void mm_set_buff(mms_model *m, mms_custom_data *d, void *buff)
{
   render_pipeline->set_buff(m, d, buff);
}


///// POLYSORT PIPELINE (INTERNAL)

// When we sort by polygon we render the model by transforming all of
// the points for all segments and then sorting the polys and
// rendering them.  The sorting method chosen is based on model
// layout.

static void _mm_poly_transform(mms_model *m, ulong flags)
{
   int i;
   mms_segment *segptr;

#if 0 // need bounding data fields in mesh structure
   // Per object lighting setup
   if (mmd_light_setup_cback)
      mmd_light_setup_cback(&m->bmin,&m->bmax);
#endif

   // decide which segment transformer to use
   SET_SEG_TRANSFORM(m->layout, flags);

   segptr = mmd_segs;
   // iterate through segs, transforming vertex positions and normals,
   // and setting uvs.
   for(i = 0; i < m->segs; i++, segptr++)
   {
      transform_seg(segptr, i);
   }
   // calc normals for stretchy segments.  must be done after all segment
   // vertices have been transformed, since triangles may have vertices in
   // more than one segment.
   segptr = mmd_segs;
   for(i = 0; i < m->segs; i++, segptr++)
   {
      if(segptr->flags & MMSEG_FLAG_STRETCHY)
      {
         mm_transform_stretchy_normals(segptr);
      }
   }
}


static void _mm_poly_sort(mms_model *m, ulong flags)
{
   SET_POLYSORT(m->layout, flags);
   polysort(m);
}


static void _mm_poly_render(mms_model *m)
{
   int i;
   current_smatr_id=-1; // cached material value;

   block_render_cback(mmd_buff_sorted_pgons, m->pgons);
//   if(m->smatrs==1) // don't bother 
//   {
//      mm_setup_material(0);
//      mm_render_pgon_smatr_block(mmd_buff_sorted_pgons,m->pgons);
//   } else
//   {
//      mm_render_pgon_block(mmd_buff_sorted_pgons,m->pgons);
//   }

   for (i = mmd_custom_data->num_attachments - 1; i >= 0; --i)
      (*mmd_attachment_cback)(&mmd_buff_attach_trans[i],
                              &mmd_custom_data->attachment_list[i]);

   current_smatr_id=-1; // cached material value;
   if(use_debug) // draw joint positions
   {
      mm_dbg_draw_joints(m);
   }
   // reset the tmap mode
   // @TODO: figger out why this is here (Mat, 3/9/98)
   mmd_tmap_mode = R3_PL_TEXTURE;
}


static int _mm_poly_buffsize(mms_model *m, mms_custom_data *d)
{
   int size;

   // need space for transformed points
   size=m->verts*r3d_glob.cur_stride;

   // space for normal dot prods
   buff_norm_off = size;
   size+=sizeof(float)*(m->pgons);

   // space for sorted pgons
   buff_spgon_off = size;
   size+=sizeof(mms_pgon)*(m->pgons);

   buff_attach_off = size;
   if (d) 
      size += sizeof(mxs_trans) * (d->num_attachments);

   return size;
}


static void _mm_poly_set_buff(mms_model *m, mms_custom_data *d, void *buff)
{
   mmd_buff=buff;
   mmd_buff_points=buff;
   mmd_buff_norms = (float *)((uchar *)buff + buff_norm_off);
   mmd_buff_sorted_pgons=(mms_pgon *)((uchar *)buff + buff_spgon_off);
   mmd_buff_attach_trans = (mxs_trans *)((uchar*)buff + buff_attach_off);
}


///// HARDWARE PIPELINE (INTERNAL)

// Our hardware rendering assumes that we are letting the hardware
// zbuffer sort our polygons for us.  We render in material order.

// We have no separate transform function, reusing the one for poly
// sorting.

static void _mm_hardware_sort(mms_model *m, ulong flags)
{
#ifndef SHIP
   if (m->layout != MM_LAYOUT_MAT)
      Warning(("_mm_hardware_sort: mesh model not in material order.\n"));
#endif // ~SHIP

   return;
}


static void _mm_hardware_render(mms_model *m)
{
   int i;

   current_smatr_id = -1; // cached material value;

   block_render_cback(mmd_pgons, m->pgons);

   current_smatr_id = -1; // cached material value;

   // use callback to render any attachments
   for (i = mmd_custom_data->num_attachments - 1; i >= 0; --i)
      (*mmd_attachment_cback)(&mmd_buff_attach_trans[i],
                              &mmd_custom_data->attachment_list[i]);

   if (use_debug) // draw joint positions
      mm_dbg_draw_joints(m);

   // reset the tmap mode
   mmd_tmap_mode = R3_PL_TEXTURE;
}


static int _mm_hardware_buffsize(mms_model *m, mms_custom_data *d)
{
   int size;

   // need space for transformed points
   size = m->verts * r3d_glob.cur_stride;

   // space for normal dot prods
   buff_norm_off = size;
   size += sizeof(float) * (m->pgons);

   buff_attach_off = size;
   if (d) 
      size +=sizeof(mxs_trans) * (d->num_attachments);

   return size;
}


static void _mm_hardware_set_buff(mms_model *m, mms_custom_data *d, 
                                  void *buff)
{
   mmd_buff = buff;
   mmd_buff_points = buff;
   mmd_buff_norms = (float *)((uchar *)buff + buff_norm_off);
   mmd_buff_attach_trans = (mxs_trans *)((uchar*)buff + buff_attach_off);
}


///////////

// currently assumes always gouraud shaded.
static void mm_setup_material(int index)
{
   mms_smatr *r=&mmd_smatrs[index];
   int gouraud = mmd_rgb_lighting ? R3_PL_RGB_GOURAUD : R3_PL_GOURAUD;

   if(r->type==MM_MAT_TMAP)
   {
      r3_set_polygon_context(gouraud | R3_PL_POLYGON | mmd_tmap_mode);
      r3_set_texture((r3s_texture)r->handle);
   } else
   {
      r3_set_polygon_context(gouraud | R3_PL_POLYGON);
      r3_set_color(((index+50)%255)+1);
   }
   current_smatr_id=index;
}

static void setup_fake_material(int index)
{
   r3_set_polygon_context(R3_PL_UNLIT | R3_PL_POLYGON | R3_PL_SOLID);
   r3_set_color(((index+50)%255)+1);
}

static r3s_phandle vlist[3];

// XXX TO DO: make these for loops better
void mm_render_pgon_block(mms_pgon *pgons, int num)
{
   int i;
   mms_pgon *p = pgons;

   // unset previously set material.
   current_smatr_id=-1;

   r3_start_block();
   for(i = num + 1; --i; p++) // iterate num times
   {
      // Bail if it isn't facing you
      // use the cached normal dot prod
      if ((mmd_buff_norms[p->norm] + p->d) <= 0.0)
         continue;
    
      // set context if material has changed
      if(p->smatr_id!=current_smatr_id)
      {
         mm_setup_material(p->smatr_id);
      }

#ifdef DBG_ON
      if(use_debug)
      {
         setup_fake_material(p->norm);
      }
#endif
      // Set vlist to contain the list of verts
      vlist[2] = mmd_buff_point(p->v[0]);
      vlist[1] = mmd_buff_point(p->v[1]);
      vlist[0] = mmd_buff_point(p->v[2]);

      r3_draw_poly(3,vlist);
   }
   r3_end_block();
}

#if 0 // don't maintain this separately.  extra IF per poly isn't going to kill 'ya
static void mm_render_pgon_smatr_block(mms_pgon *pgons,int num)
{
   int i;
   mms_pgon *p=pgons;

   r3_start_block();
   for(i=0;i<num;i++,p++)
   {
      // Bail if it isn't facing you
      // use the cached normal dot prod
      if ((mmd_buff_norms[p->norm] + p->d) <= 0.0)
         continue;

#ifdef DBG_ON
      if(use_debug)
      {
         setup_fake_material(p->norm);
      }
#endif
    
      // Set vlist to contain the list of verts
      vlist[2] = mmd_buff_point(p->v[0]);
      vlist[1] = mmd_buff_point(p->v[1]);
      vlist[0] = mmd_buff_point(p->v[2]);

      r3_draw_poly(3,vlist);
   }
   r3_end_block();
}
#endif


// not supported yet
// both may require poly sort mapping
void mm_render_smatr(mms_smatr *r);
void mm_render_segment(mms_segment *s);


// You should set to R3_PL_TEXTURE or R3_PL_TEXTURE_LINEAR
void mm_set_tmap_mode(ulong mode)
{
   mmd_tmap_mode = mode;
}

////// UTILITIES TO HELP APP PARSE MESH MODEL FORMAT

// returns -1 if none found
int mm_segment_from_joint(mms_model *m, uint joint_index)
{
   int i;
   int num_segs = m->segs;
   mms_segment *seg = mm_segment_list(m);

   for(i = 0; i < num_segs; ++seg, ++i)
      if (seg->joint_id == joint_index)
         return i;

   return -1;
}


int mm_rigid_segment_from_joint(mms_model *m, uint joint_index)
{
   int i;
   int num_segs = m->segs;
   mms_segment *seg = mm_segment_list(m);

   for(i = 0; i < num_segs; ++seg, ++i)
      if (seg->joint_id == joint_index
       && !(seg->flags & MMSEG_FLAG_STRETCHY))
         return i;

   return -1;
}

float mm_set_global_alpha(float fNewGlobalAlpha)
{
    return 0.0f; // TODO
}

float mm_set_global_glow_level(float fNewGlowLevel)
{
    return 0.0f; // TODO
}



///// RAYCAST AGAINST MESH MODEL


// a hatful of globals
static mxs_vector g_ray;
static mxs_vector g_ray_normal;       // unit vector parallel to ray
static float g_ray_length;


static bool test_ray_against_segment(mms_segment *segment,
                                     mms_ray_hit_detect_info *detect_info)
{
   int i, j, list_end;
   mms_mapping *map;
   mms_pgon *polygon;
   mms_data_chunk *data;
   mxs_plane plane;
   mxs_vector *a, *b, *c;
   mxs_vector ab, bc, ca;
   mxs_real dist_to_start, dist_to_end;
   float intersection_time;
   mxs_vector intersection_point;
   mxs_vector vert_to_intersection;
   mxs_vector cross_test;
   bool hit = FALSE;

   map = &mmd_mappings[segment->map_start];
   for(i = 0; i < segment->smatsegs; ++map, ++i) {
      data = &mmd_smatsegs[*map].data;
      polygon = &mmd_pgons[data->pgon_start];
      list_end = data->pgon_start + data->pgons;

      for (j = data->pgon_start; j < list_end; ++polygon, ++j) {
         a = &mmd_buff_point(polygon->v[0])->p;
         b = &mmd_buff_point(polygon->v[1])->p;
         c = &mmd_buff_point(polygon->v[2])->p;

         // We need the plane which contains our triangle and faces
         // out.
         mx_sub_vec(&ab, b, a);
         mx_sub_vec(&bc, c, b);
         mx_cross_vec(&plane.v, &bc, &ab);
         mx_negeq_vec(&plane.v);

         // backface check
         if (mx_dot_vec(&plane.v, &g_ray_normal) > 0.0)
            continue;

         mx_normeq_vec(&plane.v);
         plane.d = -mx_dot_vec(&plane.v, a);

         // Next, we intersect the ray against the plane.  dist_to_end
         // should actually be negative if the plane is on our cast.
         dist_to_start = mx_dot_vec(detect_info->start, &plane.v) + plane.d;
         if (dist_to_start < 0.0)
            continue;

         dist_to_end = mx_dot_vec(detect_info->end, &plane.v) + plane.d;
         if (dist_to_end > 0.0)
            continue;

         intersection_time = dist_to_start / (dist_to_start - dist_to_end);
         if (intersection_time > detect_info->hit_time)
            continue;

         mx_scale_add_vec(&intersection_point, detect_info->start,
                          &g_ray, intersection_time);

         // And now the traditional are-we-to-the-right-of-every-edge
         // test.  Since our polygons are always triangles we unroll
         // this into three tests.

         // first side: ab
         mx_sub_vec(&vert_to_intersection, &intersection_point, a);
         mx_cross_vec(&cross_test, &vert_to_intersection, &ab);
         if (mx_dot_vec(&cross_test, &g_ray) < 0.0)
            continue;

         // second side: bc
         mx_sub_vec(&vert_to_intersection, &intersection_point, b);
         mx_cross_vec(&cross_test, &vert_to_intersection, &bc);
         if (mx_dot_vec(&cross_test, &g_ray) < 0.0)
            continue;

         // third side: ca
         mx_sub_vec(&ca, a, c);

         mx_sub_vec(&vert_to_intersection, &intersection_point, c);
         mx_cross_vec(&cross_test, &vert_to_intersection, &ca);
         if (mx_dot_vec(&cross_test, &g_ray) < 0.0)
            continue;

         detect_info->polygon_index = j;
         detect_info->hit_point = intersection_point;
         detect_info->hit_normal = plane.v;
         detect_info->hit_time = intersection_time;

         hit = TRUE;
         if (detect_info->early_out)
            return TRUE;
      }
   }

   return hit;
}


static ray_sphere_test(mms_ray_hit_detect_info *detect_info)
{
   float proj, center_to_ray_2;
   mxs_vector center_to_start;

   // Is the bounding sphere past either end of our ray?
   mx_sub_vec(&center_to_start, detect_info->bound_center,
              detect_info->start);
   proj = mx_dot_vec(&center_to_start, &g_ray_normal);

   if (proj < -detect_info->bound_radius
    || proj > (g_ray_length + detect_info->bound_radius))
      return FALSE;

   // Is the sphere center further than the radius from the ray?
   center_to_ray_2 = mx_mag2_vec(&center_to_start) - (proj * proj);
   if (center_to_ray_2
     > (detect_info->bound_radius * detect_info->bound_radius))
      return FALSE;

   return TRUE;
}


// This is a raycast against a mesh model.  Before it is called, the
// client app should make sure the joint callbacks are all in place.
BOOL mm_ray_hit_detect(mms_model *model,
                       mms_ray_hit_detect_info *detect_info)
{
   mms_segment *segment;
   int i, num_segments;

   mx_sub_vec(&g_ray, detect_info->end, detect_info->start);
   g_ray_length = mx_norm_vec(&g_ray_normal, &g_ray);
   detect_info->result = FALSE;
   detect_info->hit_time = 1.0;

   _mm_set_globals(model, NULL);
   SET_SEG_O2W(model->layout, flags);

   // @TODO: bounding sphere test
   if (!ray_sphere_test(detect_info))
      return FALSE;

   // We traverse the multiped one segment at a time, rotating our
   // segments incrementally in case we're doing the early-out thing.
   // We handle all non-stretchy segments first, since the stretchy
   // ones can depend on them.  This is only really an optimization if
   // we're allowing an early exit.
   num_segments = model->segs;

   segment = mmd_segs;
   for (i = 0; i < num_segments; ++segment, ++i)
      if (!(segment->flags & MMSEG_FLAG_STRETCHY))
         (*o2w_seg)(segment);

   segment = mmd_segs;
   for (i = 0; i < num_segments; ++segment, ++i)
      if (!(segment->flags & MMSEG_FLAG_STRETCHY))
         if (test_ray_against_segment(segment, detect_info)) {
            detect_info->result = TRUE;
            detect_info->segment_index = i;
            if (detect_info->early_out)
               goto done;
         }

   segment = mmd_segs;
   for (i = 0; i < num_segments; ++segment, ++i)
      if (segment->flags & MMSEG_FLAG_STRETCHY)
         (*o2w_seg)(segment);

   segment = mmd_segs;
   for (i = 0; i < num_segments; ++segment, ++i)
      if (segment->flags & MMSEG_FLAG_STRETCHY)
         if (test_ray_against_segment(segment, detect_info)) {
            detect_info->result = TRUE;
            detect_info->segment_index = i;
            if (detect_info->early_out)
               goto done;
         }

done:
   return detect_info->result;
}
