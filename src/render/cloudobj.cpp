/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/cloudobj.cpp,v 1.9 2000/03/06 13:18:48 bfarquha Exp $


// Sky rendering

#include <lg.h>
#include <lgd3d.h>
#include <r3d.h>
#include <filevar.h>
#include <sdesbase.h>
#include <sdesc.h>
#include <resapi.h>
#include <imgrstyp.h>
#include <palrstyp.h>
#include <palmgr.h>
#include <simtime.h>
#include <command.h>

#include <skyrend.h>

#include <math.h>
#include <stdlib.h>

#include <cloudobj.h>

#include <mprintf.h>
#include <memall.h>
#include <dbmem.h>   // must be last header!

/////////////////////////////////////////////////////////////////////////
//
// Rendering implementations:
//

extern "C" BOOL g_lgd3d;
extern "C" BOOL portal_fog_on;


// We make this 256 just in case texture is wrong. Really, we only use the first 16
static ushort AlphaTable[256] =
{
   0x0fff, 0x1fff, 0x2fff, 0x3fff,
   0x4fff, 0x5fff, 0x6fff, 0x7fff,
   0x8fff, 0x9fff, 0xafff, 0xbfff,
   0xcfff, 0xdfff, 0xefff, 0xffff
};


struct goddamn_stupid_r3s_point
{
   mxs_vector p;
   ulong ccodes;
   g2s_point grp;
};

static goddamn_stupid_r3s_point *pPoints = 0;
static mxs_vector *pPointVecs = 0;
static mxs_vector *pPointColors = 0;
static r3s_phandle *vlist = 0; // Point transforming/rendering
static float *pAlphas = 0;
static float *pUs = 0;
static float *pVs = 0;
static BOOL *pGlowSubTiles = 0;


static goddamn_stupid_r3s_point *pGlowPoints = 0;
static mxs_vector *pGlowPointVecs = 0;
static mxs_vector *pGlowPointColors = 0;
static r3s_phandle *glow_vlist = 0; // Point transforming/rendering
static float *pGlowAlphas = 0;
static float *pGlowUs = 0;
static float *pGlowVs = 0;

static IRes *pRes = 0;

static int nTotalPoints;
static int nPointsPerSide;

static int nTotalGlowPoints;
static int nGlowPointUs = 0;
static int nGlowPointVs = 0;

static int nMinGlowX, nMaxGlowX;
static int nMinGlowY, nMaxGlowY;


////////////////////////////////////////
// T-Joint info


struct sTJointPolyPoint
{
   int x; // relative to upper left corner of tile, in its vlist.
   int y;
   int nXSize;
   r3s_phandle *vlist;
   float *pUs;
   float *pVs;
   float *pAlphas;
   mxs_vector *pPointColors;
   BOOL bCheckUWrap;
   BOOL bCheckVWrap;
   BOOL bIsGlow;
};

struct sTJointPoly
{
   sTJointPolyPoint Points[3];
};


// There are 8 T-Joint configs possible - one for each side of the tile, and for each
// side paired with it's adjacent. Since the glow is circular, these are the only ones possible.
//
// As viewed from above:
// 0 = Top edge has points
// 1 = right edge
// 2 = bottom edge
// 3 = left edge
// 4 = Top and right
// 5 = right and bottom
// 6 = bottom and left
// 7 = left and top

static sTJointPoly *(TJointConfigs[8]);
static int NumTJointPolys[8];

static int MaskToTJoint[16]=
{
   -1,
   0,    // 1: top only
   1,    // 2: right only
   4,    // 3: top and right
   2,    // 4: bottom only
   -1,   // 5: bottom and top - should never happen
   5,    // 6: bottom and right
   -1,   // 7: top, right, bottom - should never happen
   3,    // 8: left only
   7,    // 9: left and top
   -1,   // 10: left and right - should never happen
   -1,   // 11: left and right and top - should never happen
   6,    // 12: left and bottom
   -1,   // 13: left and bottom and top - should never happen
   -1,   // 14: left and bottom and right - should never happen
   -1,   // 15: all - should never happen
};


////////////////////////////////////////





#define PI 3.14159265359
#define fDeg90 (0.50000*PI)
#define fDeg180 (PI)
#define fDeg360 (2*PI)

#define DEG (PI/180)


////////////////////////////////////////////////////////
// Mission Loading and Saving


enum eColorMethod
{
   kMethod_Sum,
   kMethod_Interpolate
};

// This describes the sky rendering of the mission.
struct sMissionCloudObj
{
   BOOL bUseCloudDeck;
   BOOL bEnableFog;
   BOOL bIsAlphaTexture;
   char TextureName[256];
   float fAlpha;
   float fHeight;
   float fTileSize;
   int nNumTilesPerSide;
   int nNumSubTiles;
   mxs_vector WindVelocity;
   mxs_vector OverallColor;
   mxs_vector EastColor;
   eColorMethod EastColorMethod;
   float fEastScale;  // from 0 to 1 - also can be interpreted as angle after which glow tapers off.
   mxs_vector WestColor;
   eColorMethod WestColorMethod;
   float fWestScale;  // from 0 to 1 - also can be interpreted as angle after which glow tapers off.
   float fZAng;
   int nSubTileAlphaStart;
   mxs_vector GlowColor;
   eColorMethod GlowColorMethod;
   float fGlowScale;  // from 0 to 1 - also can be interpreted as angle after which glow tapers off.
   float fGlowLat;  // in degrees
   float fGlowLong; // in degrees
   float fGlowAng;  // in degrees - angle around vec
   int nGlowTiles;
};



// Here's my descriptor, which identifies my stuff to the tag file & editor
sFileVarDesc g_CloudObjFileVarDesc =
{
   kMissionVar,         // Where do I get saved?
   "CLOUDOBJVAR",        // Tag file tag
   "NewSky:CloudDeck",           // friendly name
   FILEVAR_TYPE(sMissionCloudObj),// Type (for editing)
   { 1, 1},             // version
   { 1, 0},             // last valid version
};


// the class for actual global variable
class cMissionCloudObj : public cFileVar<sMissionCloudObj,&g_CloudObjFileVarDesc>
{};

// the thing itself
static cMissionCloudObj g_CloudObj;

static const char* g_apszColorMethodName[] = { "Sum", "Interpolate" };

static sFieldDesc g_aCloudObjField[] =
{
   { "Enable Cloud Deck", kFieldTypeBool,
     FieldLocation(sMissionCloudObj, bUseCloudDeck), },
   { "Enable Fog", kFieldTypeBool,
     FieldLocation(sMissionCloudObj, bEnableFog), },
   { "Alpha Texture", kFieldTypeBool,
     FieldLocation(sMissionCloudObj, bIsAlphaTexture), },
   { "Texture", kFieldTypeString,
     FieldLocation(sMissionCloudObj, TextureName), },
   { "Alpha", kFieldTypeFloat,
     FieldLocation(sMissionCloudObj, fAlpha), },
   { "Height", kFieldTypeFloat,
     FieldLocation(sMissionCloudObj, fHeight), },
   { "Tile size", kFieldTypeFloat,
     FieldLocation(sMissionCloudObj, fTileSize), },
   { "Tiles per side", kFieldTypeInt,
     FieldLocation(sMissionCloudObj, nNumTilesPerSide), },
   { "# subtiles", kFieldTypeInt,
     FieldLocation(sMissionCloudObj, nNumSubTiles), },
   { "Wind velocity", kFieldTypeVector,
     FieldLocation(sMissionCloudObj, WindVelocity), },
   { "Overall color", kFieldTypeVector,
     FieldLocation(sMissionCloudObj, OverallColor), },
   { "East color", kFieldTypeVector,
     FieldLocation(sMissionCloudObj, EastColor), },
   { "East color method", kFieldTypeEnum, FieldLocation(sMissionCloudObj, EastColorMethod),
     FullFieldNames(g_apszColorMethodName) },
   { "East scale", kFieldTypeFloat,
     FieldLocation(sMissionCloudObj, fEastScale), },
   { "West color", kFieldTypeVector,
     FieldLocation(sMissionCloudObj, WestColor), },
   { "West color method", kFieldTypeEnum, FieldLocation(sMissionCloudObj, WestColorMethod),
     FullFieldNames(g_apszColorMethodName) },
   { "West scale", kFieldTypeFloat,
     FieldLocation(sMissionCloudObj, fWestScale), },
   { "East/West rotation", kFieldTypeFloat,
     FieldLocation(sMissionCloudObj, fZAng), },
   { "Subtile alpha start", kFieldTypeInt,
     FieldLocation(sMissionCloudObj, nSubTileAlphaStart), },
   { "Glow color", kFieldTypeVector,
     FieldLocation(sMissionCloudObj, GlowColor), },
   { "Glow color method", kFieldTypeEnum, FieldLocation(sMissionCloudObj, GlowColorMethod),
     FullFieldNames(g_apszColorMethodName) },
   { "Glow scale", kFieldTypeFloat,
     FieldLocation(sMissionCloudObj, fGlowScale), },
   { "Glow latitude", kFieldTypeFloat,
     FieldLocation(sMissionCloudObj, fGlowLat), },
   { "Glow longitude", kFieldTypeFloat,
     FieldLocation(sMissionCloudObj, fGlowLong), },
   { "Glow angle", kFieldTypeFloat,
     FieldLocation(sMissionCloudObj, fGlowAng), },
   { "Glow tiles", kFieldTypeInt,
     FieldLocation(sMissionCloudObj, nGlowTiles), },
};

static sStructDesc g_SDesc = StructDescBuild(sMissionCloudObj, kStructFlagNone,
                                             g_aCloudObjField);

static inline void SetDefaults()
{
   memset((sMissionCloudObj *)&g_CloudObj, 0, sizeof(sMissionCloudObj));
}

//////////////////////////////////////////////////////


#define MAKE_POINT_NORM(p) \
{ \
   (p).nXSize = nPointsPerSide; \
   (p).vlist = vlist; \
   (p).pUs = pUs; \
   (p).pVs = pVs; \
   (p).pAlphas = pAlphas; \
   (p).pPointColors = pPointColors; \
   (p).bIsGlow = FALSE; \
}


#define MAKE_POINT_GLOW(p) \
{ \
   (p).nXSize = nGlowPointUs; \
   (p).vlist = glow_vlist; \
   (p).pUs = pGlowUs; \
   (p).pVs = pGlowVs; \
   (p).pAlphas = pGlowAlphas; \
   (p).pPointColors = pGlowPointColors; \
   (p).bIsGlow = TRUE; \
}


// nStartY is always 1, in practice
static inline void MakeTopTJoints(int nStartIx, int nConfig, BOOL bFillOut, int nStartX, int nStartY)
{
   int i,j;

   ////////////////////////////////
   // Top:
   for (i = nStartIx; i < (nStartIx+g_CloudObj.nGlowTiles); i++)
   {
       MAKE_POINT_NORM((TJointConfigs[nConfig]+i)->Points[0])
      (TJointConfigs[nConfig]+i)->Points[0].x = nStartX;
      (TJointConfigs[nConfig]+i)->Points[0].y = nStartY;
      (TJointConfigs[nConfig]+i)->Points[0].bCheckUWrap = !!nStartX;
      (TJointConfigs[nConfig]+i)->Points[0].bCheckVWrap = FALSE;

      for (j = 0; j < 2; j++)
      {
         if (!((j+(i-nStartIx))%g_CloudObj.nGlowTiles))
         {
            MAKE_POINT_NORM((TJointConfigs[nConfig]+i)->Points[j+1])
            (TJointConfigs[nConfig]+i)->Points[j+1].x = (j+(i-nStartIx))/g_CloudObj.nGlowTiles;
            (TJointConfigs[nConfig]+i)->Points[j+1].y = 0;
            (TJointConfigs[nConfig]+i)->Points[j+1].bCheckUWrap = (((i-nStartIx)+j)==g_CloudObj.nGlowTiles);
            (TJointConfigs[nConfig]+i)->Points[j+1].bCheckVWrap = TRUE;
         }
         else
         {
            MAKE_POINT_GLOW((TJointConfigs[nConfig]+i)->Points[j+1])
            (TJointConfigs[nConfig]+i)->Points[j+1].x = (j+(i-nStartIx))%g_CloudObj.nGlowTiles;
            (TJointConfigs[nConfig]+i)->Points[j+1].y = 0;
            (TJointConfigs[nConfig]+i)->Points[j+1].bCheckUWrap = FALSE;
            (TJointConfigs[nConfig]+i)->Points[j+1].bCheckVWrap = TRUE;
         }
      }
   }
   // Then do last one:
   if (bFillOut)
   {
      MAKE_POINT_NORM((TJointConfigs[nConfig]+i)->Points[0])
      (TJointConfigs[nConfig]+i)->Points[0].x = nStartX;
      (TJointConfigs[nConfig]+i)->Points[0].y = nStartY;
      (TJointConfigs[nConfig]+i)->Points[0].bCheckUWrap = FALSE;
      (TJointConfigs[nConfig]+i)->Points[0].bCheckVWrap = FALSE;

      MAKE_POINT_NORM((TJointConfigs[nConfig]+i)->Points[1])
      (TJointConfigs[nConfig]+i)->Points[1].x = 1;
      (TJointConfigs[nConfig]+i)->Points[1].y = 0;
      (TJointConfigs[nConfig]+i)->Points[1].bCheckUWrap = TRUE;
      (TJointConfigs[nConfig]+i)->Points[1].bCheckVWrap = TRUE;

      MAKE_POINT_NORM((TJointConfigs[nConfig]+i)->Points[2])
      (TJointConfigs[nConfig]+i)->Points[2].x = 1;
      (TJointConfigs[nConfig]+i)->Points[2].y = 1;
      (TJointConfigs[nConfig]+i)->Points[2].bCheckUWrap = TRUE;
      (TJointConfigs[nConfig]+i)->Points[2].bCheckVWrap = FALSE;
   }
}


// nStartX is always 0, in practice
static inline void MakeRightTJoints(int nStartIx, int nConfig, BOOL bFillOut, int nStartX, int nStartY)
{
   int i,j;

   ///////////////////////////
   // Right:
   for (i = nStartIx; i < (nStartIx+g_CloudObj.nGlowTiles); i++)
   {
      MAKE_POINT_NORM((TJointConfigs[nConfig]+i)->Points[0])
      (TJointConfigs[nConfig]+i)->Points[0].x = nStartX;
      (TJointConfigs[nConfig]+i)->Points[0].y = nStartY;
      (TJointConfigs[nConfig]+i)->Points[0].bCheckUWrap = FALSE;
      (TJointConfigs[nConfig]+i)->Points[0].bCheckVWrap = !nStartY;

      for (j = 0; j < 2; j++)
      {
         if (!((j+(i-nStartIx))%g_CloudObj.nGlowTiles))
         {
            MAKE_POINT_NORM((TJointConfigs[nConfig]+i)->Points[j+1])
            (TJointConfigs[nConfig]+i)->Points[j+1].x = 1;
            (TJointConfigs[nConfig]+i)->Points[j+1].y = (j+(i-nStartIx))/g_CloudObj.nGlowTiles;
            (TJointConfigs[nConfig]+i)->Points[j+1].bCheckUWrap = TRUE;
            (TJointConfigs[nConfig]+i)->Points[j+1].bCheckVWrap = (((i-nStartIx)+j)!=g_CloudObj.nGlowTiles);
         }
         else
         {
            MAKE_POINT_GLOW((TJointConfigs[nConfig]+i)->Points[j+1])
            (TJointConfigs[nConfig]+i)->Points[j+1].x = g_CloudObj.nGlowTiles;
            (TJointConfigs[nConfig]+i)->Points[j+1].y = (j+(i-nStartIx))%g_CloudObj.nGlowTiles;
            (TJointConfigs[nConfig]+i)->Points[j+1].bCheckUWrap = TRUE;
            (TJointConfigs[nConfig]+i)->Points[j+1].bCheckVWrap = FALSE;
         }
      }
   }

   // Then do last one:
   if (bFillOut)
   {
      MAKE_POINT_NORM((TJointConfigs[nConfig]+i)->Points[0])
      (TJointConfigs[nConfig]+i)->Points[0].x = nStartX;
      (TJointConfigs[nConfig]+i)->Points[0].y = nStartY;
      (TJointConfigs[nConfig]+i)->Points[0].bCheckUWrap = FALSE;
      (TJointConfigs[nConfig]+i)->Points[0].bCheckVWrap = TRUE;

      MAKE_POINT_NORM((TJointConfigs[nConfig]+i)->Points[1])
      (TJointConfigs[nConfig]+i)->Points[1].x = 1;
      (TJointConfigs[nConfig]+i)->Points[1].y = 1;
      (TJointConfigs[nConfig]+i)->Points[1].bCheckUWrap = TRUE;
      (TJointConfigs[nConfig]+i)->Points[1].bCheckVWrap = FALSE;

      MAKE_POINT_NORM((TJointConfigs[nConfig]+i)->Points[2])
      (TJointConfigs[nConfig]+i)->Points[2].x = 0;
      (TJointConfigs[nConfig]+i)->Points[2].y = 1;
      (TJointConfigs[nConfig]+i)->Points[2].bCheckUWrap = FALSE;
      (TJointConfigs[nConfig]+i)->Points[2].bCheckVWrap = FALSE;
   }
}


// nStartY is always 0, in practice
static inline void MakeBottomTJoints(int nStartIx, int nConfig, BOOL bFillOut, int nStartX, int nStartY)
{
   int i,j;

   ///////////////////////////
   // Bottom:
   for (i = nStartIx; i < (nStartIx+g_CloudObj.nGlowTiles); i++)
   {
      MAKE_POINT_NORM((TJointConfigs[nConfig]+i)->Points[0])
      (TJointConfigs[nConfig]+i)->Points[0].x = nStartX;
      (TJointConfigs[nConfig]+i)->Points[0].y = nStartY;
      (TJointConfigs[nConfig]+i)->Points[0].bCheckUWrap = !!nStartX;
      (TJointConfigs[nConfig]+i)->Points[0].bCheckVWrap = TRUE;

      for (j = 0; j < 2; j++)
      {
         if (!((j+(i-nStartIx))%g_CloudObj.nGlowTiles))
         {
            MAKE_POINT_NORM((TJointConfigs[nConfig]+i)->Points[j+1])
            (TJointConfigs[nConfig]+i)->Points[j+1].x = 1-(j+(i-nStartIx))/g_CloudObj.nGlowTiles;
            (TJointConfigs[nConfig]+i)->Points[j+1].y = 1;
            (TJointConfigs[nConfig]+i)->Points[j+1].bCheckUWrap = (((i-nStartIx)+j)!=g_CloudObj.nGlowTiles);
            (TJointConfigs[nConfig]+i)->Points[j+1].bCheckVWrap = FALSE;
         }
         else
         {
            MAKE_POINT_GLOW((TJointConfigs[nConfig]+i)->Points[j+1])
            (TJointConfigs[nConfig]+i)->Points[j+1].x = g_CloudObj.nGlowTiles-(j+(i-nStartIx))%g_CloudObj.nGlowTiles;
            (TJointConfigs[nConfig]+i)->Points[j+1].y = g_CloudObj.nGlowTiles;
            (TJointConfigs[nConfig]+i)->Points[j+1].bCheckUWrap = FALSE;
            (TJointConfigs[nConfig]+i)->Points[j+1].bCheckVWrap = FALSE;
         }
      }
   }

   // Then do last one:

   if (bFillOut)
   {
      MAKE_POINT_NORM((TJointConfigs[nConfig]+i)->Points[0])
      (TJointConfigs[nConfig]+i)->Points[0].x = nStartX;
      (TJointConfigs[nConfig]+i)->Points[0].y = nStartY;
      (TJointConfigs[nConfig]+i)->Points[0].bCheckUWrap = TRUE;
      (TJointConfigs[nConfig]+i)->Points[0].bCheckVWrap = TRUE;

      MAKE_POINT_NORM((TJointConfigs[nConfig]+i)->Points[1])
      (TJointConfigs[nConfig]+i)->Points[1].x = 0;
      (TJointConfigs[nConfig]+i)->Points[1].y = 1;
      (TJointConfigs[nConfig]+i)->Points[1].bCheckUWrap = FALSE;
      (TJointConfigs[nConfig]+i)->Points[1].bCheckVWrap = FALSE;

      MAKE_POINT_NORM((TJointConfigs[nConfig]+i)->Points[2])
      (TJointConfigs[nConfig]+i)->Points[2].x = 0;
      (TJointConfigs[nConfig]+i)->Points[2].y = 0;
      (TJointConfigs[nConfig]+i)->Points[2].bCheckUWrap = FALSE;
      (TJointConfigs[nConfig]+i)->Points[2].bCheckVWrap = TRUE;
   }
}


// nStartX is always 1, in practice
static inline void MakeLeftTJoints(int nStartIx, int nConfig, BOOL bFillOut, int nStartX, int nStartY)
{
   int i,j;

   ///////////////////////////
   // Left:
   for (i = nStartIx; i < (nStartIx+g_CloudObj.nGlowTiles); i++)
   {
      MAKE_POINT_NORM((TJointConfigs[nConfig]+i)->Points[0])
      (TJointConfigs[nConfig]+i)->Points[0].x = nStartX;
      (TJointConfigs[nConfig]+i)->Points[0].y = nStartY;
      (TJointConfigs[nConfig]+i)->Points[0].bCheckUWrap = TRUE;
      (TJointConfigs[nConfig]+i)->Points[0].bCheckVWrap = !nStartY;

      for (j = 0; j < 2; j++)
      {
         if (!((j+(i-nStartIx))%g_CloudObj.nGlowTiles))
         {
            MAKE_POINT_NORM((TJointConfigs[nConfig]+i)->Points[j+1])
            (TJointConfigs[nConfig]+i)->Points[j+1].x = 0;
            (TJointConfigs[nConfig]+i)->Points[j+1].y = 1-(j+(i-nStartIx))/g_CloudObj.nGlowTiles;
            (TJointConfigs[nConfig]+i)->Points[j+1].bCheckUWrap = FALSE;
            (TJointConfigs[nConfig]+i)->Points[j+1].bCheckVWrap = (((i-nStartIx)+j)==g_CloudObj.nGlowTiles);
         }
         else
         {
            MAKE_POINT_GLOW((TJointConfigs[nConfig]+i)->Points[j+1])
            (TJointConfigs[nConfig]+i)->Points[j+1].x = 0;
            (TJointConfigs[nConfig]+i)->Points[j+1].y = g_CloudObj.nGlowTiles-(j+(i-nStartIx))%g_CloudObj.nGlowTiles;
            (TJointConfigs[nConfig]+i)->Points[j+1].bCheckUWrap = FALSE;
            (TJointConfigs[nConfig]+i)->Points[j+1].bCheckVWrap = FALSE;
         }
      }
   }

   // Then do last one:

   if (bFillOut)
   {
      MAKE_POINT_NORM((TJointConfigs[nConfig]+i)->Points[0])
      (TJointConfigs[nConfig]+i)->Points[0].x = nStartX;
      (TJointConfigs[nConfig]+i)->Points[0].y = nStartY;
      (TJointConfigs[nConfig]+i)->Points[0].bCheckUWrap = TRUE;
      (TJointConfigs[nConfig]+i)->Points[0].bCheckVWrap = FALSE;

      MAKE_POINT_NORM((TJointConfigs[nConfig]+i)->Points[1])
      (TJointConfigs[nConfig]+i)->Points[1].x = 0;
      (TJointConfigs[nConfig]+i)->Points[1].y = 0;
      (TJointConfigs[nConfig]+i)->Points[1].bCheckUWrap = FALSE;
      (TJointConfigs[nConfig]+i)->Points[1].bCheckVWrap = TRUE;

      MAKE_POINT_NORM((TJointConfigs[nConfig]+i)->Points[2])
      (TJointConfigs[nConfig]+i)->Points[2].x = 1;
      (TJointConfigs[nConfig]+i)->Points[2].y = 0;
      (TJointConfigs[nConfig]+i)->Points[2].bCheckUWrap = TRUE;
      (TJointConfigs[nConfig]+i)->Points[2].bCheckVWrap = TRUE;
   }
}


static inline void ComputeTJoints()
{
   int i;

   if (g_CloudObj.nGlowTiles < 2)
   {
      for (i = 0; i < 8; i++)
         TJointConfigs[i] = 0;
      return;
   }

   for (i = 0; i < 4; i++)
   {
      NumTJointPolys[i] = g_CloudObj.nGlowTiles+1;
      TJointConfigs[i] = new sTJointPoly[g_CloudObj.nGlowTiles+1];
   }

   for (i = 4; i < 8; i++)
   {
      NumTJointPolys[i] = g_CloudObj.nGlowTiles*2;
      TJointConfigs[i] = new sTJointPoly[g_CloudObj.nGlowTiles*2];
   }

   for (i = 0; i < 8; i++)
      memset(TJointConfigs[i], 0, sizeof(sTJointPoly));

   MakeTopTJoints(0, 0, TRUE, 0, 1);
   MakeRightTJoints(0, 1, TRUE, 0, 0);
   MakeBottomTJoints(0, 2, TRUE, 1, 0);
   MakeLeftTJoints(0, 3, TRUE, 1, 1);

   MakeTopTJoints(0, 4, FALSE, 0, 1);
   MakeRightTJoints(g_CloudObj.nGlowTiles, 4, FALSE, 0, 1);

   MakeRightTJoints(0, 5, FALSE, 0, 0);
   MakeBottomTJoints(g_CloudObj.nGlowTiles, 5, FALSE, 0, 0);

   MakeBottomTJoints(0, 6, FALSE, 1, 0);
   MakeLeftTJoints(g_CloudObj.nGlowTiles, 6, FALSE, 1, 0);

   MakeLeftTJoints(0, 7, FALSE, 1, 1);
   MakeTopTJoints(g_CloudObj.nGlowTiles, 7, FALSE, 1, 1);
}

// 1 = top
// 2 = right
// 4 = bottom
// 8 = left
static inline uchar ComputeTJointMask(int x, int y)
{
   uchar nMask = 0;

   // Top
   if (y && pGlowSubTiles[(y-1)*nPointsPerSide+x])
      nMask |= 0x01;

   // Bottom
   if ((y<(nPointsPerSide-1)) && pGlowSubTiles[(y+1)*nPointsPerSide+x])
      nMask |= 0x04;

   // Left
   if (x && pGlowSubTiles[y*nPointsPerSide+x-1])
      nMask |= 0x08;

   // Right
   if ((x<(nPointsPerSide-1)) && pGlowSubTiles[y*nPointsPerSide+x+1])
      nMask |= 0x02;

   return nMask;
}


//////////////////////////////////////////////////////


static inline void ComputeGrid()
{
   int i,j,x,y;
   float fStartX, fStartY;
   float fSubTileSize;
   float fGlowTileSize;
   mxs_vector PointVec;
   mxs_vector NormVec;
   mxs_vector TempVec;
   mxs_vector GlowVec;
   mxs_vector ZVec=  {0,0,1};
   float fGlowDegree;
   float fGlowMin;

   nPointsPerSide = g_CloudObj.nNumTilesPerSide*g_CloudObj.nNumSubTiles+1;
   nTotalPoints = nPointsPerSide*nPointsPerSide;

   fSubTileSize = g_CloudObj.fTileSize/g_CloudObj.nNumSubTiles;

   fGlowTileSize = fSubTileSize/g_CloudObj.nGlowTiles;

   pPoints = new goddamn_stupid_r3s_point[nTotalPoints];
   vlist = new r3s_phandle[nTotalPoints];
   pPointVecs = new mxs_vector[nTotalPoints];
   pPointColors = new mxs_vector[nTotalPoints];
   pAlphas = new float[nTotalPoints];
   pGlowSubTiles = new BOOL[nTotalPoints];
   pUs = new float[nPointsPerSide];
   pVs = new float[nPointsPerSide];

   for (i = 0; i < nTotalPoints; i++)
      vlist[i] = (r3s_point*)(pPoints+i);

   // We define grid from -x,+y to +x,-y, by rows.
   fStartX = -g_CloudObj.nNumTilesPerSide*0.50000*g_CloudObj.fTileSize;
   fStartY = g_CloudObj.nNumTilesPerSide*0.50000*g_CloudObj.fTileSize;

   for (y = 0; y < nPointsPerSide; y++)
   {
      for (x = 0; x < nPointsPerSide; x++)
      {
         pPointVecs[y*nPointsPerSide+x].y = fStartY-fSubTileSize*y;
         pPointVecs[y*nPointsPerSide+x].x = fStartX+fSubTileSize*x;
         pPointVecs[y*nPointsPerSide+x].z = g_CloudObj.fHeight;
      }
   }

   nMinGlowX = 100000;
   nMaxGlowX = -1;
   nMinGlowY = 100000;
   nMaxGlowY = -1;

   // Now compute Glow Grid. Glow grid contains any points within glow not in normal grid:

   nTotalGlowPoints = 0;
   memset(pGlowSubTiles, 0, nTotalPoints*sizeof(BOOL));

   if ((g_CloudObj.fGlowAng < 0.001) || (g_CloudObj.nGlowTiles < 2))
      return;

   mx_rot_y_vec(&TempVec, &ZVec, mx_rad2ang(g_CloudObj.fGlowLat*DEG));
   mx_rot_z_vec(&GlowVec, &TempVec, mx_rad2ang(g_CloudObj.fGlowLong*DEG));

   fGlowMin = (cos(g_CloudObj.fGlowAng*DEG)+1)*0.50000;

   // Test to see if glow is within any part of the subtiles:
   for (y = 0; y < nPointsPerSide-1; y++)
   {
      for (x = 0; x < nPointsPerSide-1; x++)
      {
         BOOL bHasPointInRange = FALSE;
         // sample potential glow points in this subtile to see if they're within glow:
         for (i = 0; i < g_CloudObj.nGlowTiles; i++)
         {
            for (j = 0; j < g_CloudObj.nGlowTiles; j++)
            {
               PointVec.y = fStartY-fSubTileSize*y - fGlowTileSize*i;
               PointVec.x = fStartX+fSubTileSize*x + fGlowTileSize*j;
               PointVec.z = g_CloudObj.fHeight;
               mx_norm_vec(&NormVec, &PointVec);

               fGlowDegree = (mx_dot_vec(&NormVec, &GlowVec)+1)*0.50000;
               if (fGlowDegree >= fGlowMin)
               {
                  bHasPointInRange = TRUE;

                  if (x < nMinGlowX)
                     nMinGlowX = x;
                  else if (x > nMaxGlowX)
                     nMaxGlowX = x;

                  if (y < nMinGlowY)
                     nMinGlowY = y;
                  else if (y > nMaxGlowY)
                     nMaxGlowY = y;
               }
            }
         }

         // mark so this subtile won't try to render. It'll be dealt with as a "glow" tile.
         // Also, used for t-joint computation.
         pGlowSubTiles[y*nPointsPerSide+x] = bHasPointInRange;
      }
   }

   // Now that we know extents, allocate glow grid and compute vectors:
   // Remember - only non-normal points should be accessed from here, even though we redundantly store normal point data for ease of access.

   // sanity check:
   if ((nMaxGlowX < 0) || (nMaxGlowY < 0))
   {
      Warning(("Strange error while computing glow grid\n"));
      nTotalGlowPoints = 0;
      nGlowPointVs = 0;
      nGlowPointUs = 0;
      memset(pGlowSubTiles, 0, nTotalPoints*sizeof(BOOL));
      return;
   }

/*
   nGlowPointUs = (1+nMaxGlowX-nMinGlowX)*(g_CloudObj.nGlowTiles+1);
   nGlowPointVs = (1+nMaxGlowY-nMinGlowY)*(g_CloudObj.nGlowTiles+1);
*/
   float nNum; // number of subtiles to break into glow tiles

   nMaxGlowX++; // span highest point ix
   nMaxGlowY++;

   nNum = nMaxGlowX-nMinGlowX;
   nGlowPointUs = (1+nNum) + nNum*(g_CloudObj.nGlowTiles-1);

   nNum = nMaxGlowY-nMinGlowY;
   nGlowPointVs = (1+nNum) + nNum*(g_CloudObj.nGlowTiles-1);

   nTotalGlowPoints = nGlowPointUs*nGlowPointVs;

   pGlowPoints = new goddamn_stupid_r3s_point[nTotalGlowPoints];
   glow_vlist = new r3s_phandle[nTotalGlowPoints];
   pGlowPointVecs = new mxs_vector[nTotalGlowPoints];
   pGlowPointColors = new mxs_vector[nTotalGlowPoints];
   pGlowAlphas = new float[nTotalGlowPoints];
   pGlowUs = new float[nGlowPointUs];
   pGlowVs = new float[nGlowPointVs];

   for (i = 0; i < nTotalGlowPoints; i++)
      glow_vlist[i] = (r3s_point*)(pGlowPoints+i);

   // Now compute vectors:
   fStartX = -g_CloudObj.nNumTilesPerSide*0.50000*g_CloudObj.fTileSize + nMinGlowX*fSubTileSize;
   fStartY = g_CloudObj.nNumTilesPerSide*0.50000*g_CloudObj.fTileSize - nMinGlowY*fSubTileSize;

   for (y = 0; y < nGlowPointVs; y++)
      for (x = 0; x < nGlowPointUs; x++)
      {
         pGlowPointVecs[y*nGlowPointUs+x].y = fStartY-fGlowTileSize*y;
         pGlowPointVecs[y*nGlowPointUs+x].x = fStartX+fGlowTileSize*x;
         pGlowPointVecs[y*nGlowPointUs+x].z = g_CloudObj.fHeight;
      }
}


static inline void ComputePointColor(mxs_vector *pPoint, mxs_vector *pPointColor)
{
   mxs_vector ZVec=  {0,0,1};
   mxs_vector NormVec;
   mxs_vector BaseEastVec=  {1,0,0};
   mxs_vector BaseWestVec=  {-1,0,0};
   mxs_vector EastVec;
   mxs_vector WestVec;
   mxs_vector GlowVec;
   mxs_vector TempVec;
   mxs_vector Color;
   float fEastDegree;
   float fWestDegree;
   float fGlowDegree;
   mxs_vector InterpolateVecs[4];
   float InterpolateScales[4];
   int nNumInterpolateVecs;
   mxs_vector InterpolatedColor;
   float fMaxInterpolatedScale = 0;
   float fGlowMin;

   //////////////////////////////////////////////////////////////
   // Setup direction vectors we'll be using:
   mx_norm_vec(&NormVec, pPoint);

   mx_rot_z_vec(&EastVec, &BaseEastVec, mx_rad2ang(g_CloudObj.fZAng*DEG));
   mx_rot_z_vec(&WestVec, &BaseEastVec, mx_rad2ang(g_CloudObj.fZAng*DEG));

   mx_rot_y_vec(&TempVec, &ZVec, mx_rad2ang(g_CloudObj.fGlowLat*DEG));
   mx_rot_z_vec(&GlowVec, &TempVec, mx_rad2ang(g_CloudObj.fGlowLong*DEG));


   /////////////////////////////////////////////////////////////
   // Determine weights:
   fEastDegree = (mx_dot_vec(&NormVec, &EastVec)+1)*0.50000*g_CloudObj.fEastScale;
   fWestDegree = (mx_dot_vec(&NormVec, &WestVec)+1)*0.50000*g_CloudObj.fWestScale;
   fGlowDegree = (mx_dot_vec(&NormVec, &GlowVec)+1)*0.50000;

   // Since glow angle is constrained, we apply scale after angle check:
   fGlowMin = cos(g_CloudObj.fGlowAng*DEG);
   if (fGlowDegree < fGlowMin)
      fGlowDegree = 0;
   else
      fGlowDegree = (fGlowDegree-fGlowMin)/(1-fGlowMin);
   fGlowDegree *= g_CloudObj.fGlowScale;

   // Yeah, this is set up rather stupidly, it'd be nice to have an array of these guys.
   nNumInterpolateVecs = 0;

   Color = g_CloudObj.OverallColor;

   if (g_CloudObj.EastColorMethod == kMethod_Sum)
   {
      Color.x = 1-(1-Color.x)*(1-g_CloudObj.EastColor.x*fEastDegree);
      Color.y = 1-(1-Color.y)*(1-g_CloudObj.EastColor.y*fEastDegree);
      Color.z = 1-(1-Color.z)*(1-g_CloudObj.EastColor.z*fEastDegree);
   }
   else
   {
      InterpolateVecs[nNumInterpolateVecs] = g_CloudObj.EastColor;
      InterpolateScales[nNumInterpolateVecs] = fEastDegree;
      nNumInterpolateVecs++;
   }

   if (g_CloudObj.WestColorMethod == kMethod_Sum)
   {
      Color.x = 1-(1-Color.x)*(1-g_CloudObj.WestColor.x*fWestDegree);
      Color.y = 1-(1-Color.y)*(1-g_CloudObj.WestColor.y*fWestDegree);
      Color.z = 1-(1-Color.z)*(1-g_CloudObj.WestColor.z*fWestDegree);
   }
   else
   {
      InterpolateVecs[nNumInterpolateVecs] = g_CloudObj.WestColor;
      InterpolateScales[nNumInterpolateVecs] = fWestDegree;
      nNumInterpolateVecs++;
   }

   if (g_CloudObj.fGlowAng >= 0.001)
   {
      if (g_CloudObj.GlowColorMethod == kMethod_Sum)
      {
         Color.x = 1-(1-Color.x)*(1-g_CloudObj.GlowColor.x*fGlowDegree);
         Color.y = 1-(1-Color.y)*(1-g_CloudObj.GlowColor.y*fGlowDegree);
         Color.z = 1-(1-Color.z)*(1-g_CloudObj.GlowColor.z*fGlowDegree);
      }
      else
      {
         InterpolateVecs[nNumInterpolateVecs] = g_CloudObj.GlowColor;
         InterpolateScales[nNumInterpolateVecs] = fGlowDegree;
         nNumInterpolateVecs++;
      }
   }

   // Sum up interpolated colors:
   InterpolatedColor.x = InterpolatedColor.y = InterpolatedColor.z = 0;
   for (int i = 0; i < nNumInterpolateVecs; i++)
   {
      InterpolatedColor.x = 1-(1-InterpolatedColor.x)*(1-InterpolateVecs[i].x*InterpolateScales[i]);
      InterpolatedColor.y = 1-(1-InterpolatedColor.y)*(1-InterpolateVecs[i].y*InterpolateScales[i]);
      InterpolatedColor.z = 1-(1-InterpolatedColor.z)*(1-InterpolateVecs[i].z*InterpolateScales[i]);
      if (InterpolateScales[i] > fMaxInterpolatedScale)
         fMaxInterpolatedScale = InterpolateScales[i];
   }

   // Now add interpolated colors:
   pPointColor->x = Color.x + (InterpolatedColor.x-Color.x)*fMaxInterpolatedScale;
   pPointColor->y = Color.y + (InterpolatedColor.y-Color.y)*fMaxInterpolatedScale;
   pPointColor->z = Color.z + (InterpolatedColor.z-Color.z)*fMaxInterpolatedScale;
}

static inline void ComputeColors()
{
   int x,y;

   // Normal points:
   for (y = 0; y < nPointsPerSide; y++)
      for (x = 0; x < nPointsPerSide; x++)
         ComputePointColor(pPointVecs+y*nPointsPerSide+x, pPointColors+y*nPointsPerSide+x);

   // Glow points:
   if (!pGlowPointVecs)
      return;

   for (y = 0; y < nGlowPointVs; y++)
      for (x = 0; x < nGlowPointUs; x++)
         ComputePointColor(pGlowPointVecs+y*nGlowPointUs+x, pGlowPointColors+y*nGlowPointUs+x);
}


static inline void ComputeAlphas()
{
   int x,y;
   float fInnerRadius, fOuterRadius;
   float fSubTileSize;
   float fDist;

   fSubTileSize = g_CloudObj.fTileSize/g_CloudObj.nNumSubTiles;

   // Given # of subtiles to alpha over, compute X/Y outer and inner radius:
   fOuterRadius = g_CloudObj.nNumTilesPerSide*0.50000*g_CloudObj.fTileSize;
   fInnerRadius = fOuterRadius - g_CloudObj.nSubTileAlphaStart*fSubTileSize;

   // Normal points:
   for (y = 0; y < nPointsPerSide; y++)
      for (x = 0; x < nPointsPerSide; x++)
      {
         fDist = mx_mag_vec(pPointVecs+y*nPointsPerSide+x);
         if (fDist <= fInnerRadius)
            pAlphas[y*nPointsPerSide+x] = 1;
         else if (fDist > fOuterRadius)
            pAlphas[y*nPointsPerSide+x] = 0;
         else
            pAlphas[y*nPointsPerSide+x] = 1-(fDist-fInnerRadius)/(fOuterRadius-fInnerRadius);

         pAlphas[y*nPointsPerSide+x] *= g_CloudObj.fAlpha;
      }

   if (!pGlowAlphas)
      return;

   // Glow points:
   for (y = 0; y < nGlowPointVs; y++)
      for (x = 0; x < nGlowPointUs; x++)
      {
         fDist = mx_mag_vec(pGlowPointVecs+y*nGlowPointUs+x);
         if (fDist <= fInnerRadius)
            pGlowAlphas[y*nGlowPointUs+x] = 1;
         else if (fDist > fOuterRadius)
            pGlowAlphas[y*nGlowPointUs+x] = 0;
         else
            pGlowAlphas[y*nGlowPointUs+x] = 1-(fDist-fInnerRadius)/(fOuterRadius-fInnerRadius);

         pGlowAlphas[y*nGlowPointUs+x] *= g_CloudObj.fAlpha;
      }
}


// When viewd from above, a tile is defined by lower left, lower right, upper right, upper left.
static inline void ComputeUVs()
{
   int x,y;

   // Normal points:
   for (x = 0; x < nPointsPerSide; x++)
      pUs[x] = (x%g_CloudObj.nNumSubTiles)/(float)g_CloudObj.nNumSubTiles;
   for (y = 0; y < nPointsPerSide; y++)
      pVs[y] = (((nPointsPerSide-1)-y)%g_CloudObj.nNumSubTiles)/(float)g_CloudObj.nNumSubTiles;

   if (!pGlowUs)
      return;

   // Glow points:
   for (x = 0; x < nGlowPointUs; x++)
   {
      pGlowUs[x] = (x%(g_CloudObj.nNumSubTiles*g_CloudObj.nGlowTiles))/(float)(g_CloudObj.nNumSubTiles*g_CloudObj.nGlowTiles);
      pGlowUs[x] += pUs[nMinGlowX];
      pGlowUs[x] = fmod(pGlowUs[x],1);
   }

//   float fYVal = (((nPointsPerSide-1)-(nMaxGlowY+1))%g_CloudObj.nNumSubTiles)/(float)g_CloudObj.nNumSubTiles;
   float fYVal = pVs[nMaxGlowY];
   for (y = 0; y < nGlowPointVs; y++)
   {
      pGlowVs[y] = (((nGlowPointVs-1)-y)%(g_CloudObj.nNumSubTiles*g_CloudObj.nGlowTiles))/(float)(g_CloudObj.nNumSubTiles*g_CloudObj.nGlowTiles);
      pGlowVs[y] += fYVal;
      pGlowVs[y] = fmod(pGlowVs[y],1);
   }
}



static void CloudInstallCommands();

void cCloudDeck::AppInit()
{
   AutoAppIPtr_(StructDescTools, pTools);
   pTools->Register(&g_SDesc);

   SetDefaults();
   CloudInstallCommands();
}


static BOOL bDidGameInit = FALSE;

static tSimTime nLastTime = 0;
static int nPalIx = 0;


static BOOL TestValues()
{
   BOOL bFailure = FALSE;

   if (g_CloudObj.fTileSize < 0.001)
   {
      Warning(("Invalid Cloud Tile Size %g\n", g_CloudObj.fTileSize));
      bFailure = TRUE;
   }

   if (g_CloudObj.nNumTilesPerSide < 1)
   {
      Warning(("Invalid number of cloud tiles: %d\n", g_CloudObj.nNumTilesPerSide));
      bFailure = TRUE;
   }

   if (g_CloudObj.nNumSubTiles < 1)
   {
      Warning(("Invalid number of cloud sub tiles: %d\n", g_CloudObj.nNumSubTiles));
      bFailure = TRUE;
   }

   return bFailure;
}

// Tesselate.
// Call after mission parms are loaded.
void cCloudDeck::Init()
{
   int i;

   if (!g_CloudObj.bUseCloudDeck || !SkyRendererUseEnhanced())
      return;

   if (TestValues())
      return;

   // Create Res interface.
   IResMan *pResMan = AppGetObj(IResMan);
   pRes = IResMan_Bind(pResMan, g_CloudObj.TextureName, RESTYPE_IMAGE, NULL, "fam\\skyhw\\", 0);
   if (!pRes)
      return;

   for (i = 16; i < 256; i++)
      AlphaTable[i] = 0;

   // Load palette
   if (!g_CloudObj.bIsAlphaTexture)
   {
      IRes *pPalRes = IResMan_Bind(pResMan, IRes_GetName(pRes), RESTYPE_PALETTE,
                             NULL, "fam\\skyhw\\", 0);
      if (pPalRes)
      {
         // Okay, we've got the palette -- set it up and dump it
         nPalIx = palmgr_alloc_pal((uchar *)(pPalRes->Lock()));
         pPalRes->Unlock();
         SafeRelease(pPalRes);
      }
   }


   nLastTime = 0;

   ComputeGrid();
   ComputeAlphas();
   ComputeColors();
   ComputeUVs();
   ComputeTJoints();

   bDidGameInit = TRUE;
}


void cCloudDeck::Term()
{
   SafeRelease(pRes);
   pRes = 0;

   if (pPointColors)
   {
      delete []pPointColors;
      pPointColors = 0;
   }

   if (pPointVecs)
   {
      delete []pPointVecs;
      pPointVecs = 0;
   }

   if (pPoints)
   {
      delete []pPoints;
      pPoints = 0;
   }

   if (vlist)
   {
      delete []vlist;
      vlist = 0;
   }

   if (pAlphas)
   {
      delete pAlphas;
      pAlphas = 0;
   }

   if (pUs)
   {
      delete pUs;
      pUs = 0;
   }

   if (pVs)
   {
      delete pVs;
      pVs = 0;
   }

   if (pGlowSubTiles)
   {
      delete pGlowSubTiles;
      pGlowSubTiles = 0;
   }

   /////////////
   // glow data

   if (pGlowPointColors)
   {
      delete []pGlowPointColors;
      pGlowPointColors = 0;
   }

   if (pGlowPointVecs)
   {
      delete []pGlowPointVecs;
      pGlowPointVecs = 0;
   }

   if (pGlowPoints)
   {
      delete []pGlowPoints;
      pGlowPoints = 0;
   }

   if (glow_vlist)
   {
      delete []glow_vlist;
      glow_vlist = 0;
   }

   if (pGlowAlphas)
   {
      delete pGlowAlphas;
      pGlowAlphas = 0;
   }

   if (pGlowUs)
   {
      delete pGlowUs;
      pGlowUs = 0;
   }

   if (pGlowVs)
   {
      delete pGlowVs;
      pGlowVs = 0;
   }

   for (int i = 0; i < 8; i++)
   {
      delete []TJointConfigs[i];
      TJointConfigs[i] = 0;
   }

// Interferes with database update
//   SetDefaults();

   bDidGameInit = FALSE;
}


#ifndef SHIP
static int nDebugRow = -1;
static int nDebugCol = -1;
static int nDebugGlowIx = -1;
static int nDebugConfigIx = -1;


static void SetDebugRow(int nRow)
{
   nDebugRow = nRow;
}

static void SetDebugCol(int nCol)
{
   nDebugCol = nCol;
}

static void SetDebugGlowIx(int nIx)
{
   nDebugGlowIx = nIx;
}

static void SetDebugConfigIx(int nIx)
{
   nDebugConfigIx = nIx;
}

static void DumpStats()
{
   int nTotalSize =
      MSize(pPoints)+MSize(pPointVecs)+MSize(pPointColors)+MSize(vlist)+MSize(pAlphas)+MSize(pUs)+MSize(pVs)+MSize(pGlowSubTiles);

   if (pGlowPoints)
      nTotalSize += MSize(pGlowPoints)+MSize(pGlowPointVecs)+MSize(pGlowPointColors)+MSize(glow_vlist)+MSize(pGlowAlphas)+MSize(pGlowUs)+MSize(pGlowVs);

   mprintf("There %d normal cloud points, and %d glow points, in %g kbytes\n", nTotalPoints, nTotalGlowPoints, nTotalSize/1024.000);
}


#endif


// @TBD: don't use normal points in glow points - wasted transformation.

static inline void CalcGlowPointData(int y, int x, r3s_phandle *pPoints, int nPointIx)
{
   if (!(y%g_CloudObj.nGlowTiles) && !(x%g_CloudObj.nGlowTiles)) // Normal point.
   {
      int nNormalX;
      int nNormalY;

      nNormalX = nMinGlowX+x/g_CloudObj.nGlowTiles;
      nNormalY = nMinGlowY+y/g_CloudObj.nGlowTiles;

      int nIx = nNormalY*nPointsPerSide + nNormalX;

      pPoints[nPointIx] = vlist[nIx];

      switch (nPointIx)
      {
         case 0:
            pPoints[0]->grp.u = pUs[nNormalX];
            pPoints[0]->grp.v = pVs[nNormalY];
         break;

         case 1:
            if (pUs[nNormalX] < 0.0001) // Is zero, so make it one:
               pPoints[1]->grp.u = 1;
            else
               pPoints[1]->grp.u = pUs[nNormalX];
            pPoints[1]->grp.v = pPoints[0]->grp.v;
         break;

         case 2:
            pPoints[2]->grp.u = pPoints[1]->grp.u;
            if (pVs[nNormalY] < 0.0001) // Is zero, so make it one:
               pPoints[2]->grp.v = 1;
            else
               pPoints[2]->grp.v = pVs[nNormalY];
         break;

         case 3:
            pPoints[3]->grp.u = pPoints[0]->grp.u;
            pPoints[3]->grp.v = pPoints[2]->grp.v;
         break;
      }

      ((goddamn_stupid_r3s_point *)(pPoints[nPointIx]))->grp.i = pPointColors[nIx].x;
      ((goddamn_stupid_r3s_point *)(pPoints[nPointIx]))->grp.h = pPointColors[nIx].y;
      ((goddamn_stupid_r3s_point *)(pPoints[nPointIx]))->grp.d = pPointColors[nIx].z;
      ((goddamn_stupid_r3s_point *)(pPoints[nPointIx]))->grp.a = pAlphas[nIx];
   }
   else // is a glow point
   {
      int nIx = y*nGlowPointUs+x;
      pPoints[nPointIx] = glow_vlist[nIx];

      switch (nPointIx)
      {
         case 0:
            pPoints[0]->grp.u = pGlowUs[x];
            pPoints[0]->grp.v = pGlowVs[y];
         break;

         case 1:
            if (pGlowUs[x] < 0.0001) // Is zero, so make it one:
               pPoints[1]->grp.u = 1;
            else
               pPoints[1]->grp.u = pGlowUs[x];
            pPoints[1]->grp.v = pPoints[0]->grp.v;
         break;

         case 2:
            pPoints[2]->grp.u = pPoints[1]->grp.u;
            if (pGlowVs[y] < 0.0001) // Is zero, so make it one:
               pPoints[2]->grp.v = 1;
            else
               pPoints[2]->grp.v = pGlowVs[y];
         break;

         case 3:
            pPoints[3]->grp.u = pPoints[0]->grp.u;
            pPoints[3]->grp.v = pPoints[2]->grp.v;
         break;
      }

      ((goddamn_stupid_r3s_point *)(pPoints[nPointIx]))->grp.i = pGlowPointColors[nIx].x;
      ((goddamn_stupid_r3s_point *)(pPoints[nPointIx]))->grp.h = pGlowPointColors[nIx].y;
      ((goddamn_stupid_r3s_point *)(pPoints[nPointIx]))->grp.d = pGlowPointColors[nIx].z;
      ((goddamn_stupid_r3s_point *)(pPoints[nPointIx]))->grp.a = pGlowAlphas[nIx];
   }
}


static inline void RenderTJointConfig(int nSubTileX, int nSubTileY, int nConfig, float fUOffset, float fVOffset)
{
   r3s_phandle Points[3];
   int i,j;
   int nIx;
   int x,y;
   sTJointPoly *pPoly;
   sTJointPolyPoint *pPolyPoint;

   if (nConfig < 0)
      return;

   for (i = 0; i < NumTJointPolys[nConfig]; i++)
   {
#ifndef SHIP
      if ((nDebugConfigIx != -1) && (nDebugConfigIx != i))
         continue;
#endif

      pPoly = TJointConfigs[nConfig]+i;
      // Configs are stored as viewed from above, but we're below, so reverse order:
      for (j = 2; j >= 0; j--)
      {
         pPolyPoint = pPoly->Points+(2-j);

         if (pPolyPoint->bIsGlow)
         {
            int nStartY = nSubTileY-nMinGlowY;
            int nStartX = nSubTileX-nMinGlowX;
            x = nStartX*g_CloudObj.nGlowTiles+pPolyPoint->x;
            y = nStartY*g_CloudObj.nGlowTiles+pPolyPoint->y;
         }
         else
         {
            x = nSubTileX+pPolyPoint->x;
            y = nSubTileY+pPolyPoint->y;
         }

         nIx = y*pPolyPoint->nXSize+x;

         Points[j] = pPolyPoint->vlist[nIx];
         Points[j]->grp.u = pPolyPoint->pUs[x];
         Points[j]->grp.v = pPolyPoint->pVs[y];
         ((goddamn_stupid_r3s_point *)(Points[j]))->grp.i = pPolyPoint->pPointColors[nIx].x;
         ((goddamn_stupid_r3s_point *)(Points[j]))->grp.h = pPolyPoint->pPointColors[nIx].y;
         ((goddamn_stupid_r3s_point *)(Points[j]))->grp.d = pPolyPoint->pPointColors[nIx].z;
         ((goddamn_stupid_r3s_point *)(Points[j]))->grp.a = pPolyPoint->pAlphas[nIx];

         if (pPolyPoint->bCheckUWrap && (Points[j]->grp.u < 0.0001))
            Points[j]->grp.u = 1;
         if (pPolyPoint->bCheckVWrap && (Points[j]->grp.v < 0.0001))
            Points[j]->grp.v = 1;

         Points[j]->grp.u += fUOffset;
         Points[j]->grp.v += fVOffset;
      }

      r3d_do_setup = TRUE;
      r3_draw_poly(3, Points);
   }
}



void cCloudDeck::Render()
{
   grs_bitmap *pBitmap;
   size_t nStride;
   mxs_vector OldPos;
   mxs_vector ZeroPos = {0,0,0};
   BOOL bRestoreZWrite;
   BOOL bRestoreZCompare;
   r3s_phandle Points[4];
   int PointIxs[4];
   float fUOffset;
   float fVOffset;
   tSimTime nCurrentTime;

   if (!g_lgd3d || !bDidGameInit)
      return;

   ////////////////////////////////////////////////////
   // Setup
   pBitmap = (grs_bitmap *)pRes->Lock();
   if (!pBitmap)
   {
      Warning(("Cloud texture not found\n"));
      return;
   }

   nCurrentTime = GetSimTime();

   if (nCurrentTime == nLastTime)
   {
      fUOffset = 0;
      fVOffset = 0;
   }
   else
   {
      fUOffset = (g_CloudObj.WindVelocity.x/g_CloudObj.fTileSize)*(nCurrentTime/(float)SIM_TIME_SECOND);
      fVOffset = (g_CloudObj.WindVelocity.y/g_CloudObj.fTileSize)*(nCurrentTime/(float)SIM_TIME_SECOND);
      fUOffset = fmod(fUOffset, 1);
      fVOffset = fmod(fVOffset, 1);
   }

   nLastTime = nCurrentTime;

   nStride = r3d_glob.cur_stride;
   r3d_glob.cur_stride = sizeof(goddamn_stupid_r3s_point);

   r3e_space nOldSpace = r3_get_space();
   r3_set_space(R3_CLIPPING_SPACE);

   OldPos = *r3_get_view_pos();
   r3_set_view(&ZeroPos);

   r3_set_clipmode(R3_CLIP);
   r3_set_clip_flags(R3_CLIP_UV|R3_CLIP_RGB);
   // If this is an alpha texture:
   if (g_CloudObj.bIsAlphaTexture) // Must be an alpha texture:
   {
      lgd3d_disable_palette();
      gr_set_fill_type(FILL_BLEND);
      lgd3d_set_alpha_pal(AlphaTable);
   }
   else // setup palette for resource, and mark bitmap as having transparency:
   {
      pBitmap->align = nPalIx;
// NOT a transparent bitmap!
//      pBitmap->flags|=BMF_TRANS;
   }

   r3_set_prim();
   r3_set_polygon_context(R3_PL_POLYGON|R3_PL_TEXTURE|R3_PL_RGB_GOURAUD);
   lgd3d_set_blend(TRUE);
   r3_set_color(0xffffff);
   bRestoreZWrite = lgd3d_is_zwrite_on();
   bRestoreZCompare = lgd3d_is_zcompare_on();
   lgd3d_set_zwrite(FALSE);
   lgd3d_set_zcompare(FALSE);
   lgd3d_set_alpha(1);

   lgd3d_set_fog_enable(portal_fog_on && g_CloudObj.bEnableFog);

   /////////////////////////////
   // Render
   r3_set_texture(pBitmap);
   r3_start_block();
   r3_transform_block(nTotalPoints, (r3s_point *)pPoints, pPointVecs);
   if (pGlowPointVecs)
      r3_transform_block(nTotalGlowPoints, (r3s_point *)pGlowPoints, pGlowPointVecs);

   // Per subtile:
   for (int y = 0; y < nPointsPerSide-1; y++)
   {
#ifndef SHIP
      if ((nDebugRow != -1) && (nDebugRow != y))
         continue;
#endif

      for (int x = 0; x < nPointsPerSide-1; x++)
      {
#ifndef SHIP
      if ((nDebugCol != -1) && (nDebugCol != x))
         continue;
#endif
         if (pGlowSubTiles[y*nPointsPerSide+x]) // This is a subtile containing glow tiles. Render them
         {
            int nStartY, nStartX;
            nStartY = y-nMinGlowY;
            nStartX = x-nMinGlowX;
            for (int i = 0; i < g_CloudObj.nGlowTiles; i++)
               for (int j = 0; j < g_CloudObj.nGlowTiles; j++)
               {
#ifndef SHIP
                  if ((nDebugGlowIx != -1) && (nDebugGlowIx != i*g_CloudObj.nGlowTiles+j))
                     continue;
#endif

                  CalcGlowPointData(nStartY*g_CloudObj.nGlowTiles+i+1, nStartX*g_CloudObj.nGlowTiles+j, Points, 0);
                  CalcGlowPointData(nStartY*g_CloudObj.nGlowTiles+i+1, nStartX*g_CloudObj.nGlowTiles+j+1, Points, 1);
                  CalcGlowPointData(nStartY*g_CloudObj.nGlowTiles+i, nStartX*g_CloudObj.nGlowTiles+j+1, Points, 2);
                  CalcGlowPointData(nStartY*g_CloudObj.nGlowTiles+i, nStartX*g_CloudObj.nGlowTiles+j, Points, 3);
                  Points[0]->grp.u += fUOffset;
                  Points[0]->grp.v += fVOffset;
                  Points[1]->grp.u += fUOffset;
                  Points[1]->grp.v += fVOffset;
                  Points[2]->grp.u += fUOffset;
                  Points[2]->grp.v += fVOffset;
                  Points[3]->grp.u += fUOffset;
                  Points[3]->grp.v += fVOffset;

                  r3d_do_setup = TRUE;
                  r3_draw_poly(4, Points);
               }
         }
         else
         {
            uchar nMask = ComputeTJointMask(x,y);
            if (nMask) // Need to deal with TJoints
            {
               if (MaskToTJoint[nMask] == -1) // A problem:
                  Warning(("Wierd Glow t-joint config. Doing normal render\n"));
               else
               {
                  RenderTJointConfig(x,y,MaskToTJoint[nMask], fUOffset, fVOffset);
                  continue;
               }
            }

            // just render the sub tile normally
            PointIxs[0] = (y+1)*nPointsPerSide+x;
            PointIxs[1] = (y+1)*nPointsPerSide+x+1;
            PointIxs[2] = y*nPointsPerSide+x+1;
            PointIxs[3] = y*nPointsPerSide+x;

            Points[0] = vlist[PointIxs[0]];
            Points[1] = vlist[PointIxs[1]];
            Points[2] = vlist[PointIxs[2]];
            Points[3] = vlist[PointIxs[3]];

            Points[0]->grp.u = pUs[x]+fUOffset;
            Points[0]->grp.v = pVs[y+1]+fVOffset;
            ((goddamn_stupid_r3s_point *)(Points[0]))->grp.i = pPointColors[PointIxs[0]].x;
            ((goddamn_stupid_r3s_point *)(Points[0]))->grp.h = pPointColors[PointIxs[0]].y;
            ((goddamn_stupid_r3s_point *)(Points[0]))->grp.d = pPointColors[PointIxs[0]].z;
            ((goddamn_stupid_r3s_point *)(Points[0]))->grp.a = pAlphas[PointIxs[0]];

            if (pUs[x+1] < 0.0001) // Is zero, so make it one:
               Points[1]->grp.u = 1+fUOffset;
            else
               Points[1]->grp.u = pUs[x+1]+fUOffset;
            Points[1]->grp.v = Points[0]->grp.v;
            ((goddamn_stupid_r3s_point *)(Points[1]))->grp.i = pPointColors[PointIxs[1]].x;
            ((goddamn_stupid_r3s_point *)(Points[1]))->grp.h = pPointColors[PointIxs[1]].y;
            ((goddamn_stupid_r3s_point *)(Points[1]))->grp.d = pPointColors[PointIxs[1]].z;
            ((goddamn_stupid_r3s_point *)(Points[1]))->grp.a = pAlphas[PointIxs[1]];

            Points[2]->grp.u = Points[1]->grp.u;
            if (pVs[y] < 0.0001) // Is zero, so make it one:
               Points[2]->grp.v = 1+fVOffset;
            else
               Points[2]->grp.v = pVs[y]+fVOffset;
            ((goddamn_stupid_r3s_point *)(Points[2]))->grp.i = pPointColors[PointIxs[2]].x;
            ((goddamn_stupid_r3s_point *)(Points[2]))->grp.h = pPointColors[PointIxs[2]].y;
            ((goddamn_stupid_r3s_point *)(Points[2]))->grp.d = pPointColors[PointIxs[2]].z;
            ((goddamn_stupid_r3s_point *)(Points[2]))->grp.a = pAlphas[PointIxs[2]];

            Points[3]->grp.u = Points[0]->grp.u;
            Points[3]->grp.v = Points[2]->grp.v;
            ((goddamn_stupid_r3s_point *)(Points[3]))->grp.i = pPointColors[PointIxs[3]].x;
            ((goddamn_stupid_r3s_point *)(Points[3]))->grp.h = pPointColors[PointIxs[3]].y;
            ((goddamn_stupid_r3s_point *)(Points[3]))->grp.d = pPointColors[PointIxs[3]].z;
            ((goddamn_stupid_r3s_point *)(Points[3]))->grp.a = pAlphas[PointIxs[3]];

            r3d_do_setup = TRUE;
            r3_draw_poly(4, Points);
         }
      }
   }

   r3_end_block();


   /////////////////////////////
   // Cleanup
   r3d_glob.cur_stride = nStride;
   gr_set_fill_type(FILL_NORM);
   r3_set_prim();
   lgd3d_set_blend(FALSE);
   lgd3d_set_zwrite(bRestoreZWrite);
   lgd3d_set_zcompare(bRestoreZCompare);
   lgd3d_enable_palette();
   lgd3d_set_fog_enable(portal_fog_on);
   r3_set_space(nOldSpace);
   r3_set_view(&OldPos);
   pRes->Unlock();
}


#ifndef SHIP
static Command g_CloudCommands[] =
{
   { "cloud_row", FUNC_INT, SetDebugRow, "Debug Cloud Row", HK_ALL},
   { "cloud_col", FUNC_INT, SetDebugCol, "Debug Cloud Col", HK_ALL},
   { "cloud_glow", FUNC_INT, SetDebugGlowIx, "Debug Glow Ix", HK_ALL},
   { "cloud_cfg", FUNC_INT, SetDebugConfigIx, "Debug Config Ix", HK_ALL},
   { "cloud_dump", FUNC_VOID, DumpStats, "Dump cloud deck info", HK_ALL},
};
#endif


static void CloudInstallCommands()
{
#ifndef SHIP
   COMMANDS(g_CloudCommands, HK_ALL);
#endif
}
