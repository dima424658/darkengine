/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/skyobj.cpp,v 1.8 2000/02/21 18:49:53 bfarquha Exp $


// Sky rendering

#include <lg.h>
#include <lgd3d.h>
#include <r3d.h>
#include <filevar.h>
#include <sdesbase.h>
#include <sdesc.h>

#include <command.h>

#include <math.h>
#include <stdlib.h>

#include <skyobj.h>

#include <mprintf.h>
#include <memall.h>
#include <dbmem.h>   // must be last header!


/////////////////////////////////////////////////////////////////////////
//
// Rendering implementations:
//

extern "C" BOOL g_lgd3d;
extern "C" BOOL portal_fog_on;


struct goddamn_stupid_r3s_point
{
   mxs_vector p;
   ulong ccodes;
   g2s_point grp;
};

static goddamn_stupid_r3s_point *pSkyPoints = 0;
static mxs_vector *pSkyPointVecs = 0;
static mxs_vector *pSkyPointColors = 0;
static r3s_phandle *vlist = 0; // Point transforming/rendering

static float *pSkyLats = 0;

#define PI 3.14159265359
#define fDeg90 (0.50000*PI)
#define fDeg180 (PI)
#define fDeg360 (2*PI)

#define DEG (PI/180)


#ifndef SHIP
static int nDebugLong = -1;
static int nDebugLat = -1;
#endif

////////////////////////////////////////////////////////
// Mission Loading and Saving


enum eGlowMethod
{
   kMethod_Sum,
   kMethod_Interpolate
};

// This describes the sky rendering of the mission.
struct sMissionSkyObj
{
   BOOL bUseNewSky;
   BOOL bEnableFog;
   float fRadius;
   float fCenterOffset;
   int nNumLatPoints; // # lats + horizon dip. from pole to horizon
   int nNumLongPoints; // # longs per 360
   float fHorizonDipAng; // Angle dip under horizon. 90 deg is horizon.
   mxs_vector ControlPointColors[5];
   mxs_vector GlowColor;
   float fGlowLat;  // in degrees
   float fGlowLong; // in degrees
   float fGlowAng;  // in degrees - angle around vec
   float fGlowScale;  // from 0 to 1 - also can be interpreted as angle after which glow tapers off.
   eGlowMethod GlowMethod;
   float fClipLat; // lat angle in degrees, below which no sky will be rendered.
};



// Here's my descriptor, which identifies my stuff to the tag file & editor
sFileVarDesc g_SkyObjFileVarDesc =
{
   kMissionVar,         // Where do I get saved?
   "SKYOBJVAR",        // Tag file tag
   "NewSky:Sky",           // friendly name
   FILEVAR_TYPE(sMissionSkyObj),// Type (for editing)
   { 1, 4},             // version
   { 1, 3},             // last valid version
};


// the class for actual global variable
class cMissionSkyObj : public cFileVar<sMissionSkyObj,&g_SkyObjFileVarDesc>
{
public:
   void Update()
   {
   }

   void Reset()
   {
      cFileVarBase::Reset();
   }
};

// the thing itself
static cMissionSkyObj g_SkyObj;


static const char* g_apszGlowMethodName[] = { "Sum", "Interpolate" };

static sFieldDesc g_aSkyObjField[] =
{
   { "Enable New Sky", kFieldTypeBool,
     FieldLocation(sMissionSkyObj, bUseNewSky), },
   { "Enable Fog", kFieldTypeBool,
     FieldLocation(sMissionSkyObj, bEnableFog), },
   { "Atmosphere radius", kFieldTypeFloat,
     FieldLocation(sMissionSkyObj, fRadius), },
   { "Earth Radius", kFieldTypeFloat,
     FieldLocation(sMissionSkyObj, fCenterOffset), },
   { "# of latitude points", kFieldTypeInt,
     FieldLocation(sMissionSkyObj, nNumLatPoints), },
   { "# of longitude points", kFieldTypeInt,
     FieldLocation(sMissionSkyObj, nNumLongPoints), },
   { "Horizon dip angle", kFieldTypeFloat,
     FieldLocation(sMissionSkyObj, fHorizonDipAng), },
   { "Pole color", kFieldTypeVector,
     FieldLocation(sMissionSkyObj, ControlPointColors[0]), },
   { "45 degree color", kFieldTypeVector,
     FieldLocation(sMissionSkyObj, ControlPointColors[1]), },
   { "70 degree color", kFieldTypeVector,
     FieldLocation(sMissionSkyObj, ControlPointColors[2]), },
   { "Horizon color", kFieldTypeVector,
     FieldLocation(sMissionSkyObj, ControlPointColors[3]), },
   { "Horizon dip color", kFieldTypeVector,
     FieldLocation(sMissionSkyObj, ControlPointColors[4]), },
   { "Glow color", kFieldTypeVector,
     FieldLocation(sMissionSkyObj, GlowColor), },
   { "Glow latitude", kFieldTypeFloat,
     FieldLocation(sMissionSkyObj, fGlowLat), },
   { "Glow longitude", kFieldTypeFloat,
     FieldLocation(sMissionSkyObj, fGlowLong), },
   { "Glow angle", kFieldTypeFloat,
     FieldLocation(sMissionSkyObj, fGlowAng), },
   { "Glow scale", kFieldTypeFloat,
     FieldLocation(sMissionSkyObj, fGlowScale), },
   { "Glow method", kFieldTypeEnum, FieldLocation(sMissionSkyObj, GlowMethod),
     FullFieldNames(g_apszGlowMethodName) },
   { "Clip latitude", kFieldTypeFloat,
     FieldLocation(sMissionSkyObj, fClipLat), },
};

static sStructDesc g_SDesc = StructDescBuild(sMissionSkyObj, kStructFlagNone,
                                             g_aSkyObjField);


static inline void SetDefaults()
{
   g_SkyObj.bUseNewSky = FALSE;
   g_SkyObj.bEnableFog = FALSE;

   g_SkyObj.fRadius = 1944062+7620; // Earth's radius + Atmosphere radius
   g_SkyObj.fCenterOffset = 1944062;

   // How many lat points are used from pole to horizon, plus one for horizon dip.
   // We'll start with lats every 15 degrees for a total of 6+1:
   g_SkyObj.nNumLatPoints = 7;

   // Number of lat points in 360 around. We'll start with 32, since that worked well in FUIII,
   // but we really want to being the # of points transformed down, so try 24.
   g_SkyObj.nNumLongPoints = 24;

   g_SkyObj.fHorizonDipAng = 100; // 10 degrees under horizon.

   // One at pole, one at 45 degree lat, one at 70 degree lat, one at 90.
   mx_mk_vec(&g_SkyObj.ControlPointColors[0], 0.8, 0.5, 0.1); // Pole
   mx_mk_vec(&g_SkyObj.ControlPointColors[1], 0.9, 0.6, 0.2); // 45
   mx_mk_vec(&g_SkyObj.ControlPointColors[2], 0.8, 0.6, 0.4); // 70
   mx_mk_vec(&g_SkyObj.ControlPointColors[3], 0.6, 0.3, 0.1); // 90
   mx_mk_vec(&g_SkyObj.ControlPointColors[4], 0.6, 0.2, 0.05); // 90

   memset(&g_SkyObj.GlowColor, 0, sizeof(mxs_vector));
   g_SkyObj.fGlowLat = 0;
   g_SkyObj.fGlowLong = 0;
   g_SkyObj.fGlowAng = 0;
   g_SkyObj.fGlowScale = 1;
   g_SkyObj.GlowMethod = kMethod_Sum;
}



////////////////////////////////////////////////////////////////


static float ControlPointLats[] =
{
   0, 45*PI/180, 70*PI/180, 90*PI/180, 90*PI/180     // last slot reserved for horizon dip
};
static const int nNumControlPoints = 5;



static int nTotalPoints;

/////////////////////////////////////////////////////////////
// Lat angle 0 deg is pole, angle 90 deg is horizon
static inline float ComputeSkyPointVec(mxs_vector *pVec, float fLatAng, float fLongAng)
{
   mxs_vector Vec = {0,0,1};
   mxs_vector Temp;
   float fSinTheta;
   float fCosTheta;
   float fVal;
   float fMag;

   mx_rot_y_vec(pVec, &Vec, mx_rad2ang(fLatAng));
   fSinTheta = pVec->x;

   mx_rot_z_vec(&Temp, pVec, mx_rad2ang(fLongAng));
   fCosTheta = Temp.z;

   fVal = g_SkyObj.fCenterOffset*fSinTheta;
   fMag = sqrt(g_SkyObj.fRadius*g_SkyObj.fRadius - fVal*fVal) - g_SkyObj.fCenterOffset*fCosTheta;

   mx_scale_vec(pVec, &Temp, fMag);
   return fMag;
}


#define MY_OMEGA 0.0001
static inline void ComputeSkyPointColor(mxs_vector *pSkyPointColor, float fLatAng, float fLongAng)
{
   int i;
   mxs_vector ZVec = {0,0,1};
   mxs_vector DirVec;
   mxs_vector GlowDirection;
   mxs_vector Temp;

   for (i = 0; i < nNumControlPoints; i++)
      if (fLatAng < (ControlPointLats[i]+MY_OMEGA))
         break;

   if (i==nNumControlPoints)
      *pSkyPointColor = g_SkyObj.ControlPointColors[nNumControlPoints-1];
   else if (!i)
      *pSkyPointColor = g_SkyObj.ControlPointColors[0];
   else
   {
      float fInterp = (fLatAng-ControlPointLats[i-1])/(ControlPointLats[i]-ControlPointLats[i-1]);
      pSkyPointColor->x = g_SkyObj.ControlPointColors[i-1].x + (g_SkyObj.ControlPointColors[i].x-g_SkyObj.ControlPointColors[i-1].x)*fInterp;
      pSkyPointColor->y = g_SkyObj.ControlPointColors[i-1].y + (g_SkyObj.ControlPointColors[i].y-g_SkyObj.ControlPointColors[i-1].y)*fInterp;
      pSkyPointColor->z = g_SkyObj.ControlPointColors[i-1].z + (g_SkyObj.ControlPointColors[i].z-g_SkyObj.ControlPointColors[i-1].z)*fInterp;
   }


   // Now factor in "glow"
   mx_rot_y_vec(&Temp, &ZVec, mx_rad2ang(fLatAng));
   mx_rot_z_vec(&DirVec, &Temp, mx_rad2ang(fLongAng));

   mx_rot_y_vec(&Temp, &ZVec, mx_rad2ang(g_SkyObj.fGlowLat*DEG));
   mx_rot_z_vec(&GlowDirection, &Temp, mx_rad2ang(g_SkyObj.fGlowLong*DEG));

   float fGlowDegree = mx_dot_vec(&GlowDirection, &DirVec);
   float fGlowCos = cos(g_SkyObj.fGlowAng*DEG/2);

   if (fGlowDegree < fGlowCos) // no glow here
      return;

   if (g_SkyObj.fGlowScale < MY_OMEGA)
      return;

   fGlowDegree = (fGlowDegree-fGlowCos)/(1-fGlowCos)*g_SkyObj.fGlowScale;

   if (g_SkyObj.GlowMethod == kMethod_Sum)
   {
      pSkyPointColor->x = 1-(1-pSkyPointColor->x)*(1-g_SkyObj.GlowColor.x*fGlowDegree);
      pSkyPointColor->y = 1-(1-pSkyPointColor->y)*(1-g_SkyObj.GlowColor.y*fGlowDegree);
      pSkyPointColor->z = 1-(1-pSkyPointColor->z)*(1-g_SkyObj.GlowColor.z*fGlowDegree);
   }
   else
   {
      pSkyPointColor->x = pSkyPointColor->x + (g_SkyObj.GlowColor.x-pSkyPointColor->x)*fGlowDegree;
      pSkyPointColor->y = pSkyPointColor->y + (g_SkyObj.GlowColor.y-pSkyPointColor->y)*fGlowDegree;
      pSkyPointColor->z = pSkyPointColor->z + (g_SkyObj.GlowColor.z-pSkyPointColor->z)*fGlowDegree;
   }
}



static void SkyInstallCommands();

void cSky::AppInit()
{
   AutoAppIPtr_(StructDescTools, pTools);
   pTools->Register(&g_SDesc);

   SetDefaults();
   SkyInstallCommands();
}


static BOOL TestValues()
{
   BOOL bFailure = FALSE;

   if (g_SkyObj.fRadius < 0.001)
   {
      Warning(("Invalid atmosphere radius %g\n", g_SkyObj.fRadius));
      bFailure = TRUE;
   }

   if ((g_SkyObj.fCenterOffset < 0.001) || (g_SkyObj.fCenterOffset >= g_SkyObj.fRadius))
   {
      Warning(("Invalid earth radius %g\n", g_SkyObj.fCenterOffset));
      bFailure = TRUE;
   }

   if (g_SkyObj.nNumLatPoints < 3)
   {
      Warning(("Invalid # of sky lat points %d\n", g_SkyObj.nNumLatPoints));
      bFailure = TRUE;
   }

   if (g_SkyObj.nNumLongPoints < 3)
   {
      Warning(("Invalid # of sky long points %d\n", g_SkyObj.nNumLongPoints));
      bFailure = TRUE;
   }

   if (g_SkyObj.fHorizonDipAng < 90)
   {
      Warning(("Invalid Horizon dip angle %g\n", g_SkyObj.fHorizonDipAng));
      bFailure = TRUE;
   }

   return bFailure;
}



static BOOL bDidGameInit = FALSE;

// Tesselate.
// Call after mission parms are loaded.
void cSky::Init()
{
   int i,j;

   if (!g_SkyObj.bUseNewSky)
      return;

   if (TestValues())
      return;

   // Stored in degrees. Convert to rads.
   float fHorizonDipAng = g_SkyObj.fHorizonDipAng*PI/180;

   // Set dip ang:
   ControlPointLats[4] = fHorizonDipAng;

   nTotalPoints = 1+(g_SkyObj.nNumLatPoints-1)*g_SkyObj.nNumLongPoints; // Add one for "pole" point, remove one row of longitude points since pole has none.
   float fLatAng;
   float fLongAng;

   pSkyPoints = new goddamn_stupid_r3s_point[nTotalPoints];
   vlist = new r3s_phandle[nTotalPoints];
   pSkyPointVecs = new mxs_vector[nTotalPoints];
   pSkyPointColors = new mxs_vector[nTotalPoints];
   pSkyLats = new float[g_SkyObj.nNumLatPoints-1];

   for (i = 0; i < nTotalPoints; i++)
      vlist[i] = (r3s_point*)(pSkyPoints+i);

   // Calc points from pole to horizon (0 degrees)
   for (i = 0; i < g_SkyObj.nNumLatPoints-1; i++)
   {
      if (!i) // this is pole point, no long points
      {
         ComputeSkyPointVec(pSkyPointVecs, 0, 0);
         ComputeSkyPointColor(pSkyPointColors, 0, 0);
         continue;
      }

      fLatAng = i/(float)(g_SkyObj.nNumLatPoints-2);

      // Store in degrees:
      pSkyLats[i-1] = 90*fLatAng;

      // Convert to rads:
      fLatAng = fDeg90*fLatAng;

      for (j = 0; j < g_SkyObj.nNumLongPoints; j++)
      {
         fLongAng = fDeg360*j/(float)g_SkyObj.nNumLongPoints;
         ComputeSkyPointVec(pSkyPointVecs+1+(i-1)*g_SkyObj.nNumLongPoints+j, fLatAng, fLongAng);
         ComputeSkyPointColor(pSkyPointColors+1+(i-1)*g_SkyObj.nNumLongPoints+j, fLatAng, fLongAng);
      }
   }

   // Last, do horizon dip:

   pSkyLats[i-1] = fHorizonDipAng/DEG; // convert to degrees:

   // Assumes i is at least 1.
   for (j = 0; j < g_SkyObj.nNumLongPoints; j++)
   {
      fLongAng = fDeg360*j/(float)g_SkyObj.nNumLongPoints;
      ComputeSkyPointVec(pSkyPointVecs+1+(i-1)*g_SkyObj.nNumLongPoints+j, fHorizonDipAng, fLongAng);
      ComputeSkyPointColor(pSkyPointColors+1+(i-1)*g_SkyObj.nNumLongPoints+j, fHorizonDipAng, fLongAng); // just use last computed value.
   }

   bDidGameInit = TRUE;
}


void cSky::Term()
{
   if (pSkyPointColors)
   {
      delete []pSkyPointColors;
      pSkyPointColors = 0;
   }

   if (pSkyPointVecs)
   {
      delete []pSkyPointVecs;
      pSkyPointVecs = 0;
   }

   if (pSkyPoints)
   {
      delete []pSkyPoints;
      pSkyPoints = 0;
   }

   if (vlist)
   {
      delete []vlist;
      vlist = 0;
   }

   if (pSkyLats)
   {
      delete pSkyLats;
      pSkyLats = 0;
   }

// Interferes with Database Update when loading saved game:
//   SetDefaults();

   bDidGameInit = FALSE;
}


BOOL cSky::Enabled()
{
   return g_lgd3d && g_SkyObj.bUseNewSky;
}


float cSky::GetHorizonDip()
{
   return g_SkyObj.fHorizonDipAng*PI/180;
}

float cSky::GetClipLat()
{
   return g_SkyObj.fClipLat;
}


// Do interpolation to get intensity via color in this direction:
float cSky::GetSkyIntensity(mxs_vector *pDir)
{
   mxs_vector Color;
   float fLatAng;
   float fLongAng;

   if (pDir->z > 0)
      fLatAng = acos(pDir->z);
   else
      fLatAng = fDeg180-acos(-pDir->z);

   fLongAng = atan2(pDir->y, pDir->x);

   ComputeSkyPointColor(&Color, fLatAng, fLongAng);
   return __max(__max(Color.x, Color.y), Color.z);
}


// This is not exact, since the math is pure, but the sky is large polys, so dist only approximates
// the actual dist to poly.
float cSky::GetSkyDist(mxs_vector *pDir)
{
   mxs_vector Vec;
   float fLatAng;

   if (pDir->z > 0)
      fLatAng = acos(pDir->z);
   else
      fLatAng = fDeg180-acos(-pDir->z);

   return ComputeSkyPointVec(&Vec, fLatAng, 0);
}




EXTERN BOOL r3d_do_setup;

void cSky::Render()
{
   if (!g_lgd3d || !bDidGameInit)
      return;

//      lgd3d_set_dithering(TRUE);
//   bRestoreDither = lgd3d_is_dithering_on();

   int i,j;
   int nStride;
   int nLastLongIx;
   r3s_phandle PolyPoints[4];
   mxs_vector OldPos;
   mxs_vector ZeroPos = {0,0,0};
   mxs_vector BlackColor = {0,0,0};
   mxs_vector UseColor[4];

   BOOL bRestoreZWrite = lgd3d_is_zwrite_on();
   BOOL bRestoreZCompare = lgd3d_is_zcompare_on();
   lgd3d_set_zwrite(FALSE);
   lgd3d_set_zcompare(FALSE);
   lgd3d_disable_palette();

   lgd3d_set_blend(FALSE);

   r3e_space nOldSpace = r3_get_space();
   r3_set_space(R3_CLIPPING_SPACE);

   OldPos = *r3_get_view_pos();
   r3_set_view(&ZeroPos);

   r3_start_block();
   r3_set_clipmode(R3_CLIP);
   r3_set_clip_flags(R3_CLIP_RGB);

   gr_set_fill_type(FILL_NORM);
   r3_set_prim(); // synch r3d and g2

   nStride = r3d_glob.cur_stride;
   r3d_glob.cur_stride = sizeof(goddamn_stupid_r3s_point);

   lgd3d_set_fog_enable(portal_fog_on && g_SkyObj.bEnableFog);

   r3_set_polygon_context(R3_PL_POLYGON|R3_PL_SOLID|R3_PL_RGB_GOURAUD);
   r3_set_color(0xffffff);

   r3_transform_block(nTotalPoints, (r3s_point *)pSkyPoints, pSkyPointVecs);

   // First render "polar", aka "triangle" polys:
   PolyPoints[0] = vlist[0]; // Pole point

   if (pSkyLats[0] > g_SkyObj.fClipLat) // First sky lat is non-pole.
   {
      for (i = 0; i < g_SkyObj.nNumLongPoints; i++)
      {
         nLastLongIx = (i < (g_SkyObj.nNumLongPoints-1)) ? (i+1) : 0; // Compute as if 0-based.

   #ifndef SHIP
         if (((nDebugLat != -1) && (nDebugLat != 0)) || ((nDebugLong != -1) && (nDebugLong != i)))
         {
            UseColor[0] = BlackColor;
            UseColor[1] = BlackColor;
            UseColor[2] = BlackColor;
         }
         else
   #endif
         {
            UseColor[0] = pSkyPointColors[0];
            UseColor[1] = pSkyPointColors[1+i];
            UseColor[2] = pSkyPointColors[1+nLastLongIx];
         }

         PolyPoints[1] = vlist[1+i]; // Add one since first one is pole-point.
         PolyPoints[2] = vlist[1+nLastLongIx];

         r3d_ccodes_and = 0;
         r3_code_polygon(3, PolyPoints);
         if (!r3d_ccodes_and) // Visible
         {
            ((goddamn_stupid_r3s_point *)(PolyPoints[0]))->grp.i = UseColor[0].x;
            ((goddamn_stupid_r3s_point *)(PolyPoints[0]))->grp.h = UseColor[0].y;
            ((goddamn_stupid_r3s_point *)(PolyPoints[0]))->grp.d = UseColor[0].z;

            ((goddamn_stupid_r3s_point *)(PolyPoints[1]))->grp.i = UseColor[1].x;
            ((goddamn_stupid_r3s_point *)(PolyPoints[1]))->grp.h = UseColor[1].y;
            ((goddamn_stupid_r3s_point *)(PolyPoints[1]))->grp.d = UseColor[1].z;

            ((goddamn_stupid_r3s_point *)(PolyPoints[2]))->grp.i = UseColor[2].x;
            ((goddamn_stupid_r3s_point *)(PolyPoints[2]))->grp.h = UseColor[2].y;
            ((goddamn_stupid_r3s_point *)(PolyPoints[2]))->grp.d = UseColor[2].z;

            r3d_do_setup = TRUE;
            r3_draw_poly(3, PolyPoints);
         }
      }
   }


   // Now do all other normal, 4 point polys.
   for (i = 1; i < g_SkyObj.nNumLatPoints-1; i++)
   {
      // If bottom of poly is above clip angle, then don't render.
      if (pSkyLats[i] < g_SkyObj.fClipLat)
         continue;

      for (j = 0; j < g_SkyObj.nNumLongPoints; j++)
      {
         nLastLongIx = (j+1)%g_SkyObj.nNumLongPoints;

#ifndef SHIP
         if (((nDebugLat != -1) && (nDebugLat != i)) || ((nDebugLong != -1) && (nDebugLong != j)))
         {
            UseColor[0] = BlackColor;
            UseColor[1] = BlackColor;
            UseColor[2] = BlackColor;
            UseColor[3] = BlackColor;
         }
         else
#endif
         {
            UseColor[0] = pSkyPointColors[1+(i-1)*g_SkyObj.nNumLongPoints+nLastLongIx];
            UseColor[1] = pSkyPointColors[1+(i-1)*g_SkyObj.nNumLongPoints+j];
            UseColor[2] = pSkyPointColors[1+i*g_SkyObj.nNumLongPoints+j];
            UseColor[3] = pSkyPointColors[1+i*g_SkyObj.nNumLongPoints+nLastLongIx];
         }

         PolyPoints[0] = vlist[1+(i-1)*g_SkyObj.nNumLongPoints+nLastLongIx];
         PolyPoints[1] = vlist[1+(i-1)*g_SkyObj.nNumLongPoints+j];
         PolyPoints[2] = vlist[1+i*g_SkyObj.nNumLongPoints+j];
         PolyPoints[3] = vlist[1+i*g_SkyObj.nNumLongPoints+nLastLongIx];

         r3d_ccodes_and = 0;
         r3_code_polygon(4, PolyPoints);
         if (!r3d_ccodes_and) // Visible
         {
            ((goddamn_stupid_r3s_point *)(PolyPoints[0]))->grp.i = UseColor[0].x;
            ((goddamn_stupid_r3s_point *)(PolyPoints[0]))->grp.h = UseColor[0].y;
            ((goddamn_stupid_r3s_point *)(PolyPoints[0]))->grp.d = UseColor[0].z;

            ((goddamn_stupid_r3s_point *)(PolyPoints[1]))->grp.i = UseColor[1].x;
            ((goddamn_stupid_r3s_point *)(PolyPoints[1]))->grp.h = UseColor[1].y;
            ((goddamn_stupid_r3s_point *)(PolyPoints[1]))->grp.d = UseColor[1].z;

            ((goddamn_stupid_r3s_point *)(PolyPoints[2]))->grp.i = UseColor[2].x;
            ((goddamn_stupid_r3s_point *)(PolyPoints[2]))->grp.h = UseColor[2].y;
            ((goddamn_stupid_r3s_point *)(PolyPoints[2]))->grp.d = UseColor[2].z;

            ((goddamn_stupid_r3s_point *)(PolyPoints[3]))->grp.i = UseColor[3].x;
            ((goddamn_stupid_r3s_point *)(PolyPoints[3]))->grp.h = UseColor[3].y;
            ((goddamn_stupid_r3s_point *)(PolyPoints[3]))->grp.d = UseColor[3].z;

            r3d_do_setup = TRUE;
            r3_draw_poly(4, PolyPoints);
         }
      }
   }

//   lgd3d_set_dithering(bRestoreDither);

   r3d_glob.cur_stride = nStride;
   lgd3d_enable_palette();

   r3_end_block();

   r3_set_view(&OldPos);

   lgd3d_set_zwrite(bRestoreZWrite);
   lgd3d_set_zcompare(bRestoreZCompare);

   lgd3d_set_fog_enable(portal_fog_on);

   r3_set_space(nOldSpace);
}


///////////////////////////////////
// Debug stuff:

static void SetDebugLong(int nIx)
{
#ifndef SHIP
   nDebugLong = nIx;
#endif
}

static void SetDebugLat(int nIx)
{
#ifndef SHIP
   nDebugLat = nIx;
#endif
}

static void DumpStats()
{
   int nTotalMemory = 0;
   nTotalMemory = MSize(pSkyPoints)+MSize(pSkyPointVecs)+MSize(pSkyPointColors)+MSize(vlist)+MSize(pSkyLats);

   mprintf("There %d sky points using %g kbytes\n", nTotalPoints, nTotalMemory/1024.000);
}

#ifndef SHIP
static Command g_SkyCommands[] =
{
   { "sky_setlong", FUNC_INT, SetDebugLong, "Debug Sky Longitude", HK_ALL},
   { "sky_setlat",  FUNC_INT, SetDebugLat,  "Debug Sky Latitude", HK_ALL},
   { "sky_dump", FUNC_VOID, DumpStats, "Dump sky mesh info", HK_ALL},
};
#endif



static void SkyInstallCommands()
{
#ifndef SHIP
   COMMANDS(g_SkyCommands, HK_ALL);
#endif
}

