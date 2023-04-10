/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/distobj.cpp,v 1.6 2000/02/21 18:49:52 bfarquha Exp $


// Distant art rendering

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

#include <command.h>

#include <math.h>
#include <stdlib.h>

#include <skyrend.h>
#include <distobj.h>
#include <memall.h>
#include <dbmem.h>   // must be last header!


extern "C" BOOL g_lgd3d;
extern "C" BOOL portal_fog_on;


struct goddamn_stupid_r3s_point
{
   mxs_vector p;
   ulong ccodes;
   g2s_point grp;
};

static goddamn_stupid_r3s_point *pPoints = 0;
static mxs_vector *pPointVecs = 0;
static r3s_phandle *vlist = 0; // Point transforming/rendering
static int nTotalPoints = 0;

static IRes *pDistantArtRes1 = 0;
static IRes *pDistantArtRes2 = 0;
static int nPalIx1 = 0;
static int nPalIx2 = 0;


#define PI 3.14159265359
#define fDeg90 (0.50000*PI)
#define fDeg180 (PI)
#define fDeg360 (2*PI)

#define DEG (PI/180)

// This describes the sky rendering of the mission.
struct sMissionDistantObj
{
   BOOL bUseDistantArt;
   BOOL bEnableFog;
   char Texture1Name[256];
   char Texture2Name[256];
   mxs_vector Color;
   float fDistance;
   float fTopLatAngle;
   float fBottomLatAngle;
   int nNumPanels;
   int nNumTexturePanels;
   float fAlpha;
};

// Here's my descriptor, which identifies my stuff to the tag file & editor
sFileVarDesc g_DistantObjFileVarDesc =
{
   kMissionVar,         // Where do I get saved?
   "DISTOBJVAR",        // Tag file tag
   "NewSky:DistantArt",           // friendly name
   FILEVAR_TYPE(sMissionDistantObj),// Type (for editing)
   { 1, 2},             // version
   { 1, 2},             // last valid version
};


// the class for actual global variable
class cMissionDistantObj : public cFileVar<sMissionDistantObj,&g_DistantObjFileVarDesc>
{};

// the thing itself
static cMissionDistantObj g_DistantObj;

static sFieldDesc g_aDistantObjField[] =
{
   { "Enable Distant Art", kFieldTypeBool,
     FieldLocation(sMissionDistantObj, bUseDistantArt), },
   { "Enable Fog", kFieldTypeBool,
     FieldLocation(sMissionDistantObj, bEnableFog), },
   { "Texture #1", kFieldTypeString,
     FieldLocation(sMissionDistantObj, Texture1Name), },
   { "Texture #2", kFieldTypeString,
     FieldLocation(sMissionDistantObj, Texture2Name), },
   { "Color", kFieldTypeVector,
     FieldLocation(sMissionDistantObj, Color), },
   { "Distance", kFieldTypeFloat,
     FieldLocation(sMissionDistantObj, fDistance), },
   { "Top Latitutde", kFieldTypeFloat,
     FieldLocation(sMissionDistantObj, fTopLatAngle), },
   { "Bottom Latitutde", kFieldTypeFloat,
     FieldLocation(sMissionDistantObj, fBottomLatAngle), },
   { "# of panels", kFieldTypeInt,
     FieldLocation(sMissionDistantObj, nNumPanels), },
   { "# of panels/texture", kFieldTypeInt,
     FieldLocation(sMissionDistantObj, nNumTexturePanels), },
   { "Alpha", kFieldTypeFloat,
     FieldLocation(sMissionDistantObj, fAlpha), },
};

static sStructDesc g_SDesc = StructDescBuild(sMissionDistantObj, kStructFlagNone,
                                             g_aDistantObjField);

static inline void SetDefaults()
{
   g_DistantObj.bUseDistantArt = FALSE;
   g_DistantObj.bEnableFog = FALSE;
   g_DistantObj.Texture1Name[0] = 0;
   g_DistantObj.Texture2Name[0] = 0;
   g_DistantObj.Color.x = g_DistantObj.Color.y = g_DistantObj.Color.z = 1;
   g_DistantObj.fDistance = 1000;
   g_DistantObj.fTopLatAngle = 80;
   g_DistantObj.fBottomLatAngle = 110;
   g_DistantObj.nNumPanels = 24;
   g_DistantObj.nNumTexturePanels = 6;
}


static void DistantArtInstallCommands();


static BOOL bDidAppInit = FALSE;


void cDistantArt::AppInit()
{
   if (!bDidAppInit)
   {
      AutoAppIPtr_(StructDescTools, pTools);
      pTools->Register(&g_SDesc);
      DistantArtInstallCommands();
      bDidAppInit = TRUE;
   }

   SetDefaults();
}


static BOOL TestValues()
{
   BOOL bFailure = FALSE;

   if (g_DistantObj.fDistance < 0.001)
   {
      Warning(("Invalid distant art distance %g\n", g_DistantObj.fDistance));
      bFailure = TRUE;
   }

   if (g_DistantObj.fTopLatAngle < 0)
   {
      Warning(("Invalid distant art top lat angle %g\n", g_DistantObj.fTopLatAngle));
      bFailure = TRUE;
   }

   if ((g_DistantObj.fBottomLatAngle < 0) || (g_DistantObj.fBottomLatAngle < g_DistantObj.fTopLatAngle))
   {
      Warning(("Invalid distant art bottom lat angle %g\n", g_DistantObj.fBottomLatAngle));
      bFailure = TRUE;
   }

   if (g_DistantObj.nNumPanels < 3)
   {
      Warning(("Invalid distant art number of panels %d\n", g_DistantObj.nNumPanels));
      bFailure = TRUE;
   }

   if (g_DistantObj.nNumTexturePanels < 1)
   {
      Warning(("Invalid distant art number of texture panels %d\n", g_DistantObj.nNumTexturePanels));
      bFailure = TRUE;
   }

   return bFailure;
}


static BOOL bDidGameInit = FALSE;

//
// Call after mission parms are loaded.
// Calc panel points and uv's.
//
void cDistantArt::Init()
{
   int i;
   float fLongAng;
   mxs_vector ZVec=  {0,0,1};
   mxs_vector Temp;

   if (!g_DistantObj.bUseDistantArt || !SkyRendererUseEnhanced())
      return;

   if (TestValues())
      return;

   IResMan *pResMan = AppGetObj(IResMan);
   pDistantArtRes1 = IResMan_Bind(pResMan, g_DistantObj.Texture1Name, RESTYPE_IMAGE, NULL, "fam\\skyhw\\", 0);
   if (!pDistantArtRes1)
      return;

   if (g_DistantObj.Texture2Name[0])
      pDistantArtRes2 = IResMan_Bind(pResMan, g_DistantObj.Texture2Name, RESTYPE_IMAGE, NULL, "fam\\skyhw\\", 0);

   // Load palette
   IRes *pPalRes = IResMan_Bind(pResMan, IRes_GetName(pDistantArtRes1), RESTYPE_PALETTE,
                          NULL, "fam\\skyhw\\", 0);
   if (pPalRes)
   {
      // Okay, we've got the palette -- set it up and dump it
      nPalIx1 = palmgr_alloc_pal((uchar *)IRes_Lock(pPalRes));
      pPalRes->Unlock();
      SafeRelease(pPalRes);
   }

   if (pDistantArtRes2)
   {
      pPalRes = IResMan_Bind(pResMan, IRes_GetName(pDistantArtRes2), RESTYPE_PALETTE,
                             NULL, "fam\\skyhw\\", 0);
      if (pPalRes)
      {
         // Okay, we've got the palette -- set it up and dump it
         nPalIx2 = palmgr_alloc_pal((uchar *)IRes_Lock(pPalRes));
         pPalRes->Unlock();
         SafeRelease(pPalRes);
      }
   }


   nTotalPoints = 2*g_DistantObj.nNumPanels;

   pPoints = new goddamn_stupid_r3s_point[nTotalPoints];
   vlist = new r3s_phandle[nTotalPoints];
   pPointVecs = new mxs_vector[nTotalPoints];

   for (i = 0; i < nTotalPoints; i++)
      vlist[i] = (r3s_point*)(pPoints+i);

   for (i = 0; i < g_DistantObj.nNumPanels; i++)
   {
      fLongAng = -PI*2*i/(float)g_DistantObj.nNumPanels;

      // top:
      mx_rot_y_vec(&Temp, &ZVec, mx_rad2ang(g_DistantObj.fTopLatAngle*DEG));
      mx_rot_z_vec(pPointVecs+i*2, &Temp, mx_rad2ang(fLongAng));

      // bottom:
      mx_rot_y_vec(&Temp, &ZVec, mx_rad2ang(g_DistantObj.fBottomLatAngle*DEG));
      mx_rot_z_vec(pPointVecs+i*2+1, &Temp, mx_rad2ang(fLongAng));

      // top:
      pPoints[i*2].grp.v = 0;

      // bottom:
      pPoints[i*2+1].grp.v = 1;
   }

   bDidGameInit = TRUE;
}


void cDistantArt::Term()
{
   SafeRelease(pDistantArtRes1);
   pDistantArtRes1 = 0;
   SafeRelease(pDistantArtRes2);
   pDistantArtRes2 = 0;

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


// Interferes with database update
//   SetDefaults();

   bDidGameInit = FALSE;
}



EXTERN BOOL r3d_do_setup;

void cDistantArt::Render()
{
   if (!g_lgd3d || !bDidGameInit)
      return;

   int i;
   int nStride;
   int nLastLongIx;
   r3s_phandle PolyPoints[4];
   mxs_vector OldPos;
   mxs_vector ZeroPos = {0,0,0};
   grs_bitmap *pBitmap1 = 0;
   grs_bitmap *pBitmap2 = 0;
   int nMod;

   /////////////////////////////////////////////////////////////////
   // Do Setup

   pBitmap1 = (grs_bitmap *)pDistantArtRes1->Lock();
   if (!pBitmap1)
   {
      Warning(("Distant Art texture #1 not found\n"));
      return;
   }

   pBitmap1->align = nPalIx1;
   pBitmap1->flags|=BMF_TRANS;

   int nSkipCount = 1;

   if (pDistantArtRes2)
   {
      pBitmap2 = (grs_bitmap *)pDistantArtRes2->Lock();
      if (!pBitmap2)
         Warning(("Distant Art texture #2 not found\n"));
      else
      {
         nSkipCount = 2;
         pBitmap2->align = nPalIx2;
         pBitmap2->flags|=BMF_TRANS;
      }
   }


   // First we render all the Bitmap1 panels, then all the Bitmap2 panels, so we don't switch contexts all the time.


   BOOL bRestoreZWrite = lgd3d_is_zwrite_on();
   BOOL bRestoreZCompare = lgd3d_is_zcompare_on();
   lgd3d_set_zwrite(FALSE);
   lgd3d_set_zcompare(FALSE);
   lgd3d_disable_palette();
   lgd3d_set_blend(TRUE);

   r3e_space nOldSpace = r3_get_space();
   r3_set_space(R3_CLIPPING_SPACE);

   OldPos = *r3_get_view_pos();
   r3_set_view(&ZeroPos);

   r3_start_block();
   r3_set_clipmode(R3_CLIP);
   r3_set_clip_flags(R3_CLIP_UV|R3_CLIP_RGB);

   gr_set_fill_type(FILL_NORM);
   r3_set_prim(); // synch r3d and g2

   nStride = r3d_glob.cur_stride;
   r3d_glob.cur_stride = sizeof(goddamn_stupid_r3s_point);

   lgd3d_set_fog_enable(portal_fog_on && g_DistantObj.bEnableFog);

   r3_set_polygon_context(R3_PL_POLYGON|R3_PL_TEXTURE|R3_PL_RGB_GOURAUD);
   r3_set_color(0xffffff);

   r3_transform_block(nTotalPoints, (r3s_point *)pPoints, pPointVecs);

   r3_set_texture(pBitmap1);

   lgd3d_set_alpha(g_DistantObj.fAlpha);
   lgd3d_set_texture_wrapping(0, FALSE);

   int nCycle;

   for (i = 0; i < g_DistantObj.nNumPanels; i++)
   {
      nCycle = i/g_DistantObj.nNumTexturePanels;
      if (nCycle%nSkipCount)
         continue;

      nLastLongIx = (i+1)%g_DistantObj.nNumPanels;

      PolyPoints[0] = vlist[i*2];
      PolyPoints[1] = vlist[nLastLongIx*2];
      PolyPoints[2] = vlist[nLastLongIx*2+1];
      PolyPoints[3] = vlist[i*2+1];

      r3d_ccodes_and = 0;
      r3_code_polygon(4, PolyPoints);
      if (!r3d_ccodes_and) // Visible
      {
         ((goddamn_stupid_r3s_point *)(PolyPoints[0]))->grp.i = g_DistantObj.Color.x;
         ((goddamn_stupid_r3s_point *)(PolyPoints[0]))->grp.h = g_DistantObj.Color.y;
         ((goddamn_stupid_r3s_point *)(PolyPoints[0]))->grp.d = g_DistantObj.Color.z;

         ((goddamn_stupid_r3s_point *)(PolyPoints[1]))->grp.i = g_DistantObj.Color.x;
         ((goddamn_stupid_r3s_point *)(PolyPoints[1]))->grp.h = g_DistantObj.Color.y;
         ((goddamn_stupid_r3s_point *)(PolyPoints[1]))->grp.d = g_DistantObj.Color.z;

         ((goddamn_stupid_r3s_point *)(PolyPoints[2]))->grp.i = g_DistantObj.Color.x;
         ((goddamn_stupid_r3s_point *)(PolyPoints[2]))->grp.h = g_DistantObj.Color.y;
         ((goddamn_stupid_r3s_point *)(PolyPoints[2]))->grp.d = g_DistantObj.Color.z;

         ((goddamn_stupid_r3s_point *)(PolyPoints[3]))->grp.i = g_DistantObj.Color.x;
         ((goddamn_stupid_r3s_point *)(PolyPoints[3]))->grp.h = g_DistantObj.Color.y;
         ((goddamn_stupid_r3s_point *)(PolyPoints[3]))->grp.d = g_DistantObj.Color.z;

         PolyPoints[0]->grp.u = (i%g_DistantObj.nNumTexturePanels)/(float)g_DistantObj.nNumTexturePanels;
         nMod = (i+1)%g_DistantObj.nNumTexturePanels;
         if (!nMod) // we've wrapped. Set to 1, rather than 0
            PolyPoints[1]->grp.u = 1;
         else
            PolyPoints[1]->grp.u = nMod/(float)g_DistantObj.nNumTexturePanels;

         PolyPoints[2]->grp.u = PolyPoints[1]->grp.u;
         PolyPoints[3]->grp.u = PolyPoints[0]->grp.u;

         r3d_do_setup = TRUE;
         r3_draw_poly(4, PolyPoints);
      }
   }


   // Now for optional texture2:
   if (pBitmap2)
   {
      r3_set_texture(pBitmap2);
      for (i = 0; i < g_DistantObj.nNumPanels; i++)
      {
         nCycle = i/g_DistantObj.nNumTexturePanels;
         if (!(nCycle%nSkipCount))
            continue;

         nLastLongIx = (i+1)%g_DistantObj.nNumPanels;

         PolyPoints[0] = vlist[i*2];
         PolyPoints[1] = vlist[nLastLongIx*2];
         PolyPoints[2] = vlist[nLastLongIx*2+1];
         PolyPoints[3] = vlist[i*2+1];

         r3d_ccodes_and = 0;
         r3_code_polygon(4, PolyPoints);
         if (!r3d_ccodes_and) // Visible
         {
            ((goddamn_stupid_r3s_point *)(PolyPoints[0]))->grp.i = g_DistantObj.Color.x;
            ((goddamn_stupid_r3s_point *)(PolyPoints[0]))->grp.h = g_DistantObj.Color.y;
            ((goddamn_stupid_r3s_point *)(PolyPoints[0]))->grp.d = g_DistantObj.Color.z;

            ((goddamn_stupid_r3s_point *)(PolyPoints[1]))->grp.i = g_DistantObj.Color.x;
            ((goddamn_stupid_r3s_point *)(PolyPoints[1]))->grp.h = g_DistantObj.Color.y;
            ((goddamn_stupid_r3s_point *)(PolyPoints[1]))->grp.d = g_DistantObj.Color.z;

            ((goddamn_stupid_r3s_point *)(PolyPoints[2]))->grp.i = g_DistantObj.Color.x;
            ((goddamn_stupid_r3s_point *)(PolyPoints[2]))->grp.h = g_DistantObj.Color.y;
            ((goddamn_stupid_r3s_point *)(PolyPoints[2]))->grp.d = g_DistantObj.Color.z;

            ((goddamn_stupid_r3s_point *)(PolyPoints[3]))->grp.i = g_DistantObj.Color.x;
            ((goddamn_stupid_r3s_point *)(PolyPoints[3]))->grp.h = g_DistantObj.Color.y;
            ((goddamn_stupid_r3s_point *)(PolyPoints[3]))->grp.d = g_DistantObj.Color.z;

            PolyPoints[0]->grp.u = (i%g_DistantObj.nNumTexturePanels)/(float)g_DistantObj.nNumTexturePanels;
            nMod = (i+1)%g_DistantObj.nNumTexturePanels;
            if (!nMod) // we've wrapped. Set to 1, rather than 0
               PolyPoints[1]->grp.u = 1;
            else
               PolyPoints[1]->grp.u = nMod/(float)g_DistantObj.nNumTexturePanels;

            PolyPoints[2]->grp.u = PolyPoints[1]->grp.u;
            PolyPoints[3]->grp.u = PolyPoints[0]->grp.u;

            r3d_do_setup = TRUE;
            r3_draw_poly(4, PolyPoints);
         }
      }
   }

   r3d_glob.cur_stride = nStride;
   lgd3d_enable_palette();

   r3_end_block();

   r3_set_view(&OldPos);

   lgd3d_set_zwrite(bRestoreZWrite);
   lgd3d_set_zcompare(bRestoreZCompare);
   lgd3d_set_blend(FALSE);
   lgd3d_set_texture_wrapping(0, TRUE);
   lgd3d_set_fog_enable(portal_fog_on);

   r3_set_space(nOldSpace);

   pDistantArtRes1->Unlock();
   if (pBitmap2)
      pDistantArtRes2->Unlock();
}



#if 0
static Command g_DistantArtCommands[] =
{
   { "sky_setlong", FUNC_INT, SetDebugLong, "Debug Sky Longitude", HK_ALL},
   { "sky_setlat",  FUNC_INT, SetDebugLat,  "Debug Sky Latitude", HK_ALL},
};
#endif



static void DistantArtInstallCommands()
{
#if 0
#ifndef SHIP
   COMMANDS(g_DistantArtCommands, HK_ALL);
#endif
#endif
}
