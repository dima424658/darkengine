/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/weather.cpp,v 1.17 2000/03/02 15:25:09 MAT Exp $

/* --<<= -+-/-/-/=/=/\/- <<< ((( ((( / \ ))) ))) >>> -\/\=\=\-\-\-+- =>>-- *\
   weather.cpp

   Our weather is mainly rendering effects.  This module includes
   both game-only rendering stuff and a little #ifdef EDITOR material.

   spews:
   WeatherMakeSpew                      how much precip per frame
   WeatherRoomSpew                      mapping rooms to cells
   WeatherFrameSpew                     checking for cells

\* --<<= -+-\-\-\=\=\/\- <<< ((( ((( \ / ))) ))) >>> -/\/=/=/-/-/-+- =>>-- */


#include <math.h>

#include <tagfile.h>
#include <vernum.h>
#include <mprintf.h>
#include <cfgdbg.h>
#include <r3d.h>
#include <random.h>
#include <r3ds.h>
#include <grspoint.h>
#include <lgd3d.h>

#include <prophash.h>
#include <dataops.h>
#include <obprpimp.h>
#include <sdesbase.h>
#include <sdesc.h>

#include <filevar.h>
#include <playrobj.h>
#include <objmodel.h>
#include <camera.h>
#include <port.h>
#include <wrtype.h>
#include <bspsphr.h>
#include <wrdb.h>
#include <wrdbrend.h>
#include <wrfunc.h>
#include <texmem.h>
#include <room.h>
#include <roomtowr.h>
#include <weather.h>

#include <dbmem.h>
#include <initguid.h>
#include <weatherg.h>

#include <config.h>

// outside influence
extern "C" void (*portal_post_render_cback)(void);
extern "C" BOOL g_lgd3d;

// predeclarations
void SetDerivedGlobals();
void CheckForWeatherCells();


/* --<<= -+-/-/-/=/=/\/- <<< ((( ((( / \ ))) ))) >>> -\/\=\=\-\-\-+- =>>-- *\

   constants

\* --<<= -+-\-\-\=\=\/\- <<< ((( ((( \ / ))) ))) >>> -/\/=/=/-/-/-+- =>>-- */

const float kSphereCheckDist2 = 10 * 10;
const float kSphereTeleportSpeed = .1; // feet/millisec.
const float kMoveEpsilon = .1;
const float kSnowTurnSpeed = 3;
const float kRainFromSide = .98;
const float kMaxPrecipPerFrame = 30;
// Splashes which are further from the camera have lower alphas, but
// no lower than this.
const float kSplashMinAlpha = .15;
// We can have splashes on planes within 45 degrees of horizontal.
const float kSplashMinPlaneZ = .707;

// per-cell flags
const uchar kCellHasDebris = 1;
const uchar kCellHasPrecip = 2;
const uchar kCellHasWind = 4;
const int kWeatherStringMax = 32;


/* --<<= -+-/-/-/=/=/\/- <<< ((( ((( / \ ))) ))) >>> -\/\=\=\-\-\-+- =>>-- *\

   weather property--for use on room objects

\* --<<= -+-\-\-\=\=\/\- <<< ((( ((( \ / ))) ))) >>> -/\/=/=/-/-/-+- =>>-- */

static sPropertyDesc g_WeatherDesc
= {
   "Weather",
   0,           // flags
   NULL,        // constraints
   1, 0,        // version
   { "Weather", "Local Weather", "weather for a room" },
   kPropertyChangeLocally,
};

class cWeatherOps : public cClassDataOps<sWeather>
{
};

class cWeatherStore : public cHashPropertyStore<cWeatherOps>
{
};

typedef cSpecificProperty<IWeatherProperty,
   &IID_IWeatherProperty, sWeather *,
   cWeatherStore> cBaseWeatherProperty;

class cWeatherProperty : public cBaseWeatherProperty
{
public:
   cWeatherProperty(const sPropertyDesc *desc)
      : cBaseWeatherProperty(desc)
   {
   }

   STANDARD_DESCRIBE_TYPE(cWeatherProperty);

protected:
   void CreateEditor();
};


static sFieldDesc g_aWeatherFieldDesc[]
= {
   { "fog", kFieldTypeBool,
     FieldLocation(sWeather, m_bFog) },
   { "precipitation", kFieldTypeBool,
     FieldLocation(sWeather, m_bPrecipitation) },
};

static sStructDesc g_WeatherStructDesc
= StructDescBuild(cWeatherProperty, kStructFlagNone,
                  g_aWeatherFieldDesc);

void cWeatherProperty::CreateEditor()
{
   AutoAppIPtr(StructDescTools);
   pStructDescTools->Register(&g_WeatherStructDesc);
   cPropertyBase::CreateEditor(this);
}

IWeatherProperty *g_pWeatherProperty;


/* --<<= -+-/-/-/=/=/\/- <<< ((( ((( / \ ))) ))) >>> -\/\=\=\-\-\-+- =>>-- *\

   current game state (stuff not saved/loaded)

\* --<<= -+-\-\-\=\=\/\- <<< ((( ((( \ / ))) ))) >>> -/\/=/=/-/-/-+- =>>-- */

struct sPrecipPos
{
   mxs_vector m_Pos;
   mxs_vector m_Impact;
   long m_iMillisecsLeft;
   uchar m_Facing;      // only matters for snow: 0..7
   uchar m_Flake;       // which quarter of snow texture: 0..3
   bool m_bImpact;
   bool m_bSplash;
};


static BOOL g_bNearbyWeatherCells;
static float g_fAccumNewPrecip;
static mxs_vector g_MakeNewPos;
static mxs_vector g_LastSphereCheck;
static cDynArray<sPrecipPos> g_Precip;
static cDynArray<sPrecipPos> g_Splash;


/* --<<= -+-/-/-/=/=/\/- <<< ((( ((( / \ ))) ))) >>> -\/\=\=\-\-\-+- =>>-- *\

   The global weather state is loaded and saved through the filevar
   system.

\* --<<= -+-\-\-\=\=\/\- <<< ((( ((( \ / ))) ))) >>> -/\/=/=/-/-/-+- =>>-- */

enum ePrecipType
{
   kWeatherSnow,
   kWeatherRain,

   kWeatherPad = 65536
};


// This describes the overall weather of the mission.  Each cell can
// have each part of this on or off independently.
struct sMissionWeather
{
   ePrecipType m_Precip;
   float m_fPrecipFreq;
   float m_fPrecipFallSpeed;
   float m_fVisibleDist;
   float m_fRenderRadius;
   float m_fAlpha;
   float m_fBrightness;
   float m_fSnowJitter;
   float m_fRainLength;
   float m_fSplashFreq;
   float m_fSplashRadius;
   float m_fSplashHeight;
   float m_fSplashDuration;
   char m_aszTexture[kWeatherStringMax];
   mxs_vector m_Wind;
};


// Here's my descriptor, which identifies my stuff to the tag file & editor
sFileVarDesc g_WeatherFileVarDesc =
{
   kMissionVar,         // Where do I get saved?
   "WEATHERVAR",        // Tag file tag
   "Weather",           // friendly name
   FILEVAR_TYPE(sMissionWeather),// Type (for editing)
   { 1, 0},             // version
   { 1, 0},             // last valid version
};

// the class for actual global variable
class cMissionWeather : public cFileVar<sMissionWeather,&g_WeatherFileVarDesc>
{
public:
   void Update()
   {
      SetDerivedGlobals();
      CheckForWeatherCells();
   }

   void Reset()
   {
      SetDerivedGlobals();
      CheckForWeatherCells();
      cFileVarBase::Reset();
   }
};
// the thing itself
static cMissionWeather g_Weather;


static const char* g_apszPrecipTypeName[] = { "snow", "rain" };

static sFieldDesc g_aWeatherField[] =
{
   { "precip type", kFieldTypeEnum, FieldLocation(sMissionWeather, m_Precip),
     FullFieldNames(g_apszPrecipTypeName) },
   { "# new drops/sec.", kFieldTypeFloat,
     FieldLocation(sMissionWeather, m_fPrecipFreq), },
   { "precip fall (ft./sec.)", kFieldTypeFloat,
     FieldLocation(sMissionWeather, m_fPrecipFallSpeed), },
   { "visible distance (feet)", kFieldTypeFloat,
     FieldLocation(sMissionWeather, m_fVisibleDist), },
   { "render radius (feet)", kFieldTypeFloat,
     FieldLocation(sMissionWeather, m_fRenderRadius), },
   { "rendering alpha (0-1)", kFieldTypeFloat,
     FieldLocation(sMissionWeather, m_fAlpha), },
   { "precip brightness (0-1)", kFieldTypeFloat,
     FieldLocation(sMissionWeather, m_fBrightness), },
   { "snow jitter (feet)", kFieldTypeFloat,
     FieldLocation(sMissionWeather, m_fSnowJitter), },
   { "rain length (feet)", kFieldTypeFloat,
     FieldLocation(sMissionWeather, m_fRainLength), },
   { "splash frequency (0-1)", kFieldTypeFloat,
     FieldLocation(sMissionWeather, m_fSplashFreq), },
   { "splash radius (feet)", kFieldTypeFloat,
     FieldLocation(sMissionWeather, m_fSplashRadius), },
   { "splash height (feet)", kFieldTypeFloat,
     FieldLocation(sMissionWeather, m_fSplashHeight), },
   { "splash duration (sec)", kFieldTypeFloat,
     FieldLocation(sMissionWeather, m_fSplashDuration), },
   { "texture", kFieldTypeString,
     FieldLocation(sMissionWeather, m_aszTexture), },
   { "wind (ft./sec.)", kFieldTypeVector,
     FieldLocation(sMissionWeather, m_Wind), },
};

static sStructDesc g_SDesc = StructDescBuild(sMissionWeather, kStructFlagNone,
                                             g_aWeatherField);


/* --<<= -+-/-/-/=/=/\/- <<< ((( ((( / \ ))) ))) >>> -\/\=\=\-\-\-+- =>>-- *\

   globals derived from static state (above)

\* --<<= -+-\-\-\=\=\/\- <<< ((( ((( \ / ))) ))) >>> -/\/=/=/-/-/-+- =>>-- */

static uchar *g_pWeatherPerCell;
static BOOL g_bAnyWeatherCells;
static mxs_vector g_PrecipFall;
static mxs_vector g_PrecipFallUnit;
static mxs_vector g_PrecipRainTail;
static mxs_vector g_PrecipPerp1;
static mxs_vector g_PrecipPerp2;
static float g_fPrecipFallSpeed;
static int g_iBitmapModelNum = MIDX_INVALID;
static int g_iSplashChance;
static int g_iSplashDuration;
static float g_fSplashRescale;
static float g_fVisibleDistRecip;

// offsets in worldspace of splash corners
static mxs_vector g_aSplashCorner0[3];
static mxs_vector g_aSplashCorner1[3];
static mxs_vector g_aSplashCorner2[3];
static mxs_vector g_aSplashCorner3[3];

static mxs_vector g_aJitter[9];


/* --<<= -+-/-/-/=/=/\/- <<< ((( ((( / \ ))) ))) >>> -\/\=\=\-\-\-+- =>>-- *\

   rendering globals

\* --<<= -+-\-\-\=\=\/\- <<< ((( ((( \ / ))) ))) >>> -/\/=/=/-/-/-+- =>>-- */

static float g_afSnowTopU[4] = {0, .5, 0, .5};
static float g_afSnowTopV[4] = {0, 0, .5, 5};

static r3s_point g_aPt[4];
static r3s_point *g_apPt[4] = {g_aPt, g_aPt+1, g_aPt+2, g_aPt+3};
static mxs_vector g_aV[4];

// snowflake orientation globals

// We update the mxs_angvecs every frame, then when we render make
// matrices from them.  We scale to the right size and treat them as
// sets of perpendicular vectors.
static mxs_angvec g_SnowAngles[3];
static mxs_matrix g_aSnowOrient[3];
static mxs_vector *g_apSnowAng1[9]
= {
   &g_aSnowOrient[0].vec[0],&g_aSnowOrient[0].vec[2],&g_aSnowOrient[0].vec[1],
   &g_aSnowOrient[1].vec[0],&g_aSnowOrient[1].vec[2],&g_aSnowOrient[1].vec[1],
   &g_aSnowOrient[2].vec[0],&g_aSnowOrient[2].vec[2],&g_aSnowOrient[2].vec[1]
};
static mxs_vector *g_apSnowAng2[9]
= {
   &g_aSnowOrient[0].vec[1],&g_aSnowOrient[0].vec[0],&g_aSnowOrient[0].vec[2],
   &g_aSnowOrient[1].vec[1],&g_aSnowOrient[1].vec[0],&g_aSnowOrient[1].vec[2],
   &g_aSnowOrient[2].vec[1],&g_aSnowOrient[2].vec[0],&g_aSnowOrient[2].vec[2],
};
static mxs_vector *g_apSnowPerp[9]
= {
   &g_aSnowOrient[0].vec[2],&g_aSnowOrient[0].vec[1],&g_aSnowOrient[0].vec[0],
   &g_aSnowOrient[1].vec[2],&g_aSnowOrient[1].vec[1],&g_aSnowOrient[1].vec[0],
   &g_aSnowOrient[2].vec[2],&g_aSnowOrient[2].vec[1],&g_aSnowOrient[2].vec[0],
};

static uchar g_Facing = 0;
static uchar g_Flake = 0;

// raindrop rendering--for head-on veiw
static mxs_vector g_RainPerp1;
static mxs_vector g_RainPerp2;

// next in post_rendering callback chain
static void (*g_pfnNextPostRenderCallback)(void) = 0;


/* --<<= -+-/-/-/=/=/\/- <<< ((( ((( / \ ))) ))) >>> -\/\=\=\-\-\-+- =>>-- *\

   load/save globals

\* --<<= -+-\-\-\=\=\/\- <<< ((( ((( \ / ))) ))) >>> -/\/=/=/-/-/-+- =>>-- */

static TagVersion g_WeatherVersion = { 1, 3 };
static TagFileTag g_WeatherTag = { "WEATHER" };


/* --<<= -+-/-/-/=/=/\/- <<< ((( ((( / \ ))) ))) >>> -\/\=\=\-\-\-+- =>>-- *\

   internal helper functions

\* --<<= -+-\-\-\=\=\/\- <<< ((( ((( \ / ))) ))) >>> -/\/=/=/-/-/-+- =>>-- */

// allocate our per-cell array of flags
static inline void SetNumCells()
{
   // (I don't know if we're still having trouble with realloc.)
   if (g_pWeatherPerCell)
      free(g_pWeatherPerCell);
   if (wr_num_cells) {
      g_pWeatherPerCell = (uchar *) malloc(wr_num_cells);
      memset(g_pWeatherPerCell, 0, wr_num_cells);
   } else
      g_pWeatherPerCell = 0;
}


// Are there any weather to speak of?
static void CheckForWeatherCells()
{
   g_bAnyWeatherCells = FALSE;
   for (int i = 0; i < wr_num_cells; ++i)
      if (g_pWeatherPerCell[i]) {
         g_bAnyWeatherCells = TRUE;
         break;
      }
}


// This should be called any time g_Weather changes.
static void SetDerivedGlobals()
{
   mx_mk_vec(&g_LastSphereCheck, 0, 0, -100000000);
   mx_mk_vec(&g_MakeNewPos, 0, 0, -100000000);
   g_Precip.SetSize(0);
   g_Splash.SetSize(0);

   mx_mk_vec(&g_PrecipFall, g_Weather.m_Wind.x, g_Weather.m_Wind.y,
             g_Weather.m_Wind.z - g_Weather.m_fPrecipFallSpeed);
   g_fPrecipFallSpeed = mx_norm_vec(&g_PrecipFallUnit, &g_PrecipFall) * .001;

   // We need two unit vectors, perpendicular to the direction in
   // which the precipitation's falling and to each other.
   mxs_vector YAxis;
   mx_mk_vec(&YAxis, 0, 1, 0);
   mx_cross_vec(&g_PrecipPerp1, &YAxis, &g_PrecipFallUnit);
   mx_cross_vec(&g_PrecipPerp2, &g_PrecipPerp1, &g_PrecipFallUnit);

   g_fAccumNewPrecip = 0;

   if (g_Weather.m_aszTexture[0]) {
      g_iBitmapModelNum = objmodelLoadModel(g_Weather.m_aszTexture);

#ifndef SHIP
      if (g_iBitmapModelNum == MIDX_INVALID)
         mprintf("weather: Can't find texture %s for precipitation.\n",
                  g_Weather.m_aszTexture);
#endif // ~SHIP
   }

   if (g_Weather.m_Precip == kWeatherRain) {
      // things we only need for rain
      mx_scale_vec(&g_PrecipRainTail, &g_PrecipFallUnit,
                   g_Weather.m_fRainLength);
      g_iSplashChance = g_Weather.m_fSplashFreq * 32767.0;
      g_iSplashDuration = g_Weather.m_fSplashDuration * 1000.0;
      g_fSplashRescale = MX_REAL_PI / (g_Weather.m_fSplashDuration * 1000.0);
      g_fVisibleDistRecip = 1.0 / g_Weather.m_fVisibleDist;

      // make vectors for corners of splashes
      mx_mk_vec(&g_aSplashCorner0[0], -g_Weather.m_fSplashRadius,
                0, g_Weather.m_fSplashHeight);
      mx_mk_vec(&g_aSplashCorner0[1], g_Weather.m_fSplashRadius * -.5,
                g_Weather.m_fSplashRadius * .866, g_Weather.m_fSplashHeight);
      mx_mk_vec(&g_aSplashCorner0[2], g_Weather.m_fSplashRadius * .5,
                g_Weather.m_fSplashRadius * .866, g_Weather.m_fSplashHeight);

      mx_mk_vec(&g_aSplashCorner1[0], g_Weather.m_fSplashRadius,
                0, g_Weather.m_fSplashHeight);
      mx_mk_vec(&g_aSplashCorner1[1], g_Weather.m_fSplashRadius * .5,
                g_Weather.m_fSplashRadius * -.866, g_Weather.m_fSplashHeight);
      mx_mk_vec(&g_aSplashCorner1[2], g_Weather.m_fSplashRadius * -.5,
                g_Weather.m_fSplashRadius * -.866, g_Weather.m_fSplashHeight);

      mx_mk_vec(&g_aSplashCorner2[0], g_Weather.m_fSplashRadius,
                0, 0);
      mx_mk_vec(&g_aSplashCorner2[1], g_Weather.m_fSplashRadius * .5,
                g_Weather.m_fSplashRadius * -.866, 0);
      mx_mk_vec(&g_aSplashCorner2[2], g_Weather.m_fSplashRadius * -.5,
                g_Weather.m_fSplashRadius * -.866, 0);

      mx_mk_vec(&g_aSplashCorner3[0], -g_Weather.m_fSplashRadius,
                0, 0);
      mx_mk_vec(&g_aSplashCorner3[1], g_Weather.m_fSplashRadius * -.5,
                g_Weather.m_fSplashRadius * .866, 0);
      mx_mk_vec(&g_aSplashCorner3[2], g_Weather.m_fSplashRadius * .5,
                g_Weather.m_fSplashRadius * .866, 0);
   }

   for (int i = 0; i < 4; ++i) {
      g2s_point *p = (g2s_point *) (g_aPt + i);
      p->i = p->h = p->d = g_Weather.m_fBrightness;
   }
}


static inline BOOL TouchedTheSky()
{
   int iPoly = PortalRaycastFindPolygon();
   int iTex = (WR_CELL(PortalRaycastCell)->render_list + iPoly)->texture_id;

   return iTex >= BACKHACK_IDX;
}


// add any new little raindrops/snowflakes
static void MakeNewPrecip(mxs_vector *pCamPos, ulong nMillisec)
{
   // This is how many objects we'll try to make.  We don't really
   // make those which are blocked by terrain.
   g_fAccumNewPrecip += nMillisec * g_Weather.m_fPrecipFreq * .001;
   if (g_fAccumNewPrecip < 1.0)
      return;

   int iNumNewPrecip = g_fAccumNewPrecip;

   ConfigSpew("WeatherMakeSpew", ("weather: trying to make %d.\n",
                                  iNumNewPrecip));

   g_fAccumNewPrecip -= iNumNewPrecip;
   if (iNumNewPrecip > kMaxPrecipPerFrame)
      iNumNewPrecip = kMaxPrecipPerFrame;

   // Here's a vector at the camera position, but up.
   mxs_vector Above;
   mx_scale_add_vec(&Above, pCamPos, &g_PrecipFallUnit,
                    -g_Weather.m_fVisibleDist);

   while (iNumNewPrecip) {
      mxs_vector StartPos, EndPos;

      mx_scale_add_vec(&StartPos, &Above, &g_PrecipPerp1,
                       (RandFloat() - .5) * 2.0 * g_Weather.m_fVisibleDist);
      mx_scale_addeq_vec(&StartPos, &g_PrecipPerp2,
                       (RandFloat() - .5) * 2.0 * g_Weather.m_fVisibleDist);

      // Now then.  Is this position within the world; and in a cell
      // with weather; and not obscured by terrain if we raycast up
      // the wind direction some significant distance?
      Location StartLoc, EndLoc, HitLoc;
      MakeLocationFromVector(&StartLoc, &StartPos);

      int iStartCell = ComputeCellForLocation(&StartLoc);
      if (iStartCell != CELL_INVALID
       && (g_pWeatherPerCell[iStartCell] & kCellHasPrecip)) {
         // make location we're casting to
         mx_scale_add_vec(&EndPos, &StartPos, &g_PrecipFallUnit,
                          -g_Weather.m_fVisibleDist);
         MakeLocationFromVector(&EndLoc, &EndPos);

         if (PortalRaycast(&StartLoc, &EndLoc, &HitLoc, FALSE)
          || TouchedTheSky()) {
            // So far, so good.  We have a starting point in a cell
            // which has precipitation, and it's not blocked by
            // terrain.  We determine how long it will last using a
            // second raycast going the other direction.
            mx_scale_add_vec(&EndPos, &StartPos, &g_PrecipFallUnit,
                             3.0 * g_Weather.m_fVisibleDist);
            MakeLocationFromVector(&EndLoc, &EndPos);

            // Finally--we get to make something!
            g_Precip.Grow(1);
            sPrecipPos *pNew = g_Precip.AsPointer() + (g_Precip.Size() - 1);

            float fFallDist;
            // Will this one hit any terrain, or just disappear into
            // the void?
            if (!PortalRaycast(&StartLoc, &EndLoc, &HitLoc, FALSE)) {
               pNew->m_bImpact = TRUE;
               pNew->m_Impact = HitLoc.vec;
               fFallDist = mx_dist_vec(&StartPos, &HitLoc.vec);

               pNew->m_bSplash = FALSE;
               if (g_Weather.m_Precip == kWeatherRain) {
                  PortalPlane *pPlane = WR_CELL(PortalRaycastCell)
                     ->plane_list + PortalRaycastPlane;
                  if (pPlane->normal.z > kSplashMinPlaneZ
                   && rand() <= g_iSplashChance)
                     pNew->m_bSplash = TRUE;
               }
            } else {
               fFallDist = mx_dist_vec(&StartPos, &EndPos);
               pNew->m_bImpact = FALSE;
            }

            pNew->m_Pos = StartPos;
            pNew->m_iMillisecsLeft = fFallDist / g_fPrecipFallSpeed;
            if (g_Weather.m_Precip == kWeatherSnow) {
               pNew->m_Facing = g_Facing;
               g_Facing += 1;
               if (g_Facing == 9)
                  g_Facing = 0;

               pNew->m_Flake = g_Flake;
               g_Flake = (g_Flake + 1) & 3;
            }
         }
      }

      --iNumNewPrecip;
   }
}


static void MovePrecip(ulong nMillisec)
{
   mxs_vector MoveVec;
   mx_scale_vec(&MoveVec, &g_PrecipFall, ((float) nMillisec) * .001);

   int i, j, iNum = g_Precip.Size();

   for (i = 0; i < iNum; ++i) {
      sPrecipPos *pPos = g_Precip.AsPointer() + i;
      pPos->m_iMillisecsLeft -= nMillisec;
      if (pPos->m_iMillisecsLeft < 0) {
         // Do we make a splash?
         if (g_Weather.m_Precip == kWeatherRain && pPos->m_bSplash) {
            pPos->m_iMillisecsLeft = g_iSplashDuration;
            g_Splash.Append(*pPos);
         }

         g_Precip.FastDeleteItem(i);
         --iNum;
         --i;
      } else
         mx_addeq_vec(&pPos->m_Pos, &MoveVec);
   }

   if (g_Weather.m_Precip == kWeatherSnow) {
      // update the snow angles
      mxs_ang MoveAng = 0;
      mxs_ang MoveAngDelta = kSnowTurnSpeed * nMillisec;
      for (i = 0; i < 3; ++i) {
         for (j = 0; j < 3; ++j) {
            MoveAng += MoveAngDelta;
            g_SnowAngles[i].el[j] += MoveAng;
         }
      }
   } else {
      // update any rain splashes
      iNum = g_Splash.Size();
      for (i = 0; i < iNum; ++i) {
         sPrecipPos *pSplash = g_Splash.AsPointer() + i;
         if (pSplash->m_iMillisecsLeft < 0) {
            g_Splash.FastDeleteItem(i);
            --iNum;
            --i;
         } else
            pSplash->m_iMillisecsLeft -= nMillisec;
      }
   }
}


// fake enough .05 sec. frames to fill the air with precipitation
static void ForcePrecip(mxs_vector *pCamPos)
{
   float fSecs = g_Weather.m_fVisibleDist * 2.0 / g_Weather.m_fPrecipFallSpeed;
   int iFrames = fSecs / .05;

   for (int i = 0; i < iFrames; ++i) {
      MakeNewPrecip(pCamPos, 50);
      MovePrecip(50);
   }
}


/* --<<= -+-/-/-/=/=/\/- <<< ((( ((( / \ ))) ))) >>> -\/\=\=\-\-\-+- =>>-- *\

   callbacks to hook us into other systems (RoomToWR and Portal)

\* --<<= -+-\-\-\=\=\/\- <<< ((( ((( \ / ))) ))) >>> -/\/=/=/-/-/-+- =>>-- */

#ifdef EDITOR

// called for every room-cell pair
static void WeatherRoomToWRCallback(int iCell, cRoom *pRoom, void *pData)
{
   PortalCell *pCell = WR_CELL(iCell);
   ConfigSpew("WeatherRoomSpew", ("Cell %d x room %d w/objid %d.\n",
                                  iCell, pRoom->GetRoomID(), pRoom->GetObjID()));

   sWeather *pWeather;
   if (g_pWeatherProperty->Get(pRoom->GetObjID(), &pWeather)) {
      if (pWeather->m_bDebris)
         g_pWeatherPerCell[iCell] |= kCellHasDebris;
      if (pWeather->m_bFog)
         pCell->flags |= CELL_FOG;
      if (pWeather->m_bPrecipitation)
         g_pWeatherPerCell[iCell] |= kCellHasPrecip;
      if (pWeather->m_bWind)
         g_pWeatherPerCell[iCell] |= kCellHasWind;
   }
}


// called right before we apply room properties to WR
static void WeatherRoomToWRStartCallback()
{
   for (int i = 0; i < wr_num_cells; ++i)
      WR_CELL(i)->flags &= ~CELL_FOG;

   // sanity checking on property
   sPropertyObjIter Iter;
   ObjID Obj;
   sWeather *pWeather;

   g_pWeatherProperty->IterStart(&Iter);
   while (g_pWeatherProperty->IterNext(&Iter, &Obj)) {
      g_pWeatherProperty->Get(Obj, &pWeather);
      if (pWeather->m_bDebris && !(pWeather->m_bWind))
         Warning(("weather: Room obj %d has debris but no wind--"
                  "debris won't move.\n"));
   }
   g_pWeatherProperty->IterStop(&Iter);      // clean up iteration

   // set up flags for per-cell weather
   SetNumCells();
}


// called right after we apply room properties to WR
static void WeatherRoomToWREndCallback()
{
   // Any portal between fogged and unfogged cells should split
   // objects, for the same reason medium boundaries do.  We have two
   // cases: the cell we're in is fogged, or it isn't.
   for (int i = 0; i < wr_num_cells; ++i) {
      PortalCell *pCell = WR_CELL(i);
      if (pCell->flags & CELL_FOG) {
         for (int j = 0; j < pCell->num_portal_polys; ++j) {
            PortalPolygonCore *pPortal = pCell->portal_poly_list + j;
            PortalCell *pDestCell = WR_CELL(pPortal->destination);
            if ((pDestCell->flags & CELL_FOG) == 0)
               pPortal->flags |= PORTAL_SPLITS_OBJECT;
         }
      } else {
         for (int j = 0; j < pCell->num_portal_polys; ++j) {
            PortalPolygonCore *pPortal = pCell->portal_poly_list + j;
            PortalCell *pDestCell = WR_CELL(pPortal->destination);
            if (pDestCell->flags & CELL_FOG)
               pPortal->flags |= PORTAL_SPLITS_OBJECT;
         }
      }
   }

   SetDerivedGlobals();
   CheckForWeatherCells();
}

#endif // EDITOR


BOOL g_bRenderWeather = FALSE;

// called from Portal after entire scene is rendered (actually chained
// from rendobj after objects are also rendered)
void WeatherRender()
{
   int i, j, iNum, iNumSplash;
   Camera* pCam;
   mxs_vector CamPos;
   mxs_angvec CamAng;
   mxs_matrix CamMat;
   sPrecipPos *pPos;

#ifdef EDITOR
   if (!g_lgd3d)
      goto next_callback;
#endif // EDITOR

   iNum = g_Precip.Size();
   iNumSplash = g_Splash.Size();
   if (!(iNum | iNumSplash))
      goto next_callback;

   r3_start_block();
   r3_set_polygon_context(R3_PL_POLYGON | R3_PL_TEXTURE);

   lgd3d_set_zcompare(TRUE);
   lgd3d_set_zwrite(FALSE);
   lgd3d_set_alpha(g_Weather.m_fAlpha);
   lgd3d_set_blend(TRUE);

   pCam = PlayerCamera();
   CameraGetLocation(pCam, &CamPos, &CamAng);
   mx_ang2mat(&CamMat, &CamAng);

   pPos = g_Precip.AsPointer();

   if (g_iBitmapModelNum != MIDX_INVALID)
      lgd3d_set_texture(objmodelSetupBitmapTexture(g_iBitmapModelNum, 0));

   r3d_do_setup = TRUE;

   switch (g_Weather.m_Precip)
   {
      case kWeatherSnow:
      {
         for (i = 0; i < 3; ++i) {
            mx_ang2mat(&g_aSnowOrient[i], &g_SnowAngles[i]);
            mx_scaleeq_mat(&g_aSnowOrient[i], g_Weather.m_fRenderRadius);
         }

         for (i = 0; i < 9; ++i) {
            mx_scale_vec(&g_aJitter[i], g_apSnowAng1[i],
                         g_Weather.m_fSnowJitter);
            mx_scale_addeq_vec(&g_aJitter[i], g_apSnowAng2[i],
                             g_Weather.m_fSnowJitter);
         }

         for (i = 0; i < iNum; ++pPos, ++i) {
            int iFacing = pPos->m_Facing;
            mxs_vector *pV1 = g_apSnowAng1[iFacing];
            mxs_vector *pV2 = g_apSnowAng2[iFacing];

            mxs_vector Pos;
            mx_add_vec(&Pos, &pPos->m_Pos, &g_aJitter[iFacing]);

            // set up the corners of our bitmap in world space
            mx_scale_add_vec(&g_aV[0], &Pos, pV1, -1);
            mx_scale_addeq_vec(&g_aV[0], pV2, -1);

            mx_scale_add_vec(&g_aV[1], &Pos, pV1, -1);
            mx_addeq_vec(&g_aV[1], pV2);

            mx_add_vec(&g_aV[2], &Pos, pV1);
            mx_addeq_vec(&g_aV[2], pV2);

            mx_add_vec(&g_aV[3], &Pos, pV1);
            mx_scale_addeq_vec(&g_aV[3], pV2, -1);

            r3_transform_block(4, g_aPt, g_aV);
            if (g_aPt[0].ccodes | g_aPt[1].ccodes
              | g_aPt[2].ccodes | g_aPt[3].ccodes)
               continue;

            float fBaseU = g_afSnowTopU[pPos->m_Flake];
            g_aPt[0].grp.u = g_aPt[3].grp.u = fBaseU;
            g_aPt[1].grp.u = g_aPt[2].grp.u = fBaseU + .5;

            float fBaseV = g_afSnowTopV[pPos->m_Flake];
            g_aPt[0].grp.v = g_aPt[1].grp.v = fBaseV;
            g_aPt[2].grp.v = g_aPt[3].grp.v = fBaseV + .5;

            // backface check
            float fNegPlaneConst = mx_dot_vec(&pPos->m_Pos,
                                              g_apSnowPerp[iFacing]);
            if (mx_dot_vec(g_apSnowPerp[iFacing], &CamPos) > fNegPlaneConst) {
               g_apPt[0] = g_aPt;
               g_apPt[1] = g_aPt + 1;
               g_apPt[2] = g_aPt + 2;
               g_apPt[3] = g_aPt + 3;
            } else {
               g_apPt[0] = g_aPt + 3;
               g_apPt[1] = g_aPt + 2;
               g_apPt[2] = g_aPt + 1;
               g_apPt[3] = g_aPt;
            }
            lgd3d_lit_trifan(4, g_apPt);
         }
      }
      break;

      case kWeatherRain:
      {
         // raindrops

         mx_scale_vec(&g_RainPerp1, &CamMat.vec[1], g_Weather.m_fRenderRadius);
         mx_scale_vec(&g_RainPerp2, &CamMat.vec[2], g_Weather.m_fRenderRadius);

         for (i = 0; i < iNum; ++pPos, ++i) {
            mxs_vector ToCam;
            mx_sub_vec(&ToCam, &pPos->m_Pos, &CamPos);
            mx_normeq_vec(&ToCam);

            // We can draw the drop either head-on or from the side.
            float fCamDot = mx_dot_vec(&g_PrecipFallUnit, &ToCam);
            if (fabs(fCamDot) < kRainFromSide) {
               // side case

               mxs_vector ToCamPerp, Tail;

               // tail of raindrop in world space
               mx_sub_vec(&Tail, &pPos->m_Pos, &g_PrecipRainTail);

               // vector perp to rain
               mx_cross_vec(&ToCamPerp, &ToCam, &g_PrecipFallUnit);
               mx_scaleeq_vec(&ToCamPerp, g_Weather.m_fRenderRadius);

               // the corners of our raindrop, in worldspace
               mx_add_vec(&g_aV[0], &pPos->m_Pos, &ToCamPerp);
               mx_add_vec(&g_aV[1], &Tail, &ToCamPerp);
               mx_sub_vec(&g_aV[2], &pPos->m_Pos, &ToCamPerp);
               mx_sub_vec(&g_aV[3], &Tail, &ToCamPerp);

               g_aPt[0].grp.u = g_aPt[3].grp.u = 0;
               g_aPt[1].grp.u = g_aPt[2].grp.u = .5;
               g_aPt[0].grp.v = g_aPt[1].grp.v = 0;
               g_aPt[2].grp.v = g_aPt[3].grp.v = 1.0;

               r3_transform_block(4, g_aPt, g_aV);
               if (g_aPt[0].ccodes | g_aPt[1].ccodes
                 | g_aPt[2].ccodes | g_aPt[3].ccodes)
                  continue;

               g_apPt[0] = g_aPt;
               g_apPt[1] = g_aPt + 1;
               g_apPt[2] = g_aPt + 2;
               g_apPt[3] = g_aPt + 3;

               lgd3d_lit_trifan(4, g_apPt);
            } else {
               // head-on case
               mx_sub_vec(&g_aV[0], &pPos->m_Pos, &g_RainPerp1);
               mx_subeq_vec(&g_aV[0], &g_RainPerp2);

               mx_sub_vec(&g_aV[1], &pPos->m_Pos, &g_RainPerp1);
               mx_addeq_vec(&g_aV[1], &g_RainPerp2);

               mx_add_vec(&g_aV[2], &pPos->m_Pos, &g_RainPerp1);
               mx_addeq_vec(&g_aV[2], &g_RainPerp2);

               mx_add_vec(&g_aV[3], &pPos->m_Pos, &g_RainPerp1);
               mx_subeq_vec(&g_aV[3], &g_RainPerp2);

               r3_transform_block(4, g_aPt, g_aV);
               if (g_aPt[0].ccodes | g_aPt[1].ccodes
                 | g_aPt[2].ccodes | g_aPt[3].ccodes)
                  continue;

               if (fCamDot < 0) {
                  g_aPt[0].grp.u = g_aPt[3].grp.u = .5;
                  g_aPt[1].grp.u = g_aPt[2].grp.u = 1.0;
                  g_aPt[0].grp.v = g_aPt[1].grp.v = .5;
                  g_aPt[2].grp.v = g_aPt[3].grp.v = 1.0;
               } else {
                  g_apPt[0] = g_aPt + 3;
                  g_apPt[1] = g_aPt + 2;
                  g_apPt[2] = g_aPt + 1;
                  g_apPt[3] = g_aPt;
               }

               lgd3d_lit_trifan(4, g_apPt);
            }
         }

         // splashes
         if (g_Splash.Size()) {
            g_aPt[0].grp.u=g_aPt[2].grp.v=g_aPt[3].grp.u=g_aPt[3].grp.v=.5;
            g_aPt[0].grp.v = g_aPt[1].grp.v = 0;
            g_aPt[1].grp.u = g_aPt[2].grp.u = 1.0;

            pPos = g_Splash.AsPointer();
            for (i = 0; i < iNumSplash; ++pPos, ++i) {
               float fS = sin(pPos->m_iMillisecsLeft * g_fSplashRescale);

               // Each splash uses the same texture three times.
               for (j = 0; j < 3; ++j) {
                  mx_scale_add_vec(&g_aV[0], &pPos->m_Impact,
                                   &g_aSplashCorner0[j], fS);
                  mx_scale_add_vec(&g_aV[1], &pPos->m_Impact,
                                   &g_aSplashCorner1[j], fS);
                  mx_scale_add_vec(&g_aV[2], &pPos->m_Impact,
                                   &g_aSplashCorner2[j], fS);
                  mx_scale_add_vec(&g_aV[3], &pPos->m_Impact,
                                   &g_aSplashCorner3[j], fS);

                  // @HACK: Should any of the three polygons fail to be
                  // on the screen, we stop drawing the splash.  Usually
                  // this will do the right thing: a splash which can't
                  // be seen gets culled.  Now and then we may notice a
                  // missing polygon.
                  r3_transform_block(4, g_aPt, g_aV);
                  if (g_aPt[0].ccodes | g_aPt[1].ccodes
                    | g_aPt[2].ccodes | g_aPt[3].ccodes)
                     break;

                  float fDist = mx_dist_vec(&CamPos, &pPos->m_Impact);
                  float fAlpha = (1.0 - fDist * g_fVisibleDistRecip)
                                * g_Weather.m_fAlpha;
                  if (fAlpha < kSplashMinAlpha)
                     fAlpha = kSplashMinAlpha;
                  lgd3d_set_alpha(fAlpha);

                  // to get around backface culling...
                  g_apPt[0] = g_aPt;
                  g_apPt[1] = g_aPt + 1;
                  g_apPt[2] = g_aPt + 2;
                  g_apPt[3] = g_aPt + 3;
                  lgd3d_lit_trifan(4, g_apPt);

                  g_apPt[0] = g_aPt + 3;
                  g_apPt[1] = g_aPt + 2;
                  g_apPt[2] = g_aPt + 1;
                  g_apPt[3] = g_aPt;
                  lgd3d_lit_trifan(4, g_apPt);
               }
            }
         }
      }
      break;
   }

   lgd3d_set_blend(FALSE);
   lgd3d_set_alpha(1.0);
   if (g_iBitmapModelNum != MIDX_INVALID)
      objmodelReleaseBitmapTexture(g_iBitmapModelNum, 0);

   r3_end_block();

next_callback:
   if (g_pfnNextPostRenderCallback)
      g_pfnNextPostRenderCallback();
}


/* --<<= -+-/-/-/=/=/\/- <<< ((( ((( / \ ))) ))) >>> -\/\=\=\-\-\-+- =>>-- *\

   exposed functions

\* --<<= -+-\-\-\=\=\/\- <<< ((( ((( \ / ))) ))) >>> -/\/=/=/-/-/-+- =>>-- */

EXTERN void WeatherInit()
{
#ifdef EDITOR
   RoomToWRAddCallback(WeatherRoomToWRCallback, 0);
   RoomToWRAddStartCallback(WeatherRoomToWRStartCallback);
   RoomToWRAddEndCallback(WeatherRoomToWREndCallback);

   AutoAppIPtr_(StructDescTools, pTools);
   pTools->Register(&g_SDesc);
#endif // EDITOR

   g_pfnNextPostRenderCallback = portal_post_render_cback;
   portal_post_render_cback = WeatherRender;
   g_pWeatherProperty = new cWeatherProperty(&g_WeatherDesc);

   int cfgval;
   config_get_int("render_weather",&cfgval);
   g_bRenderWeather = (cfgval==1);
}

EXTERN void WeatherTerm()
{
   WeatherReset();
}


EXTERN void WeatherReset()
{
   // force cell gathering & such on first frame after reset
   mx_mk_vec(&g_LastSphereCheck, 0, 0, -100000000);
   mx_mk_vec(&g_MakeNewPos, 0, 0, -100000000);

   // clean out any unfreed memory
   free(g_pWeatherPerCell);
   g_pWeatherPerCell = 0;

   g_bAnyWeatherCells = FALSE;
   g_bNearbyWeatherCells = FALSE;
}

EXTERN BOOL WeatherIsAllowed()
{
   return(g_bRenderWeather);
}

EXTERN void WeatherSetAllowed(BOOL allow)
{
   if (allow)
     {
       g_bRenderWeather=TRUE;
       config_set_int("render_weather",1);
     }
   else
     {
       g_bRenderWeather=FALSE;
       config_set_int("render_weather",0);
     }
}

EXTERN void WeatherSave(ITagFile *pFile)
{
   if (!g_pWeatherPerCell)
      return;

   HRESULT result=ITagFile_OpenBlock(pFile, &g_WeatherTag, &g_WeatherVersion);

   if (result == S_OK) {
      pFile->Write((char *) g_pWeatherPerCell, wr_num_cells);
      ITagFile_CloseBlock(pFile);
   }
}


EXTERN void WeatherLoad(ITagFile *pFile)
{
   TagVersion FoundVersion = g_WeatherVersion;
   HRESULT result=ITagFile_OpenBlock(pFile, &g_WeatherTag, &FoundVersion);

   WeatherReset();
   SetNumCells();

   if (result == S_OK
    && FoundVersion.major == g_WeatherVersion.major
    && FoundVersion.minor == g_WeatherVersion.minor) {
      pFile->Read((char *) g_pWeatherPerCell, wr_num_cells);
      ITagFile_CloseBlock(pFile);

      SetDerivedGlobals();
      CheckForWeatherCells();
   } else {
      g_bAnyWeatherCells = FALSE;
      if (g_pWeatherPerCell)
         memset(g_pWeatherPerCell, 0, wr_num_cells);
   }
}


// Here's where we update the states of all the effects.
EXTERN void WeatherFrame(ulong nMillisec)
{
   // Does this mission use any of this stuff?
   if (!g_bAnyWeatherCells || !g_bRenderWeather)
      return;

   // If the camera has moved enough we double-check that there's
   // still weather around by snagging a list of cells and seeing if
   // they have weather.
   Camera* pCam = PlayerCamera();
   mxs_vector CamPos;
   mxs_angvec CamAng;
   CameraGetLocation(pCam, &CamPos, &CamAng);

   if (mx_dist2_vec(&g_LastSphereCheck, &CamPos) > kSphereCheckDist2) {
      // remember where we snagged from
      mx_copy_vec(&g_LastSphereCheck, &CamPos);

      // find all cells within our radius of the camera
      Location CamLoc;
      MakeLocationFromVector(&CamLoc, &CamPos);

      int aCellList[BSPSPHR_OUTPUT_LIMIT];
      int iNumCells
         = (portal_cells_intersecting_sphere(&CamLoc, g_Weather.m_fVisibleDist,
                                             aCellList));
      iNumCells = portal_contiguous_cells(aCellList, iNumCells, CamLoc.cell);

      // Any nearby cells with weather?
      g_bNearbyWeatherCells = FALSE;
      for (int i = 0; i < iNumCells; ++i) {
         if (g_pWeatherPerCell[aCellList[i]]) {
            g_bNearbyWeatherCells = TRUE;
            break;
         }
      }

      ConfigSpew("WeatherFrameSpew",("weather: got %d cells; %s weather.\n",
                                     iNumCells, g_bNearbyWeatherCells?
                                     "Have" : "Do not have"));
   }

   if (g_bNearbyWeatherCells) {
      mxs_vector MoveUnitZappa;
      mx_sub_vec(&MoveUnitZappa, &CamPos, &g_MakeNewPos);

      BOOL bMoving = MoveUnitZappa.x < -kMoveEpsilon
                  || MoveUnitZappa.x > kMoveEpsilon
                  || MoveUnitZappa.y < -kMoveEpsilon
                  || MoveUnitZappa.y > kMoveEpsilon
                  || MoveUnitZappa.z < -kMoveEpsilon
                  || MoveUnitZappa.z > kMoveEpsilon;

      float fCamSpeed;

      if (bMoving) {
         float fMoveDist = mx_normeq_vec(&MoveUnitZappa);
         fCamSpeed = fMoveDist / (float) nMillisec;
      }

      if (fCamSpeed >= kSphereTeleportSpeed) {
         ForcePrecip(&CamPos);
      } else if (bMoving) {
         // anticipate where the camera's heading & make rain there
         mxs_vector ToPos;
         mx_scale_add_vec(&ToPos, &CamPos, &MoveUnitZappa,
            fCamSpeed / g_fPrecipFallSpeed * g_Weather.m_fVisibleDist);
         MakeNewPrecip(&ToPos, nMillisec);
      } else {
         MakeNewPrecip(&CamPos, nMillisec);
      }
   }

   g_MakeNewPos = CamPos;

   // move our little raindrops/snowflakes
   MovePrecip(nMillisec);
}
