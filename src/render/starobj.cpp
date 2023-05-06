/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/starobj.cpp,v 1.6 2000/02/21 18:49:54 bfarquha Exp $


// Star rendering

#include <lg.h>
#include <lgd3d.h>
#include <r3d.h>
#include <filevar.h>
#include <sdesbase.h>
#include <sdesc.h>
#include <resapi.h>
#include <binrstyp.h>

#include <command.h>

#include <math.h>
#include <stdlib.h>

#include <simtime.h>

#include <star.h>

#include <starobj.h>

#include <skyrend.h>
#include <memall.h>
#include <dbmem.h>   // must be last header!



/////////////////////////////////////////////////////////////////////////
//
// Rendering implementations:
//

extern "C" BOOL g_lgd3d;
extern "C" BOOL portal_fog_on;



#define PI 3.14159265359
#define fDeg90 (0.50000*PI)
#define fDeg180 (PI)
#define fDeg360 (2*PI)

#define DEG (PI/180)

////////////////////////////////////////////////////////
// Mission Loading and Saving


// This describes the sky rendering of the mission.
struct sMissionStarObj
{
   BOOL bUseNewStars;
   BOOL bEnableFog;
   float fDensity;
   float fStarOffset; // 0 says no offset - stars fog like sky, higher numbers make stars "closer" than sky, in feet.
   float fMaxIntensity;
};



// Here's my descriptor, which identifies my stuff to the tag file & editor
sFileVarDesc g_StarObjFileVarDesc =
{
   kMissionVar,         // Where do I get saved?
   "STAROBJVAR",        // Tag file tag
   "NewSky:Stars",           // friendly name
   FILEVAR_TYPE(sMissionStarObj),// Type (for editing)
   { 1, 3},             // version
   { 1, 2},             // last valid version
};


// the class for actual global variable
class cMissionStarObj : public cFileVar<sMissionStarObj,&g_StarObjFileVarDesc>
{};

// the thing itself
static cMissionStarObj g_StarObj;


static sFieldDesc g_aStarObjField[] =
{
   { "Enable Stars", kFieldTypeBool,
     FieldLocation(sMissionStarObj, bUseNewStars), },
   { "Enable Fog", kFieldTypeBool,
     FieldLocation(sMissionStarObj, bEnableFog), },
   { "Star Density", kFieldTypeFloat,
     FieldLocation(sMissionStarObj, fDensity), },
   { "Star Offset", kFieldTypeFloat,
     FieldLocation(sMissionStarObj, fStarOffset), },
   { "Max Sky Intensity", kFieldTypeFloat,
     FieldLocation(sMissionStarObj, fMaxIntensity), },
};

static sStructDesc g_SDesc = StructDescBuild(sMissionStarObj, kStructFlagNone,
                                             g_aStarObjField);

static inline void SetDefaults()
{
   g_StarObj.bUseNewStars = FALSE;
   g_StarObj.bEnableFog = FALSE;
   g_StarObj.fDensity = 1;
   g_StarObj.fStarOffset = 0;
   g_StarObj.fMaxIntensity = 0.4;
}


////////////////////////////////////////////////////////////////

static void StarsInstallCommands();

void cStars::AppInit()
{
   AutoAppIPtr_(StructDescTools, pTools);
   pTools->Register(&g_SDesc);

   SetDefaults();

   StarsInstallCommands();
}


static BOOL bDidGameInit = FALSE;

static char *pRawStarData = 0;
static char *pStarData = 0;
static int nNumRawStars = 0;

static IRes *pStarRes = 0;

static uchar *pStarColors = 0;
static mxs_vector *pStarPositions = 0;
static int nNumStars = 0;

static ushort g_aBitmapBits[256];
static grs_bitmap g_Bitmap;
static g2s_point g_HWp[4];
static g2s_point *g_aHWPt[4] = {&g_HWp[0], &g_HWp[1], &g_HWp[2], &g_HWp[3]};

static BOOL bSpinStars = FALSE;
static tSimTime nLastTimeTime;
static const float fRadsPerSecond = 0.3490658503989; // 20 degrees

extern "C"
{
   BOOL StarRenderCallback(sStarRenderCallbackData *pData);
}



#define STAR_ALPHA_MIN 64
#define STAR_ALPHA_MAX 255

//
// Here's where we take the raw data, and turn it into a set of vectors and alphas (aka "colors").
// The alpha is based on the magnitude of the star and the intensity of the sky.
//
// calc RA. based on local time of day (at region 0,0) current longitude offset from origin, and and time of year.
// For time of year, fall is angle -90, winter is -180, etc..
//
static BOOL CalcStarData()
{
   // Chose a vector that represents where in the universe is straight up:
   // We'll do Winter solstice, at about 42 degrees latitude:
   mxs_vector TempVec;
   float fMag;
   float fDist;
   float fStarYAng = fDeg180;     // Winter, midnight
   float fStarXAng = 42.0*PI/180; // 42 degrees latitude.
   float fMinZ = sin(fDeg90-SkyRendererGetHorizonDip()); // Angle is relative to pole, so normalize.
   float fIntensity;
   float fClipAng = SkyRendererGetClipLat()*DEG;
   float fMaxZ = cos(fClipAng);

   IResMan *pResMan = AppGetObj(IResMan);
   pStarRes = IResMan_Bind(pResMan, "stars.bin", RESTYPE_BINARY, NULL, "fam\\skyhw\\", 0);
   if (!pStarRes)
      return FALSE;
   pRawStarData = (char *)pStarRes->Lock();
   if (!pRawStarData)
   {
      SafeRelease(pStarRes);
      pStarRes = 0;
      return FALSE;
   }

   nNumRawStars = *(int *)pRawStarData;
   pRawStarData += sizeof(int);
   pStarData = pRawStarData;

   if (!pStarPositions)
      pStarPositions = new mxs_vector[nNumRawStars];
   if (!pStarColors)
      pStarColors = new uchar[nNumRawStars];

#ifndef SHIP
   if (bSpinStars)
      fStarYAng += (GetSimTime()/(float)SIM_TIME_SECOND)*fRadsPerSecond;
#endif

   nNumStars = 0;
   for (int i = 0; i < nNumRawStars; i++)
   {
      pStarPositions[nNumStars].x = *(float *)pStarData;
      pStarData += sizeof(float);
      pStarPositions[nNumStars].y = *(float *)pStarData;
      pStarData += sizeof(float);
      pStarPositions[nNumStars].z = *(float *)pStarData;
      pStarData += sizeof(float);

      fMag = (1-*(float *)pStarData); // 0 mag is really highest, for astronomical reasons, reverse it.
      pStarData += sizeof(float);

      mx_rot_y_vec(&TempVec, pStarPositions+nNumStars, mx_rad2ang(fStarYAng));
      mx_rot_x_vec(pStarPositions+nNumStars, &TempVec, mx_rad2ang(fStarXAng));

      if (pStarPositions[nNumStars].z < fMinZ) // Below horizon. Don't bother with it.
         continue;

      if ((fClipAng >= 0) && pStarPositions[nNumStars].z > fMaxZ) // above clip, don't render.
         continue;

      if (fMag < (1-g_StarObj.fDensity))
         continue;

      // Base alpha on star's magnitude and intensity of sky:
      fIntensity = SkyRendererGetSkyIntensity(pStarPositions+nNumStars);
      fIntensity = __min(g_StarObj.fMaxIntensity, fIntensity)/g_StarObj.fMaxIntensity;
      pStarColors[nNumStars] = STAR_ALPHA_MIN + (STAR_ALPHA_MAX-STAR_ALPHA_MIN)*fMag*(1-fIntensity);

      // @TBD: set dist of stars based on sky dist minus a little? so stars will fog correctly.
      fDist = SkyRendererGetSkyDist(pStarPositions+nNumStars);
      mx_scaleeq_vec(pStarPositions+nNumStars, fDist-g_StarObj.fStarOffset);

      nNumStars++;
   }

   if (pRawStarData)
   {
      pRawStarData = 0;
      pStarRes->Unlock();
   }

   if (pStarRes)
   {
      SafeRelease(pStarRes);
      pStarRes = 0;
   }

   return TRUE;
}



// Call after mission parms are loaded, and after enhanced sky is inited.
// We Associate an alpha level for the stars based on the brightness of the sky at that point,
// and the overall intensity of the star itself. Then, it also gets fogged.
void cStars::Init()
{
   if (!g_StarObj.bUseNewStars || !SkyRendererUseEnhanced())
      return;

   if (!CalcStarData())
      return;

   StarSetStarRenderCallback(StarRenderCallback);

   StarSet(nNumStars,pStarPositions,pStarColors);

   // Now build star texture:
   float fDist;
   float fAlpha;
   ushort nAlpha;
   for (int u = 0; u < 16; ++u)
      for (int v = 0; v < 16; ++v) {
         fDist = sqrt((8 - u) * (8 - u) + (8 - v) * (8 - v));
         fAlpha = 8.0 - (ushort) ((fDist > 8.0)? 8.0 : fDist);  // clamp
         nAlpha = fAlpha * 15.0 / 8.0;
         g_aBitmapBits[u + 16 * v] = 0xfff | (nAlpha << 12);
      }

   gr_init_bitmap(&g_Bitmap, (uchar*) g_aBitmapBits,
                  BMT_FLAT16, BMF_RGB_4444, 16, 16);

   g_HWp[0].i = 1;
   g_HWp[0].u = 0;
   g_HWp[0].v = 0;
   g_HWp[0].w = .00001;

   g_HWp[1].i = 1;
   g_HWp[1].u = 1;
   g_HWp[1].v = 0;
   g_HWp[1].w = .00001;

   g_HWp[2].i = 1;
   g_HWp[2].u = 1;
   g_HWp[2].v = 1;
   g_HWp[2].w = .00001;

   g_HWp[3].i = 1;
   g_HWp[3].u = 0;
   g_HWp[3].v = 1;
   g_HWp[3].w = .00001;

   nLastTimeTime = 0;
   bDidGameInit = TRUE;
}


void cStars::Term()
{
   // Free stuff here

   if (pRawStarData)
   {
      pRawStarData = 0;
      pStarRes->Unlock();
   }

   if (pStarRes)
   {
      SafeRelease(pStarRes);
      pStarRes = 0;
   }

   if (pStarPositions)
   {
      delete []pStarPositions;
      pStarPositions = 0;
   }

   if (pStarColors)
   {
      delete []pStarColors;
      pStarColors = 0;
   }

// Interferes with database update
//   SetDefaults();

   StarSet(0,NULL,NULL);

   bDidGameInit = FALSE;
}

// From lgd3d:
EXTERN double z2d;
EXTERN double w2d;

void cStars::Render()
{
   if (!g_lgd3d || !bDidGameInit)
      return;

#ifndef SHIP
   if (bSpinStars)
      CalcStarData();
#endif

   r3e_space nOldSpace = r3_get_space();
   r3_set_space(R3_CLIPPING_SPACE);

   r3_set_clipmode(R3_CLIP);
   r3_set_clip_flags(R3_CLIP_UV);
   gr_set_fill_type(FILL_BLEND);
   r3_set_polygon_context(R3_PL_POLYGON|R3_PL_TEXTURE);
   r3_set_prim();

   lgd3d_set_blend(TRUE);
   lgd3d_set_texture(&g_Bitmap);

   BOOL bRestoreZWrite = lgd3d_is_zwrite_on();
   BOOL bRestoreZCompare = lgd3d_is_zcompare_on();
   lgd3d_set_zwrite(FALSE);
   lgd3d_set_zcompare(FALSE);

   lgd3d_set_fog_enable(portal_fog_on && g_StarObj.bEnableFog);

   StarSetCheckBkgnd(FALSE);
   StarSky();
   StarRenderSimple();
   gr_set_fill_type(FILL_NORM);
   r3_set_prim();
   lgd3d_set_blend(FALSE);
   lgd3d_set_zwrite(bRestoreZWrite);
   lgd3d_set_zcompare(bRestoreZCompare);
   lgd3d_set_fog_enable(portal_fog_on);
   r3_set_space(nOldSpace);
   z2d = 1;
   w2d = 1;
}


extern "C" BOOL StarRenderCallback(sStarRenderCallbackData *pData)
{
   w2d = pData->pPoint->grp.w;
   z2d = pData->pPoint->p.z;

   // upper left
   g_HWp[0].sx = pData->pPoint->grp.sx - fix_make(-1, 32767);
   g_HWp[0].sy = pData->pPoint->grp.sy - fix_make(-1, 32767);
   g_HWp[0].w = pData->pPoint->grp.w;

   // upper right
   g_HWp[1].sx = g_HWp[0].sx + fix_make(3, 0);
   g_HWp[1].sy = g_HWp[0].sy;
   g_HWp[1].w = pData->pPoint->grp.w;

   // lower right
   g_HWp[2].sx = g_HWp[1].sx;
   g_HWp[2].sy = g_HWp[0].sy + fix_make(3, 0);
   g_HWp[2].w = pData->pPoint->grp.w;

   // lower left
   g_HWp[3].sx = g_HWp[0].sx;
   g_HWp[3].sy = g_HWp[2].sy;
   g_HWp[3].w = pData->pPoint->grp.w;

   lgd3d_set_alpha(pData->color/(float)255);
   lgd3d_g2trifan(4, g_aHWPt);

   return FALSE; // Don't pass on to previous handler
}


static Command g_StarCommands[] =
{
   { "stars_spin", TOGGLE_BOOL, &bSpinStars, "Spin stars", HK_ALL},
};



static void StarsInstallCommands()
{
#ifndef SHIP
   COMMANDS(g_StarCommands, HK_ALL);
#endif
}
