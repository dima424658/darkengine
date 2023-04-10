/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/render.cpp,v 1.41 2000/02/08 13:38:55 zarko Exp $

#include <wrtype.h>
#include <portal.h>
#include <lgd3d.h>
#include <render.h>
#include <simstate.h>
#include <simflags.h>
#include <starhack.h>
#include <objmodel.h>
#include <rendprop.h>
#include <config.h>
#include <texmem.h>
#include <scrnman.H>
#include <math.h>
#include <mipmap.h>
#include <palmgr.h>
#include <dbasemsg.h>
#include <command.h>
#include <objlight.h>
#include <missrend.h>
#include <h2ocolor.h>
#include <media.h>

#include <sdestool.h>
#include <sdesbase.h>
#include <filevar.h>
#include <appagg.h>
#include <dispapi.h>
#include <mprintf.h>

#include <skyrend.h>


// @TODO: replace this with real animation system
#include <anim_txt.h>

#include <palette.h>
#include <g2.h>
#include <g2pt.h>
#include <mm.h>

#include <simtime.h>
#include <playrobj.h>

// Must be last header
#include <dbmem.h>

EXTERN BOOL g_lgd3d; //zb

//
// Core renderer tools
//

//zb static double znear=0.25;
static double znear = 1.0;

static double zfar=256.0;

EXTERN void lgd3d_set_znearfar(double z_near, double z_far);

void SetZNearFar(double z_near, double z_far)
{
   znear = z_near;
   zfar = z_far;
   lgd3d_set_znearfar(z_near, z_far);
   portal_set_znearfar(z_near, z_far);
   starfield_set_z(z_far*0.95);
}

r3s_texture texture_lookup(int n);
void init_color_tables(void);

static bool new_gamma = FALSE;
static float gamma_level = 1.0;

EXTERN void do_set_gamma(void)
{
   if (!new_gamma)
      return;

   IDisplayDevice *pDispDev = AppGetObj(IDisplayDevice);
   if (pDispDev) {
      pDispDev->SetGamma(gamma_level);
      // silently fail
   }

   new_gamma = FALSE;
   SafeRelease( pDispDev);
}

EXTERN void set_hardware_gamma_level(float level)
{
   new_gamma = TRUE;
   gamma_level = level;

   // if we're rendering, wait until the right moment so we don't
   // crash the @#$%%!@ riva128
   if (SimStateCheckFlags(kSimRender))
      return;

   do_set_gamma();
}

#ifdef PLAYTEST

static void set_znear(float z_near)
{
   znear = z_near;
   SetZNearFar(znear, zfar);
}

static void set_zfar(float z_far)
{
   zfar = z_far;
   SetZNearFar(znear, zfar);
}

EXTERN bool lgd3d_punt_buffer;
EXTERN ulong g_MeshRenderFlags;
EXTERN float g_MeshRenderLowDetailDist2;
EXTERN BOOL mm_punt_render;
EXTERN BOOL mm_punt_sort;
EXTERN BOOL mm_punt_xform;
EXTERN BOOL lgd3d_blend_trans;
EXTERN BOOL g_render_on_begin_frame;
EXTERN BOOL pt_lightmap_first;
EXTERN BOOL portal_draw_lgd3d;
EXTERN BOOL portal_punt_draw_surface;
EXTERN BOOL portal_multitexture;
EXTERN BOOL punt_hardware_lighting;
EXTERN BOOL g_bRenderMesh;
EXTERN BOOL portal_true_two_pass;
EXTERN void g2_set_new_poly_funcs(void);
EXTERN void g2_set_old_poly_funcs(void);
EXTERN bool g_multitexture;
EXTERN float g_obj_zbias;
extern int flash_clamp_time;

static void set_mesh_detail_dist(float dist)
{
   g_MeshRenderLowDetailDist2 = dist * dist;

   mprintf("Low-detail meshes rendered at %g\n", dist);
}

static void toggle_mm_indexed(void)
{
   g_MeshRenderFlags ^= MMF_INDEXED;
   mprintf("mesh render flags: %x\n", g_MeshRenderFlags);
}

static void toggle_multitexture(void)
{
   if (!g_multitexture)
   {
      mprintf("multitexture unavailable.\n");
      return;
   }
   portal_multitexture ^= 1;
   mprintf("multitexture: %x\n", portal_multitexture);
}

static void set_obj_zbias(int n)
{
   g_obj_zbias = n * (1.0 / 65536.0);
}

//zb:
static void toggle_dithering(void)
{
	int huh = lgd3d_is_dithering_on();
	lgd3d_set_dithering( !huh );
}

//zb:
static void toggle_antialiasing(void)
{
	int huh = lgd3d_is_antialiasing_on();
	lgd3d_set_antialiasing( !huh );
}


static void set_invisible(int nVal)
{
   g_pIsInvisibleProperty->Set(PlayerObject(), nVal);
}


static Command all_keys[]=
{
   { "toggle_mm_indexed", FUNC_VOID, toggle_mm_indexed, "toggle using indexed r3d interface"},
   { "toggle_mm_sort", TOGGLE_BOOL, &mm_punt_sort, "punt mm sort"},
   { "toggle_mm_xform", TOGGLE_BOOL, &mm_punt_xform, "punt mm transforms"},
   { "toggle_mm_render", TOGGLE_BOOL, &mm_punt_render, "punt mm render"},
   { "toggle_render_mesh", TOGGLE_BOOL, &g_bRenderMesh, "toggle render mesh"},
   { "toggle_draw_lgd3d", TOGGLE_BOOL, &portal_draw_lgd3d, "toggle draw lgd3d"},
   { "toggle_lightmap_first", TOGGLE_BOOL, &pt_lightmap_first, "toggle lightmap rendering order"},
   { "toggle_d3d_buffer", TOGGLE_BOOL, &lgd3d_punt_buffer, "toggle d3d primitive buffering"},
   { "toggle_d3d_blend_trans", TOGGLE_BOOL, &lgd3d_blend_trans, "toggle transparent texel color search"},
   { "toggle_draw_surface", TOGGLE_BOOL, &portal_punt_draw_surface, "toggle draw surface"},
   { "toggle_hardware_lighting", TOGGLE_BOOL, &punt_hardware_lighting, "toggle hardware lighting"},
   { "toggle_two_pass", TOGGLE_BOOL, &portal_true_two_pass, "toggle two pass rendering"},
   { "toggle_multitexture", FUNC_VOID, &toggle_multitexture, "toggle multitexture support"},
   { "fast_poly_setup", FUNC_VOID, g2_set_new_poly_funcs, "do fast poly setup"},
   { "slow_poly_setup", FUNC_VOID, g2_set_old_poly_funcs, "do slow poly setup"},
   { "toggle_render_on_start_frame", TOGGLE_BOOL, &g_render_on_begin_frame, "toggle normal render loop"},
   { "flash_clamp", VAR_INT, &flash_clamp_time, "Set flashbomb max effect time in ms" },
   { "set_mesh_detail_dist", FUNC_FLOAT, set_mesh_detail_dist, "set distance at which stretchy mesh is punted"},
   { "set_gamma", FUNC_FLOAT, set_hardware_gamma_level, "set gamma correction level. 1.0 = no correction"},
   { "set_obj_zbias", FUNC_INT, set_obj_zbias, "set object rendering z bias (in bits)"},
   { "set_znear", FUNC_FLOAT, set_znear, "set max sorting z distance"},
   { "set_zfar", FUNC_FLOAT, set_zfar, "set min sorting z distance"},
   { "toggle_dithering", FUNC_VOID, toggle_dithering, "toggle dithering"}, //zb
   { "toggle_antialiasing", FUNC_VOID, toggle_antialiasing, "toggle antialiasing"}, //zb
   { "set_invisible", FUNC_INT, set_invisible, "Set player invisibility"},
};
#endif

void register_sky_mode();

void RendererInit()
{
   objmodelInit();
   RenderPropsInit();
   SetZNearFar(znear, zfar);
   portal_get_texture = texture_lookup;
   ectsAnimTxtInit();
   register_sky_mode();
   ObjShadowInit();
   MissionRenderInit();
   WaterColorPropInit();
#ifdef PLAYTEST
   COMMANDS(all_keys, HK_ALL);
#endif
}


EXTERN void ParticleGroupTerm(void);
void RendererTerm()
{
   objmodelShutdown();
   RenderPropsShutdown();
   ectsAnimTxtTerm();
   ParticleGroupTerm();
   ObjShadowTerm();
   WaterColorPropTerm();
   MissionRenderTerm();
   portal_cleanup_water_hack();
}


void RendererReset()
{
   portal_cleanup_water_hack();
}

void RendererPalChange()
{
   init_color_tables();
}
////////////////////////////////////////////////////////////
// THIS STUFF HERE WAS CUT OUT OF EDITGEOM
//
//

extern BOOL any_stars;  // from rendloop, for now
r3s_texture texture_lookup(int n)
{
#ifdef DBG_ON
   {
      bool ok=FALSE;
      if (n >= 0 && n < texmemGetMax()) ok=TRUE;
      if ((n>=WATERIN_IDX)&&(n<=255)) ok=TRUE;
      if (!ok) { Warning(("Bad texture id %d\n",n)); n = 0; }
   }
#endif

   // @OPTIMIZE: THIS MUST GO, extra if per texture lookup = ~10000 per second?
   //  or at least bias it towards the expected jump (ie. not backhack)
   if (((GetSkyMode() == kSkyModeStars) && (n==BACKHACK_IDX)) && !SkyRendererUseEnhanced())
   any_stars=TRUE;

   return texmemGetTexture(n);
}

static float _RGBDistSquared(mxs_vector *c, uchar *pal)
{
   return (double)(c->x-pal[0])*(c->x-pal[0]) +
          (double)(c->y-pal[1])*(c->y-pal[1]) +
          (double)(c->z-pal[2])*(c->z-pal[2]);
}

static float _RGBDistLinear(mxs_vector *c, uchar *pal)
{
   return fabs((double)(c->x-pal[0])) +
          fabs((double)(c->y-pal[1])) +
          fabs((double)(c->z-pal[2]));
}

#define BuildIntensity(r,g,b) (((r)*0.3)+((g)*0.55)+((b)*0.15))
static float _RGBDist_wIntensity(mxs_vector *c, uchar *pal)
{
   float dist = _RGBDistSquared(c,pal);
   float i_diff=BuildIntensity(c->x,c->y,c->z)-BuildIntensity(pal[0],pal[1],pal[2]);
   return dist + (i_diff*i_diff);
}

///////////////////
// controls how we get the color match
#define ALLOW_IPAL
bool shade_using_ipal=TRUE;


static float (*RGBDistanceFunction)(mxs_vector *c, uchar *pal)=_RGBDistSquared;

// actually do the closest color matching
uchar ShadClosestCol(mxs_vector *c, uchar pal[768], bool trans)
{
   double dist, best_dist = 256*256*3; // bigger than biggest possible
   uchar best=0;
   uchar j;

   for (j = trans?1:0; j<256; ++j)
   {
      dist = (*RGBDistanceFunction)(c,&(pal[j*3]));
      if (dist<best_dist)
      {  // return if perfect match
         if (dist==0) return j;
         best=j;
         best_dist=dist;
      }
   }
   return best;
}

extern "C" void PickDistanceFunction(int which)
{
   if (which==0)
      shade_using_ipal=TRUE;
   else
   {
      shade_using_ipal=FALSE;
      switch (which)
      {
      case 1: RGBDistanceFunction=_RGBDistSquared; break;
      case 2: RGBDistanceFunction=_RGBDistLinear; break;
      case 3: RGBDistanceFunction=_RGBDist_wIntensity; break;
      }
   }
   init_color_tables(); // actually rebuild lighting w/new distance func
}


// compute best match for a * (FIX_UNIT-weight) + b * weight
uchar compute_color_mix(int a, int r, int g, int b, fix weight)
{
#ifdef ALLOW_IPAL
   if (shade_using_ipal)
   {
      uchar *base_tmap_ipal=palmgr_get_ipal(0);
      const ubyte* ipal = base_tmap_ipal?base_tmap_ipal:ScrnGetInvPalette();
      r = (grd_pal[a*3+0] * (FIX_UNIT - weight) + r * weight);
      g = (grd_pal[a*3+1] * (FIX_UNIT - weight) + g * weight);
      b = (grd_pal[a*3+2] * (FIX_UNIT - weight) + b * weight);
      return ipal[gr_index_rgb(r,g,b)];
   }
   else
#endif
   {
      float fweight=fix_float(weight);
      mxs_vector c;
      c.x = (grd_pal[a*3+0] * (1.0 - fweight) + r * fweight);
      c.y = (grd_pal[a*3+1] * (1.0 - fweight) + g * fweight);
      c.z = (grd_pal[a*3+2] * (1.0 - fweight) + b * fweight);
      return ShadClosestCol(&c,grd_pal,FALSE);
   }
}

#ifdef SOFTWARE_RGB

#define NUM_R  4
#define NUM_G  4
#define NUM_B  3
#define NUM_SHADES 8

#define MAX_R      (NUM_R-1)
#define MAX_G      (NUM_G-1)
#define MAX_B      (NUM_B-1)
#define MAX_SHADES (NUM_SHADES-1)

EXTERN uchar light_pal[768] = { 0 };
EXTERN uchar light_ipal[32768] = { 0 };

EXTERN void calc_ipal(uchar *pal_data, uchar *ipal);
void init_lighting_palette(void)
{
   int r,g,b;
   int lum;
   int n=0;
   // tesselate the RGB color cube in the following way:
   //   r: 4 values
   //   g: 4 values
   //   b: 3 values
   // (30 combinations)
   // make white first

   n = 0;
   for (r=MAX_R; r >= 0; --r)
   for (g=MAX_G; g >= 0; --g)
   for (b=MAX_B; b >= 0; --b) {
      if (r == MAX_R || g == MAX_G || b == MAX_B) {
         for (lum=0; lum < NUM_SHADES; ++lum) {
            light_pal[n++] = r * lum * 256 / NUM_SHADES / NUM_R;
            light_pal[n++] = g * lum * 256 / NUM_SHADES / NUM_G;
            light_pal[n++] = b * lum * 256 / NUM_SHADES / NUM_B;
         }
      }
   }

   calc_ipal(light_pal, light_ipal);
}
#endif

enum eTLucBanks
{
   kBlackBank = 128,
   kFirstWaterBank = 192,
   kWaterBankSize = 16,
   kWhiteBank = 144,

};

int GetWaterBankClut(int i)
{
   Assert_(i < kNumWaterBanks);
   return i* kWaterBankSize + kFirstWaterBank;
}

static uchar g_tluc[256][256];

// set up the tluc table
static void build_tluc_table(uchar tluc[256][256])
{
   int x,y;

   // everything defaults to opaque
   for (y=0; y < 256; ++y)
      for (x=0; x < 256; ++x)
         tluc[x][y] = x;

   // except 0, which is transparent
   for (y=0; y < 256; ++y)
      tluc[0][y] = y;

   for (int i = 0; i < kNumWaterBanks; i++)
   {
      int base = GetWaterBankClut(i);

      const sRGBA* prgba = GetWaterColorForBank(i);
      const uchar* rgb = prgba->rgb;

      // now compute the weighted blends to our color
      for (x= base; x < base + kWaterBankSize; ++x)
      {
         fix wt = (x-base) << 12;   // map it from 0..1
         wt = 2*wt/3 + FIX_UNIT/3;   // map it from 1/3..1
         for (y=0; y < 256; ++y)
            tluc[x][y] = compute_color_mix(y,rgb[0],rgb[1],rgb[2],wt);
      }
   }

   // now compute the weighted blends to black
   for (x=kBlackBank; x < kBlackBank +16; ++x) {
      fix wt = (x-kBlackBank) << 12; // map it from 0..1
      for (y=0; y < 256; ++y) // cnn - to fix texas water problem
         tluc[x][y] = compute_color_mix(y,0,0,0,FIX_UNIT-wt);
   }

      // and the weighted blends to white
   for (x=kWhiteBank; x < kWhiteBank+8; ++x) {
      fix wt = (x-kWhiteBank) << 13; // map it from 0..1
      for (y=0; y < 256; ++y) // dc - should this one change as well??
         tluc[x][y] = compute_color_mix(y,255,255,255,wt/2);
   }
}



static void init_color_tables(void)
{//zb
   if( !g_lgd3d ){
      build_tluc_table(g_tluc);


      for (int x = kFirstWaterBank; x < kFirstWaterBank + kNumWaterBanks*kWaterBankSize; x++)
         pt_clut_list[x] = g_tluc[x];

      gr_set_light_tab(g_tluc[kBlackBank]);
      g2pt_tluc_table = g_tluc[0];

      // setup our media entry/exit cluts
      for (x=0; x < 256; ++x) {
         pt_medium_entry_clut[x] = 0;
         pt_medium_exit_clut[x] = 0;
         pt_medium_haze_clut[x] = 0;
      }

      // sigh, can't find the magic water constant anywhere.
      pt_medium_haze_clut[MEDIA_WATER] = GetWaterBankClut(0);
#ifdef SOFTWARE_RGB
      init_lighting_palette();
#endif
   }
   else
   {
      g2pt_tluc_table = NULL;
      gr_set_light_tab(NULL);
      memset( g_tluc, 0, sizeof(g_tluc));
      for (int x = kFirstWaterBank; x < kFirstWaterBank + kNumWaterBanks*kWaterBankSize; x++)
         pt_clut_list[x] = NULL;

      // setup our media entry/exit cluts
      for (x=0; x < 256; ++x) {
         pt_medium_entry_clut[x] = 0;
         pt_medium_exit_clut[x] = 0;
         pt_medium_haze_clut[x] = 0;
      }
      // sigh, can't find the magic water constant anywhere.
      pt_medium_haze_clut[MEDIA_WATER] = GetWaterBankClut(0);
   }

      /*

   build_tluc_table(g_tluc);

   for (int x = kFirstWaterBank; x < kFirstWaterBank + kNumWaterBanks*kWaterBankSize; x++)
      pt_clut_list[x] = g_tluc[x];

   gr_set_light_tab(g_tluc[kBlackBank]);
   g2pt_tluc_table = g_tluc[0];

   // setup our media entry/exit cluts
   for (x=0; x < 256; ++x) {
      pt_medium_entry_clut[x] = 0;
      pt_medium_exit_clut[x] = 0;
      pt_medium_haze_clut[x] = 0;
   }

   // sigh, can't find the magic water constant anywhere.
   pt_medium_haze_clut[MEDIA_WATER] = GetWaterBankClut(0);

#ifdef SOFTWARE_RGB
   init_lighting_palette();
#endif
   */
}

////////////////////////////////////////////////////////////
// SKY_STAR_MODE
//


struct sSkyMode
{
   int mode;
};

// Here's my descriptor, which identifies my stuff to the tag file & editor
sFileVarDesc gSkyModeDesc =
{
   kMissionVar,         // Where do I get saved?
   "SKYMODE",          // Tag file tag
   "Sky Rendering Mode",// friendly name
   FILEVAR_TYPE(sSkyMode),  // Type (for editing)
   { 1, 0},             // version
   { 1, 0},             // last valid version
};

// The actual global variable
class cSkyMode : public cFileVar<sSkyMode,&gSkyModeDesc>
{
public:
   void Update()
   {
      texmemSkyStarControl(mode == kSkyModeStars);
   }

   void Reset()
   {
      mode = kSkyModeStars;
   }
};

static cSkyMode gSkyMode;

int GetSkyMode(void)
{
   return gSkyMode.mode;
}

void SetSkyMode(int mode)
{
   gSkyMode.mode = mode;
   gSkyMode.Update();
}

//
// SKY MODE SDESC
//

static const char* sky_names[] =
{
   "Textures",
   "Stars",
};

#define NUM_SKIES (sizeof(sky_names)/sizeof(sky_names[0]))

static sFieldDesc sky_field[] =
{
   { "", kFieldTypeEnum, FieldLocation(sSkyMode,mode), kFieldFlagNone, 0, NUM_SKIES, NUM_SKIES, sky_names },
};

static sStructDesc sky_sdesc = StructDescBuild(sSkyMode,kStructFlagNone,sky_field);

static void register_sky_mode()
{
   AutoAppIPtr_(StructDescTools,pTools);
   pTools->Register(&sky_sdesc);
}


////////////////////////////////////////////////////////////
// DATABASE MESSAGE HANDLER
//

void RendererDBMessage(ulong msg, IUnknown* /* file */ )
{
   switch (DB_MSG(msg))
   {
      case kDatabaseReset:
         RendererReset();
         objmodelReset();
         ObjShadowClearCache();
         break;

      case kDatabaseDefault:
         break;
   }
}


EXTERN void set_mm_sort(BOOL sort)
{
   if (sort)
      mm_set_render_pipeline(MM_RPIPE_POLYSORT);
   else
      mm_set_render_pipeline(MM_RPIPE_HARDWARE);
}

