/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/celstobj.cpp,v 1.10 2000/03/20 16:11:02 bfarquha Exp $

//
// Sun/Moon, etc rendering. We can have only one celestial object in the sky at a time.
// A celestial object may be based on a flat16 texture (for the moon, say), or a palettized
// texture where only the first 16 entries are used, from black to white, and they represent
// alpha levels (for the sun with it's glare).
//


#include <lg.h>
#include <lgd3d.h>
#include <r3d.h>
#include <config.h>
#include <filevar.h>
#include <sdesbase.h>
#include <sdesc.h>
#include <resapi.h>
#include <imgrstyp.h>
#include <palrstyp.h>

#include <command.h>

#include <palmgr.h>

#include <math.h>
#include <stdlib.h>

#include <celstobj.h>

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
struct sMissionCelestialObj
{
   BOOL bUseCelestialObject;
   BOOL bEnableFog;
   BOOL bIsAlphaTexture;
   char TextureName[256];
   float fAlpha; // it's overall alpha level
   float fOffset; // it's offset (positive means closer to camera) toward camera from sky dist.
   float fAng; // it's angular size, in degrees, along diagnal.
   float fLatAng; // in degrees
   float fLongAng; // in degrees
   float fRotation; // in degrees
   mxs_vector Color; // It's color
};


// We store a max of three seperate objects:
#define MAX_CEL_OBJECTS 3



// Here's my descriptor, which identifies my stuff to the tag file & editor

sFileVarDesc g_CelestialObjFileVarDesc1 =
   {
      kMissionVar,         // Where do I get saved?
      "CELOBJVAR1",        // Tag file tag
      "NewSky:Celestial1",           // friendly name
      FILEVAR_TYPE(sMissionCelestialObj),// Type (for editing)
      { 1, 2},             // version
      { 1, 0},             // last valid version
   };

sFileVarDesc g_CelestialObjFileVarDesc2 =
   {
      kMissionVar,         // Where do I get saved?
      "CELOBJVAR2",        // Tag file tag
      "NewSky:Celestial2",           // friendly name
      FILEVAR_TYPE(sMissionCelestialObj),// Type (for editing)
      { 1, 2},             // version
      { 1, 0},             // last valid version
   };


sFileVarDesc g_CelestialObjFileVarDesc3 =
   {
      kMissionVar,         // Where do I get saved?
      "CELOBJVAR3",        // Tag file tag
      "NewSky:Celestial3",           // friendly name
      FILEVAR_TYPE(sMissionCelestialObj),// Type (for editing)
      { 1, 2},             // version
      { 1, 0},             // last valid version
   };

template <class TYPE>
class cMyFileVar : public TYPE, public cFileVarBase
{
   TYPE& Var() { return *this; };
public:
   cMyFileVar(const sFileVarDesc*pDesc) { mDesc = pDesc; mSize = sizeof(TYPE), mBlock = &Var(); }
};

// the class for actual global variables
class cMissionCelestialObj : public cMyFileVar<sMissionCelestialObj>
{
public:
   cMissionCelestialObj(const sFileVarDesc*pDesc)
      : cMyFileVar<sMissionCelestialObj>(pDesc)
   {}
};

static cMissionCelestialObj CelestialObj0(&g_CelestialObjFileVarDesc1);
static cMissionCelestialObj CelestialObj1(&g_CelestialObjFileVarDesc2);
static cMissionCelestialObj CelestialObj2(&g_CelestialObjFileVarDesc3);
static sMissionCelestialObj CelestialObj3; // Actually not saved in tag file


static sFieldDesc g_aCelestialObjField[] =
{
   { "Enable Celestial object", kFieldTypeBool,
     FieldLocation(sMissionCelestialObj, bUseCelestialObject), },
   { "Enable Fog", kFieldTypeBool,
     FieldLocation(sMissionCelestialObj, bEnableFog), },
   { "Is Alpha Texture", kFieldTypeBool,
     FieldLocation(sMissionCelestialObj, bIsAlphaTexture), },
   { "Texture", kFieldTypeString,
     FieldLocation(sMissionCelestialObj, TextureName), },
   { "Alpha", kFieldTypeFloat,
     FieldLocation(sMissionCelestialObj, fAlpha), },
   { "Celestial Offset", kFieldTypeFloat,
     FieldLocation(sMissionCelestialObj, fOffset), },
   { "Angular Size", kFieldTypeFloat,
     FieldLocation(sMissionCelestialObj, fAng), },
   { "Latitude", kFieldTypeFloat,
     FieldLocation(sMissionCelestialObj, fLatAng), },
   { "Longitude", kFieldTypeFloat,
     FieldLocation(sMissionCelestialObj, fLongAng), },
   { "Rotation", kFieldTypeFloat,
     FieldLocation(sMissionCelestialObj, fRotation), },
   { "Color", kFieldTypeVector,
     FieldLocation(sMissionCelestialObj, Color), },
};

static sStructDesc g_SDesc = StructDescBuild(sMissionCelestialObj, kStructFlagNone,
                                             g_aCelestialObjField);


inline void cCelestialObject::SetDefaults()
{
   pCelestialObj->bUseCelestialObject = FALSE;
   pCelestialObj->bEnableFog = FALSE;
   pCelestialObj->bIsAlphaTexture = FALSE;
   pCelestialObj->TextureName[0] = 0;
   pCelestialObj->fAlpha = 1;
   pCelestialObj->fOffset = 0;
   pCelestialObj->fAng = 5;
   pCelestialObj->fLatAng = 0;
   pCelestialObj->fLongAng = 0;
   pCelestialObj->fRotation = 0;
   pCelestialObj->Color.x = pCelestialObj->Color.y =pCelestialObj->Color.z = 1;
}


inline void cCelestialObject::SetEasterEggDefaults()
{
   pCelestialObj->bUseCelestialObject = TRUE;
   pCelestialObj->bEnableFog = TRUE;
   pCelestialObj->bIsAlphaTexture = FALSE;
   strcpy(pCelestialObj->TextureName,"smooth.pcx");
   pCelestialObj->fAlpha = 0.4;
   pCelestialObj->fOffset = 0;
   pCelestialObj->fAng = 30;
   pCelestialObj->fLatAng = 75;
   pCelestialObj->fLongAng = 0;
   pCelestialObj->fRotation = -90;
   pCelestialObj->Color.x = pCelestialObj->Color.y =pCelestialObj->Color.z = 1;
}




////////////////////////////////////////////////////////////////

static void CelestialInstallCommands();


cCelestialObject::cCelestialObject(int index)
   : nIndex(index),
     pCelestialObj(0)
{
   bDidGameInit = FALSE;
   pCelestRes = 0;
   nPalIx = 0;
}


static BOOL bDidAppInit = FALSE;

void cCelestialObject::AppInit()
{
   if (nIndex != 3)
   {
      if (nIndex >= MAX_CEL_OBJECTS)
      {
         Warning(("Invalid Celestial object index\n"));
         return;
      }

      if (!bDidAppInit)
      {
         AutoAppIPtr_(StructDescTools, pTools);
         pTools->Register(&g_SDesc);
         CelestialInstallCommands();
         bDidAppInit = TRUE;
      }

      switch (nIndex)
      {
      case 0:
         pCelestialObj = &CelestialObj0;
      break;

      case 1:
         pCelestialObj = &CelestialObj1;
      break;

      case 2:
         pCelestialObj = &CelestialObj2;
      break;
      }

      SetDefaults();
   }
   else if (config_is_defined("Smooth")) // Special easter egg:
   {
      pCelestialObj = &CelestialObj3;
      SetEasterEggDefaults();
   }
}


// We make this 256 just in case texture is wrong. Really, we only use the first 16
static ushort AlphaTable[256] =
{
   0x0fff, 0x1fff, 0x2fff, 0x3fff,
   0x4fff, 0x5fff, 0x6fff, 0x7fff,
   0x8fff, 0x9fff, 0xafff, 0xbfff,
   0xcfff, 0xdfff, 0xefff, 0xffff
};


// Call after mission parms are loaded, and after enhanced sky is inited.
// We Associate an alpha level for the stars based on the brightness of the sky at that point,
// and the overall intensity of the star itself. Then, it also gets fogged.
void cCelestialObject::Init()
{
   mxs_vector Temp[4];
   mxs_vector ZVec = {0,0,1};
   mxs_vector CenterVec;
   mxs_vector TempVec;
   int i;
   float fDist;

   if (!pCelestialObj || !pCelestialObj->bUseCelestialObject || !SkyRendererUseEnhanced())
      return;

   // Create Res interface.
   IResMan *pResMan = AppGetObj(IResMan);
   pCelestRes = IResMan_Bind(pResMan, pCelestialObj->TextureName, RESTYPE_IMAGE, NULL, "fam\\skyhw\\", 0);
   if (!pCelestRes)
      return;

   ////////////////////////////////////////////////////////////////////////////////////////
   // Compute 3d points based on direction, angular size, and sky shape, and distance.
   // Rotate along axis from camera below, point 0 at upper left:
   mx_rot_y_vec(Temp+0, &ZVec, mx_rad2ang(-(pCelestialObj->fAng*DEG)/2));
   mx_rot_x_vec(Temp+1, &ZVec, mx_rad2ang((pCelestialObj->fAng*DEG)/2));
   mx_rot_y_vec(Temp+2, &ZVec, mx_rad2ang((pCelestialObj->fAng*DEG)/2));
   mx_rot_x_vec(Temp+3, &ZVec, mx_rad2ang(-(pCelestialObj->fAng*DEG)/2));

   // Now rotate the whole thing by 45 degrees plus rotation around z to get corner points:
   mx_rot_z_vec(Points+0, Temp+0, mx_rad2ang(pCelestialObj->fRotation*DEG+(45*DEG)));
   mx_rot_z_vec(Points+1, Temp+1, mx_rad2ang(pCelestialObj->fRotation*DEG+(45*DEG)));
   mx_rot_z_vec(Points+2, Temp+2, mx_rad2ang(pCelestialObj->fRotation*DEG+(45*DEG)));
   mx_rot_z_vec(Points+3, Temp+3, mx_rad2ang(pCelestialObj->fRotation*DEG+(45*DEG)));

   // Now rotate by lat:
   mx_rot_y_vec(Temp+0, Points+0, mx_rad2ang(pCelestialObj->fLatAng*DEG));
   mx_rot_y_vec(Temp+1, Points+1, mx_rad2ang(pCelestialObj->fLatAng*DEG));
   mx_rot_y_vec(Temp+2, Points+2, mx_rad2ang(pCelestialObj->fLatAng*DEG));
   mx_rot_y_vec(Temp+3, Points+3, mx_rad2ang(pCelestialObj->fLatAng*DEG));
   mx_rot_y_vec(&TempVec, &ZVec, mx_rad2ang(pCelestialObj->fLatAng*DEG));

   // rotate by long:
   mx_rot_z_vec(Points+0, Temp+0, mx_rad2ang(pCelestialObj->fLongAng*DEG));
   mx_rot_z_vec(Points+1, Temp+1, mx_rad2ang(pCelestialObj->fLongAng*DEG));
   mx_rot_z_vec(Points+2, Temp+2, mx_rad2ang(pCelestialObj->fLongAng*DEG));
   mx_rot_z_vec(Points+3, Temp+3, mx_rad2ang(pCelestialObj->fLongAng*DEG));
   mx_rot_z_vec(&CenterVec, &TempVec, mx_rad2ang(pCelestialObj->fLongAng*DEG));

   // Now get sky distances and scale:
   fDist = SkyRendererGetSkyDist(&CenterVec)-pCelestialObj->fOffset;

   for (i = 0; i < 4; i++)
   {
      // As it turns out, you can really see the polys when the angular size gets large,
      // so we just use dist to center:
      // fDist = SkyRendererGetSkyDist(Points+i)-pCelestialObj->fOffset;
      mx_scaleeq_vec(Points+i, fDist);
   }

   vlist[0] = (r3s_point*)(pcorn+0);
   vlist[1] = (r3s_point*)(pcorn+1);
   vlist[2] = (r3s_point*)(pcorn+2);
   vlist[3] = (r3s_point*)(pcorn+3);

   pcorn[0].grp.u = 0;
   pcorn[0].grp.v = 0;
   pcorn[0].grp.i = pCelestialObj->Color.x;
   pcorn[0].grp.h = pCelestialObj->Color.y;
   pcorn[0].grp.d = pCelestialObj->Color.z;

   pcorn[1].grp.u = 1;
   pcorn[1].grp.v = 0;
   pcorn[1].grp.i = pCelestialObj->Color.x;
   pcorn[1].grp.h = pCelestialObj->Color.y;
   pcorn[1].grp.d = pCelestialObj->Color.z;

   pcorn[2].grp.u = 1;
   pcorn[2].grp.v = 1;
   pcorn[2].grp.i = pCelestialObj->Color.x;
   pcorn[2].grp.h = pCelestialObj->Color.y;
   pcorn[2].grp.d = pCelestialObj->Color.z;

   pcorn[3].grp.u = 0;
   pcorn[3].grp.v = 1;
   pcorn[3].grp.i = pCelestialObj->Color.x;
   pcorn[3].grp.h = pCelestialObj->Color.y;
   pcorn[3].grp.d = pCelestialObj->Color.z;

   for (i = 16; i < 256; i++)
      AlphaTable[i] = 0;

   // Load palette
   if (!pCelestialObj->bIsAlphaTexture)
   {
      IRes *pPalRes = IResMan_Bind(pResMan, IRes_GetName(pCelestRes), RESTYPE_PALETTE,
                             NULL, "fam\\skyhw\\", 0);
      if (pPalRes)
      {
         // Okay, we've got the palette -- set it up and dump it
         nPalIx = palmgr_alloc_pal((uchar *)IRes_Lock(pPalRes));
         pPalRes->Unlock();
         SafeRelease(pPalRes);
      }
   }

   bDidGameInit = TRUE;
}


void cCelestialObject::Term()
{
   SafeRelease(pCelestRes);
   pCelestRes = 0;

#if 0 // interferes with database update:
   if (pCelestialObj==&CelestialObj3)
      SetEasterEggDefaults();
   else if (pCelestialObj)
      SetDefaults();
#endif

   bDidGameInit = FALSE;
}


void cCelestialObject::Render()
{
   grs_bitmap *pBitmap;
   size_t nStride;
   mxs_vector OldPos;
   mxs_vector ZeroPos = {0,0,0};
   BOOL bRestoreZWrite;
   BOOL bRestoreZCompare;

   if (!g_lgd3d || !bDidGameInit)
      return;

   ////////////////////////////////////////////////////
   // Setup
   pBitmap = (grs_bitmap *)pCelestRes->Lock();
   if (!pBitmap)
   {
      Warning(("Celestial texture not found\n"));
      return;
   }

   nStride = r3d_glob.cur_stride;
   r3d_glob.cur_stride = sizeof(goddamn_stupid_r3s_point);

   r3e_space nOldSpace = r3_get_space();
   r3_set_space(R3_CLIPPING_SPACE);

   OldPos = *r3_get_view_pos();
   r3_set_view(&ZeroPos);

   r3_set_clipmode(R3_CLIP);
   r3_set_clip_flags(R3_CLIP_UV|R3_CLIP_RGB);
   // If this is an alpha texture:
   if (pCelestialObj->bIsAlphaTexture) // Must be an alpha texture:
   {
      gr_set_fill_type(FILL_BLEND);
      lgd3d_set_alpha_pal(AlphaTable);
   }
   else // setup palette for resource, and mark bitmap as having transparency:
   {
      pBitmap->align = nPalIx;
      pBitmap->flags|=BMF_TRANS;
   }

   r3_set_prim();
   r3_set_polygon_context(R3_PL_POLYGON|R3_PL_TEXTURE|R3_PL_RGB_GOURAUD);
   lgd3d_set_blend(TRUE);
   r3_set_color(0xffffff);
   bRestoreZWrite = lgd3d_is_zwrite_on();
   bRestoreZCompare = lgd3d_is_zcompare_on();
   lgd3d_set_zwrite(FALSE);
   lgd3d_set_zcompare(FALSE);
   lgd3d_set_alpha(pCelestialObj->fAlpha);
   lgd3d_set_texture_wrapping(0, FALSE);
   lgd3d_disable_palette();

   lgd3d_set_fog_enable(portal_fog_on && pCelestialObj->bEnableFog);

   /////////////////////////////
   // Render
   r3_set_texture(pBitmap);
   r3_start_block();
   r3_transform_block(4, (r3s_point *)pcorn, Points);
   r3_draw_poly(4, vlist);
   r3_end_block();


   /////////////////////////////
   // Cleanup
   r3d_glob.cur_stride = nStride;
   gr_set_fill_type(FILL_NORM);
   r3_set_prim();
   lgd3d_set_blend(FALSE);
   lgd3d_set_zwrite(bRestoreZWrite);
   lgd3d_set_zcompare(bRestoreZCompare);
   lgd3d_set_texture_wrapping(0, TRUE);
   lgd3d_set_alpha(1);
   lgd3d_enable_palette();
   lgd3d_set_fog_enable(portal_fog_on);
   r3_set_space(nOldSpace);
   r3_set_view(&OldPos);
   pCelestRes->Unlock();
}

#if 0
static Command g_CelestialCommands[] =
{
   { "celest_test", TOGGLE_BOOL, &bSpinStars, "Spin stars", HK_ALL},
};
#endif



static void CelestialInstallCommands()
{
#if 0
#ifndef SHIP
   COMMANDS(g_CelestialCommands, HK_ALL);
#endif
#endif
}
