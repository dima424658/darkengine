// $Header: x:/prj/tech/libsrc/lgd3d/RCS/texture.c 1.45 1998/04/27 15:00:24 KEVIN Exp $
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d.h>

#include <appagg.h>
#include <cbchain.h>
#include <dbg.h>
#include <dispapi.h>
#include <g2.h>
#include <lgassert.h>
#include <wdispapi.h>
#include <wdispcb.h>

#include <lgd3d.h>
#include <setup.h>
#include <tdrv.h>
#include <texture.h>
#include <tmgr.h>

#ifndef SHIP
static const char *hResErrorMsg = "%s:\nFacility %i, Error %i";
#endif
#define CheckHResult(hRes, msg) \
AssertMsg3(hRes==0, hResErrorMsg, msg, (hRes>>16)&0x7fff, hRes&0xffff)

#ifndef SHIP
#include <mprintf.h>
static BOOL spew;
#define mono_printf(x) \
do {              \
   if (spew)      \
      mprintf x ; \
} while (0)
#else
#define mono_printf(x)
#endif

#ifdef MONO_SPEW
#define put_mono(c) \
do { \
   uchar *p_mono = (uchar *)0xb0086; \
   *p_mono = c;                      \
} while (0)
#else
#define put_mono(c)
#endif

#define GBitMask15 0x3e0
#define GBitMask16 0x7e0

D3DMATERIALHANDLE MaterialHandle[LGD3D_MAX_TEXTURES];
D3DTEXTUREHANDLE TextureHandle[LGD3D_MAX_TEXTURES];
grs_bitmap *TdrvBitmap[LGD3D_MAX_TEXTURES];

#define PF_GENERIC   0
#define PF_RGB       1
#define PF_ALPHA     2
#define PF_MASK      3
#define PF_MAX       3
#define PF_TRANS     128


typedef struct d3d_cookie {
   union {
      struct {
         uchar wlog, hlog;
         uchar flags;
         uchar palette;
      };
      ulong value;
   };
} d3d_cookie;

static DDPIXELFORMAT AlphaTextureFormat;
static DDPIXELFORMAT RGBTextureFormat;
static DDPIXELFORMAT PalTextureFormat;
static LPDDPIXELFORMAT FormatList[PF_MAX];
static DWORD DeviceSurfaceCaps;
static BOOL texture_RGB = FALSE;
static BOOL prefer_RGB = FALSE;
static BOOL using_local = FALSE;
static BOOL local_available = FALSE;
static BOOL agp_available = FALSE;

static BOOL TextureListInited = FALSE;
static LPD3DITEXTURE Texture[LGD3D_MAX_TEXTURES];
static LPDIRECTDRAWSURFACE Surface[LGD3D_MAX_TEXTURES];

static LPD3DIMATERIAL Material[LGD3D_MAX_TEXTURES];

#define MAX_PALETTES 256

static grs_bitmap *default_bm = NULL;
static LPDIRECTDRAWPALETTE lpDDPalTexture[MAX_PALETTES];
static LPDIRECTDRAW lpdd=NULL;
static LPDIRECTDRAW2 lpdd2=NULL;
static IWinDisplayDevice * pWinDisplayDevice = NULL;

static void CheckSurfaces(sWinDispDevCallbackInfo *data);
static int callback_id = 0;

static ushort default_alpha_pal[256];
static ushort *texture_pal[MAX_PALETTES];
static ushort *alpha_pal = NULL;
static uchar *texture_clut=NULL;
static DevDesc *device_desc;
static DWORD texture_caps;

void ShutdownTextureSys(void)
{
   int i;

   if (TextureListInited) {
      if (g_tmgr!=NULL) {
         g_tmgr->shutdown();
         g_tmgr = NULL;
      }

      pWinDisplayDevice->lpVtbl->RemoveTaskSwitchCallback(
         pWinDisplayDevice, callback_id);

      // this is actually redundant since tmgr should take 
      // care of it, but it can't really hurt...
      for (i=0; i<LGD3D_MAX_TEXTURES; i++) {
         SafeRelease(Material[i]);
         SafeRelease(Texture[i]);
         SafeRelease(Surface[i]);
         MaterialHandle[i]=0;
         TextureHandle[i]=0;
      }
      TextureListInited = FALSE;
   }
   if (default_bm != NULL) {
      gr_free(default_bm);
      default_bm = NULL;
   }
   for (i=0; i<MAX_PALETTES; i++)
   {
      if (texture_pal[i]!=NULL) {
         gr_free(texture_pal[i]);
         texture_pal[i] = NULL;
      }
      SafeRelease(lpDDPalTexture[i]);
   }
   SafeRelease(lpdd2);
   SafeRelease(lpdd);
   SafeRelease(pWinDisplayDevice);
}

void lgd3d_set_texture_clut(uchar *clut)
{
   texture_clut = clut;
   if (g_tmgr!=NULL)
      g_tmgr->set_clut(clut);
}

void lgd3d_set_alpha_pal(ushort *pal)
{
   alpha_pal = pal;
}

void lgd3d_texture_set_RGB(bool is_RGB)
{
   prefer_RGB = is_RGB;
}

void lgd3d_get_texture_bitmask(grs_rgb_bitmask *bitmask)
{
   if (RGBTextureFormat.dwFlags == 0)
      RGBTextureFormat.dwRBitMask = 
      RGBTextureFormat.dwGBitMask =
      RGBTextureFormat.dwBBitMask = 0;

   bitmask->red   = RGBTextureFormat.dwRBitMask;
   bitmask->green = RGBTextureFormat.dwGBitMask;
   bitmask->blue  = RGBTextureFormat.dwBBitMask;
}


static HRESULT InitMaterial(int tex_id)
{
   D3DMATERIAL d3dMaterial;
   HRESULT     hRes;
   LPD3DIMATERIAL Mat;

   AssertMsg((tex_id>=0)&&(tex_id<LGD3D_MAX_TEXTURES), "Texture id out of range");

   hRes = lpd3d->lpVtbl->CreateMaterial(
      lpd3d,
      &Mat,
      NULL);

   if (FAILED(hRes))
      return hRes;

   Material[tex_id] = Mat;

   ZeroMemory(&d3dMaterial, sizeof(d3dMaterial));
   d3dMaterial.dwSize = sizeof(d3dMaterial);

   d3dMaterial.diffuse.r  = D3DVAL(1.0);
   d3dMaterial.diffuse.g  = D3DVAL(1.0);
   d3dMaterial.diffuse.b  = D3DVAL(1.0);
   d3dMaterial.ambient.r  = D3DVAL(1.0);
   d3dMaterial.ambient.g  = D3DVAL(1.0);
   d3dMaterial.ambient.b  = D3DVAL(1.0);
   d3dMaterial.specular.r = D3DVAL(0.0);
   d3dMaterial.specular.g = D3DVAL(0.0);
   d3dMaterial.specular.b = D3DVAL(0.0);
   d3dMaterial.dvPower    = D3DVAL(0.0);
   d3dMaterial.hTexture   = TextureHandle[tex_id];
   d3dMaterial.dwRampSize = 1UL;
   
   hRes = Mat->lpVtbl->SetMaterial(Mat, &d3dMaterial);
   if (FAILED(hRes))
      return hRes;

   hRes = Mat->lpVtbl->GetHandle(Mat,
      lpd3dDevice,
      &MaterialHandle[tex_id]);

   if (FAILED(hRes))
      return hRes;

   return DD_OK;
}


static HRESULT CALLBACK EnumTextureFormatsCallback(LPDDSURFACEDESC lpDDSD, LPVOID lpContext)
{
   DDPIXELFORMAT *fmt = &lpDDSD->ddpfPixelFormat;

#ifdef SPEW_FORMATS
   if (fmt->dwFlags&DDPF_RGB) {
      mono_printf(("RGB bits: %i, alpha bits: %i\n", fmt->dwRGBBitCount, fmt->dwAlphaBitDepth));
      mono_printf(("bitmasks: A %x, R %x G %x B %x\n",
         fmt->dwRGBAlphaBitMask, fmt->dwRBitMask, fmt->dwGBitMask, fmt->dwBBitMask));
   }
#endif

   // Grab 4444 alpha format...
   if ((fmt->dwFlags&DDPF_RGB)&&(fmt->dwRGBBitCount==16)&&
       (fmt->dwRGBAlphaBitMask == 0xf000)&&
       (fmt->dwRBitMask == 0xf00)&&
       (fmt->dwGBitMask == 0xf0)&&
       (fmt->dwBBitMask == 0xf)) {
      memcpy(&AlphaTextureFormat, fmt, sizeof(AlphaTextureFormat));
   }
   // Grab 16 bit no alpha format...
   else if ((fmt->dwFlags == DDPF_RGB)&&
       (fmt->dwRGBBitCount == 16))
   {
      if ((fmt->dwGBitMask == GBitMask16) ||
          (fmt->dwGBitMask == GBitMask15)) {
         memcpy(&RGBTextureFormat, fmt, sizeof(RGBTextureFormat));
      }
   }
   // Grab 8 bit palettized format
   else if (lpDDSD->ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED8) {
      memcpy(&PalTextureFormat, &lpDDSD->ddpfPixelFormat, sizeof(PalTextureFormat));
   }

   return DDENUMRET_OK;
}

#define NONLOCAL_CAPS DDSCAPS_TEXTURE | DDSCAPS_VIDEOMEMORY | DDSCAPS_NONLOCALVIDMEM | DDSCAPS_ALLOCONLOAD
#define LOCAL_CAPS DDSCAPS_TEXTURE | DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM | DDSCAPS_ALLOCONLOAD

static void EnumTextureFormats(void)
{
   HRESULT hRes;
   D3DDEVICEDESC hal, hel;
   LPDIRECTDRAWSURFACE test_surf;
   DDSURFACEDESC ddsd;


   ZeroMemory(&RGBTextureFormat, sizeof(RGBTextureFormat));
   ZeroMemory(&PalTextureFormat, sizeof(PalTextureFormat));

   hRes = lpd3dDevice->lpVtbl->EnumTextureFormats(lpd3dDevice, EnumTextureFormatsCallback, NULL);
   CheckHResult(hRes, "EnumTextureFormats failed");

   texture_RGB = prefer_RGB;
   FormatList[PF_GENERIC] = texture_RGB ? &RGBTextureFormat : &PalTextureFormat;
   if (FormatList[PF_GENERIC]->dwFlags == 0) {
      texture_RGB = !texture_RGB;
      FormatList[PF_GENERIC] = texture_RGB ? &RGBTextureFormat : &PalTextureFormat;
      if (FormatList[PF_GENERIC]->dwFlags == 0)
         CriticalMsg("Direct3D device does not support 8 bit palettized or 15 or 16 bit RGB textures");
   }

   FormatList[PF_RGB] = &RGBTextureFormat;
   FormatList[PF_ALPHA] = &AlphaTextureFormat;

   mono_printf(("Using %s textures\n",texture_RGB ? "16 bit RGB" : "Palettized"));

   if (AlphaTextureFormat.dwFlags == 0)
      mono_printf(("no alpha texture format available.\n"));


   ZeroMemory(&hal, sizeof(hal));
   hal.dwSize = sizeof(hal);
   ZeroMemory(&hel, sizeof(hel));
   hel.dwSize = sizeof(hel);

   hRes = lpd3dDevice->lpVtbl->GetCaps(lpd3dDevice, &hal, &hel);
   CheckHResult(hRes, "Failed to obtain device caps");

   AssertMsg(hal.dwFlags & D3DDD_DEVCAPS, "No HAL device!");

   ZeroMemory(&ddsd, sizeof(ddsd));
   memcpy(&ddsd.ddpfPixelFormat, FormatList[PF_GENERIC], sizeof(ddsd.ddpfPixelFormat));
   ddsd.dwHeight = 256;
   ddsd.dwWidth = 256;
   agp_available = FALSE;
   local_available = FALSE;

   if (hal.dwDevCaps & D3DDEVCAPS_TEXTURENONLOCALVIDMEM) {
      ddsd.dwSize = sizeof(DDSURFACEDESC);
      ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
      ddsd.ddsCaps.dwCaps = NONLOCAL_CAPS;
      hRes  = lpdd->lpVtbl->CreateSurface(lpdd, &ddsd, &test_surf, NULL);
      if ((hRes == DD_OK) && (test_surf != NULL)) {
         SafeRelease(test_surf);
         agp_available = TRUE;
         DeviceSurfaceCaps = NONLOCAL_CAPS;
         mono_printf(("nonlocal videomemory textures available.\n"));
      }
   }

   if (hal.dwDevCaps & D3DDEVCAPS_TEXTUREVIDEOMEMORY) {
      ddsd.dwSize = sizeof(DDSURFACEDESC);
      ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
      ddsd.ddsCaps.dwCaps = LOCAL_CAPS;
      hRes  = lpdd->lpVtbl->CreateSurface(lpdd, &ddsd, &test_surf, NULL);
      if ((hRes == DD_OK) && (test_surf != NULL)) {
         SafeRelease(test_surf);
         local_available = TRUE;
         DeviceSurfaceCaps = LOCAL_CAPS;
         mono_printf(("local videomemory textures available.\n"));
      }
   }

   if (!(agp_available||local_available)) {
      mono_printf(("No local or nonlocal texture memory! Using system memory textures.\n"));
		DeviceSurfaceCaps = DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY | DDSCAPS_ALLOCONLOAD;
   }
   using_local = local_available;
}

static void GetDirectDraw(void)
{
   HRESULT hRes;

   pWinDisplayDevice = AppGetObj (IWinDisplayDevice);

   lpdd = NULL;
   IWinDisplayDevice_GetDirectDraw(pWinDisplayDevice, &lpdd);
   AssertMsg(lpdd!=NULL, "GetDirectDraw failed");

   hRes = lpdd->lpVtbl->QueryInterface(lpdd, &IID_IDirectDraw2, (LPVOID *)&lpdd2);
   CheckHResult(hRes, "Query for DirectDraw2 failed");
}


static void SetTexturePalette(int start, int n, uchar *pal, int slot)
{
   PALETTEENTRY         peColorTable[256];
   HRESULT              hRes;
   int i;

   if (lpDDPalTexture[slot] == NULL)
   {
      hRes = lpdd->lpVtbl->CreatePalette(lpdd,
                                          DDPCAPS_8BIT | DDPCAPS_ALLOW256,
                                          peColorTable,
                                          &lpDDPalTexture[slot],
                                          NULL);
      CheckHResult(hRes, "CreatePalette failed.");
   }

   for (i = 0; i < n; i++) {
      peColorTable[i].peFlags = D3DPAL_READONLY | PC_RESERVED;
      peColorTable[i].peRed = pal[3*i];
      peColorTable[i].peGreen = pal[3*i+1];
      peColorTable[i].peBlue = pal[3*i+2];
   }

   hRes = lpDDPalTexture[slot]->lpVtbl->SetEntries(lpDDPalTexture[slot],
                                       0, start, n, peColorTable);

   CheckHResult(hRes, "SetEntries failed.");
}

void lgd3d_set_pal_slot(uint start, uint n, uchar *pal_data, int slot)
{
   grs_rgb_bitmask bitmask;

   if (lpdd == NULL)
      return;

   if (!texture_RGB)
   {
      SetTexturePalette(start, n, pal_data, slot);
   }
   else
   {
      if (texture_pal[slot]==NULL)
         texture_pal[slot] = (ushort *)gr_malloc(512);
      lgd3d_get_texture_bitmask(&bitmask);
      gr_make_pal16(start, n, texture_pal[slot], pal_data, &bitmask);
   }
}

void lgd3d_set_pal(uint start, uint n, uchar *pal_data)
{
   lgd3d_set_pal_slot(start, n, pal_data, 0);
}

static int chroma_r=0;
static int chroma_g=0;
static int chroma_b=0;
static DWORD chroma_key;

// values should be 0..255
void lgd3d_set_chromakey(int r, int g, int b)
{
   chroma_r = r;
   chroma_g = g;
   chroma_b = b;
   if (RGBTextureFormat.dwRBitMask == 0x1f) {
      // swap red and blue
      r = chroma_b;
      b = chroma_r;
   }
   chroma_key = b>>3;
   if (RGBTextureFormat.dwGBitMask == GBitMask15) {
      chroma_key += ((g>>3)<<5) + ((r>>3)<<10);
   } else {
      chroma_key += ((g>>2)<<5) + ((r>>3)<<11);
   }
}


static int FindClosestColor(float r, float g, float b)
{
   float best=3*256*256;
   uchar *pal = grd_pal;
   int i, color = -1;

   for (i=0; i<256; i++) {
      float test, dr, dg, db;
      dr = r - pal[0];
      dg = g - pal[1];
      db = b - pal[2];
      pal += 3;
      test = dr*dr + db*db + dg*dg;
      if (test < best) {
         best = test;
         color = i;
      }
   }
   AssertMsg(color>=0,"Couldn't fit color.");
   return color;
}

static void InitDefaultTexture(int size)
{
   grs_bitmap *bm;
   int c0, c1, i, j;

   bm = gr_alloc_bitmap(BMT_FLAT8, 0, size, size);
   c0 = FindClosestColor(180.0, 10.0, 10.0);
   c1 = FindClosestColor(10.0, 180.0, 10.0);
   for (i=0; i<size; i++)
      for (j=0; j<size; j++)
         bm->bits[size*i+j] = ((i+j)&1) ? c0:c1;

   default_bm = bm;
}

static void InitTextureList(void)
{
   int i;
   for (i=0; i<LGD3D_MAX_TEXTURES; i++) {
      Surface[i]=NULL;
      Texture[i]=NULL;
      Material[i]=NULL;
      MaterialHandle[i]=0;
      TextureHandle[i]=0;
      TdrvBitmap[i] = NULL;
   }
   TextureListInited = TRUE;
}

static texture_driver driver;

static void CookInfo(tdrv_texture_info *info);
static int LoadTexture(tdrv_texture_info *info);
static int ReloadTexture(tdrv_texture_info *info);
static void ReleaseTexture(int n);
static void UnloadTexture(int n);
extern void SetTextureId(BOOL t);
void SynchD3D(void);

static void InitTextureManager(lgd3ds_device_info *info)
{
   driver.release_texture = ReleaseTexture;
   driver.load_texture = LoadTexture;
   driver.set_texture_id = SetTextureId;
   driver.unload_texture = UnloadTexture;
   driver.synchronize = SynchD3D;
   driver.start_frame = lgd3d_start_frame;
   driver.end_frame = lgd3d_end_frame;
   driver.reload_texture = ReloadTexture;
   driver.cook_info = CookInfo;

   if (g_tmgr!=NULL) {
      g_tmgr->shutdown();
      g_tmgr = NULL;
   }
   
   g_tmgr = get_dopey_texture_manager(&driver);
   g_tmgr->init(
      default_bm,
      LGD3D_MAX_TEXTURES, NULL, 0,
      (info->flags & LGD3DF_SPEW) ? TMGRF_SPEW:0);

}


void InitTextureSys(lgd3ds_device_info *info)
{
   int i;

   if (lpdd!=NULL) {
      Warning(("Texture system already inited.\n"));
      return;
   }

#ifndef SHIP
   spew = ((info->flags & LGD3DF_SPEW) != 0);
#endif
   device_desc = info->device_desc;
   texture_caps = device_desc->dpcTriCaps.dwTextureCaps;
   GetDirectDraw();
   EnumTextureFormats();
   for (i=0; i<MAX_PALETTES; i++)
   {
      texture_pal[i] = NULL;
      lpDDPalTexture[i] = NULL;
   }
   lgd3d_set_pal_slot(0,256,grd_pal,0);
   lgd3d_set_alpha_pal(default_alpha_pal);
   lgd3d_set_chromakey(chroma_r, chroma_g, chroma_b);
   InitTextureList();
   pWinDisplayDevice->lpVtbl->AddTaskSwitchCallback(
      pWinDisplayDevice, CheckSurfaces);
   InitDefaultTexture(16);
   InitTextureManager(info);
}



/***************************************************************************/
/*                    Loading a grs_bitmap into a system memory surface    */
/***************************************************************************/
/*
 * LoadSurface
 * Loads a grs_bitmap into a texture map DD surface of the given format.  The
 * memory flag specifies DDSCAPS_SYSTEMMEMORY or DDSCAPS_VIDEOMEMORY.
 */

static HRESULT CreateSurface(d3d_cookie cookie, DDSURFACEDESC *ddsd, LPDIRECTDRAWSURFACE *ppDDS)
{
   DDCOLORKEY ck;
   HRESULT ddrval;
   LPDIRECTDRAWSURFACE pDDS;
   DWORD ckey;

   ddrval = lpdd->lpVtbl->CreateSurface(lpdd, ddsd, ppDDS, NULL);
   if (ddrval != DD_OK)
      return ddrval;

   pDDS = *ppDDS;

   // Bind the palette, if necessary
   if (ddsd->ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED8) {
      ddrval = pDDS->lpVtbl->SetPalette(pDDS, lpDDPalTexture[cookie.palette]);
      CheckHResult(ddrval, "SetPalette failed while creating surface.");
      ckey = 0;
   } else
      ckey = chroma_key;

   // Set colorkey, if necessary
   if (PF_TRANS & cookie.flags) {
      ck.dwColorSpaceLowValue = ckey;
      ck.dwColorSpaceHighValue = ckey;
      ddrval = pDDS->lpVtbl->SetColorKey(pDDS, DDCKEY_SRCBLT, &ck);
      CheckHResult(ddrval, "SetColorKey failed while creating surface.");
   }

   return DD_OK;
}


// OK, here are our various specialized blitters

static void blit_8to16(tdrv_texture_info *info, ushort *dst, int drow, ushort *pal)
{
   int i, j;
   grs_bitmap *bm = info->bm;
   uchar *src = info->bits;

   for (i=0; i<bm->h; i++) {
      for (j=0; j<bm->w; j++)
         dst[j] = pal[src[j]];
      src += bm->row;
      dst += drow;
   }
}

static void blit_8to16_clut(tdrv_texture_info *info, ushort *dst, int drow, ushort *pal)
{
   int i, j;
   grs_bitmap *bm = info->bm;
   uchar *src = info->bits;

   for (i=0; i<bm->h; i++) {
      for (j=0; j<bm->w; j++)
         dst[j] = pal[texture_clut[src[j]]];
      src += bm->row;
      dst += drow;
   }
}

static void blit_8to16_scale(tdrv_texture_info *info, ushort *dst, int drow, ushort *pal)
{
   int i, j, k;
   int scale_w = info->scale_w;
   int scale_h = info->scale_h;
   int step_w = 1<<(scale_w-1);
   int step_h = (1<<scale_h)-1;
   grs_bitmap *bm = info->bm;
   uchar *src = info->bits;

   for (i=0; i<bm->h; i++) {
      if (scale_w>0)
      {
         ulong *base = (ulong *)dst;
         for (j=0; j<bm->w; j++)
         {
            ulong c32 = pal[src[j]];
            c32 += (c32<<16);
            for (k=0; k<step_w; k++)
               base[k] = c32;
            base += step_w;
         }
      } else {
         for (j=0; j<bm->w; j++)
            dst[j] = pal[src[j]];
      }
      for (j=0; j<step_h; j++) {
         memcpy(dst + drow, dst, 2*(bm->w<<scale_w));
         dst += drow;
      }
      src += bm->row;
      dst += drow;
   }
}

static void blit_8to16_clut_scale(tdrv_texture_info *info, ushort *dst, int drow, ushort *pal)
{
   int i, j, k;
   int scale_w = info->scale_w;
   int scale_h = info->scale_h;
   int step_w = 1<<(scale_w-1);
   int step_h = (1<<scale_h)-1;
   grs_bitmap *bm = info->bm;
   uchar *src = info->bits;

   for (i=0; i<bm->h; i++) {
      if (scale_w>0)
      {
         ulong *base = (ulong *)dst;
         for (j=0; j<bm->w; j++)
         {
            ulong c32 = pal[texture_clut[src[j]]];
            c32 += (c32<<16);
            for (k=0; k<step_w; k++)
               base[k] = c32;
            base += step_w;
         }
      } else {
         for (j=0; j<bm->w; j++)
            dst[j] = pal[texture_clut[src[j]]];
      }
      for (j=0; j<step_h; j++) {
         memcpy(dst + drow, dst, 2*(bm->w<<scale_w));
         dst += drow;
      }
      src += bm->row;
      dst += drow;
   }
}


static void blit_8to8(tdrv_texture_info *info, uchar *dst, int drow)
{
   int i;
   grs_bitmap *bm = info->bm;
   uchar *src = info->bits;

   for (i=0; i<bm->h; i++) {
      memcpy(dst, src, bm->w);
      src += bm->row;
      dst += drow;
   }
}

static void blit_8to8_clut(tdrv_texture_info *info, uchar *dst, int drow)
{
   int i, j;
   grs_bitmap *bm = info->bm;
   uchar *src = info->bits;

   for (i=0; i<bm->h; i++) {
      for (j=0; j<bm->w; j++)
         dst[j] = texture_clut[src[j]];
      src += bm->row;
      dst += drow;
   }
}

static void blit_8to8_scale(tdrv_texture_info *info, uchar *dst, int drow)
{
   int i, j, k;
   int scale_w = info->scale_w;
   int scale_h = info->scale_h;
   int step_w = 1<<scale_w;
   int step_h = (1<<scale_h) - 1;
   grs_bitmap *bm = info->bm;
   uchar *src = info->bits;

   for (i=0; i<bm->h; i++) {
      if (scale_w > 0) {
         uchar *base = dst;
         for (j=0; j<bm->w; j++) {
            uchar c = src[j];
            for (k=0; k<step_w; k++)
               base[k] = c;
            base += step_w;
         }
      } else
         memcpy(dst, src, bm->w);
      for (j = 0; j<step_h; j++) {
         memcpy(dst + drow, dst, bm->w);
         dst += drow;
      }
      src += bm->row;
      dst += drow;
   }
}

static void blit_8to8_clut_scale(tdrv_texture_info *info, uchar *dst, int drow)
{
   int i, j, k;
   int scale_w = info->scale_w;
   int scale_h = info->scale_h;
   int step_w = 1<<scale_w;
   int step_h = (1<<scale_h) - 1;
   grs_bitmap *bm = info->bm;
   uchar *src = info->bits;

   for (i=0; i<bm->h; i++) {
      uchar *base = dst;
      for (j=0; j<bm->w; j++) {
         uchar c = texture_clut[src[j]];
         for (k=0; k<step_w; k++)
            base[k] = c;
         base += step_w;
      }
      for (j = 0; j<step_h; j++) {
         memcpy(dst + drow, dst, bm->w);
         dst += drow;
      }
      src += bm->row;
      dst += drow;
   }
}


static void blit_16to16(tdrv_texture_info *info, ushort *dst, int drow)
{
   int i;
   grs_bitmap *bm = info->bm;
   uchar *src = info->bits;

   for (i=0; i<bm->h; i++) {
      memcpy(dst, src, 2*bm->w);
      src += bm->row;
      dst += drow;
   }
}


static void blit_16to16_scale(tdrv_texture_info *info, ushort *dst, int drow)
{
   int i, j, k;
   int scale_w = info->scale_w;
   int scale_h = info->scale_h;
   int step_w = 1<<scale_w;
   int step_h = (1<<scale_h) - 1;
   grs_bitmap *bm = info->bm;
   int srow = bm->row>>1;
   ushort *src = (ushort *)info->bits;

   for (i=0; i<bm->h; i++) {
      if (step_w > 1) {
         ushort *base = dst;
         for (j=0; j<bm->w; j++) {
            ushort c = src[j];
            for (k=0; k<step_w; k++)
               base[k] = c;
            base += step_w;
         }
      } else
         memcpy(dst, src, 2*bm->w);
      for (j = 0; j<step_h; j++) {
         memcpy(dst + drow, dst, 2*bm->w);
         dst += drow;
      }
      src += srow;
      dst += drow;
   }
}


static void LoadSurface(tdrv_texture_info *info, DDSURFACEDESC *ddsd)
{
   uchar *src;
   grs_bitmap *bm = info->bm;
   int flags = info->flags;
   uchar zero_save = 0;

   if ((texture_clut != NULL) && (bm->flags & BMF_TRANS))
   {
      zero_save = texture_clut[0];
      texture_clut[0] = 0;
   }

   if (ddsd->ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED8) {
	   uchar *dst = (uchar *)ddsd->lpSurface;
      if ((info->scale_w|info->scale_h) != 0)
         if (texture_clut!=NULL)
            blit_8to8_clut_scale(info, dst, ddsd->lPitch);
         else
            blit_8to8_scale(info, dst, ddsd->lPitch);
      else
         if (texture_clut!=NULL)
            blit_8to8_clut(info, dst, ddsd->lPitch);
         else
            blit_8to8(info, dst, ddsd->lPitch);
   } else {
	   ushort *dst = (ushort *)ddsd->lpSurface;
      int drow = ddsd->lPitch/2;

      if (bm->type == BMT_FLAT8) {
         ushort *pal;
         ushort zero_save;

         pal = (flags & TF_ALPHA) ? alpha_pal : texture_pal[bm->align];
         if (pal==NULL)
            pal = grd_pal16_list[0];

         if (flags & TF_TRANS) {
            zero_save = pal[0];
            pal[0] = chroma_key;
         }

         AssertMsg(pal!=NULL, "Hey! trying to use NULL 16 bit palette to load texture!");
         src = bm->bits;

         if ((info->scale_w|info->scale_h) != 0)
            if ((flags & TF_ALPHA)||(texture_clut == NULL))
               blit_8to16_scale(info, dst, drow, pal);
            else
               blit_8to16_clut_scale(info, dst, drow, pal);
         else
            if ((flags & TF_ALPHA)||(texture_clut == NULL))
               blit_8to16(info, dst, drow, pal);
            else
               blit_8to16_clut(info, dst, drow, pal);

         if (flags & TF_TRANS)
            pal[0] = zero_save;

      } else { // assume straight 16 to 16
         if ((info->scale_w|info->scale_h) != 0)
            blit_16to16_scale(info, dst, drow);
         else
            blit_16to16(info, dst, drow);
      }
   }
   if (zero_save!=0)
      texture_clut[0] = zero_save;
}

static void GetAvailableTexMem(DWORD *local, DWORD *agp)
{
   DDSCAPS ddscaps;
   DWORD total;

   AssertMsg(NULL!=lpdd2, "No DirectDraw object!");

   if (local!=NULL) {
      ddscaps.dwCaps = LOCAL_CAPS;
      lpdd2->lpVtbl->GetAvailableVidMem(lpdd2, &ddscaps, &total, local);
   } 
   if (agp!=NULL) {
      ddscaps.dwCaps = NONLOCAL_CAPS;
      lpdd2->lpVtbl->GetAvailableVidMem(lpdd2, &ddscaps, &total, agp);
   } 
}


static void CookInfo(tdrv_texture_info *info)
{
   d3d_cookie cookie;
   int v, w=info->w, h=info->h;

   cookie.hlog = 0;
   cookie.wlog = 0;
   for (v=2; v<=w; v+=v)
   {
      cookie.wlog++;
   }
   for (v=2; v<=h; v+=v)
   {
      cookie.hlog++;
   }

   AssertMsg((info->h == (1<<cookie.hlog)) && (info->w == (1<<cookie.wlog)),
      "hlog/wlog does not match texture width/height");

   if (info->flags & TF_ALPHA)
      cookie.flags = PF_ALPHA;
   else if (info->bm->type == BMT_FLAT16)
      cookie.flags = PF_RGB;
   else
      cookie.flags = PF_GENERIC;

   if (info->flags & TF_TRANS)
      cookie.flags |= PF_TRANS;

   if (info->bm->type == BMT_FLAT8)
      cookie.palette = info->bm->align;
   else
      cookie.palette = 0;

   info->cookie = cookie.value;
}


static int ReloadTexture(tdrv_texture_info *info)
{
   LPDDPIXELFORMAT pixel_format;
   DDSURFACEDESC ddsd;
   LPDIRECTDRAWSURFACE SysmemSurface, *pDeviceSurface;
   LPD3DITEXTURE SysmemTexture=NULL, *pDeviceTexture;
   HRESULT LastError;
   d3d_cookie cookie;

   put_mono('b');

   pDeviceTexture = &Texture[info->id];
   pDeviceSurface = &Surface[info->id];

   SynchD3D();

   cookie.value = info->cookie;
   pixel_format = FormatList[cookie.flags&PF_MASK];

   if (pixel_format->dwFlags==0)
      // unsupported texture format
      return TDRV_FAILURE;

   // Create a surface of the given format using the dimensions of the grs_bitmap

   ZeroMemory(&ddsd, sizeof(ddsd));

   ddsd.dwSize = sizeof(DDSURFACEDESC);
   ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
   ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY;
   memcpy(&ddsd.ddpfPixelFormat, pixel_format, sizeof(ddsd.ddpfPixelFormat));
   ddsd.dwHeight = info->h;
   ddsd.dwWidth = info->w;

   put_mono('c');

   LastError = CreateSurface(cookie, &ddsd, &SysmemSurface);
   CheckHResult(LastError, "CreateSurface() failed");
   AssertMsg(SysmemSurface!=NULL, "NULL SysmemSurface");
   put_mono('h');
   LastError = SysmemSurface->lpVtbl->QueryInterface(SysmemSurface,
					     &IID_ID3DTexture,
					     (LPVOID*)&SysmemTexture);

   CheckHResult(LastError, "Failed to obtain D3D texture interface for sysmem surface");

   ddsd.dwSize = sizeof(DDSURFACEDESC);
   ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;



   put_mono('i');
   if (*pDeviceTexture == NULL) {
      LPDIRECTDRAWSURFACE DeviceSurface;

	   ddsd.ddsCaps.dwCaps = DeviceSurfaceCaps;
      LastError = CreateSurface(cookie, &ddsd, pDeviceSurface);
      if ((LastError!=DD_OK) && using_local && agp_available)
      {
         mono_printf(("using nonlocal vidmem textures...\n"));
         // try AGP textures...
         using_local = FALSE;
         DeviceSurfaceCaps = NONLOCAL_CAPS;
	      ddsd.ddsCaps.dwCaps = DeviceSurfaceCaps;
         LastError = CreateSurface(cookie, &ddsd, pDeviceSurface);
      }

      if (LastError!=DD_OK) {
         mono_printf(("couldn't load texture: error %i.\n", LastError&0xffff));
         SafeRelease(SysmemTexture);
         SafeRelease(SysmemSurface);
         *pDeviceSurface = NULL;
         put_mono('.');
         return TDRV_FAILURE;
      }

      DeviceSurface = *pDeviceSurface;

   #define MEMORY_TYPE_MASK \
      (DDSCAPS_SYSTEMMEMORY|DDSCAPS_VIDEOMEMORY|DDSCAPS_LOCALVIDMEM|DDSCAPS_NONLOCALVIDMEM)

      if ((DeviceSurfaceCaps&MEMORY_TYPE_MASK) != (ddsd.ddsCaps.dwCaps&MEMORY_TYPE_MASK))
         mono_printf((
            "device texture not created in requested memory location\nRequested %x Received %x\n",
            DeviceSurfaceCaps&MEMORY_TYPE_MASK, ddsd.ddsCaps.dwCaps&MEMORY_TYPE_MASK));


      // Query our device surface for a texture interface
      put_mono('k');
      LastError = DeviceSurface->lpVtbl->QueryInterface(DeviceSurface,
					      &IID_ID3DTexture,
					      (LPVOID*)pDeviceTexture);
	   CheckHResult(LastError, "Failed to obtain D3D texture interface for device surface.");
   }

   // Load the bitmap into the sysmem surface
   LastError = SysmemSurface->lpVtbl->Lock(SysmemSurface, NULL, &ddsd, 0, NULL);
   CheckHResult(LastError, "Failed to lock sysmem surface.");

   LoadSurface(info, &ddsd);

   LastError = SysmemSurface->lpVtbl->Unlock(SysmemSurface, NULL);
   CheckHResult(LastError, "Failed to unlock sysmem surface.");

   // Load the sysmem texture into the device texture.  During this call, a
   // driver could compress or reformat the texture surface and put it in
   // video memory.

   put_mono('l');
   LastError = pDeviceTexture[0]->lpVtbl->Load(pDeviceTexture[0], SysmemTexture);
	CheckHResult(LastError, "Failed to load device texture from sysmem texture.");

   // Now we are done with sysmem surface

   put_mono('m');
   SafeRelease(SysmemTexture);
   put_mono('n');
   SafeRelease(SysmemSurface);

   TdrvBitmap[info->id] = info->bm;

   put_mono('.');
   return TDRV_SUCCESS;
}

static int doLoad(tdrv_texture_info *info)
{
   HRESULT LastError;
   int result;
   int n = info->id;

   result = ReloadTexture(info);
   if (result != TDRV_SUCCESS)
      return result;

   put_mono('o');
   LastError = Texture[n]->lpVtbl->GetHandle(Texture[n], lpd3dDevice, &TextureHandle[n]);

   if (LastError!=DD_OK) {
      mono_printf(("GetTextureHandle failed! facility %i error %i width %i height %i\n",
         (LastError>>16)&0x7fff, LastError&0xffff, info->w, info->h));
      SafeRelease(Texture[n]);
      SafeRelease(Surface[n]);
      put_mono('.');
      return TDRV_FAILURE;
   }

   return TDRV_SUCCESS;
}


static int LoadTexture(tdrv_texture_info *info)
{
   int n = info->id;
   DWORD local_start=0, agp_start=0;
   int result;

   put_mono('a');

#if 0
   if ((info->h < device_desc->dwMinTextureHeight) ||
       (info->h > device_desc->dwMaxTextureHeight) ||
       (info->w < device_desc->dwMinTextureWidth) ||
       (info->w > device_desc->dwMaxTextureWidth))
      return TDRV_FAILURE;
#endif
   if ((texture_caps & D3DPTEXTURECAPS_SQUAREONLY) && (info->w != info->h)) {
      put_mono('.');
      return TDRV_FAILURE;
   }

   GetAvailableTexMem(
      local_available ? &local_start : NULL,
      agp_available ? &agp_start : NULL);

   if ((local_start < info->size) && using_local && agp_available)
   {  // switch to agp and try again...
      mono_printf(("using nonlocal vidmem textures...\n"));

      using_local = FALSE;
      DeviceSurfaceCaps = NONLOCAL_CAPS;
   }

   if ((using_local ? local_start : agp_start) < info->size)
   {
      put_mono('.');
      return TDRV_FAILURE;
   }

   AssertMsg((n>=0)&&(n<LGD3D_MAX_TEXTURES), "Texture id out of range");

   // These should already be taken care of, but just in case...
   SafeRelease(Material[n]);
   SafeRelease(Texture[n]);
   SafeRelease(Surface[n]);
   TextureHandle[n]=0;
   MaterialHandle[n]=0;

   result = doLoad(info);
   put_mono('p');
   if ((result != TDRV_SUCCESS) && using_local && agp_available) {
      // switch to AGP and try again...
      mono_printf(("using nonlocal vidmem textures...\n"));

      using_local = FALSE;
      DeviceSurfaceCaps = NONLOCAL_CAPS;
      result = doLoad(info);
   }

   if (result != TDRV_SUCCESS) {
      put_mono('.');
      return result;
   }

   InitMaterial(n);

   // calculate load size if requested
   if (info->size == 0) {
      DWORD local_end, agp_end;

      // calculate size
      GetAvailableTexMem(
         local_available ? &local_end : NULL,
         agp_available ? &agp_end : NULL);

      if (local_available && (local_start > local_end))
      {
         AssertMsg( (!agp_available)||(agp_start==agp_end),
            "Texture load modified local and nonlocal vidmem!");
         info->size = local_start - local_end;
      } else {
         AssertMsg(agp_available && (agp_start > agp_end),
            "Texture load took no space!");
         info->size = agp_start - agp_end;
      }
   }

   put_mono('.');
   return TDRV_SUCCESS;
}

static void CheckSurfaces(sWinDispDevCallbackInfo *info)
{
   static void (*chain)(sWinDispDevCallbackInfo *);
   int i;

   switch (info->message) {
   case kWinDispDevCallbackActive:
      for (i=0; i<LGD3D_MAX_TEXTURES; i++) {
         LPDIRECTDRAWSURFACE lpTS = Surface[i];
         if ((lpTS == NULL)||
             (lpTS->lpVtbl->IsLost(lpTS) != DDERR_SURFACELOST))
            continue;

         VerifyMsg(lpTS->lpVtbl->Restore(lpTS)==DD_OK, "Could not restore lost surface!");
         if (TdrvBitmap[i]!=NULL) {
            AssertMsg(g_tmgr != NULL, "Hmmm.  Should have a non-NULL texture manager here.");
            g_tmgr->unload_texture(TdrvBitmap[i]);
         }
      }
      break;
   case kWinDispDevCallbackUpdateChain:
      GenericCallbackChainHandler(&callback_id, (callback_chain_func **)&chain, (callback_chain_info *)info);
      return;
   }
   if (chain!=NULL)
      chain(info);
}

extern void UnsetTextureId(int n);
// Disconnect texture from bitmap, but don't release texture yet
static void UnloadTexture(int n)
{
   AssertMsg((n>=0)&&(n<LGD3D_MAX_TEXTURES), "Texture id out of range");
   TdrvBitmap[n] = NULL;
}

static void ReleaseTexture(int n)
{
   AssertMsg((n>=0)&&(n<LGD3D_MAX_TEXTURES), "Texture id out of range");
   UnsetTextureId(n);
   Assert(Surface[n]!=NULL, ("texture %i already released\n", n));

   if ((Surface[n]!=NULL) && local_available && (!using_local))
   {
      DDSCAPS ddscaps;
      Surface[n]->lpVtbl->GetCaps(Surface[n], &ddscaps);
      if (ddscaps.dwCaps & DDSCAPS_LOCALVIDMEM)
      {
         mono_printf(("Using Local vidmem textures\n"));
         DeviceSurfaceCaps = LOCAL_CAPS;
         using_local = TRUE;
      }
   }

   SafeRelease(Material[n]);
   SafeRelease(Texture[n]);
   SafeRelease(Surface[n]);
   MaterialHandle[n]=0;
   TextureHandle[n]=0;
   TdrvBitmap[n] = NULL;
}

void SynchD3D(void)
{
#ifndef SHIP
   HRESULT hRes =
#endif
   lpd3dDevice->lpVtbl->SetRenderState(lpd3dDevice,
      D3DRENDERSTATE_FLUSHBATCH, 0);
   CheckHResult(hRes, "FlushBatch failed");
}
