/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/engfeat/pick.cpp,v 1.10 2000/01/10 18:23:15 adurant Exp $

/* --<<= --/-/-/-/-/-/-/ <<< ((( ((( /\ ))) ))) >>> \-\-\-\-\-\-\-- =>>-- *\
   pick.cpp

   The pick system should be integrated into the rendering pipleline.
   See drkrend and drkgame to see how we've set it up for Dark.

\* --<<= --\-\-\-\-\-\-\ <<< ((( ((( \/ ))) ))) >>> /-/-/-/-/-/-/-- =>>-- */

#include <r3d.h>
#include <math.h>
#include <g2.h>
#include <config.h>
#include <mprintf.h>
#include <appagg.h>

#include <camera.h>
#include <playrobj.h>
#include <wrtype.h>
#include <iobjsys.h>
#include <objdef.h>
#include <objnotif.h>
#include <frobprop.h>
#include <objshape.h>
#include <objpos.h>

#include <propman.h>
#include <property.h>
#include <propface.h>
#include <propbase.h>

#include <pick.h>

// Must be last header 
#include <dbmem.h>

static float g_EllipticPickMod;
static float g_BestPickWeight;
static fix g_FocusX, g_FocusY;
static float g_fFudge;
// @HACK Jon says: i made this global because i want to use the Weight() function with a different distance. I'm bad.
float g_fMaxDistSquared;        // If this is 0 it's ignored.
static float g_fDistWeight;
static mxs_vector g_CamVec;
static mxs_angvec g_CamAngle;
static mxs_plane g_Plane;

static IFloatProperty *g_pPickBiasProp=NULL;
static IFloatProperty *g_pPickDistProp=NULL;


ObjID g_PickCurrentObj;


#define TO_FLOAT(fx) ((float)(fx) * (1.0 / 65536.0))
#define MAX(f1, f2) ((f1) > (f2)? (f1) : (f2))

#define kFudgeLog2 4
#define kZWeightRatio .0025
#define kPickInfinity 1000000000.0

#define TOP 0
#define BOTTOM 1
#define LEFT 2
#define RIGHT 3

static r3s_point box_pts[8];
static r3s_phandle box_vlist[6][4] =
{
   { box_pts+0, box_pts+1, box_pts+3, box_pts+2 },
   { box_pts+0, box_pts+1, box_pts+5, box_pts+4 },
   { box_pts+0, box_pts+2, box_pts+6, box_pts+4 },
   { box_pts+1, box_pts+3, box_pts+7, box_pts+5 },
   { box_pts+2, box_pts+3, box_pts+7, box_pts+6 },
   { box_pts+4, box_pts+5, box_pts+7, box_pts+6 }
};

///////////////////////////////////////////////////////////

// This is based on a similar function in rendobj.c, but does not clip
// to the view cone or support octagonal bounds.
static void GetUnclippedBBox(ObjID obj, fix *mn, fix *mx)
{
   mxs_vector vec[8], bmin, bmax;
   int i, j, n, boxType;
   r3s_phandle *vl;
   Position *pos = ObjPosGet(obj);
   BOOL startObj;
   int saveflags; 
   r3e_clipmode savemode;

   boxType = ObjGetFavoriteBBox(obj, &bmin, &bmax);
   if (boxType == OBJ_BBOX_TYPE_NONE)
   {
      *mx = *mn = 0;
      return;
   }

   if(TRUE == (startObj = (boxType != OBJ_BBOX_TYPE_WORLD)))
      r3_start_object_angles(&pos->loc.vec, &pos->fac, R3_DEFANG);

   r3_push_clip_plane(&g_Plane);

   savemode = r3_get_clipmode();
   r3_set_clipmode(R3_USER_CLIP);
   saveflags = r3_get_clip_flags();
   r3_set_clip_flags(0);

   vec[0].x = vec[2].x = vec[4].x = vec[6].x = bmin.x;
   vec[1].x = vec[3].x = vec[5].x = vec[7].x = bmax.x;

   vec[0].y = vec[1].y = vec[4].y = vec[5].y = bmin.y;
   vec[2].y = vec[3].y = vec[6].y = vec[7].y = bmax.y;

   vec[0].z = vec[1].z = vec[2].z = vec[3].z = bmin.z;
   vec[4].z = vec[5].z = vec[6].z = vec[7].z = bmax.z;

   r3_start_block();
   r3_transform_block(8, box_pts, vec);

   mn[0] = mn[1] = 0x7fffffff;
   mx[0] = mx[1] = 0x80000000;

   for (i = 0; i < 6; ++i) {
      n = r3_clip_polygon(4, box_vlist[i], &vl);
      for (j = 0; j < n; ++j) {
         grs_point *p = &vl[j]->grp;
         if (p->sx < mn[0]) mn[0] = p->sx;
         if (p->sx > mx[0]) mx[0] = p->sx;
         if (p->sy < mn[1]) mn[1] = p->sy;
         if (p->sy > mx[1]) mx[1] = p->sy;
      }
   }

   r3_pop_clip_plane();
   r3_end_block();
   
   if (startObj)
      r3_end_object();

   r3_set_clip_flags(saveflags);
   r3_set_clipmode(savemode);
}

///////////////////////////////////////////////////////////

// The weighting is a hacked set of heuristics with no theoretical
// underpinnings.  We include a fudge factor around the object to make
// it easier to snag small objects.

// Lower weights are stronger.

float Weight(ObjID obj)
{
   float fWeight = 0.0;
   float use_bias = 1.0;
   int i;               // every function needs an iterator called i

   fix mn[2], mx[2];
   float dist[4];

   // More distant rectangles are harder to pick.
   Position *pos = ObjPosGet(obj);
   // now lets make pick distance biased against z distance.
   // by increasing z distance a bit.  
   mxs_vector objposvec;
   objposvec = pos->loc.vec;
   objposvec.z += (objposvec.z - g_CamVec.z)*g_EllipticPickMod;
   float fCenterDist = mx_dist_vec(&objposvec, &g_CamVec)
                     - ObjGetRadius(obj);
   if (g_fMaxDistSquared != 0.0
    && ((fCenterDist * fCenterDist) > g_fMaxDistSquared))
      return kPickInfinity;

   float fObjMaxDist = 0.0; 
   if (g_pPickDistProp->Get(obj,&fObjMaxDist)
       && fCenterDist > fObjMaxDist)
      return kPickInfinity; 
      
   GetUnclippedBBox(obj, mn, mx);

   g_pPickBiasProp->Get(obj,&use_bias);
   use_bias*=g_fFudge;

   // A rectangle which includes our focus point will have positive
   // values for all of these.
   dist[TOP] = TO_FLOAT(g_FocusY - mn[1]) + use_bias;
   dist[BOTTOM] = TO_FLOAT(mx[1] - g_FocusY) + use_bias;
   dist[LEFT] = TO_FLOAT(g_FocusX - mn[0]) + use_bias;
   dist[RIGHT] = TO_FLOAT(mx[0] - g_FocusX) + use_bias;

#ifdef DBG_ON
   if (config_is_defined("PickSpew"))
      mprintf("(%d) %g %g %g %g\n",
              obj, dist[TOP], dist[BOTTOM], dist[LEFT], dist[RIGHT]);
#endif // DBG_ON

   // Say, are we within our fudge value of the focus point?
   for (i = 0; i < 4; ++i)
      if (dist[i] < 0.0)
         return kPickInfinity;

   float fDX2 = (dist[RIGHT] - dist[LEFT]) / (dist[RIGHT] + dist[LEFT]);
   float fDY2 = (dist[BOTTOM] - dist[TOP]) / (dist[BOTTOM] + dist[TOP]);

   fWeight = fDX2 * fDX2 + fDY2 * fDY2;

   // Throwing in the fudge term here ensures that all of our
   // calculations vary with screen resolution in the same way.
   fWeight += fCenterDist * g_fDistWeight;

#ifdef DBG_ON
   if (config_is_defined("PickSpew"))
      mprintf(" ==> %g (%g + %g)\n", fWeight,
              fWeight - fCenterDist * g_fDistWeight,
              fCenterDist * g_fDistWeight);
#endif // DBG_ON

   return fWeight;
}

///////////////////////////////////////////////////////////

// Call this for every pickable object you render.
ObjID PickWeighObject(ObjID obj)
{
   float fNewPickWeight = Weight(obj);
   if (fNewPickWeight < g_BestPickWeight)
   {
      g_BestPickWeight = fNewPickWeight;
      g_PickCurrentObj = obj;              // for now, just do this here...
   }

   return g_PickCurrentObj;
}

///////////////////////////////////////////////////////////

void PickSetFocus(fix FocusX, fix FocusY, float fMaxDistSquared)
{
   g_FocusX = FocusX;
   g_FocusY = FocusY;
   g_fMaxDistSquared = fMaxDistSquared;
   g_fDistWeight = sqrt(fMaxDistSquared) * kZWeightRatio;
}

///////////////////////////////////////////////////////////

void PickSetCanvas()
{
   g_fFudge = (float)(grd_canvas->bm.h >> kFudgeLog2);
}

///////////////////////////////////////////////////////////

// We don't have that much state to reset each frame.
// Our one clipping plane is in front of the camera by one unit.
void PickFrameUpdate()
{
   g_BestPickWeight = kPickInfinity;
   g_PickCurrentObj = OBJ_NULL;

   // Is it too restrictive to hard-code the player camera here?
   // (vs. any Position, say?)
   CameraGetLocation(PlayerCamera(), &g_CamVec, &g_CamAngle);

   mxs_matrix m;
   mxs_vector v;
   mxs_vector v2;
   mx_ang2mat(&m, &g_CamAngle);
   mx_mk_vec(&v, 1.0, 0, 0);
   mx_mat_mul_vec(&g_Plane.v, &m, &v);
   mx_scale_add_vec(&v2, &g_CamVec, &g_Plane.v, 1.0);
   g_Plane.d = -mx_dot_vec(&v2, &g_Plane.v);
}

///////////////////////////////////////////////////////////

static void ObjListener(ObjID obj, eObjNotifyMsg msg, void* data)
{
   if (msg == kObjNotifyDelete && obj == g_PickCurrentObj)
      g_PickCurrentObj = OBJ_NULL;
}
static tObjListenerHandle g_ObjListenerHandle = 0;

static sObjListenerDesc g_ObjListenerDesc =
{
   ObjListener, 0,
};

///////////////////////////////////////////////////////////

static sPropertyDesc PickBiasDesc = 
{ 
   "PickBias", 
   0,  // flags
   NULL, //constraints
   0, 0, // version
   {
      "Inventory",
      "Pick Bias", 
   }
};

static sPropertyDesc PickDistDesc = 
{ 
   "PickDist", 
   0,  // flags
   NULL, //constraints
   0, 0, // version
   {
      "Inventory",
      "Max Pick Distance", 
   }
};


// This sets up our objects destruction callback--the canvas and
// focus data are initialized separately since we don't know how
// often they'll be used.
void PickInit()
{
   g_ObjListenerHandle = AppGetObj(IObjectSystem)->Listen(&g_ObjListenerDesc);
   g_pPickBiasProp = CreateFloatProperty(&PickBiasDesc,kPropertyImplVerySparse);
   g_pPickDistProp = CreateFloatProperty(&PickDistDesc,kPropertyImplVerySparse);
   config_get_float("elliptic_pick_mod",&g_EllipticPickMod);
}

///////////////////////////////////////////////////////////

void PickTerm()
{
   AppGetObj(IObjectSystem)->Unlisten(g_ObjListenerHandle);
   SafeRelease(g_pPickBiasProp);
   SafeRelease(g_pPickDistProp);

}
