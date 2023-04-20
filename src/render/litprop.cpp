/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/litprop.cpp,v 1.58 2000/01/27 20:03:44 BFarquha Exp $

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\
   litprop.cpp

   This module handles both of our properties for lights which
   contribute to raycast lighting.  The regular ones, sLightProp, are
   the foundation, and the animated ones, sAnimLightProp, are a layer
   over that.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */

#include <matrix.h>
#include <rand.h>
#include <mprintf.h>

#include <r3d.h>
#include <md.h>

#include <portal.h>
#include <gedit.h>

#include <animlgt.h>
#include <ged_csg.h>
#include <scrptapi.h>
#include <scrptbas.h>
#include <scrptsrv.h>
#include <litscrpt.h>
#include <status.h>
#include <mnumprop.h>
#include <posprop.h>
#include <objpos.h>
#include <objlight.h>
#include <objmodel.h>
#include <objscale.h>
#include <osysbase.h>
#include <propert_.h>
#include <dataops_.h>
#include <prophash.h>
#include <propbase.h>
#include <propface.h>
#include <sdesc.h>
#include <sdesbase.h>
#include <vhot.h>

#include <iobjsys.h>

#include <litprop.h>
#include <litprops.h>
#include <litdbg.h>

#include <dbmem.h>      // must be last header

EXTERN BOOL g_lgd3d;

#ifdef EDITOR
int _watch_light=OBJ_NULL;
#endif

////////////////////////////////////////////////////////////
// Light Property Classes
//

//
// Data Ops
//
class cLightOps : public cClassDataOps<sLightProp>
{
};

class cColorOps : public cClassDataOps<ColorInfo>
{
};

//
// Store
//
class cLightStore : public cHashPropertyStore<cLightOps>
{
};

class cColorStore : public cHashPropertyStore<cColorOps>
{
};

// forward decl.
static bool ShineOneLight(sLightPropCore *light, ObjID obj,
                          uchar lighting_type, float brightness,
                          int *light_index, float radius, float inner_radius,
                          BOOL bConditionalAmbient = FALSE);

//
// Property
//
class cLightProperty : public cSpecificProperty<ILightProperty,&IID_ILightProperty,sLightProp*,cLightStore>
{
   typedef cSpecificProperty<ILightProperty,&IID_ILightProperty,sLightProp*,cLightStore> cParent;

public:

   cLightProperty(const sPropertyDesc* desc)
      : cParent(desc)
   {
   };

   STANDARD_DESCRIBE_TYPE(sLightProp);

   //   Whenever a light or animated light property is created or
   //   destroyed, we run a listener which invalidates the lighting
   //   state of the world rep.
#ifdef EDITOR
   void OnListenMsg(ePropertyListenMsg type, ObjID obj, uPropListenerValue val)
   {
      cParent::OnListenMsg(type,obj,val);

      if (!val.ptrval)
         return;

      sLightProp *light = (sLightProp *) val.ptrval;

      gedit_change_current_brush();

      if (type & kListenPropUnset) {
         if (light->brightness)
            gedcsg_lighting_changed();
         return;
      }
   }
#endif // ~EDITOR

};

class cColorProperty : public cSpecificProperty<IColorProperty,&IID_IColorProperty,ColorInfo*,cColorStore>
{
   typedef cSpecificProperty<IColorProperty,&IID_IColorProperty,ColorInfo*,cColorStore> cParent;

public:

   cColorProperty(const sPropertyDesc* desc)
      : cParent(desc)
   {
   };

   STANDARD_DESCRIBE_TYPE(ColorInfo);

};

//
// Structure descriptor
//

static sFieldDesc light_fields [] =
{
   { "brightness",
     kFieldTypeFloat, FieldLocation(sLightProp, brightness) },
   { "offset from object",
     kFieldTypeVector, FieldLocation(sLightProp, offset) },
   { "radius (0 for infinite)",
     kFieldTypeFloat, FieldLocation(sLightProp, radius) },
   { "inner radius (0 for none)",
     kFieldTypeFloat, FieldLocation(sLightProp, inner_radius) },
   { "quad lit",
     kFieldTypeBool, FieldLocation(sLightProp, quad) },
};

static sStructDesc light_struct
   = StructDescBuild(sLightProp, kStructFlagNone, light_fields);

// used by both light & anim light
static sPropertyConstraint light_constraints[] =
{
   { kPropertyRequires, PROP_POSITION_NAME },
   { kPropertyNullConstraint }
};


static sPropertyDesc light_desc =
{
   PROP_LIGHT_DATA_NAME,
   kPropertyNoInherit | kPropertyInstantiate,
   light_constraints, 0, 0,  // constraints, version
   { "Renderer", "Light" },
};

ILightProperty *g_LightProp = NULL;

static sFieldDesc color_fields [] =
{
   { "hue",
     kFieldTypeFloat, FieldLocation(ColorInfo, hue) },
   { "saturation",
     kFieldTypeFloat, FieldLocation(ColorInfo, saturation) },
};

static sStructDesc color_struct
   = StructDescBuild(ColorInfo, kStructFlagNone, color_fields);

static sPropertyDesc color_desc =
{
   PROP_LIGHT_COLOR_NAME,
   0,
   0, 0, 0,  // constraints, version
   { "Renderer", "LightColor" },
};

IColorProperty *g_ColorProp = NULL;

//
// Anim Light Classes
//

class cAnimOps : public cClassDataOps<sAnimLightProp>
{
};

class cAnimStore : public cHashPropertyStore<cAnimOps>
{
};

class cAnimLightProperty : public cSpecificProperty<IAnimLightProperty, &IID_IAnimLightProperty, sAnimLightProp*,cAnimStore>
{
   typedef cSpecificProperty<IAnimLightProperty, &IID_IAnimLightProperty, sAnimLightProp*, cAnimStore> cParent;

public:
   cAnimLightProperty(const sPropertyDesc* desc)
      : cParent(desc)
   {
   }

   STANDARD_DESCRIBE_TYPE(sAnimLightProp);

#ifdef EDITOR
   void OnListenMsg(ePropertyListenMsg type, ObjID obj, uPropListenerValue val)
   {
      cParent::OnListenMsg(type,obj,val);

      if (!val.ptrval)
         return;

      sAnimLightProp *anim_light = (sAnimLightProp *) val.ptrval;

      if ((type & kListenPropSet) && (type & kListenPropLoad))
         anim_light->animation.refresh = TRUE;

      // deletion
      if (type & kListenPropUnset) {
         if (anim_light->animation.max_brightness)
            gedcsg_lighting_changed();
         return;
      }

   }
#endif // ~EDITOR
};

// ummm, i wish these were in ship builds...
const char* mode_strings[]
= {
   "flip between min & max",
   "slide smoothly",
   "random",
   "minimum brightness",
   "maximum brightness",
   "zero brightness",
   "smoothly brighten",
   "smoothly dim",
   "random but coherent",
   "flicker min/max"
};
#define NUM_MODE_STRINGS (sizeof(mode_strings)/sizeof(mode_strings[0]))

static sFieldDesc anim_light_fields [] =
{
   { "Mode", kFieldTypeEnum, FieldLocation(sAnimLightProp, animation.mode),
     kFieldFlagNone, 0, NUM_MODE_STRINGS, NUM_MODE_STRINGS, mode_strings },
   { "millisecs to brighten", kFieldTypeInt,
     FieldLocation(sAnimLightProp, animation.time_rising_ms), kFieldFlagNone },
   { "millisecs to dim", kFieldTypeInt,
     FieldLocation(sAnimLightProp, animation.time_falling_ms),
     kFieldFlagNone },
   { "max brightness", kFieldTypeFloat,
     FieldLocation(sAnimLightProp, animation.max_brightness), kFieldFlagNone},
   { "min brightness", kFieldTypeFloat,
     FieldLocation(sAnimLightProp, animation.min_brightness), kFieldFlagNone },
   { "radius (0 for infinite)", kFieldTypeFloat,
     FieldLocation(sAnimLightProp, radius), kFieldFlagNone },
   { "inner radius (0 for none)",
     kFieldTypeFloat, FieldLocation(sAnimLightProp, inner_radius) },
   { "quad lit",
     kFieldTypeBool, FieldLocation(sLightProp, quad) },

   { "offset from object", kFieldTypeVector,
     FieldLocation(sAnimLightProp, offset), kFieldFlagNone },

   { "currently rising?", kFieldTypeBool,
     FieldLocation(sAnimLightProp, animation.is_rising), kFieldFlagNone },
   { "current countdown", kFieldTypeInt,
     FieldLocation(sAnimLightProp, animation.countdown_ms), kFieldFlagNone },
   { "inactive", kFieldTypeBool,
     FieldLocation(sAnimLightProp, animation.inactive), kFieldFlagNone },
};


static sStructDesc anim_light_struct
   = StructDescBuild(sAnimLightProp, kStructFlagNone, anim_light_fields);
// @TBD (justin 10-14-98): Currently, this property is being set locally
// on each machine. This isn't ideal -- we would like at least major
// changes to be reflected the same on all machines. However, this property
// is currently being changed for every animated light, every frame, causing
// huge numbers of network messages. We need to make this subtle, so that
// only real, important state changes get sent.
static sPropertyDesc anim_light_desc =
{
   PROP_ANIM_LIGHT_DATA_NAME,
   kPropertyNoInherit | kPropertyInstantiate,
   light_constraints, 0, 0,  // constraints, version
   { "Renderer", "Anim Light" },
   kPropertyChangeLocally,
};

IAnimLightProperty *g_AnimLightProp = NULL;


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   The properties have separate initialization routines so their
   return values will be distinct.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */

#define SPOTLIGHT_IMP  kPropertyImplVerySparse

sPropertyDesc spotlight_desc =
{
   PROP_SPOTLIGHT_NAME,
   0,      // flags
   NULL,   // constraints
   1,0,    // version
   {
      "Renderer",
      "Spotlight"
   }
};


sPropertyDesc spotlightandambient_desc =
{
   PROP_SPOTLIGHTAMB_NAME,
   0,      // flags
   NULL,   // constraints
   1,0,    // version
   {
      "Renderer",
      "SpotlightAndAmbient"
   }
};


IVectorProperty *g_SpotlightProp;

IVectorProperty *g_SpotlightAndAmbientProp;

BOOL LightPropInit(void)
{
   StructDescRegister(&light_struct);
   StructDescRegister(&color_struct);

   g_LightProp = new cLightProperty(&light_desc);
   g_ColorProp = new cColorProperty(&color_desc);

   g_SpotlightProp = CreateVectorProperty(&spotlight_desc, SPOTLIGHT_IMP);
   g_SpotlightAndAmbientProp = CreateVectorProperty(&spotlightandambient_desc, SPOTLIGHT_IMP);

   return TRUE;
}

BOOL ObjGetSpotlight(ObjID obj, mxs_vector **data)
{
   return g_SpotlightProp->Get(obj, data);
}


BOOL ObjGetSpotlightAndAmbient(ObjID obj, mxs_vector **data)
{
   return g_SpotlightAndAmbientProp->Get(obj, data);
}

BOOL AnimLightPropInit(void)
{
   StructDescRegister(&anim_light_struct);

   g_AnimLightProp = new cAnimLightProperty(&anim_light_desc);

   return TRUE;
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   cleanup for exit code

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
void LightPropRelease(void)
{
   SafeRelease(g_LightProp);
   SafeRelease(g_ColorProp);
   SafeRelease(g_SpotlightProp);
   SafeRelease(g_SpotlightAndAmbientProp);
}

void AnimLightPropRelease(void)
{
   SafeRelease(g_AnimLightProp);
}

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   COM-based getters

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
BOOL ObjLightGet(ObjID obj, sLightProp **light)
{
   return (g_LightProp->Get(obj, light));
}


BOOL ObjAnimLightGet(ObjID obj, sAnimLightProp **light)
{
   return (g_AnimLightProp->Get(obj, light));
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   COM-based setters

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
BOOL ObjLightSet(ObjID obj, sLightProp *light)
{
   g_LightProp->Set(obj, light);
   return TRUE;
}


BOOL ObjAnimLightSet(ObjID obj, sAnimLightProp *light)
{
   g_AnimLightProp->Set(obj, light);
   return TRUE;
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   These give us the brightness of the light properties on the given
   object, if any; if there's none, we set the brightness to 0.  For
   an animated light this means is maximum brightness.

   The return value indicates the presence of the property.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
BOOL ObjLightGetBrightness(ObjID obj, float *brightness)
{
   sLightProp *light;

   if (g_LightProp->Get(obj, &light)) {
      *brightness = light->brightness;
      return TRUE;
   } else {
      *brightness = 0.0;
      return FALSE;
   }
}

BOOL ObjColorGet(ObjID obj, float *hue, float *saturation)
{
   ColorInfo *color;

   if (g_ColorProp->Get(obj, &color)) {
      *hue = color->hue;
      *saturation = color->saturation;
      return TRUE;
   } else {
      *hue = 0.0;
      *saturation = 0.0;
      return FALSE;
   }
}

BOOL ObjAnimLightGetBrightness(ObjID obj, float *brightness)
{
   sAnimLightProp *anim_light;

   if (g_AnimLightProp->Get(obj, &anim_light)) {
      *brightness = anim_light->animation.max_brightness;
      return TRUE;
   } else {
      *brightness = 0.0;
      return FALSE;
   }
}

// ummm, this now just slams loc to ZeroVec, since really, what else would you want
EXTERN BOOL FindVhotLocation(mxs_vector *loc, ObjID obj, int vhot)
{
   mxs_vector scale;
   int idx;

   // i guess we are disallowing parameterized lights, then?
   mds_parm parms[] = { 0, 0, 0, 0, 0, 0, 0, 0 };

   ObjPos *pos = ObjPosGet(obj);

   // ummm, i think this should have been here all along, eh?
#ifndef SHIP
   if (config_is_defined("check_bad_lights"))
      if (fabs(loc->x)+fabs(loc->y)+fabs(loc->z)>0.2)
         mprintf("lightvhot bogus loc (%d) (wrong %g %g %g)\n",obj,loc->x,loc->y,loc->z);
#endif
   loc->x=loc->y=loc->z=0;

   BOOL bRetVal = FALSE;

   // Vhot time.  We get the model type from the model idx, which we
   // get from the model name.  That will tell us whether it's an md
   // model, which is the only kind with vhots.  There is almost
   // certainly a simpler way to do this.
   if (ObjGetModelNumber(obj, &idx))
   {
      if (objmodelGetModelType(idx) == OM_TYPE_MD)
      {
         mds_model *model = (mds_model *) objmodelGetModel(idx);

         r3_start_frame();
         md_eval_vhots(model, parms);
         r3_end_frame();

         mds_vhot *vh = md_vhot_list(model);

         int i = model->vhots;
         while (i--) {
            if (vh->id == vhot) {
               *loc = vh->v;
               bRetVal = TRUE;
               break;
            }
            vh++;
         }
         if (i<0)
            loud_light_mprint(("Never found VHOT %d for %d (has %d)\n",vhot,obj,model->vhots));
      }
      else
         loud_light_mprint(("Light %d isn't an MD model\n",obj));
   }
   else
      loud_light_mprint(("no model for %d\n",obj));

   // Of course, the object may have a scale property.
   if (ObjGetScale(obj, &scale))
      mx_elmuleq_vec(loc, &scale);

   watch_light_mprint(obj,("Base VHOT %g %g %g\n",loc->x,loc->y,loc->z));

   // And, at last, we put the offset into worldspace and add in the
   // object's coordinates.
   mxs_matrix trans;
   mxs_vector unrot_loc;

   unrot_loc = *loc;

   mx_ang2mat(&trans, &pos->fac);
   mx_mat_mul_vec(loc, &trans, &unrot_loc);

   mx_addeq_vec(loc, &pos->loc.vec);

   return bRetVal;
}

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   The same core work is needed for the regular and animated
   properties.  This bit takes into account the scale property; the
   offset in the light property; and any vhot in an md model.

   The resulting point is passed out on the floor.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
static void CalcWorldspaceLocation(mxs_vector *relpoint,
                                   mxs_vector *worldpoint, ObjID obj,
                                   Position *pos, sLightPropCore *light)
{
   // zero the point here, i think, if i can prove this is the bug...
   FindVhotLocation(relpoint, obj, kVHotLightSource);

   // transform the light's offset, and add that in
   mxs_matrix obj_rot;
   mxs_vector obj_offset;
   mxs_vector world_offset;

   mx_ang2mat(&obj_rot, &pos->fac);
   mx_copy_vec(&obj_offset, &light->offset);

   mx_mat_mul_vec(&world_offset, &obj_rot, &obj_offset);

   watch_light_mprint(obj,("Point %g %g %g - World %g %g %g \n",
                           worldpoint->x,worldpoint->y,worldpoint->z,
                           world_offset.x,world_offset.y,world_offset.z));

   mx_add_vec(worldpoint, relpoint, &world_offset);
}


//
// We know this object has a LightSource, now see if it has a direction defined.
//
static BOOL CalcSpotlightVec(mxs_vector *pDir, mxs_vector *pStartPoint, ObjID obj)
{
   mxs_vector EndPoint;
   if (FindVhotLocation(&EndPoint, obj, kVHotLightSource2))
   {
      mxs_vector Vec;
      mx_sub_vec(&Vec, &EndPoint, pStartPoint);
      mx_norm_vec(pDir, &Vec);
      return FALSE;
   }
   else // Default to straight down.
   {
      pDir->x = 0;
      pDir->y = 0;
      pDir->z = -1;
      return TRUE;
   }
}



EXTERN bool       portal_spotlight;
EXTERN mxs_vector portal_spotlight_loc;
EXTERN mxs_vector portal_spotlight_dir;
EXTERN float      portal_spotlight_inner;
EXTERN float      portal_spotlight_outer;

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   This shines a single light in the level.  We return TRUE if at
   least one cell was lit.  light_index is exported, but is set to 0
   if the light didn't affect any cells.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
static bool ShineOneLight(sLightPropCore *light, ObjID obj,
                          uchar lighting_type, float brightness,
                          int *light_index, float radius, float inner_radius,
                          BOOL bConditionalAmbient)
{
   int cells_reached;
   mxs_vector light_point;
   mxs_vector relpoint;
   Location light_location;
   Position *obj_pos;
   mxs_vector *spot;
   mxs_vector dir;
   float inner, outer;
   bool spotlight;
   bool spotlight_and_ambient;
#ifdef RGB_LIGHTING
   rgb_vector temp;
   int r,g,b;
   float hue, saturation;
   ObjColorGet(obj, &hue, &saturation);
   portal_convert_hsb_to_rgb(&r, &g, &b, hue, saturation);
   temp.x = r*brightness/255.0;
   temp.y = g*brightness/255.0;
   temp.z = b*brightness/255.0;
   portal_set_normalized_color(r,g,b);
#endif

   obj_pos = ObjPosGet(obj);
   AssertMsg1(obj_pos != NULL,
              "Light or AnimLight prop with no position on obj %d!\n", obj);

   CalcWorldspaceLocation(&relpoint, &light_point, obj, obj_pos, light);
   MakeLocationFromVector(&light_location, &light_point);

   spotlight = ObjGetSpotlight(obj, &spot);
   spotlight_and_ambient = ObjGetSpotlightAndAmbient(obj, &spot);

   if (inner_radius != 0 && inner_radius >= radius)
   {
      Warning(("litprop: Light's inner radius should be less than radius.\n"));
      inner_radius = 0;
   }

   // If this is TRUE, then only worry about adding an non-spotlight light, and only
   // if spotlight_and_ambient is TRUE.
   //
   // Else if bConditionalAmbient is FALSE, then we add a spotlight if either
   // spotlight or spotlight_and_ambient is true. If neither is, then we add a
   // non-spotlight.
   if (bConditionalAmbient)
   {
      if (!spotlight_and_ambient)
      {
         *light_index = 0;
         return FALSE;
      }

      portal_spotlight = FALSE;

      cells_reached = portal_add_omni_light(brightness, brightness,
                                            &light_location, lighting_type,
                                            radius, inner_radius);

      watch_light_mprint(obj,("Light %d reached %d cells rad %d bright %d at %g %g %g\n",
                              obj,cells_reached,radius,brightness,
                              light_location.vec.x,light_location.vec.y,light_location.vec.z));

      if (cells_reached > 0)
      {
#ifdef RGB_LIGHTING
         *light_index = add_object_light(&light_location.vec, &temp, radius);
#else
         *light_index = add_object_light(&light_location.vec, brightness, radius);
#endif
         portal_shine_omni_light(*light_index, &light_location,
                                 lighting_type);
         return TRUE;
      }
      else
      {
         *light_index = 0; // known safe value
         return FALSE;
      }
   }
   else // ConditionalAmbient is FALSE
   {
      if (spotlight||spotlight_and_ambient)
      {
         mxs_matrix trans;

         // dir.z = -1; dir.x = 0; dir.y = 0;
         if (CalcSpotlightVec(&dir, &relpoint, obj))
         {
            mx_ang2mat(&trans, &obj_pos->fac);
            mx_mat_muleq_vec(&trans, &dir);
         }

         inner = cos(3.141592*spot->x/180);
         outer = cos(3.141592*spot->y/180);
         // we should just have a function call to pass these in or something
         portal_spotlight = TRUE;
         portal_spotlight_loc = light_point;
         portal_spotlight_dir = dir;
         portal_spotlight_inner = inner;
         portal_spotlight_outer = outer;
      }

      if (spotlight_and_ambient)
         brightness = spot->z;

      cells_reached = portal_add_omni_light(brightness, brightness,
                                         &light_location, lighting_type,
                                         radius, inner_radius);

      watch_light_mprint(obj,("Light %d reached %d cells rad %d bright %d at %g %g %g\n",
                           obj,cells_reached,radius,brightness,
                           light_location.vec.x,light_location.vec.y,light_location.vec.z));

      portal_spotlight = FALSE;

      if (cells_reached > 0)
      {
         if (spotlight||spotlight_and_ambient)
         {
#ifdef RGB_LIGHTING
            *light_index = add_object_spotlight(&light_location.vec, &temp, &dir, inner, outer, radius);
#else
            *light_index = add_object_spotlight(&light_location.vec, brightness, &dir, inner, outer, radius);
#endif
         }
         else
         {
#ifdef RGB_LIGHTING
            *light_index = add_object_light(&light_location.vec, &temp, radius);
#else
            *light_index = add_object_light(&light_location.vec, brightness, radius);
#endif
         }
         portal_shine_omni_light(*light_index, &light_location,
                                 lighting_type);
         return TRUE;
      }
      else
      {
         *light_index = 0; // known safe value
         return FALSE;
      }
   }
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   Whenever a light or animated light is created, we shine it in the
   level.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
#ifdef EDITOR
void ObjLightShine(ObjID obj)
{
   sLightProp *pLight;
   if (g_LightProp->Get(obj, &pLight)) {
      gedit_change_current_brush();

      if (pLight->brightness) {
         int dummy_light_index;
         ShineOneLight(pLight, obj, LIGHT_QUICK, pLight->brightness,
                       &dummy_light_index, pLight->radius,
                       pLight->inner_radius);
         ShineOneLight(pLight, obj, LIGHT_QUICK, pLight->brightness,
                       &dummy_light_index, pLight->radius,
                       pLight->inner_radius, TRUE);
      }
   }
}


void ObjAnimLightShine(ObjID obj)
{
   sAnimLightProp *pAnimLight;
   if (g_AnimLightProp->Get(obj, &pAnimLight)) {
      gedit_change_current_brush();

      if (pAnimLight->animation.max_brightness) {
         int dummy_light_index;

         if (ShineOneLight(pAnimLight, obj,
                           LIGHT_QUICK | LIGHT_ANIMATED,
                           pAnimLight->animation.max_brightness,
                           &dummy_light_index, pAnimLight->radius,
                           pAnimLight->inner_radius)) {
            AnimLightSetCellList(&pAnimLight->animation, dummy_light_index);

#if 0 // wsf: We'd like to do this, but there's only one animated light per object.
            // Do possible additional ambient component, if first was spotlight:
            if (ShineOneLight(pAnimLight, obj,
                              LIGHT_QUICK | LIGHT_ANIMATED,
                              pAnimLight->animation.max_brightness,
                              &dummy_light_index, pAnimLight->radius,
                              pAnimLight->inner_radius, TRUE))
               AnimLightSetCellList(&pAnimLight->animation, dummy_light_index);
#endif

            ObjAnimLightSet(obj, pAnimLight);
            pAnimLight->animation.refresh = TRUE;
         } else
            AnimLightSetCellList(&pAnimLight->animation, -1);
      }
   }
}
#endif // EDITOR

#ifdef EDITOR
/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   This iterates over all objects with LightProp and shines their
   lights in the level.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
int ObjLightLightLevel(uchar lighting_type)
{
   sPropertyObjIter iter;
   ObjID obj;
   sLightProp *light;
   int light_index;
   int num_lights = 0, num_lt_seen=0;

   AssertMsg1((lighting_type & LIGHT_ANIMATED) == 0,
              "ObjLightLightLevel: Why is this light animated?  Type: %d.",
              int(lighting_type));

   g_LightProp->IterStart(&iter);
   mprintf("\nObjLightLightLevel starting out...");
   Status("Applying object lights...");
   while (g_LightProp->IterNext(&iter, &obj))
   {
      if (OBJ_IS_ABSTRACT(obj))
         continue;

      ObjLightGet(obj, &light);

      uchar used_lighting_type = lighting_type;
      if (light->quad && gedcsg_light_objcast)
         used_lighting_type |= LIGHT_QUAD;

      if (ShineOneLight(light, obj, used_lighting_type, light->brightness,
                        &light_index, light->radius, light->inner_radius))
      {
         mprintf("%d.",obj);
         num_lights++;

         // Do possible ambient, if spotlight succeeded:
         if (ShineOneLight(light, obj, used_lighting_type, light->brightness,
                           &light_index, light->radius, light->inner_radius,
                           TRUE))
            num_lights++;
      }
      else
         mprintf("[%d].",obj);

      num_lt_seen++;
   }

   g_LightProp->IterStop(&iter);
   mprintf("\n%d of %d object lights reached surfaces\n",num_lights,num_lt_seen);
   return num_lights;
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   The iteration over animated lights is a little more involved.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
int ObjAnimLightLightLevel(uchar lighting_type)
{
   sPropertyObjIter iter;
   ObjID obj;
   sAnimLightProp *anim_light;
   int light_index;
   int num_lights = 0, num_lt_seen=0;

   // these mprintf's are the "loud" anim light light level code
   mprintf("\nObjAnimLightLevel starting out...");
   Status("Applying animated lights...");
   g_AnimLightProp->IterStart(&iter);
   while (g_AnimLightProp->IterNext(&iter, &obj))
   {
      if (OBJ_IS_ABSTRACT(obj))
         continue;

      ObjAnimLightGet(obj, &anim_light);

      uchar used_lighting_type = lighting_type | LIGHT_ANIMATED;
      if (anim_light->quad && gedcsg_light_objcast)
         used_lighting_type |= LIGHT_QUAD;

      if (ShineOneLight(anim_light, obj, used_lighting_type,
                        anim_light->animation.max_brightness, &light_index,
                        anim_light->radius, anim_light->inner_radius))
      {
         AnimLightSetCellList(&anim_light->animation, light_index);
         // The light now knows how many cells it reaches and other
         // world-reppish details.
         ObjAnimLightSet(obj, anim_light);
         mprintf("%d.",obj);
         num_lights++;

#if 0 // wsf: we'd like to do this, but there's only one animated light per object.
         // Do possible additional ambient, if first was spotlight:
         if (ShineOneLight(anim_light, obj, used_lighting_type,
                           anim_light->animation.max_brightness, &light_index,
                           anim_light->radius, anim_light->inner_radius, TRUE))
         {
            AnimLightSetCellList(&anim_light->animation, light_index);
            num_lights++;
         }
#endif
      }
      else
      {
         mprintf("[%d].",obj);
         AnimLightSetCellList(&anim_light->animation, -1);
      }
      num_lt_seen++;
   }
   g_AnimLightProp->IterStop(&iter);
   mprintf("\n%d of %d anim lights reached surfaces\n",num_lights,num_lt_seen);
   return num_lights;
}
#endif // EDITOR

static IScriptMan *GetScriptMan()
{
   static IScriptMan *pScriptMan = 0;

   if (!pScriptMan)
      pScriptMan = AppGetObj(IScriptMan);

   return pScriptMan;
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   This iterates over all objects with the animated light property,
   runs their animations, and flags those cells reached by lights
   which have changed.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */

void ObjAnimLightFrameUpdate(long time_change)
{
   sPropertyObjIter iter;
   ObjID obj;
   sAnimLightProp *anim_light;

   // We need to Lock the objsys around this iterator because of the
   // PumpMessages within it. That can cause animated lights to be
   // deleted, causing crashes if it happens within the iterator...
   AutoAppIPtr(ObjectSystem);
   pObjectSystem->Lock();

   g_AnimLightProp->IterStart(&iter);
   while (g_AnimLightProp->IterNext(&iter, &obj))
   {
      if (OBJ_IS_ABSTRACT(obj))
         continue;

      g_AnimLightProp->Get(obj, &anim_light);

      if (AnimLightUpdateTimer(&anim_light->animation, time_change))
      {
         if (anim_light->notify_script)
         {
            // we'll send out the brightness data to allow scripts to
            // make a (relatively) informed decision about the state
            // of the animation
            sScrMsg *pMsg = new sScrMsg(obj, obj, "LightChange",
               anim_light->animation.brightness,
               anim_light->animation.min_brightness,
               anim_light->animation.max_brightness);
            // If we're networking, send this message to proxies as well
            // as hosts, so they can deal with animated lights individually.
            // In general, networking animated lights gets very expensive
            // in bandwidth.
            pMsg->flags |= kSMF_MsgSendToProxy;
            GetScriptMan()->PostMessage(pMsg);
            // pump messages so that we can keep in synch with rendering
            GetScriptMan()->PumpMessages();

            SafeRelease(pMsg);
         }
         watch_light_mprint(obj,("AnimUpdate %d ref %d\n",obj,anim_light->animation.refresh));
      }
      if (anim_light->animation.refresh)
         AnimLightFlagCells(&anim_light->animation,obj);   //zb

      // would be nice to not set unless we changed something
      ObjAnimLightSet(obj, anim_light);
   }
   g_AnimLightProp->IterStop(&iter);

   pObjectSystem->Unlock();
}

// go through and reset all the anim lights to their current states
void ObjAnimLightReset(void)
{
   sAnimLightProp *anim_light;
   sPropertyObjIter iter;
   ObjID obj;

   g_AnimLightProp->IterStart(&iter);
   while (g_AnimLightProp->IterNext(&iter, &obj))
   {
      if (OBJ_IS_ABSTRACT(obj))
         continue;
      g_AnimLightProp->Get(obj, &anim_light);
      AnimLightFlagCells(&anim_light->animation,obj);//zb
   }
   g_AnimLightProp->IterStop(&iter);
}
