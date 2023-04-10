/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/mlight.c,v 1.18 2000/02/19 12:35:29 toml Exp $

#include <stdlib.h>
#include <md.h>
#include <mlight.h>
#include <matrix.h>
#include <mprintf.h>
#include <r3d.h>
#include <mm.h>
#include <lg.h>
// Need to delve in to get secret material info
#include <md_.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 
// TODO:
//   Make near lights interpolate from far to near so it doesn't pop
//
//   

#define MAG(r,g,b) (0.25*(r) + 0.5*(g) + 0.25*(b))
#define MAG_VEC(a) MAG((a).x, (a).y, (a).z)

// show debugging info
// #define MULTI_DEBUG

// maximum actual runtime lights we could compute per point
#define MAX_LIGHTS 32
int mld_multi_max_lights = 8;  // this should be <= MAX_LIGHTS

// use at most 4 near light computations
int mld_multi_max_near = 4;

// maximum number of lights we will process and sort
// it's odd to make it no bigger than MAX_LIGHTS, but mainly
// it should be much bigger than mld_multi_max_lights
#define MAX_LIGHTS_CONSIDERED 32

// Always add in 0.1 of ambient, this should always be overriden by the app
float mld_multi_ambient = 0.1;                          
#ifdef RGB_LIGHTING
rgb_vector mld_multi_rgb_ambient = { 0.0,0.0,0.0 };
#endif

// if the light is within 3 radii of the object, use a near light
float mld_multi_near_ratio = 3.0;

// explicitly compute the following percentage of the light, and
// turn surplus into ambient
float mld_multi_light_minimum = 0.95;

// if a light is turned into ambient, use 50% of the max (i.e.
// as if the dot product of the normal and light was 0.5)
float mld_multi_far_ambient_fraction = 0.5;

// value to which to clamp each light value
float mld_multi_bright_max = 1.0;

// print out info about lighting
bool mld_multi_debug = FALSE;

// set this to a highlight value if you want this object brighter
float mld_multi_hilight = 0.0;

static float multi_ambient;
#ifdef RGB_LIGHTING
static rgb_vector multi_rgb_ambient;
#endif
static mls_multi_light light_data[MAX_LIGHTS];
static mls_multi_light world_light_data[MAX_LIGHTS];
static int num_near_light, num_far_light;
static BOOL light_invalid = TRUE;

BOOL mld_multi_ambient_only = FALSE;
BOOL mld_multi_unlit=FALSE;

bool mld_multi_rgb=FALSE;

void ml_multi_compute_lights_in_object_space(void)
{
   int i,j;

   for (i=0; i < num_near_light; ++i)
      r3_transform_w2o(&light_data[i].loc, &world_light_data[i].loc);

   for (j=0; j < num_far_light; ++j, ++i)
      r3_rotate_w2o(&light_data[i].loc, &world_light_data[MAX_LIGHTS-1-j].loc);
}

#define A_FLT_ENTRY(array,index,step) (*(float *)(((uchar *)array)+(step)*(index)))
#define A_ULONG_ENTRY(array,index,step) (*(ulong *)(((uchar *)array)+(step)*(index)))
#define A_USHORT_ENTRY(array,index,step) (*(ushort *)(((uchar *)array)+(step)*(index)))

#define A_FLT_PTR(array,index,step) ((float *)(((uchar *)array)+(step)*(index)))

void ml_multi_light_cback(int num, float *out, ulong *norm,
                          int norm_step, ushort *pmap, int pmap_step,
                          mxs_vector *pts)
{
   float tmp;
   int i, j;
   mls_multi_light *light;


#ifdef RGB_LIGHTING
   rgb_vector init;
   if (mld_multi_rgb) {
      init.x = multi_rgb_ambient.x + mld_multi_hilight;
      init.y = multi_rgb_ambient.y + mld_multi_hilight;
      init.z = multi_rgb_ambient.z + mld_multi_hilight;
   } else
      tmp = MAG_VEC(multi_rgb_ambient) + mld_multi_hilight;
#else
   tmp = multi_ambient + mld_multi_hilight;
#endif

   if (mld_multi_unlit)
   {
#ifdef RGB_LIGHTING
      if (mld_multi_rgb) {
         for (i=0,j=0; i < num; ++i, j += 3)
            out[j] = out[j+1] = out[j+2] = mld_multi_bright_max;
      } else
#endif
      {
         for (i=0; i < num; ++i)
            out[i] = mld_multi_bright_max;
      }
      return;
   }

   // set ambient for all lights
#ifdef RGB_LIGHTING
   if (mld_multi_rgb) {
      for (j=0,i=0; i < num; ++i) {
         out[j++] = init.x;
         out[j++] = init.y;
         out[j++] = init.z;
      }
   } else
#endif
   for (i=0; i < num; ++i)
      out[i] = tmp;

   if (mld_multi_ambient_only) {
      md_light_set_type(MD_LT_AMB);
      return;
   } else
      md_light_set_type(MD_LT_AMB | MD_LT_DIFF);

   if (light_invalid)
      ml_multi_compute_lights_in_object_space();      

   light = light_data;

   // compute all near lights
   if(pmap)
   {
      for (i=0; i < num_near_light; ++i) {
#ifdef RGB_LIGHTING
         float bright = MAG_VEC(light->bright);
#endif
         for (j=0; j < num; ++j) {
            mxs_vector light_vec;
            float lt;
            int n = A_ULONG_ENTRY(norm,j,norm_step);
            int pt = A_USHORT_ENTRY(pmap,j,pmap_step);
            light_vec.x = light->loc.x - pts[pt].x;
            light_vec.y = light->loc.y - pts[pt].y;
            light_vec.z = light->loc.z - pts[pt].z;
            lt = (X_NORM(n) * light_vec.x +
                  Y_NORM(n) * light_vec.y +
                  Z_NORM(n) * light_vec.z);
            if (lt > 0) {
               lt = lt /  mx_mag2_vec(&light_vec);
#ifdef RGB_LIGHTING
               if (mld_multi_rgb) {
                  out[j*3+0] += lt * light->bright.x;
                  out[j*3+1] += lt * light->bright.y;
                  out[j*3+2] += lt * light->bright.z;
               } else
                  out[j] += lt * bright;
#else
               out[j] += lt * light->bright;
#endif
            }
         }
         ++light;
      }
   } else
   {
      for (i=0; i < num_near_light; ++i) {
#ifdef RGB_LIGHTING
         float bright = MAG_VEC(light->bright);
#endif
         for (j=0; j < num; ++j) {
            mxs_vector light_vec;
            float lt;
            int n = A_ULONG_ENTRY(norm,j,norm_step);
            light_vec.x = light->loc.x - pts[j].x;
            light_vec.y = light->loc.y - pts[j].y;
            light_vec.z = light->loc.z - pts[j].z;
            lt = (X_NORM(n) * light_vec.x +
                  Y_NORM(n) * light_vec.y +
                  Z_NORM(n) * light_vec.z);
            if (lt > 0) {
               lt = lt /  mx_mag2_vec(&light_vec);
#ifdef RGB_LIGHTING
               if (mld_multi_rgb) {
                  out[j*3+0] += lt * light->bright.x;
                  out[j*3+1] += lt * light->bright.y;
                  out[j*3+2] += lt * light->bright.z;
               } else
                  out[j] += lt * bright;
#else
               out[j] += lt * light->bright;
#endif
            }
         }
         ++light;
      }
   }

   // compute all far lights
   for (i=0; i < num_far_light; ++i) {
#ifdef RGB_LIGHTING
      float bright = MAG_VEC(light->bright);
#endif
      for (j=0; j < num; ++j) {
         int n = A_ULONG_ENTRY(norm,j,norm_step);
         float lt = X_NORM(n) * light->loc.x +
                    Y_NORM(n) * light->loc.y +
                    Z_NORM(n) * light->loc.z;
         if (lt > 0)
#ifdef RGB_LIGHTING
            if (mld_multi_rgb) {
               out[j*3+0] += lt * light->bright.x;
               out[j*3+1] += lt * light->bright.y;
               out[j*3+2] += lt * light->bright.z;
            } else
               out[j] += lt * bright;
#else
            out[j] += lt;
#endif
      }
      ++light;
   }

#ifdef RGB_LIGHTING
   if (mld_multi_rgb)
      num *= 3;
   return;
#endif

   for (j=0; j < num; ++j)
   {
      if (out[j] > mld_multi_bright_max)
         out[j] = mld_multi_bright_max;
      else if (out[j] < 0)
         out[j] = 0;
   }
}

void ml_multi_light_obj(int which)
{
   light_invalid = TRUE;
}

//// initialize data structures

static void add_far_light(mls_multi_light *light, mxs_vector *obj)
{
   mxs_vector light_dir;

   mx_sub_vec(&light_dir, &light->loc, obj);
#ifdef RGB_LIGHTING
   mx_scaleeq_vec(&light_dir, 1 / mx_mag2_vec(&light_dir));
#else
   mx_scaleeq_vec(&light_dir, light->bright / mx_mag2_vec(&light_dir));
#endif
   
   ++num_far_light;
   world_light_data[MAX_LIGHTS-num_far_light].loc = light_dir;
   world_light_data[MAX_LIGHTS-num_far_light].bright = light->bright;
}

static void fixup_far_lights(void)
{
   int i;
   for (i=0; i < num_far_light; ++i)
      light_data[i+num_near_light].bright = world_light_data[MAX_LIGHTS-i-1].bright;
}

static void add_near_light(mls_multi_light *light, mxs_vector *obj)
{
   world_light_data[num_near_light] = *light;
   light_data[num_near_light].bright = light->bright;
   num_near_light++;
}

static float *far_bright_ptr;
static int light_compare(const void *p, const void *q)
{
   int a = * (uchar *) p;
   int b = * (uchar *) q;

   if (far_bright_ptr[a] > far_bright_ptr[b])
      return -1;  // move large guys to the front
   return far_bright_ptr[a] < far_bright_ptr[b];
}

float ml_multi_light_on_object(mls_multi_light *inp, mxs_vector *obj_loc)
{
   mxs_vector light_vec;
   mx_sub_vec(&light_vec, &inp->loc, obj_loc);
#ifdef RGB_LIGHTING
   return MAG_VEC(inp->bright) / mx_mag_vec(&light_vec);
#else
   return inp->bright / mx_mag_vec(&light_vec);
#endif
}

int ml_multi_set_lights_for_object(int num, mls_multi_light *inp, 
                                   mxs_vector *obj_loc, float obj_radius)
{
   int i, num_inserted, max;
   uchar light_list[MAX_LIGHTS_CONSIDERED];
   uchar light_near[MAX_LIGHTS_CONSIDERED];
   float far_bright[MAX_LIGHTS_CONSIDERED];
   float total_bright, added_bright, enough_light, near_light_dist;

   // clear existing lights
   num_near_light = num_far_light = 0;

   // initialize ambient to user ambient (we may add more ambient
   // to simulate far away/faint lights)
   multi_ambient = mld_multi_ambient;
#ifdef RGB_LIGHTING
   multi_rgb_ambient = mld_multi_rgb_ambient;
#endif

   // if we have too many lights, turn the surplus into ambient
   if (num > MAX_LIGHTS_CONSIDERED) {
#ifdef MULTI_DEBUG
      if (mld_multi_debug)
         mprintf("Too many input lights to sort, using %d of %d\n",
                  MAX_LIGHTS_CONSIDERED, num);
#endif
      for (i = MAX_LIGHTS_CONSIDERED; i < num; ++i) {
         // compute the lighting for this to the object center
         mxs_vector light_vec;
         mx_sub_vec(&light_vec, &inp[i].loc, obj_loc);
#ifdef RGB_LIGHTING
         if (mld_multi_rgb)
            mx_scale_addeq_vec(&multi_rgb_ambient, &inp[i].bright,
               mld_multi_far_ambient_fraction / mx_mag_vec(&light_vec));
         else
           multi_ambient += MAG_VEC(inp[i].bright) / mx_mag_vec(&light_vec)
               * mld_multi_far_ambient_fraction;
#else
         multi_ambient += inp[i].bright / mx_mag_vec(&light_vec)
               * mld_multi_far_ambient_fraction;

#endif
      }
      num = MAX_LIGHTS_CONSIDERED;
   }

   // determine for what light distance squared we should use near lights

   near_light_dist = obj_radius * mld_multi_near_ratio;

   total_bright = 0;
   // now, iterate through the remaining lights, compute their far brightness
   for (i=0; i < num; ++i) {
      mxs_vector light_vec;
      float mag;

      // vector from object to light
      mx_sub_vec(&light_vec, &inp[i].loc, obj_loc);

      // length of vector
      mag = mx_mag_vec(&light_vec);

      // determine if we make it a near light
      light_near[i] = (mag < near_light_dist);
#if 0
      if (mld_multi_debug && light_near[i])
         mprintf("mld_debug: using near light for light %d\n", i);
#endif

      // "far brightness" is brightness / dist
#ifdef RGB_LIGHTING
      far_bright[i] = MAG_VEC(inp[i].bright)/mag;
#else
      far_bright[i] = inp[i].bright / mag;
#endif

      total_bright += far_bright[i];
      light_list[i] = i;
   }

   // setup context for sorting on the far_bright values
   far_bright_ptr = far_bright;

   qsort(light_list, num, sizeof(light_list[0]), light_compare);

   added_bright = 0;
   enough_light = total_bright * mld_multi_light_minimum;

   max = mld_multi_max_lights;
   if (max > MAX_LIGHTS) max = MAX_LIGHTS;

   if (num < max)
      max = num;

   for (i=0; i < max; ++i) {
      int j = light_list[i];
      // add this light
      if (light_near[j] && num_near_light < mld_multi_max_near) {
         add_near_light(inp+j, obj_loc);
      } else {
#ifdef MULTI_DEBUG
         if (mld_multi_debug && light_near[j])
            mprintf("Too many near lights; forcing %d to be far.\n", j);
#endif
         add_far_light(inp+j, obj_loc);
      }
      added_bright += far_bright[j];
      // don't print debug stuff if we got to the end
#ifdef MULTI_DEBUG
      if (mld_multi_debug && i == num-1) continue;
#endif
      if (added_bright > enough_light) {
#ifdef MULTI_DEBUG
         if (mld_multi_debug)
            mprintf("Bright enough to stop at %d of %d lights.\n", i+1, num);
#endif
         ++i;
         break;
      }
   }

   fixup_far_lights();

   num_inserted = i;
#ifdef MULTI_DEBUG
   if (mld_multi_debug && i == max && max != num)
      mprintf("Hit maximum number lights: %d of %d\n", max, num);

   if (mld_multi_debug)
      mprintf("%d near, %d far, %d total\n",
            num_near_light, num_far_light, num);
#endif

   // add remaining lights as ambient
   for (; i < num; ++i) {
      int j = light_list[i];
#ifdef RGB_LIGHTINGo
      // if we're adding a far lighting in colored mode, we need to
      // actually use a full light calculation again
      float mag = MAG_VEC(inp[j].bright);
      if (mag > 0) {
         mx_scale_addeq_vec(&multi_rgb_ambient, &inp[j].bright,
            far_bright[j] / mag * mld_multi_far_ambient_fraction);
      }
#else
      multi_ambient += far_bright[j] * mld_multi_far_ambient_fraction;
#endif
   }

   return num_inserted;
}


#define MD_LIGHT_NORM_OFF 4
#define MD_LIGHT_POINT_OFF 2

#define S_OFFSET(ptr,offset) (((uchar *)ptr)+(offset))

#ifdef RGB_LIGHTING
// Set this if you want a colored light in the bulb, rather
// than a white light.  It will scale accordingly
// For instance, if you had a green light, set to 0,1,0
// or something like that
mxs_vector mld_illum_rgb = {1,1,1};
#endif
// Change this scale to change the total illum, both for rgb and non
float mld_illum_scale = 1.0;

void md_multi_light_cback(int num, float *out, mds_light *lts, mxs_vector *pts, r3s_point *tps)
{
    mds_model *m = mdd.model;

    ml_multi_light_cback(num, out,
                         (ulong *)S_OFFSET(lts,MD_LIGHT_NORM_OFF),sizeof(*lts),
                         (ushort *)S_OFFSET(lts,MD_LIGHT_POINT_OFF),sizeof(*lts),
                         pts);

    // Augment for new version, doesn't exist for older models
    if (mdd.use_illum)
    {
        int i;
        float illum;
        for (i=0;i<num;++i) {
            illum = md_amat(m,lts[i].mat)->illum * mld_illum_scale;
            if (illum>0) {
#ifdef RGB_LIGHTING
                if (mld_multi_rgb) {//zb
                    int j = i*3;
                    out[j++] += illum*mld_illum_rgb.x;
                    out[j++] += illum*mld_illum_rgb.y;
                    out[j] += illum*mld_illum_rgb.z;
                } else {//zb
                    out[i] += illum; 
                    if (out[i]>mld_multi_bright_max) 
                        out[i]=mld_multi_bright_max;
                }
#else
                out[i] += illum;
                // @TODO this is dumb, the 2d and 3d should deal with overflow
                // and truncate on it's own, or the shading table should be 
                // big enough to deal with light x 2
                // Or, we should find out how big the table really is and 
                // truncate it there.
                if (out[i]>mld_multi_bright_max) out[i]=mld_multi_bright_max;
#endif
            }
        }
    }
}

#define MM_PT_I_OFF 32
#define MM_UVN_NORM_OFF 8

void mm_multi_light_cback(int num, float *tps, mms_uvn *uvns, mxs_vector *pts)
{
   ml_multi_light_obj(MD_OBJ_PUSH);
   ml_multi_light_cback(num,tps,
      (ulong *)S_OFFSET(uvns,MM_UVN_NORM_OFF),sizeof(*uvns),
      NULL,0,pts);
   ml_multi_light_obj(MD_OBJ_POP);
}

void ml_init_multi_lighting(void)
{
   // install the lighting routine
   md_set_light_callback(md_multi_light_cback);
   md_set_light_obj_callback(ml_multi_light_obj);

   mm_set_light_callback(mm_multi_light_cback);
}
