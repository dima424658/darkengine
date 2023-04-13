/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/


// $Header: r:/t2repos/thief2/src/render/sprkprop.cpp,v 1.12 1998/03/31 19:30:07 MAT Exp $

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\
   sprkprop.cpp

   property of being rendered as a little spark dingus

   This includes both hard-core property stuff and the SDESC.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */


#include <matrix.h>
#include <rand.h>
#include <fix.h>
#include <r3d.h>
#include <2d.h>

#include <wrtype.h>
#include <objpos.h>
#include <objscale.h>
#include <objmodel.h>
#include <propert_.h>
#include <dataops_.h>
#include <propbase.h>
#include <sdesc.h>
#include <sdesbase.h>
#include <sprkprop.h>
#include <sprks.h>

// Must be last header
#include <dbmem.h>

#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))


typedef cGenericProperty<ISparkProperty,&IID_ISparkProperty, sSpark*> cSparkPropertyBase;

class cSparkProperty : public cSparkPropertyBase
{
   cClassDataOps< sSpark> mOps; 

public: 
   cSparkProperty(const sPropertyDesc* desc, IPropertyStore* store)
      : cSparkPropertyBase(desc,store)
   {
      SetOps(&mOps); 
   }

   cSparkProperty(const sPropertyDesc* desc, ePropertyImpl impl)
      : cSparkPropertyBase(desc,CreateGenericPropertyStore(impl))
   {
      SetOps(&mOps); 
   }

   STANDARD_DESCRIBE_TYPE( sSpark);
};


static sFieldDesc spark_fields []
= {
   { "flags",
     kFieldTypeBits, FieldLocation(sSpark, flags) },
   { "color",
     kFieldTypeShort, FieldLocation(sSpark, color) },

   { "angle offset 1",
    kFieldTypeShort, FieldLocation(sSpark, angle_offset.tx) },
   { "angle offset 2",
    kFieldTypeShort, FieldLocation(sSpark, angle_offset.ty) },
   { "angle offset 3",
    kFieldTypeShort, FieldLocation(sSpark, angle_offset.tz) },

   { "angle jitter",
    kFieldTypeShort, FieldLocation(sSpark, angle_jitter) },

   { "size 1",
     kFieldTypeFloat, FieldLocation(sSpark, size[0]) },
   { "size 2",
     kFieldTypeFloat, FieldLocation(sSpark, size[1]) },
   { "size 3",
     kFieldTypeFloat, FieldLocation(sSpark, size[2]) },
   { "size jitter",
     kFieldTypeFloat, FieldLocation(sSpark, size_jitter) },

   { "light (0-1)",
     kFieldTypeFloat, FieldLocation(sSpark, base_light_level) },
   { "light jitter",
     kFieldTypeFloat, FieldLocation(sSpark, light_jitter) },
};

static sStructDesc spark_struct
   = StructDescBuild(sSpark, kStructFlagNone, spark_fields);

// the property descriptor
static sPropertyDesc spark_desc = 
{ 
   PROP_SPARK_DATA_NAME, 
   kPropertyNoInherit | kPropertyInstantiate, NULL, 0, 0,
   { "SFX", "Spark" }, 
};

PropertyID spark_id = PROPID_NULL;
ISparkProperty *g_SparkProp = NULL;


// In the Beginning...
extern "C" BOOL SparkPropInit(void)
{
   StructDescRegister(&spark_struct);

   g_SparkProp = new cSparkProperty(&spark_desc, kPropertyImplVerySparse);
   spark_id = g_SparkProp->GetID();

   return TRUE;
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   one COM thing

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
extern "C" BOOL ObjSparkGet(ObjID obj, sSpark **spark)
{
   return g_SparkProp->Get(obj, spark);
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   another COM thing

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
extern "C" BOOL ObjSparkSet(ObjID obj, sSpark *spark)
{
   return g_SparkProp->Set(obj, spark);
}


static r3s_point screen_vertex[3]
   = { { {0, 0, 0}, 0, {0, 0, 0.0, 0, 0.0, 0.0, 0.0 } },
       { {0, 0, 0}, 0, {0, 0, 0.0, 0, 0.0, /*15.0 / 16.0*/ 0.0, 0.0 } },
       { {0, 0, 0}, 0, {0, 0, 0.0, 0, 0.0, 0.0, /*15.0 / 16.0*/ 0.0 } }
   };

static r3s_point *screen_vertex_pointer[3] =
   { &screen_vertex[0], &screen_vertex[1], &screen_vertex[2] };


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   something triangular inside a COM thing

   Since a spark doesn't have an orientation in the usual way, we use
   that angvec for some other things (yeah, this is brutally ugly, but
   it means we can mess with a spark object through a tweq property).

   tx: angle adjustment
   ty: extra lighting multiplier

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
extern "C" void ObjSparkRender(ObjID obj, sSpark *spark, grs_bitmap *bitmap)
{
   static r3s_point screen_projection;
   static BOOL fInit; // to workaround watcom internal error bug (toml 10-09-97)

   if (!fInit)
   {
      screen_vertex[1].grp.u = 15.0 / 16.0;
      screen_vertex[2].grp.v = 15.0 / 16.0;
      fInit = TRUE;
   }

   int i;
   mxs_real scaled_size;
   fix fix_scaled_size;
   fix sine, cosine;
   fix min_x, max_x, min_y, max_y;
   mxs_vector scale;
   mxs_angvec spokes;
   Position *pos = ObjPosGet(obj);
   mxs_ang rotation = pos->fac.tx;
   int polygon_context = R3_PL_POLYGON | R3_PL_SOLID;   // solid-color => r3
   mxs_real base_light_level = spark->base_light_level;
   bool has_scale_property;
   mxs_real random_scaling;
   uchar color;

   int flags = spark->flags;

   r3_start_block();
   r3_transform_point(&screen_projection, &pos->loc.vec);

   // We won't draw anything that's even vaguely behind us.
   if (screen_projection.grp.w > 50.0
    || screen_projection.grp.w < 0.0)
      goto finished;

   // Next: vertices.  These are points of a triangle, found in polar
   // coordinates.
   mx_mk_angvec(&spokes, 0, 21845, 43691);      // starts ~equilateral
   spokes.el[0] += rotation + spark->angle_offset.tx;
   spokes.el[1] += rotation + spark->angle_offset.ty;
   spokes.el[2] += rotation + spark->angle_offset.tz;

   // Are we perturbing our angles?
   if (spark->angle_jitter)
      for (i = 0; i < 3; i++)
         spokes.el[i] += ((Rand() - 16384) * spark->angle_jitter) >> 15;

   if (spark->size_jitter)
      random_scaling = float(Rand() - 16384) * (spark->size_jitter / 32768.0);

   if (ObjGetScale(obj, &scale)) {
      has_scale_property = TRUE;

      if (flags & kSprkFlagBrightHighScale)
         base_light_level *= scale.el[0];
      else if (flags & kSprkFlagBrightLowScale)
         base_light_level *= (1.0 - scale.el[0]);
   } else
      has_scale_property = FALSE;

   for (i = 0; i < 3; ++i) {
      if (has_scale_property)
         scaled_size = spark->size[i] * scale.el[i];
      else
         scaled_size = spark->size[i];

      if (spark->size_jitter)
         scaled_size += (scaled_size * random_scaling);

      scaled_size *= screen_projection.grp.w;   // this is w, as in 1/z
      scaled_size *= float(grd_bm.w);          // this is w, bitmap width
      fix_scaled_size = scaled_size;
      fix_scaled_size = max(fix_scaled_size, 65536);    // somewhat arbitrary

      fix_fastsincos(spokes.el[i], &sine, &cosine);

      // if we're zbuffering, this is important!
      screen_vertex[i].grp.w = screen_projection.grp.w;
      screen_vertex[i].p.z   = screen_projection.p.z;

      screen_vertex[i].grp.sx
         = screen_projection.grp.sx + fix_mul(cosine, fix_scaled_size);
      screen_vertex[i].grp.sy
         = screen_projection.grp.sy + fix_mul(sine, fix_scaled_size);
   }

   min_x = min(screen_vertex[0].grp.sx,
               min(screen_vertex[1].grp.sx, screen_vertex[2].grp.sx));
   min_y = min(screen_vertex[0].grp.sy,
               min(screen_vertex[1].grp.sy, screen_vertex[2].grp.sy));

   max_x = max(screen_vertex[0].grp.sx,
               max(screen_vertex[1].grp.sx, screen_vertex[2].grp.sx));
   max_y = max(screen_vertex[0].grp.sy,
               max(screen_vertex[1].grp.sy, screen_vertex[2].grp.sy));

   // We won't draw anything that's even vaguely off-screen, to dodge
   // the cost of clipping.  So let's keep these suckers small!
   if (min_x < grd_canvas->gc.clip.f.left
    || max_x > grd_canvas->gc.clip.f.right
    || min_y < grd_canvas->gc.clip.f.top
    || max_y > grd_canvas->gc.clip.f.bot)
      goto finished;

   if (flags & kSprkFlagTextured) {
      polygon_context |= R3_PL_TEXTURE_LINEAR;
      r3_set_texture(bitmap);
   }

   if (flags & kSprkFlagBrightHighAngle)
      base_light_level
        *= float(ushort(pos->fac.ty)) * 1.0 / 65535.0;
   else if (flags & kSprkFlagBrightLowAngle)
      base_light_level
        *= (1.0 - float(ushort(pos->fac.ty)) * 1.0 / 65535.0);

   if (flags & kSprkFlagLightByVertex) {
      polygon_context |= R3_PL_GOURAUD;
      color = (unsigned char) spark->color;

      screen_vertex[0].grp.i
         = screen_vertex[1].grp.i
         = screen_vertex[2].grp.i = base_light_level;

      if (spark->light_jitter) {
         screen_vertex[0].grp.i
            *= (spark->light_jitter * (((float)Rand()) * (1.0 / 32767)));
         screen_vertex[1].grp.i
            *= (spark->light_jitter * (((float)Rand()) * (1.0 / 32767)));
         screen_vertex[2].grp.i
            *= (spark->light_jitter * (((float)Rand()) * (1.0 / 32767)));
      }

   } else {
      color = grd_light_table[((uint)(spark->base_light_level * 15.0) << 8)
                            + spark->color];
   }

   r3_set_color(color);
   r3_set_polygon_context(polygon_context);
   r3_set_prim();
   r3_draw_poly(3, &screen_vertex_pointer[0]);

finished:
   r3_end_block();
}
