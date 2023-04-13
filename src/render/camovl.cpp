/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/camovl.cpp,v 1.10 2000/03/16 19:47:18 bfarquha Exp $

// "Overlay" (read "2d") effects for when we're looking through the remote camera.


//
// We're thinking of doing three things:
// 1) A fade to black around the edges
// 2) chipped lens effect (like FU raindrops), perhaps with cracks, too.
// 3) Alpha'd smudges.
//

#include <lg.h>

// For frosted edges:
#include <stdlib.h>
#include <math.h>
#include <r3d.h>
#include <fix.h>
#include <lgd3d.h>
#include <scrnmode.h>
#include <config.h>


// For smudges:
#include <guistyle.h>
#include <resapi.h>
#include <imgrstyp.h>
#include <palrstyp.h>
#include <binrstyp.h>
#include <palmgr.h>
#include <appagg.h>
#include <matrix.h>

#include <objlight.h>
#include <camera.h>
#include <playrobj.h>

// For chips:
#include <scrnman.h>


#include <camovl.h>

#include <mprintf.h>
#include <dev2d.h>
#include <memall.h>
#include <dbmem.h>   // must be last header!


#define PI 3.14159265
#define TWOPI (2*PI)

extern "C" BOOL g_lgd3d;


class IRemoteCameraEffect
{
public:
   virtual void Init() = 0;
   virtual void Term() = 0;
   virtual void OnFrame() = 0;
   virtual void Render(BOOL bIsScreenLocked) = 0;
};

struct goddamn_stupid_r3s_point
{
   mxs_vector p;
   ulong ccodes;
   g2s_point grp;
};


///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////


// Rendered as subdivided quads. Each quad goes from center of screen to corners of screen.
// quad is subdivied into 'n' wedge-like shapes. On top of that, there is an "inner" circle,
// into which no polys are rendered, and an outer circle, after which the polys are solid black.
// The outer circle goes from top to bottom of screen. Aspect ratio should be considered when
// determining screen width extents of both circles.



struct sFrostyPolygon
{
   r3s_point *(pPoints[4]);
};


class cFrostedEdgesEffect : public IRemoteCameraEffect
{
public:
   cFrostedEdgesEffect();

   virtual void Init();
   virtual void Term();
   virtual void OnFrame();
   virtual void Render(BOOL bIsScreenLocked);

private:
   void ComputePolys();
   void ReadFrostedData();
   void SetupPoint(int pt, int x, int y, float a);

   int nPointsPerQuad;
   float fInnerCircleSize;
   float fOuterCircleSize;

   int nWidth; // Screen width and height
   int nHeight;
   int nNumPoints;
   int nNumPolygons;
   sFrostyPolygon *pPolygons;
   goddamn_stupid_r3s_point *pPoints;
};



cFrostedEdgesEffect::cFrostedEdgesEffect()
   : nWidth(-1), nHeight(-1), pPoints(0), pPolygons(0)
{}


// @TBD: when Zarko has hooks, set this on init.
BOOL bCardSupportsIteratedAlpha = TRUE;

void cFrostedEdgesEffect::ReadFrostedData()
{
   IRes *pFrostedDataRes;
   char *pFrostedData;
   char *pFound;
   char TempStr[256];

   IResMan *pResMan = AppGetObj(IResMan);

   pFrostedDataRes = IResMan_Bind(pResMan, "camovl.txt", RESTYPE_BINARY, NULL, "camera\\", 0);
   if (!pFrostedDataRes)
      return;

   pFrostedData = (char *)pFrostedDataRes->Lock();
   if (!pFrostedData)
   {
      Warning(("Strange Error getting Frosted data\n"));
      pFrostedDataRes->Release();
      return;
   }

   // It is expected that the smudge  fields are defined in this order:
   // FrostedInnerSize [0 to 1]
   // FrostedOuterSize [0 to 1]
   // FrostedPoints [positive int]

   BOOL bFailed = FALSE;

   if (bCardSupportsIteratedAlpha)
   {
      if ((pFound = strstr(pFrostedData, "FrostedInnerSize")) == 0)
         bFailed = TRUE;
      else if (sscanf(pFound, "%s %f", TempStr, &fInnerCircleSize) != 2)
         bFailed = TRUE;
      else if ((pFound = strstr(pFound, "FrostedOuterSize")) == 0)
         bFailed = TRUE;
      else if (sscanf(pFound, "%s %f", TempStr, &fOuterCircleSize) != 2)
         bFailed = TRUE;
      else if ((pFound = strstr(pFound, "FrostedPoints")) == 0)
         bFailed = TRUE;
      else if (sscanf(pFound, "%s %d", TempStr, &nPointsPerQuad) != 2)
         bFailed = TRUE;
   }
   else
   {
      if ((pFound = strstr(pFrostedData, "FrostedInnerSizeNIA")) == 0)
         bFailed = TRUE;
      else if (sscanf(pFound, "%s %f", TempStr, &fInnerCircleSize) != 2)
         bFailed = TRUE;
      else if ((pFound = strstr(pFound, "FrostedOuterSizeNIA")) == 0)
         bFailed = TRUE;
      else if (sscanf(pFound, "%s %f", TempStr, &fOuterCircleSize) != 2)
         bFailed = TRUE;
      else if ((pFound = strstr(pFound, "FrostedPointsNIA")) == 0)
         bFailed = TRUE;
      else if (sscanf(pFound, "%s %d", TempStr, &nPointsPerQuad) != 2)
         bFailed = TRUE;
   }

   // Just to make sure:
   if (bFailed)
      Warning(("Failed to read Frosted data\n"));

   pFrostedDataRes->Unlock();
   pFrostedDataRes->Release();
}



static BOOL bForcedIteratedAlphaSupported;
static BOOL bUseForcedIteratedAlpha = FALSE;

void cFrostedEdgesEffect::Init()
{
   if (config_is_defined("ForceCameraOverlaySimple"))
   {
      bForcedIteratedAlphaSupported = FALSE;
      bUseForcedIteratedAlpha = TRUE;
   }
   else if (config_is_defined("ForceCameraOverlayComplex"))
   {
      bForcedIteratedAlphaSupported = TRUE;
      bUseForcedIteratedAlpha = TRUE;
   }
   else
      bUseForcedIteratedAlpha = FALSE;
}

void cFrostedEdgesEffect::OnFrame()
{}

void cFrostedEdgesEffect::Term()
{
   if (pPoints)
   {
      delete []pPoints;
      pPoints = 0;
   }
   if (pPolygons)
   {
      delete []pPolygons;
      pPolygons = 0;
   }

   nNumPoints = 0;
   nNumPolygons = 0;
   nWidth = -1;
   nHeight = -1;
}


inline void cFrostedEdgesEffect::SetupPoint(int pt, int x, int y, float a)
{
   pPoints[(pt)].grp.sx = fix_make((x),0);
   pPoints[(pt)].grp.sy = fix_make((y),0);
   pPoints[(pt)].grp.i = 0;
   pPoints[(pt)].grp.h = 0;
   pPoints[(pt)].grp.d = 0;
   pPoints[(pt)].grp.w = 1;
   if (bCardSupportsIteratedAlpha)
      pPoints[(pt)].grp.a = a;
   else
      pPoints[(pt)].grp.a = 1;
}

extern "C" int g_lgd3d_device_index;


// DUMB DUMB DUMB. Some cards don't think an upper left of 0,0 is sufficient, so we must use -1,-1.

#define DUMB_ADJUST 1

void cFrostedEdgesEffect::ComputePolys()
{
   int nMidX, nMidY;
   float fTheta;
   int nX, nY;
   int i;
   int nPointsPerSide;
   int nQuadPoints;
   int nOuterWidth;
   int nInnerWidth;
   int nOuterHeight;
   int nInnerHeight;
   int nNextPointSet;
   float fAspect;
   int nQuad;

   BOOL bIteratedAlphaMode;

   sScrnMode Mode;

   ScrnModeGet(&Mode);

   if (bUseForcedIteratedAlpha)
      bIteratedAlphaMode = bForcedIteratedAlphaSupported;
   else
      bIteratedAlphaMode = !!(lgd3d_get_device_info(g_lgd3d_device_index)->flags & LGD3DF_CAN_DO_ITERATE_ALPHA);

// mprintf("Using device %d, alpha mode = %d\n", g_lgd3d_device_index, (int)bIteratedAlphaMode);

   if ((Mode.w != nWidth) || (Mode.h != nHeight) || (bIteratedAlphaMode != bCardSupportsIteratedAlpha)) // Not already computed
   {
      bCardSupportsIteratedAlpha = bIteratedAlphaMode;

      ReadFrostedData(); // Read data that applies to alpha mode.

      nWidth = Mode.w;
      nHeight = Mode.h;
      nMidX = nWidth/2;
      nMidY = nHeight/2;

      // from dev2d if context is available, else find some other way to get it:
      // aspect is w/h
      fAspect = fix_float(grd_mode_cap.aspect);

      nOuterHeight = fOuterCircleSize*nHeight*0.50000;
      nInnerHeight = fInnerCircleSize*nHeight*0.50000;
      nOuterWidth = fOuterCircleSize*nHeight*0.50000/fAspect;
      nInnerWidth = fInnerCircleSize*nHeight*0.50000/fAspect;

      nNumPoints = (nPointsPerQuad+2)*4*3-12; // corner points are shared.
      nNumPolygons = (nPointsPerQuad+1)*4*2;

      if (pPoints)
         delete []pPoints;
      pPoints = new goddamn_stupid_r3s_point[nNumPoints];

      if (pPolygons)
         delete pPolygons;
      pPolygons = new sFrostyPolygon[nNumPolygons];

      nPointsPerSide = nPointsPerQuad+1;
      nQuadPoints = nPointsPerSide*3;

      // Points are stored by quad, outer, mid then inner.

      // Top quad:
      nQuad = 0;
      for (i = 0; i < nPointsPerSide; i++)
      {
         nX = (i/(float)nPointsPerSide)*nWidth-DUMB_ADJUST;
         fTheta = atan2((nHeight/2),(nX-nMidX));

         // Outer points (along edge of screen)
         SetupPoint(nQuad*nQuadPoints+i*3, nX, 0-DUMB_ADJUST, 1);

         // Mid Points (given outer radius)
         nX = nMidX+cos(fTheta)*nOuterWidth;
         nY = nMidY-sin(fTheta)*nOuterHeight;
         SetupPoint(nQuad*nQuadPoints+i*3+1, nX, nY, 1);

         // Inner Points (given inner radius)
         nX = nMidX+cos(fTheta)*nInnerWidth;
         nY = nMidY-sin(fTheta)*nInnerHeight;
         SetupPoint(nQuad*nQuadPoints+i*3+2, nX, nY, 0);
      }

      // Right quad:
      nQuad = 1;
      for (i = 0; i < nPointsPerSide; i++)
      {
         nY = (i/(float)nPointsPerSide)*nHeight - DUMB_ADJUST;
         fTheta = atan2((nMidY-nY),(nWidth/2));

         // Outer points (along edge of screen)
         SetupPoint(nQuad*nQuadPoints+i*3, nWidth, nY, 1);

         // Mid Points (given outer radius)
         nX = nMidX+cos(fTheta)*nOuterWidth;
         nY = nMidY-sin(fTheta)*nOuterHeight;
         SetupPoint(nQuad*nQuadPoints+i*3+1, nX, nY, 1);

         // Inner Points (given inner radius)
         nX = nMidX+cos(fTheta)*nInnerWidth;
         nY = nMidY-sin(fTheta)*nInnerHeight;
         SetupPoint(nQuad*nQuadPoints+i*3+2, nX, nY, 0);
      }


      // Bottom quad:
      nQuad = 2;
      for (i = 0; i < nPointsPerSide; i++)
      {
         nX = nWidth-(i/(float)nPointsPerSide)*nWidth;
         fTheta = atan2((-nHeight/2),(nX-nMidX));

         // Outer points (along edge of screen)
         SetupPoint(nQuad*nQuadPoints+i*3, nX, nHeight, 1);

         // Mid Points (given outer radius)
         nX = nMidX+cos(fTheta)*nOuterWidth;
         nY = nMidY-sin(fTheta)*nOuterHeight;
         SetupPoint(nQuad*nQuadPoints+i*3+1, nX, nY, 1);

         // Inner Points (given inner radius)
         nX = nMidX+cos(fTheta)*nInnerWidth;
         nY = nMidY-sin(fTheta)*nInnerHeight;
         SetupPoint(nQuad*nQuadPoints+i*3+2, nX, nY, 0);
      }


      // Left quad:
      nQuad = 3;
      for (i = 0; i < nPointsPerSide; i++)
      {
         nY = nHeight-(i/(float)nPointsPerSide)*nHeight;
         fTheta = atan2((nMidY-nY),(-nWidth/2));

         // Outer points (along edge of screen)
         SetupPoint(nQuad*nQuadPoints+i*3, 0-DUMB_ADJUST, nY, 1);

         // Mid Points (given outer radius)
         nX = nMidX+cos(fTheta)*nOuterWidth;
         nY = nMidY-sin(fTheta)*nOuterHeight;
         SetupPoint(nQuad*nQuadPoints+i*3+1, nX, nY, 1);

         // Inner Points (given inner radius)
         nX = nMidX+cos(fTheta)*nInnerWidth;
         nY = nMidY-sin(fTheta)*nInnerHeight;
         SetupPoint(nQuad*nQuadPoints+i*3+2, nX, nY, 0);
      }


      // Then we associated polys with points. First poly has upper left point in upper left corner:
      for (i = 0; i < nNumPolygons/2; i++)
      {
         nNextPointSet = i+1;
         if (nNextPointSet*3 == nNumPoints)
            nNextPointSet = 0;

         // Outer polygon:
         pPolygons[i*2].pPoints[0] = (r3s_point *)(pPoints+i*3);
         pPolygons[i*2].pPoints[1] = (r3s_point *)(pPoints+nNextPointSet*3);
         pPolygons[i*2].pPoints[2] = (r3s_point *)(pPoints+nNextPointSet*3+1);
         pPolygons[i*2].pPoints[3] = (r3s_point *)(pPoints+i*3+1);

         // Inner polygon:
         pPolygons[i*2+1].pPoints[0] = (r3s_point *)(pPoints+i*3+1);
         pPolygons[i*2+1].pPoints[1] = (r3s_point *)(pPoints+nNextPointSet*3+1);
         pPolygons[i*2+1].pPoints[2] = (r3s_point *)(pPoints+nNextPointSet*3+2);
         pPolygons[i*2+1].pPoints[3] = (r3s_point *)(pPoints+i*3+2);
      }
   }
}


// OK, this sucks. There are no lgd3d_g2 routines that can do iterated alpha.
// *sigh*. We'll have to splurf these into 3d polys.
void cFrostedEdgesEffect::Render(BOOL bIsScreenLocked)
{
   if (!g_lgd3d || bIsScreenLocked || !r3d_state.cur_con)
      return;

   ////////////////////////////
   // Setup:

   int nStride;
   BOOL bRestoreZWrite = lgd3d_is_zwrite_on();
   BOOL bRestoreZCompare = lgd3d_is_zcompare_on();

   lgd3d_set_zwrite(FALSE);
   lgd3d_set_zcompare(FALSE);

   lgd3d_set_fog_enable(FALSE);
   lgd3d_set_blend(TRUE);

   r3_set_clipmode(R3_NO_CLIP);
   r3_set_clip_flags(R3_CLIP_RGB);

   gr_set_fill_type(FILL_NORM);
   r3_set_prim(); // synch r3d and g2

   nStride = r3d_glob.cur_stride;
   r3d_glob.cur_stride = sizeof(goddamn_stupid_r3s_point);

   r3_set_polygon_context(R3_PL_POLYGON|R3_PL_SOLID|R3_PL_RGB_GOURAUD);
   r3_set_color(0xffffff);

   // Make sure our polys match current screen mode:
   ComputePolys();

   ////////////////////////////
   // Render:

   for (int i = 0; i < nNumPolygons; i++)
   {
      r3d_do_setup = TRUE;
      r3_draw_poly(4, pPolygons[i].pPoints);
   }


   ////////////////////////////
   // Cleanup:

   r3d_glob.cur_stride = nStride;
   lgd3d_set_zwrite(bRestoreZWrite);
   lgd3d_set_zcompare(bRestoreZCompare);

   lgd3d_set_blend(FALSE);
}



///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

// Smudge data is stored in text file that defines:
//    1) Texture name (lives in art\finals\camera\*.*)
//    2) If it's an alpha texture or not
//    3) Overall alpha
//    4) Overall color
//    5) Position on screen expressed as 0-1 values
//    6) X&Y Scale, expressed as 0-1
//    7) Rotation, expressed in degrees
//

struct sSmudgeData
{
   char Name[256];
   IRes *pRes;

   goddamn_stupid_r3s_point Points[4];
   float fAlpha;
   BOOL bIsAlphaTexture;
   mxs_vector Color;
   float fXScale;
   float fYScale;
   float fXPos;
   float fYPos;
   float fRotation; // in rads.
   int nPalIx;
};

class cSmudgesEffect : public IRemoteCameraEffect
{
public:
   cSmudgesEffect();

   virtual void Init();
   virtual void Term();
   virtual void OnFrame();
   virtual void Render(BOOL bIsScreenLocked);

private:
   void ReadSmudgeData();
   void ComputeSmudgeData();

   int nWidth; // Screen width and height
   int nHeight;

   float fOverallLighting;
   BOOL bRendered;

   sSmudgeData *pSmudges;
   int nNumSmudges;
};


cSmudgesEffect::cSmudgesEffect()
   : nWidth(-1), nHeight(-1), pSmudges(0), nNumSmudges(0), fOverallLighting(-1), bRendered(FALSE)
{}


void cSmudgesEffect::ReadSmudgeData()
{
   IRes *pSmudgeDataRes;
   char *pSmudgeData;
   char *pFound;
   char TempStr[256];

   IResMan *pResMan = AppGetObj(IResMan);

   pSmudgeDataRes = IResMan_Bind(pResMan, "camovl.txt", RESTYPE_BINARY, NULL, "camera\\", 0);
   if (!pSmudgeDataRes)
      return;

   pSmudgeData = (char *)pSmudgeDataRes->Lock();
   if (!pSmudgeData)
   {
      Warning(("Strange Error getting smudge data\n"));
      pSmudgeDataRes->Release();
      return;
   }

   nNumSmudges = 0;

   // It is expected that the smudge  fields are defined in this order:
   // Name [str]
   // IsAlpha [0 or 1]
   // AlphaLevel [0 to 1]
   // Color [r g b]
   // Scale [x:0->1, y:0->1]
   // SmudgePos [x:0->1, y:0->1]
   // SmudgeRotation [degrees]

   BOOL bFailed = FALSE;

   pFound = strstr(pSmudgeData, "SmudgeName");
   while (pFound)
   {
      if (!pSmudges)
         pSmudges = (sSmudgeData *)Malloc(sizeof(sSmudgeData));
      else
         pSmudges = (sSmudgeData *)Realloc(pSmudges, sizeof(sSmudgeData)*(nNumSmudges+1));
      memset(pSmudges+nNumSmudges, 0, sizeof(sSmudgeData));

      if (sscanf(pFound, "%s %s", TempStr, pSmudges[nNumSmudges].Name) != 2)
      {
         bFailed = TRUE;
         break;
      }

      pFound = strstr(pFound, "IsAlpha");
      if (!pFound)
      {
         bFailed = TRUE;
         break;
      }
      if (sscanf(pFound, "%s %d", TempStr, &pSmudges[nNumSmudges].bIsAlphaTexture) != 2)
      {
         bFailed = TRUE;
         break;
      }

      pFound = strstr(pFound, "AlphaLevel");
      if (!pFound)
      {
         bFailed = TRUE;
         break;
      }
      if (sscanf(pFound, "%s %f", TempStr, &pSmudges[nNumSmudges].fAlpha) != 2)
      {
         bFailed = TRUE;
         break;
      }

      pFound = strstr(pFound, "Color");
      if (!pFound)
      {
         bFailed = TRUE;
         break;
      }
      if (sscanf(pFound, "%s %f %f %f", TempStr, &pSmudges[nNumSmudges].Color.x, &pSmudges[nNumSmudges].Color.y, &pSmudges[nNumSmudges].Color.z) != 4)
      {
         bFailed = TRUE;
         break;
      }

      pFound = strstr(pFound, "Scale");
      if (!pFound)
      {
         bFailed = TRUE;
         break;
      }
      if (sscanf(pFound, "%s %f %f", TempStr, &pSmudges[nNumSmudges].fXScale, &pSmudges[nNumSmudges].fYScale) != 3)
      {
         bFailed = TRUE;
         break;
      }

      pFound = strstr(pFound, "SmudgePos");
      if (!pFound)
      {
         bFailed = TRUE;
         break;
      }
      if (sscanf(pFound, "%s %f %f", TempStr, &pSmudges[nNumSmudges].fXPos, &pSmudges[nNumSmudges].fYPos) != 3)
      {
         bFailed = TRUE;
         break;
      }

      pFound = strstr(pFound, "SmudgeRotation");
      if (!pFound)
      {
         bFailed = TRUE;
         break;
      }
      if (sscanf(pFound, "%s %f", TempStr, &pSmudges[nNumSmudges].fRotation) != 2)
      {
         bFailed = TRUE;
         break;
      }

      pSmudges[nNumSmudges].fRotation = pSmudges[nNumSmudges].fRotation*PI/180;

      nNumSmudges++;
      pFound = strstr(pFound, "SmudgeName");
   }

   // Just to make sure:
   if (bFailed)
      Warning(("Failed to read smudge data\n"));

   pSmudgeDataRes->Unlock();
   pSmudgeDataRes->Release();
}


void cSmudgesEffect::ComputeSmudgeData()
{
   int i,j;
   sScrnMode Mode;
   float fCenterX, fCenterY;
   mxs_vector Corners[4];
   mxs_vector RotCorners[4];
   int nPosX, nPosY;

   ScrnModeGet(&Mode);

   if ((Mode.w == nWidth) && (Mode.h == nHeight)) // Already computed
      return;

   nWidth = Mode.w;
   nHeight = Mode.h;

   IResMan *pResMan = AppGetObj(IResMan);

   // Ah, don't worry about aspect ratio. Who cares?

   for (i = 0; i < nNumSmudges; i++)
   {
      if (!pSmudges[i].pRes)
         pSmudges[i].pRes = IResMan_Bind(pResMan, pSmudges[i].Name, RESTYPE_IMAGE, NULL, "camera\\", 0);

      if (!pSmudges[i].pRes)
      {
         Warning(("Unable to load smudge %s\n", pSmudges[i].Name));
         continue;
      }

      Corners[0].x = -pSmudges[i].fXScale*0.50000;
      Corners[0].y = -pSmudges[i].fYScale*0.50000;
      Corners[1].x = pSmudges[i].fXScale*0.50000;
      Corners[1].y = Corners[0].y;
      Corners[2].x = Corners[1].x;
      Corners[2].y = pSmudges[i].fYScale*0.50000;
      Corners[3].x = Corners[0].x;
      Corners[3].y = Corners[2].y;

      fCenterX = nWidth*pSmudges[i].fXPos;
      fCenterY = nHeight*pSmudges[i].fYPos;

      for (j = 0; j < 4; j++)
      {
         mx_rot_z_vec(RotCorners+j, Corners+j, mx_rad2ang(pSmudges[i].fRotation));
         nPosX = fCenterX+RotCorners[j].x*nWidth;
         nPosX = __max(__min(nPosX, nWidth), 0);
         nPosY = fCenterY+RotCorners[j].y*nHeight;
         nPosY = __max(__min(nPosY, nHeight), 0);
         pSmudges[i].Points[j].grp.sx = fix_make(nPosX, 0);
         pSmudges[i].Points[j].grp.sy = fix_make(nPosY, 0);
         pSmudges[i].Points[j].grp.w = 1;
         pSmudges[i].Points[j].p.z = 1;
      }

      pSmudges[i].Points[0].grp.u = 0;
      pSmudges[i].Points[0].grp.v = 0;
      pSmudges[i].Points[1].grp.u = 1;
      pSmudges[i].Points[1].grp.v = 0;
      pSmudges[i].Points[2].grp.u = 1;
      pSmudges[i].Points[2].grp.v = 1;
      pSmudges[i].Points[3].grp.u = 0;
      pSmudges[i].Points[3].grp.v = 1;

      if (!pSmudges[i].bIsAlphaTexture)
      {
         IRes *pPalRes = IResMan_Bind(pResMan, IRes_GetName(pSmudges[i].pRes), RESTYPE_PALETTE,
                                NULL, "camera\\", 0);
         if (pPalRes)
         {
            // Okay, we've got the palette -- set it up and dump it
            pSmudges[i].nPalIx = palmgr_alloc_pal((uchar *)IRes_Lock(pPalRes));
            palmgr_update_pal_slot(pSmudges[i].nPalIx);
            pPalRes->Unlock();
            SafeRelease(pPalRes);
         }
         else
            pSmudges[i].nPalIx = -1;
      }
   }
}


void cSmudgesEffect::Init()
{
   ReadSmudgeData();
   fOverallLighting = -1;
   bRendered = FALSE;
}

void cSmudgesEffect::Term()
{
   if (pSmudges)
   {
      for (int i = 0; i < nNumSmudges; i++)
         if (pSmudges[i].pRes)
            SafeRelease(pSmudges[i].pRes);

      Free(pSmudges);
      pSmudges = 0;
      nNumSmudges = 0;
   }

   nWidth = -1;
   nHeight = -1;
   fOverallLighting = -1;
   bRendered = FALSE;
}



// We make this 256 just in case texture is wrong. Really, we only use the first 16
static ushort AlphaTable[256] =
{
   0x0fff, 0x1fff, 0x2fff, 0x3fff,
   0x4fff, 0x5fff, 0x6fff, 0x7fff,
   0x8fff, 0x9fff, 0xafff, 0xbfff,
   0xcfff, 0xdfff, 0xefff, 0xffff
};


// If we didn't render last frame, setup so we can compute new lighting next time we render:
void cSmudgesEffect::OnFrame()
{
   if (!bRendered)
      fOverallLighting = -1;
   bRendered = FALSE;
}

void cSmudgesEffect::Render(BOOL bIsScreenLocked)
{
   int i,j;
//   int r,g,b;
   size_t nStride;

   if (!g_lgd3d || bIsScreenLocked || !nNumSmudges)
      return;

   ComputeSmudgeData();

   bRendered = TRUE;

   if (fOverallLighting < 0) // Needs to be computed. We just do this once, when remote camera first activates.
      fOverallLighting = compute_object_lighting(PlayerCamera()->objid);

   grs_bitmap *pBitmap;
   r3s_point *(Points[4]);
   BOOL bRestoreZWrite = lgd3d_is_zwrite_on();
   BOOL bRestoreZCompare = lgd3d_is_zcompare_on();

   lgd3d_set_zwrite(FALSE);
   lgd3d_set_zcompare(FALSE);
   lgd3d_set_fog_enable(FALSE);
   lgd3d_set_blend(TRUE);
   lgd3d_disable_palette();

   r3_set_clipmode(R3_NO_CLIP);
   r3_set_clip_flags(R3_CLIP_UV|R3_CLIP_RGB);

   r3_set_polygon_context(R3_PL_POLYGON|R3_PL_TEXTURE|R3_PL_RGB_GOURAUD);

   nStride = r3d_glob.cur_stride;
   r3d_glob.cur_stride = sizeof(goddamn_stupid_r3s_point);

   r3_set_color(0xffffff);

   for (i = 0; i < nNumSmudges; i++)
   {
      if (!pSmudges[i].pRes)
         continue;

      pBitmap = (grs_bitmap *)pSmudges[i].pRes->Lock();
      if (!pBitmap)
         continue;

      if (pSmudges[i].bIsAlphaTexture) // Must be an alpha texture:
      {
         gr_set_fill_type(FILL_BLEND);
         lgd3d_set_alpha_pal(AlphaTable);
      }
      else // setup palette for resource, and mark bitmap as having transparency:
      {
         if (pSmudges[i].nPalIx < 0)
         {
            pSmudges[i].pRes->Unlock();
            continue;
         }
         gr_set_fill_type(FILL_NORM);
         pBitmap->align = pSmudges[i].nPalIx;
         pBitmap->flags|=BMF_TRANS;
      }

      lgd3d_set_alpha(pSmudges[i].fAlpha);

      r3_set_prim();

// We do this per-vertex now
//      r = pSmudges[i].Color.x*255*fOverallLighting;
//      g = pSmudges[i].Color.y*255*fOverallLighting;
//      b = pSmudges[i].Color.z*255*fOverallLighting;
//      r = __max(__min(r, 255), 0);
//      g = __max(__min(g, 255), 0);
//      b = __max(__min(b, 255), 0);

//      r3_set_color((r<<16)|(g<<8)|b);

      r3_set_texture(pBitmap);
      for (j = 0; j < 4; j++)
      {
         Points[j] = (r3s_point *)(pSmudges[i].Points+j);

         pSmudges[i].Points[j].grp.i = pSmudges[i].Color.x*fOverallLighting;
         pSmudges[i].Points[j].grp.h = pSmudges[i].Color.y*fOverallLighting;
         pSmudges[i].Points[j].grp.d = pSmudges[i].Color.z*fOverallLighting;
         pSmudges[i].Points[j].grp.i = __max(__min(pSmudges[i].Points[j].grp.i, 1), 0);
         pSmudges[i].Points[j].grp.h = __max(__min(pSmudges[i].Points[j].grp.h, 1), 0);
         pSmudges[i].Points[j].grp.d = __max(__min(pSmudges[i].Points[j].grp.d, 1), 0);
      }
      r3_draw_poly(4, Points);

      pSmudges[i].pRes->Unlock();
   }

   lgd3d_set_zwrite(bRestoreZWrite);
   lgd3d_set_zcompare(bRestoreZCompare);
   lgd3d_set_blend(FALSE);
   lgd3d_set_alpha(1);
   lgd3d_enable_palette();
   gr_set_fill_type(FILL_NORM);
   r3_set_prim();
   r3d_glob.cur_stride = nStride;
}



///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

static uchar *pCanvasBits = 0;
static int nBitmapRow = 0;

// Size in pixels:
#define CHIP_SIZE 16

// Half of CHIP_SIZE:
#define CHIP_PART_SIZE 8

// The chips are constructed from a circular element, by specifying which parts of circle are
// to be used, as broken into octants:
//
//    Single octants:
//    0 = 12 to 1:30
//    1 = 1:30 to 3
//    2 = 3 to 4:30
//    3 = 4:30 to 6
//    4 = 6 to 7:30
//    5 = 7:30 to 9
//    6 = 9 to 10:30
//    7 = 10:30 to 12
//
//    Quarters:
//    8  = 12 to 3
//    9  = 1:30 to 3:30
//    10 = 3 to 6
//    11 = 4:30 7:30
//    12 = 6 to 9
//    13 = 7:30 to 10:30
//    14 = 9 to 12
//    15 = 10:30 to 1:30
//
//    Halves:
//    16  = 12 to 6
//    17  = 1:30 to 6:30
//    18 = 3 to 9
//    19 = 4:30 10:30
//    20 = 6 to 12
//    21 = 7:30 to 1:30
//    22 = 9 to 3
//    23 = 10:30 to 4:30
//
//    24 = whole thing
//

struct sStartStop
{
   int nStartIx;
   int nEndIx;
};

struct sScreenPos
{
   float fX, fY;
};

class cChipData
{
public:
   void Render();

   float fXPos;
   float fYPos;
   int nProfile;

   sScreenPos SrcPos[CHIP_SIZE*CHIP_SIZE];

   // Row each row, what is first and last index.
   sStartStop StartStop[CHIP_SIZE];

   int nXUpperLeft;
   int nYUpperLeft;
};


// Assumes 16-bit rendering:
void cChipData::Render()
{
   int i,j;
   ushort *dst;
   ushort *src;

   for (i = 0; i < CHIP_SIZE; i++)
      for (j = StartStop[i].nStartIx; j <= StartStop[i].nEndIx; j++)
      {
         dst = (ushort *)&pCanvasBits[(nYUpperLeft+i)*nBitmapRow];
         src = (ushort *)&pCanvasBits[(int)(SrcPos[i*CHIP_SIZE+j].fY)*nBitmapRow];
         dst[nXUpperLeft+j] = src[(int)(SrcPos[i*CHIP_SIZE+j].fX)];
      }
}



class cChipsEffect : public IRemoteCameraEffect
{
public:
   cChipsEffect();

   virtual void Init();
   virtual void Term();
   virtual void OnFrame();
   virtual void Render(BOOL bIsScreenLocked);

private:
   void ReadChipData();
   void ComputeChipData();
   void ComputeChipBits(int ix, float fStartAng, float fStopAng);

   cChipData *pChips;
   int nNumChips;
   BOOL bComputedMainChip;
   int nWidth;
   int nHeight;

   BOOL bNoChips;
};


cChipsEffect::cChipsEffect()
   : pChips(0),
     nNumChips(0),
     nWidth(-1),
     nHeight(-1),
     bComputedMainChip(FALSE),
     bNoChips(FALSE)
{}



void cChipsEffect::ReadChipData()
{
   IRes *pChipDataRes;
   char *pChipData;
   char *pFound;
   char TempStr[256];

   IResMan *pResMan = AppGetObj(IResMan);

   pChipDataRes = IResMan_Bind(pResMan, "camovl.txt", RESTYPE_BINARY, NULL, "camera\\", 0);
   if (!pChipDataRes)
      return;

   pChipData = (char *)pChipDataRes->Lock();
   if (!pChipData)
   {
      Warning(("Strange Error getting chip data\n"));
      pChipDataRes->Release();
      return;
   }

   nNumChips = 0;

   // It is expected that the chip fields are defined in this order:
   // ChipProfile [0 to 24]
   // ChipPos [x:0->1, y:0->1]

   BOOL bFailed = FALSE;

   pFound = strstr(pChipData, "ChipProfile");
   while (pFound)
   {
      if (!pChips)
         pChips = (cChipData *)Malloc(sizeof(cChipData));
      else
         pChips = (cChipData *)Realloc(pChips, sizeof(cChipData)*(nNumChips+1));
      memset(pChips+nNumChips, 0, sizeof(cChipData));

      if (sscanf(pFound, "%s %d", TempStr, &pChips[nNumChips].nProfile) != 2)
      {
         bFailed = TRUE;
         break;
      }

      pFound = strstr(pFound, "ChipPos");
      if (!pFound)
      {
         bFailed = TRUE;
         break;
      }
      if (sscanf(pFound, "%s %f %f", TempStr, &pChips[nNumChips].fXPos, &pChips[nNumChips].fYPos) != 3)
      {
         bFailed = TRUE;
         break;
      }

      nNumChips++;
      pFound = strstr(pFound, "ChipProfile");
   }

   // Just to make sure:
   if (bFailed)
      Warning(("Failed to read chip data\n"));

   pChipDataRes->Unlock();
   pChipDataRes->Release();
}


struct sChipDot
{
   float fXOffset;
   float fYOffset;
};


static sChipDot MainChip[CHIP_SIZE*CHIP_SIZE];
// This is the part along y axis - left edge of upper-right quadrant. This gets rotated to form
// a symmetrical chip.
static const sChipDot ChipPart[CHIP_PART_SIZE] =
{
   {0,-0.250000},
   {0,-0.218750},
   {0,-0.187500},
   {0,-0.156250},
   {0,-0.125000},
   {0,-0.093750},
   {0,-0.006250},
   {0,-0.003125},
};


void cChipsEffect::ComputeChipBits(int ix, float fStartAng, float fStopAng)
{
   int i,j;
   float fCenterX;
   float fCenterY;
   float fAng;

   fStartAng = fStartAng*PI/180;
   fStopAng = fStopAng*PI/180;

   // First copy over from main chip directly:
      for (i = 0; i < CHIP_SIZE; i++) // For each column
      {
         fCenterX = CHIP_PART_SIZE - ((CHIP_SIZE-1) - i + 0.5);
         for (j = 0; j < CHIP_SIZE; j++) // For each row
         {
            fCenterY = CHIP_PART_SIZE - ((CHIP_SIZE-1) - j + 0.5);
            fAng = atan2(fCenterY,fCenterX);
            // Make sure it's positive
            fAng = fAng+TWOPI;

            // We expect ang to be clockwise from noon, so convert screen ang to this:
            fAng = (TWOPI-fAng)+PI*0.50000;

            // normalize it:
            fAng = fmod(fAng, TWOPI);

            if (fStopAng < TWOPI)
            {
               if ((fAng >= fStartAng) && (fAng <= fStopAng))
               {
                  pChips[ix].SrcPos[j*CHIP_SIZE+i].fX = MainChip[j*CHIP_SIZE+i].fXOffset;
                  pChips[ix].SrcPos[j*CHIP_SIZE+i].fY = MainChip[j*CHIP_SIZE+i].fYOffset;
               }
               else
               {
                  pChips[ix].SrcPos[j*CHIP_SIZE+i].fX = 0;
                  pChips[ix].SrcPos[j*CHIP_SIZE+i].fY = 0;
               }
            }
            else // we've wrapped.
            {
               if ((fAng >= fStartAng) || (fAng <= (fStopAng-TWOPI)))
               {
                  pChips[ix].SrcPos[j*CHIP_SIZE+i].fX = MainChip[j*CHIP_SIZE+i].fXOffset;
                  pChips[ix].SrcPos[j*CHIP_SIZE+i].fY = MainChip[j*CHIP_SIZE+i].fYOffset;
               }
               else
               {
                  pChips[ix].SrcPos[j*CHIP_SIZE+i].fX = 0;
                  pChips[ix].SrcPos[j*CHIP_SIZE+i].fY = 0;
               }
            }
         }
      }

   // Determine start and stop ix:
      for (i = 0; i < CHIP_SIZE; i++) // For each row
      {
         // from left:
         for (j = 0; j < CHIP_SIZE; j++)
            if ((fabs(pChips[ix].SrcPos[i*CHIP_SIZE+j].fX) > 0.001) || (fabs(pChips[ix].SrcPos[i*CHIP_SIZE+j].fY) > 0.001))
               break;
         pChips[ix].StartStop[i].nStartIx = j;

         // from right:
         for (j = CHIP_SIZE-1; j >= 0; j--)
            if ((fabs(pChips[ix].SrcPos[i*CHIP_SIZE+j].fX) > 0.001) || (fabs(pChips[ix].SrcPos[i*CHIP_SIZE+j].fY) > 0.001))
               break;
         pChips[ix].StartStop[i].nEndIx = j;
      }

   // Then determine screen coords, making sure we don't go off screen:
   pChips[ix].nXUpperLeft = pChips[ix].fXPos*nWidth;
   pChips[ix].nYUpperLeft = pChips[ix].fYPos*nHeight;

   if ((pChips[ix].nXUpperLeft+CHIP_SIZE) >= nWidth)
      pChips[ix].nXUpperLeft = nWidth-1-CHIP_SIZE;

   if ((pChips[ix].nYUpperLeft+CHIP_SIZE) >= nHeight)
      pChips[ix].nYUpperLeft = nHeight-1-CHIP_SIZE;

   // Now calc screen coords for chip:
   for (i = 0; i < CHIP_SIZE; i++)
      for (j = pChips[ix].StartStop[i].nStartIx; j <= pChips[ix].StartStop[i].nEndIx; j++)
      {
         int nRemainingWidth, nRemainingHeight;

         if (pChips[ix].SrcPos[i*CHIP_SIZE+j].fX < 0)
            nRemainingWidth = pChips[ix].nXUpperLeft+j;
         else
            nRemainingWidth = (nWidth-pChips[ix].nXUpperLeft+j);

         if (pChips[ix].SrcPos[i*CHIP_SIZE+j].fY < 0)
            nRemainingHeight = pChips[ix].nYUpperLeft+i;
         else
            nRemainingHeight = (nHeight-pChips[ix].nYUpperLeft+i);

         pChips[ix].SrcPos[i*CHIP_SIZE+j].fX = pChips[ix].nXUpperLeft+j+pChips[ix].SrcPos[i*CHIP_SIZE+j].fX*nRemainingWidth;
         pChips[ix].SrcPos[i*CHIP_SIZE+j].fX = __max(__min(pChips[ix].SrcPos[i*CHIP_SIZE+j].fX, nWidth-1), 1);

         pChips[ix].SrcPos[i*CHIP_SIZE+j].fY = pChips[ix].nYUpperLeft+i+pChips[ix].SrcPos[i*CHIP_SIZE+j].fY*nRemainingHeight;
         pChips[ix].SrcPos[i*CHIP_SIZE+j].fY = __max(__min(pChips[ix].SrcPos[i*CHIP_SIZE+j].fY, nHeight-1), 1);
      }
}


void cChipsEffect::ComputeChipData()
{
   if (!bComputedMainChip)
   {
      mxs_vector OffsetVec, DestVec;
      float fMaxDist;
      float fCenterX, fCenterY; // coords relative to center.
      float fXOff, fYOff;
      float fXOffLow, fYOffLow;
      float fXOffHigh, fYOffHigh;
      float fDist; // Distance from center of bitmap.
      float fWeight;
      float fAngle;
      int nHighIx, nLowIx;
      int i,j;

      bComputedMainChip = TRUE;
      memset(MainChip, 0, sizeof(sChipDot)*CHIP_SIZE*CHIP_SIZE);

   // We are given leftmost vertical strip in upper right quad. Generate rest of upper right quad:

      fMaxDist = sqrt((CHIP_PART_SIZE-0.5)*(CHIP_PART_SIZE-0.5)+0.25);

      for (i = 0; i < CHIP_PART_SIZE; i++) // For each column
      {
         fCenterX = i + 0.5;
         for (j = 0; j < CHIP_PART_SIZE; j++) // For each row
         {
            fCenterY = (CHIP_PART_SIZE-1) - j + 0.5;
            fDist = sqrt(fCenterX*fCenterX+fCenterY*fCenterY);
            if (fDist > fMaxDist) // too far, it's 0.
            {
               MainChip[j*CHIP_SIZE + CHIP_PART_SIZE + i].fXOffset = 0;
               MainChip[j*CHIP_SIZE + CHIP_PART_SIZE + i].fYOffset = 0;
               continue;
            }

            // OK, we need to calc rotate value for this element.
            // First grab nearest strip elements, and do weighted average based on distance
            nHighIx = ceil(fDist-0.5);
            nLowIx = floor(fDist-0.5);
            if (nHighIx < 0)
               nHighIx = 0;
            if (nLowIx < 0)
               nLowIx = 0;
            if (nHighIx >= CHIP_PART_SIZE)
               nHighIx = CHIP_PART_SIZE-1;
            if (nLowIx >= CHIP_PART_SIZE)
               nLowIx = CHIP_PART_SIZE-1;

            // A weight of 0 is all HighIx, a weight of 1 is all LowIx
            fWeight = (nHighIx+0.5)-fDist;

            // Apply weight to make new offsets
            fXOffLow = ChipPart[CHIP_PART_SIZE-1-nLowIx].fXOffset;
            fYOffLow = ChipPart[CHIP_PART_SIZE-1-nLowIx].fYOffset;
            fXOffHigh = ChipPart[CHIP_PART_SIZE-1-nHighIx].fXOffset;
            fYOffHigh = ChipPart[CHIP_PART_SIZE-1-nHighIx].fYOffset;

            fXOff = fXOffHigh + (fXOffHigh-fXOffLow)*fWeight;
            fYOff = fYOffHigh + (fYOffHigh-fYOffLow)*fWeight;

            // OK, now rotate these based on element's angle around center, relative to vertical strip (y axis).
            fAngle = -(PI/2-atan2(fCenterY, fCenterX));

            OffsetVec.x = fXOff;
            OffsetVec.y = fYOff;
            mx_rot_z_vec(&DestVec, &OffsetVec, mx_rad2ang(fAngle));
            fXOff = -DestVec.x; // make sign negative, since x and y signs are reversed for screen coords.
            fYOff = DestVec.y;

            MainChip[j*CHIP_SIZE + CHIP_PART_SIZE + i].fXOffset = fXOff;
            MainChip[j*CHIP_SIZE + CHIP_PART_SIZE + i].fYOffset = fYOff;
         }
      }

      // Right. Now mirror this quad to fill out all four quads.

      // First flip upper right to upper left, which reverses x offsets.
      for (i = 0; i < CHIP_PART_SIZE; i++) // rows
         for (j = 0; j < CHIP_PART_SIZE; j++) // cols
         {
            MainChip[i*CHIP_SIZE + (CHIP_PART_SIZE-1-j)] = MainChip[i*CHIP_SIZE + CHIP_PART_SIZE + j];
            MainChip[i*CHIP_SIZE + (CHIP_PART_SIZE-1-j)].fXOffset = -MainChip[i*CHIP_SIZE + (CHIP_PART_SIZE-1-j)].fXOffset;
         }

      // Then reverse top to bottom, which reverses y offsets.
      for (i = 0; i < CHIP_PART_SIZE; i++) // rows
         for (j = 0; j < CHIP_SIZE; j++) // cols
         {
            MainChip[(i+CHIP_PART_SIZE)*CHIP_SIZE + j] = MainChip[(CHIP_PART_SIZE-1-i)*CHIP_SIZE + j];
            MainChip[(i+CHIP_PART_SIZE)*CHIP_SIZE + j].fYOffset = -MainChip[(i+CHIP_PART_SIZE)*CHIP_SIZE + j].fYOffset;
         }

/*
mprintf("MainChip: \n");
for (i = 0; i < CHIP_SIZE; i++)
{
   for (j = 0; j < CHIP_SIZE; j++)
      mprintf("   (%.3g, %.3g)", MainChip[i*CHIP_SIZE+j].fXOffset, MainChip[i*CHIP_SIZE+j].fYOffset);
   mprintf("\n");
}
*/
   }


   sScrnMode Mode;
   ScrnModeGet(&Mode);

   if ((Mode.w == nWidth) && (Mode.h == nHeight)) // Already computed
      return;

   nWidth = Mode.w;
   nHeight = Mode.h;

   // Compute profiles:
   for (int i = 0; i < nNumChips; i++)
   {
      float fStartAng, fStopAng; // 12 oclock is 0, 3 is 90, etc.

      if (pChips[i].nProfile < 8) // octants
      {
         fStartAng = 45*pChips[i].nProfile;
         fStopAng = fStartAng+45;
      }
      else if (pChips[i].nProfile < 16) // quarters
      {
         fStartAng = 45*(pChips[i].nProfile-8);
         fStopAng = fStartAng+90;
      }
      else if (pChips[i].nProfile < 24) // halves
      {
         fStartAng = 45*(pChips[i].nProfile-16);
         fStopAng = fStartAng+180;
      }
      else // is whole chip
      {
         fStartAng = 0;
         fStopAng = 360;
      }

      ComputeChipBits(i, fStartAng, fStopAng);

/*
mprintf("ChipBits(%d): \n", i);
for (int j = 0; j < CHIP_SIZE; j++)
{
   for (int k = 0; k < CHIP_SIZE; k++)
      mprintf("   (%4.0g, %4.0g)", pChips[i].SrcPos[j*CHIP_SIZE+k].fX, pChips[i].SrcPos[j*CHIP_SIZE+k].fY);
   mprintf("\n");
}
*/
   }
}



void cChipsEffect::Init()
{
   ReadChipData();
   bNoChips = config_is_defined("ReducedRemoteCameraEffects");
}

void cChipsEffect::OnFrame()
{
}

void cChipsEffect::Term()
{
   if (pChips)
   {
      Free(pChips);
      pChips = 0;
      nNumChips = 0;
   }

   nWidth = -1;
   nHeight = -1;
}


void cChipsEffect::Render(BOOL bIsScreenLocked)
{
   if (!g_lgd3d || !nNumChips || bNoChips)
      return;

   ComputeChipData();

   if (!grd_canvas)
      return;

   // This is EVIL!
   if (!bIsScreenLocked)
      ScrnLockDrawCanvas();

   pCanvasBits = grd_canvas->bm.bits;
   if (!pCanvasBits)
      return;

   nBitmapRow = grd_canvas->bm.row;

   for (int i = 0; i < nNumChips; i++)
      pChips[i].Render();

   if (!bIsScreenLocked)
      ScrnUnlockDrawCanvas();
}


///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////



static cFrostedEdgesEffect FrostedEdges;
static cSmudgesEffect Smudges;
static cChipsEffect Chips;
static IRemoteCameraEffect *(Effects[]) =
{
   &Chips,
   &Smudges,
   &FrostedEdges,
   0
};


static mxs_angvec FacingAng;

extern "C"
void CameraOverlayInit()
{
   mxs_matrix FacingMat;
   int i = 0;
   while(Effects[i])
   {
      Effects[i]->Init();
      i++;
   }

   mx_identity_mat(&FacingMat);
   mx_mat2ang(&FacingAng, &FacingMat);
}

extern "C"
void CameraOverlayTerm()
{
   int i = 0;
   while(Effects[i])
   {
      Effects[i]->Term();
      i++;
   }
}

extern "C"
// Called before any rendering
void CameraOverlayOnFrame()
{
   int i = 0;
   while(Effects[i])
   {
      Effects[i]->OnFrame();
      i++;
   }
}

extern "C"
void CameraOverlayRender(BOOL bIsScreenLocked)
{
   int i = 0;
   mxs_vector OriginPos={0,0,0};

   if (!bIsScreenLocked)
   {
      // Wow if we could just do lgd3d_g2 stuff, then we could avoid all this.
      r3_start_frame();
      r3_set_view_angles(&OriginPos, &FacingAng, R3_DEFANG);
      r3_set_space(R3_CLIPPING_SPACE);

      while(Effects[i])
      {
         Effects[i]->Render(bIsScreenLocked);
         i++;
      }

      r3_end_frame();
   }
   else
   {
      while(Effects[i])
      {
         Effects[i]->Render(bIsScreenLocked);
         i++;
      }
   }
}

