/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/hdskprop.cpp,v 1.7 1998/03/26 13:50:27 mahk Exp $

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\
   hdskprop.cpp

   property of being rendered with a heat disk

   This includes both hard-core property stuff and the SDESC.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */

#include <matrix.h>
#include <rand.h>

#include <wrtype.h>
#include <objpos.h>
#include <objscale.h>
#include <propert_.h>
#include <dataops_.h>
#include <propsprs.h>

#include <propbase.h>
#include <sdesc.h>
#include <sdesbase.h>

#include <fxdisk.h>
#include <hdskprop.h>

// Must be last header
#include <dbmem.h>

EXTERN BOOL g_lgd3d;

//
// Data ops.  This describes how to manipulate HeatDisk data.  
// If you want to change how heat disks get written or read, overload the default
// methods. 
// 

class cHeatDiskOps : public cClassDataOps<sHeatDiskCluster>
{
};

//
// Link list property store 
// 

class cHeatDiskStore : public cSparseHashPropertyStore<cHeatDiskOps>
{
}; 

//
// Finally, the property class
//

class cHeatDiskProperty : public cSpecificProperty<IHeatDiskProperty, &IID_IHeatDiskProperty, sHeatDiskCluster*,cHeatDiskStore>
{
  typedef cSpecificProperty<IHeatDiskProperty, &IID_IHeatDiskProperty, sHeatDiskCluster*,cHeatDiskStore> cParent; 

public:
   cHeatDiskProperty(const sPropertyDesc* desc) 
      : cParent(desc)
   {
   }

   // use the standard DescribeType method. 
   STANDARD_DESCRIBE_TYPE(sHeatDiskCluster); 
};


static sFieldDesc heat_disk_fields [] =
{
   { "start offset x", 
     kFieldTypeFloat, FieldLocation(sHeatDiskCluster, start_offset.x) },
   { "start offset y", 
     kFieldTypeFloat, FieldLocation(sHeatDiskCluster, start_offset.y) },
   { "start offset z", 
     kFieldTypeFloat, FieldLocation(sHeatDiskCluster, start_offset.z) },

   { "start radius", 
     kFieldTypeFloat, FieldLocation(sHeatDiskCluster, start_radius) },

   { "end offset x", 
     kFieldTypeFloat, FieldLocation(sHeatDiskCluster, end_offset.x) },
   { "end offset y", 
     kFieldTypeFloat, FieldLocation(sHeatDiskCluster, end_offset.y) },
   { "end offset z", 
     kFieldTypeFloat, FieldLocation(sHeatDiskCluster, end_offset.z) },

   { "end radius", 
     kFieldTypeFloat, FieldLocation(sHeatDiskCluster, end_radius) },

   { "bottom jitter", 
     kFieldTypeFloat, FieldLocation(sHeatDiskCluster, bottom_jitter) },
   { "top jitter", 
     kFieldTypeFloat, FieldLocation(sHeatDiskCluster, top_jitter) },

   { "number of blobs",
     kFieldTypeInt, FieldLocation(sHeatDiskCluster, num_effects) },

   { "max disks/blob",
     kFieldTypeInt, FieldLocation(sHeatDiskCluster, num_disks) },

   { "height of effects",
     kFieldTypeFloat, FieldLocation(sHeatDiskCluster, height) }
};

static sStructDesc heat_disk_struct
   = StructDescBuild(sHeatDiskCluster, kStructFlagNone, heat_disk_fields);

// the property descriptor
static sPropertyDesc heat_disk_desc = 
{ 
   PROP_HEAT_DISK_DATA_NAME, 
   kPropertyNoInherit | kPropertyInstantiate, NULL, 0, 0,
   { "SFX", "Heat Disks" }, 
};

static cHeatDiskProperty* pHeatDiskProp = NULL; 

// In the Beginning...
extern "C" BOOL HeatDiskPropInit(void)
{
   StructDescRegister(&heat_disk_struct);

   pHeatDiskProp = new cHeatDiskProperty(&heat_disk_desc); 

   return TRUE;
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   one COM thing

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
extern "C" BOOL ObjHeatDiskGet(ObjID obj, sHeatDiskCluster **cluster)
{
   return pHeatDiskProp->Get(obj,cluster); 
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   another COM thing

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
extern "C" BOOL ObjHeatDiskSet(ObjID obj, sHeatDiskCluster *cluster)
{
   return pHeatDiskProp->Set(obj,cluster); 
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   The random numbers we want are in [-1, 1].

   Unfortunately, this doesn't get us all the way to 1.0, but it comes
   close enough.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
inline mxs_real RandUnit()
{
   int r = Rand();
   return float(r - 16384) * (1.0 / 16384.0);
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   When we check whether an object has this property, we end up with
   its renderig information.  So we expect to have that by the time
   this is called.

   @BUG: If you ask for a string of, say, three effects, you'll
   actually get four.  Also, I'm interpolating some things in screen
   space and others with correct perspective.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
extern "C" void ObjHeatDiskRender(ObjID obj, sHeatDiskCluster *cluster)
{
   if (g_lgd3d)
      return;

   Position *p = ObjPosGet(obj);
   mxs_real real_num_effects = (float) cluster->num_effects;

#ifndef SHIP
   if (real_num_effects == 0) {
      Warning(("You can't draw a heat effect with 0 blobs, silly.\n"));
      return;
   }
#endif

   mxs_vector bottom, top;
   mxs_vector start, end;
   mxs_matrix orientation;
   mxs_real top_radius;

   int num_disks = cluster->num_disks;
   mxs_real bottom_jitter = cluster->bottom_jitter;
   mxs_real top_jitter = cluster->top_jitter;
   mxs_real start_radius = cluster->start_radius;
   mxs_real end_radius = cluster->end_radius;
   mxs_real bottom_radius = start_radius;
   mxs_real delta_radius = (end_radius - start_radius) / real_num_effects;
   mxs_real delta_time = 1.0 / real_num_effects;

   // Let's find our starting and ending points in world space.
   mx_ang2mat(&orientation, &p->fac);
   mx_mat_mul_vec(&start, &orientation, &cluster->start_offset);
   mx_addeq_vec(&start, &p->loc.vec);
   mx_mat_mul_vec(&end, &orientation, &cluster->end_offset);
   mx_addeq_vec(&end, &p->loc.vec);

   // If we have the scale property, we apply it to our radii and
   // jitter, but not our width, so that the trail gets narrower.
   // For the heckuvit we scale the radii by x and the jitter by y.
   // This is expressed nowhere in the Dromed interface, you betcha.
   mxs_vector scale;
   if (ObjGetScale(obj, &scale)) {
      bottom_radius *= scale.x;
      delta_radius *= scale.x;
      top_jitter *= scale.y;
      bottom_jitter *= scale.y;
   }

   for (mxs_real time = 0; time < 1; time += delta_time) {
      mx_interpolate_vec(&bottom, &start, &end, time);
      top = bottom;
      top.z += cluster->height;
      top_radius = bottom_radius * .333333333;

      top.x += RandUnit() * top_jitter;
      top.y += RandUnit() * top_jitter;
      top.z += RandUnit() * top_jitter;

      bottom.x += RandUnit() * bottom_jitter;
      bottom.y += RandUnit() * bottom_jitter;
      bottom.z += RandUnit() * bottom_jitter;

      FXDiskDrawHeatDisksInWorld(&top, top_radius,
                                 &bottom, bottom_radius,
                                 num_disks);

      bottom_radius += delta_radius;
   }
}
