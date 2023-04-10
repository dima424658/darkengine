/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/objlight.c,v 1.46 2000/02/24 23:42:46 mahk Exp $
// code for multiple light source lighting

#include <lg.h>
#include <r3d.h>
#include <mprintf.h>

#include <mlight.h>
#include <portal.h>
#include <wr.h>
#include <rendprop.h>
#include <osystype.h>
#include <objlight.h>
#include <objpos.h>
#include <elitprop.h>
#include <osysbase.h>
#include <iobjsys.h>
#include <objnotif.h>
#include <appagg.h>
#include <missrend.h>
#include <objshape.h>

// for new lighting calc:
#include <objcast.h>

ObjID highlit_obj=OBJ_NULL;

///////////////////////////////////////
// multiple light system/setup

float objlight_highlight_level=0.35;

// XXX should only be used in debug
BOOL g_bAllObjsUnlit=FALSE;

int num_light, num_dyn;
mls_multi_light light_data[MAX_STATIC];
mls_multi_light light_this[32];

#define SUNLIGHT_INDEX 0
#define FIRST_LIGHT (SUNLIGHT_INDEX + 1)// reserve one index for sunlight

// how many feet away sunlight is considered to be for object lighting
#define SUNLIGHT_DISTANCE 125.0


void reset_object_lighting(void)
{
   num_light=FIRST_LIGHT;
}

void reset_dynamic_lighting(void)
{
   reset_dynamic_lights();
   num_dyn = 0;
}

extern int hack_red, hack_green, hack_blue;
float light_scale=32.0;
void add_dynamic_light(Location *loc, float bright, float clamp_radius)
{
   portal_add_simple_dynamic_light(bright, 0, loc, clamp_radius);
   if (num_dyn < MAX_DYNAMIC) {
      light_data[num_light + num_dyn].loc = loc->vec;
#ifdef RGB_LIGHTING
      bright /= 255;
      light_data[num_light + num_dyn].bright.x = hack_red*bright/light_scale;
      light_data[num_light + num_dyn].bright.y = hack_green*bright/light_scale;
      light_data[num_light + num_dyn].bright.z = hack_blue*bright/light_scale;
#else
      light_data[num_light + num_dyn].bright = bright/light_scale;
#endif
      ++num_dyn;
   }
   else
      CriticalMsg("Out of Dynamic Lights");
}

#ifdef RGB_LIGHTING
int add_object_light(mxs_vector *where, rgb_vector *bright, float radius)
#else
int add_object_light(mxs_vector *where, float bright, float radius)
#endif
{
   if (num_light < MAX_STATIC - MAX_DYNAMIC) {
      light_data[num_light].loc = *where;
#ifdef RGB_LIGHTING
      light_data[num_light].bright.x = bright->x/light_scale;
      light_data[num_light].bright.y = bright->y/light_scale;
      light_data[num_light].bright.z = bright->z/light_scale;
#else
      light_data[num_light].bright = bright/light_scale;
#endif
      light_data[num_light].inner = -1.0;
      light_data[num_light].radius = radius;
      return num_light++;
   }
   else
   {
      CriticalMsg2("Out of Lights (at %d allowed %d)",num_light,MAX_STATIC-MAX_DYNAMIC);
      return -1;
   }
}

#ifdef RGB_LIGHTING
int add_object_spotlight(mxs_vector *where, rgb_vector *bright, mxs_vector *dir, float inner, float outer, float radius)
#else
int add_object_spotlight(mxs_vector *where, float bright, mxs_vector *dir, float inner, float outer, float radius)
#endif
{
   if (num_light < MAX_STATIC - MAX_DYNAMIC) {
      light_data[num_light].loc = *where;
#ifdef RGB_LIGHTING
      light_data[num_light].bright.x = bright->x/light_scale;
      light_data[num_light].bright.y = bright->y/light_scale;
      light_data[num_light].bright.z = bright->z/light_scale;
#else
      light_data[num_light].bright = bright/light_scale;
#endif
      light_data[num_light].dir   = *dir;
      light_data[num_light].inner = inner;
      light_data[num_light].outer = outer;
      light_data[num_light].radius = radius;
      return num_light++;
   } else
      return -1;
}


#ifdef RGB_LIGHTING //zb
void set_object_light(int light_index, float bright, rgb_vector* p_color)
#else
void set_object_light(int light_index, float bright)
#endif
{
   if (light_index == -1)
      return;

#ifdef RGB_LIGHTING
   //hack the color

   light_data[light_index].bright.x = p_color->x/light_scale;
   light_data[light_index].bright.y = p_color->y/light_scale;
   light_data[light_index].bright.z = p_color->z/light_scale;
   /* gray:
   light_data[light_index].bright.x = bright/light_scale;
   light_data[light_index].bright.y = bright/light_scale;
   light_data[light_index].bright.z = bright/light_scale;
   */
#else
   light_data[light_index].bright = bright/light_scale;
#endif
}

BOOL is_object_light_defined(int handle)
{
   return handle<num_light;
}

/////////////////////////////////////////////////////////
//
//  cache raycasts from objects to lights
//

BOOL location_sees_light(Location *loc, int light_id)
{
   Location dummy;
   Location p;
   if (light_id == SUNLIGHT_INDEX) {
      return portal_loc_has_sunlight(loc);
   } else {
      MakeLocationFromVector(&p, &light_data[light_id].loc);

      if (PortalRaycast(loc, &p, &dummy, 0))
      {
         return ObjRaycastC(loc, &p, &dummy, TRUE,
                        0, 1, g_pObjShadowProp, FALSE) == kObjCastNone;
      }
      else
         return FALSE;
   }
}



struct
{
   int    cell_id;     // last place we saw this object
   ulong  shadows[3];  // bits of shadow casts; 0 = shadowed, 1 = visible
} shadow_cache[HACK_MAX_OBJ];

ulong *objGetShadows(ObjID obj)
{
   Location *loc= &ObjPosGet(obj)->loc;
   int cell = CellFromLoc(loc);
   Assert_(gMaxObjID <= HACK_MAX_OBJ);

   if (shadow_cache[obj].cell_id != cell)
   {
      int *shadPtr=&shadow_cache[obj].shadows[0];
      int i,k,n,inner_frac;
      ushort *lt = WR_CELL(cell)->light_indices;
      n = *lt++;
      for (i=0; i<n; i+=32, shadPtr++)
      {
         int shadows=0;
         if (i+32<=n)
            inner_frac=32;
         else
            inner_frac=n&0x1f;
         for (k=0; k<inner_frac; ++k)
            if (location_sees_light(loc, lt[i+k]))
               shadows |= (1 << k);
         *shadPtr=shadows;
      }
      shadow_cache[obj].cell_id = cell;
   }
   return shadow_cache[obj].shadows;
}

void objShadowUncache(ObjID obj)
{
   Assert_(obj < HACK_MAX_OBJ && obj >= 0);
   shadow_cache[obj].cell_id = -1;
}

void ObjShadowClearCache(void)
{
   int i;
   for (i=0; i < HACK_MAX_OBJ; ++i)
      objShadowUncache(i);
}

static void ObjListener(ObjID obj, eObjNotifyMsg msg, void* data)
{
   if (msg == kObjNotifyDelete && OBJ_IS_CONCRETE(obj))
      objShadowUncache(obj);
}
static sObjListenerDesc g_ObjListenerDesc =
{
   ObjListener, 0,
};

static void ObjPosListener(ObjID obj, const ObjPos* pos, void* data)
{
   objShadowUncache(obj);
}

static tObjListenerHandle g_ObjListenerHandle = 0;

// Reref all "dirty" objects & call callbacks
void ObjShadowInit(void)
{
   IObjectSystem *ios = AppGetObj(IObjectSystem);
   g_ObjListenerHandle = IObjectSystem_Listen(ios, &g_ObjListenerDesc);
   ObjPosListen(ObjPosListener, NULL);
   ObjShadowClearCache();
}

void ObjShadowTerm(void)
{
   IObjectSystem *ios = AppGetObj(IObjectSystem);
   IObjectSystem_Unlisten(ios, g_ObjListenerHandle);
}

/////////////////////////////////////////////////////////
//
//  evaluate lighting at location / on object

extern float portal_evaluate_spotlight(mxs_vector *loc, mxs_vector *light_loc, mxs_vector *light_dir, float inner, float outer);

// return FALSE to mean out of light space, else TRUE to mean keep going
static BOOL _apply_one_location_light(int i, Location *loc, int *n)
{
   if (light_data[i].radius &&
       mx_dist2_vec(&loc->vec, &light_data[i].loc) > light_data[i].radius*light_data[i].radius)
      return TRUE;
   if (light_data[i].inner != -1.0)
   {
      float scale = portal_evaluate_spotlight(&loc->vec,
                 &light_data[i].loc, &light_data[i].dir,
                 light_data[i].inner, light_data[i].outer);
      if (scale != 0)
      {
         light_this[*n] = light_data[i];
#ifdef RGB_LIGHTING
         mx_scaleeq_vec(&light_this[*n].bright, scale);
#else
         light_this[*n].bright *= scale;
#endif
         (*n)++;   // @TODO: see if this is the best way to do the N inc
      }
   }
   else
      light_this[(*n)++] = light_data[i];
   return (*n)<32;
}


static void setup_location_lighting(Location *loc, mxs_vector *newlight, int *shadPtr,float radius)
{
   int i, n, num, k, inner_frac;
   int cell = CellFromLoc(loc);
   ushort *lt;
   int shadows;

   if(cell<0||cell>=wr_num_cells)
   {
      Warning(("Invalid location for lighting: %g,%g,%g\n",loc->vec.x,loc->vec.y,loc->vec.z));
      return;
   }
   if (cell==CELL_INVALID) return;

   lt = WR_CELL(cell)->light_indices;

   num = num_light + num_dyn;

   // put sunlight next to object
   mx_scale_add_vec(&light_data[SUNLIGHT_INDEX].loc, &loc->vec,
                    &portal_sunlight_norm, SUNLIGHT_DISTANCE + radius);

   // first iterate over non-dynamic lights
   for (n=0, i=0; i <= lt[0]; i += 32, shadPtr++)
   {
      if (i+32<=lt[0])
         inner_frac=32;
      else
         inner_frac=lt[0]&0x1f;
      for (k=1, shadows=*shadPtr; k <= inner_frac; ++k, shadows >>= 1)
         if (shadows & 1)
            if (!_apply_one_location_light(lt[i+k],loc,&n))
               goto got_lights;
   }

   for (i=num_light; i < num; ++i)
   {
      Location p, hit;
      MakeLocationFromVector(&p, &light_data[i].loc);
      if (PortalRaycast(loc, &p, &hit, 0))
      {
         light_this[n++] = light_data[i];
         if (n == 32) break;
      }
   }

   // @QUESTION: hmmmm. why is this always 2.0?
got_lights:
   if (n==32)
      Warning(("Hey, 32 lights at %g %g %g\n",loc->vec.x,loc->vec.y,loc->vec.z));
   ml_multi_set_lights_for_object(n, light_this, &loc->vec, radius); // obj radius
}

void setup_object_lighting(ObjID o, mxs_vector *newlight, float bright)
{
   sExtraLightProp *extra_light;
   const sMissionRenderParams *parms;
   Location *loc= &ObjPosGet(o)->loc;
#ifdef DBG_ON
   mld_multi_unlit=(ObjRenderType(o)==kRenderUnlit)||g_bAllObjsUnlit;
#else
   mld_multi_unlit=(ObjRenderType(o)==kRenderUnlit);
#endif
   mld_multi_hilight=(o==highlit_obj)?objlight_highlight_level:0;

   mld_multi_ambient_only = FALSE;

   // get info for level
   parms = GetMissionRenderParams();
   mx_copy_vec(&mld_multi_rgb_ambient, &parms->ambient_light);
   mx_copy_vec(&light_data[SUNLIGHT_INDEX].bright, &parms->sun_scaled_rgb);

   if (ObjExtraLightGet(o, &extra_light)) {
#ifdef RGB_LIGHTING
      int i;
      for (i = 0; i < 3; ++i) {
         mld_multi_rgb_ambient.el[i] += extra_light->m_fValue;
         if ((mld_multi_rgb_ambient.el[i] + mld_multi_hilight) > 1.0)
            mld_multi_rgb_ambient.el[i] = 1.0 - mld_multi_hilight;
         if (!extra_light->m_bAdditive)
            mld_multi_ambient_only = TRUE;
      }
#else
      mld_multi_ambient += extra_light->m_fValue;
      if ((mld_multi_ambient + mld_multi_hilight) > 1.0)
         mld_multi_ambient = 1.0 - mld_multi_hilight;
      if (!extra_light->m_bAdditive)
         mld_multi_ambient_only = TRUE;
#endif

#ifndef SHIP
      if (extra_light->m_fValue < -1.0 || extra_light->m_fValue > 1.0)
         mprintf("Extra light on object %d not in range -1 to 1: %g\n",
                 o, extra_light->m_fValue);
#endif // ~SHIP
   }

   if (!mld_multi_unlit && CellFromLoc(loc) != CELL_INVALID)
      setup_location_lighting(loc,newlight,objGetShadows(o),ObjGetRadius(o));
}

// apply light index i to an obj at location loc
static float _apply_one_light(int i, Location *loc)
{
   float scale = 1.0;

   if (i == SUNLIGHT_INDEX) {
      mx_scale_add_vec(&light_data[SUNLIGHT_INDEX].loc, &loc->vec,
                       &portal_sunlight_norm, SUNLIGHT_DISTANCE);
      return ml_multi_light_on_object(&light_data[i], &loc->vec);
   }

   if (light_data[i].radius &&
       mx_dist2_vec(&loc->vec, &light_data[i].loc) > light_data[i].radius*light_data[i].radius)
      return 0.0;
   if (light_data[i].inner != -1.0)
      scale = portal_evaluate_spotlight(&loc->vec,
                   &light_data[i].loc,  &light_data[i].dir,
                    light_data[i].inner, light_data[i].outer);
   return scale*ml_multi_light_on_object(&light_data[i], &loc->vec);
}

//
// wsf: new for this function: check for objects, too. We do this for a limited # of objects, however.
//
//
float compute_object_lighting(ObjID o)
{
   Location dummy;
   Position* pos = ObjPosGet(o);
   Location *loc = &pos->loc;
   int cell = CellFromLoc(loc);
   int shadows, *shadPtr;
   int i, num, k, inner_frac; // yea, lets loop factor ourselves
   ushort *lt;
   float total=0;

   if (cell==CELL_INVALID) return 0.00;

   lt = WR_CELL(cell)->light_indices;
   num = num_light + num_dyn;

   // first iterate over non-dynamic lights
   shadPtr = objGetShadows(o);
   for (i=0; i <= lt[0]; i += 32, shadPtr++)
   {
      if (i+32<=lt[0])
         inner_frac=32;
      else
         inner_frac=lt[0]&0x1f;
      for (k=1, shadows=*shadPtr; k <= inner_frac; ++k, shadows >>= 1)
         if (shadows & 1)
            total+=_apply_one_light(lt[i+k],loc);
   }

   // now the dynamic ones
   for (i=num_light; i < num; ++i)
   {
      Location p;
      MakeLocationFromVector(&p, &light_data[i].loc);

      if (PortalRaycast(loc, &p, &dummy, 0))
      {
         if (ObjRaycastC(loc, &p, &dummy, TRUE,
                        0, 0, g_pObjShadowProp, FALSE) == kObjCastNone)
            total += ml_multi_light_on_object(&light_data[i], &loc->vec);
      }
   }

   return total;
}


#ifndef SHIP
#include <objedit.h>
#include <stdlib.h>  // qsort, min?
#include <memall.h>
#include <dbmem.h>   // must be last header! 

typedef struct {
   float       val;
   ObjID       obj; // if we can get it
   int         l_idx;
   mxs_vector *vec;
} sObjLightSort;

#define MAX_LIGHTS_PER 96
static sObjLightSort seen_lights[MAX_LIGHTS_PER];
static int seen_index;

static float _add_seen(float val, ObjID obj, int idx, mxs_vector *loc)
{
   if (val>0.0)
   {
      seen_lights[seen_index].val = val;
      seen_lights[seen_index].obj = obj;
      seen_lights[seen_index].l_idx = idx;
      seen_lights[seen_index].vec = loc;
      if (++seen_index>=MAX_LIGHTS_PER)
      {
         Warning(("Hey! out of static light space to think about\n"));
         --seen_index;
      }
   }
   return val;
}

static int compare_seen(const void *p, const void *q)
{
   sObjLightSort *a = (sObjLightSort *)p;
   sObjLightSort *b = (sObjLightSort *)q;

   if (a->val>b->val)
      return -1;
   return a->val<b->val;
}

static void _show_seen(int how_many)
{
   int i;
   qsort(seen_lights,seen_index,sizeof(sObjLightSort),compare_seen);
   for (i=0; i<min(how_many,seen_index); i++)
   {
      sObjLightSort *s=&seen_lights[i];
      char buf[256];
      sprintf(buf,"Contributor %2.2d: val %f idx %d ",i, s->val, s->l_idx);
      if (s->obj)
         sprintf(buf+strlen(buf),"is obj %s\n",ObjEditName(seen_lights[i].obj));
      else
         sprintf(buf+strlen(buf),"is at %f %f %f\n",s->vec->x,s->vec->y,s->vec->z);
      mprintf(buf);
   }
}

float blame_object_lighting(ObjID o)
{
   Position* pos = ObjPosGet(o);
   Location *loc = &pos->loc;
   int cell = CellFromLoc(loc);
   int shadows, *shadPtr;
   int i, num, k, inner_frac;
   ushort *lt;
   float total=0;

   if (cell==CELL_INVALID) return 0.00;

   seen_index=0;

   lt = WR_CELL(cell)->light_indices;
   num = num_light + num_dyn;

   // first iterate over non-dynamic lights
   shadPtr = objGetShadows(o);

   for (i=0; i <= lt[0]; i += 32, shadPtr++)
   {
      if (i+32<=lt[0])
         inner_frac=32;
      else
         inner_frac=lt[0]&0x1f;
      for (k=1, shadows=*shadPtr; k <= inner_frac; ++k, shadows >>= 1)
         if (shadows & 1)
         {
            int j=lt[i+k];
            total+=_add_seen(_apply_one_light(j,loc),OBJ_NULL,j,&light_data[j].loc);
         }
   }

   // now the dynamic ones
   for (i=num_light; i < num; ++i)
   {
      Location p, hit;
      MakeLocationFromVector(&p, &light_data[i].loc);
      if (PortalRaycast(loc, &p, &hit, 0))
         total += _add_seen(ml_multi_light_on_object(&light_data[i], &loc->vec),
                            OBJ_NULL, i, &light_data[i].loc);
   }

   _show_seen(8);

   if (total - compute_object_lighting(o) > 0.05)
      mprintf("Light values dont match, BUG! (compute got %g)\n",compute_object_lighting(o));
   mprintf("Got lighting %g from %d vis lights, %d total lights\n",100.0*total,seen_index,lt[0]);

   return total;
}
#endif
