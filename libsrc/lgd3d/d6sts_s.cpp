// @SH_CODE
#include <d3d.h>
#include <stdlib.h>		// min() and max().
#include <comtools.h>
#include <wdispapi.h>

#include <d3d.h>
#include <ddraw.h>

#include <wdispcb.h>
#include <appagg.h>
#include <mprintf.h>


#ifndef SHIP
#define mono_printf(x) \
   if (bSpewOn)      \
      mprintf x;
#else
#define mono_printf(x)
#endif

#ifdef MONO_SPEW
#define put_mono(c) (*((uchar *)0xb0078)) = (c)	// hello dossie
#else
#define put_mono(c)
#endif

// #include "d3dmacs.h"
#include <pal16.h>
#include <lgassert.h>
#include <dbg.h>
#include <lgd3d.h>
#include <lgSS2P.h>

#include <d6States.h>
#include <d6Prim.h>
#include <d6Render.h>


#ifndef SHIP
static const char* hResErrorMsg = "%s:\nFacility %i, Error %i";
#endif
static char* pcDXef = (char*)"%s: error %d\n%s";
static char* pcLGD3Def = (char*)"LGD3D error no %d : %s : message: %d\n%s";

#define CheckHResult(hRes, msg) \
AssertMsg3(hRes==0, pcDXef, msg, hRes, GetDDErrorMsg(hRes))

extern cD6Primitives* pcRenderBuffer;
extern cD6Renderer* pcRenderer;
cD6States *pcStates = nullptr;

class cImStates : public cD6States
{
public:
    // singleton?
    static cD6States* Instance();
    virtual cD6States *DeInstance() override;

protected:
    // cImStates(const class cImStates &);
    cImStates();
    virtual ~cImStates();

private:
    static cImStates *m_Instance;
};

class cMSStates : public cD6States {
public:
    static cD6States * Instance();
    virtual cD6States * DeInstance() override;
    virtual int Initialize(DWORD dwRequestedFlags) override;
    virtual int SetDefaultsStates(DWORD dwRequestedFlags) override;

protected:
    // cMSStates(const cMSStates &);
    cMSStates();
    virtual ~cMSStates();

private:
    static cMSStates * m_Instance;
    unsigned long m_dwCurrentTexLevel;
    int m_bTexturePending;
    grs_bitmap * m_LastLightMapBm;

public:
    virtual void set_texture_id(int n) override;
    virtual int reload_texture(tdrv_texture_info *info) override;
    virtual void cook_info(tdrv_texture_info *info) override;
    virtual void SetLightMapMode(DWORD dwFlag) override;
    virtual void SetTextureLevel(int n) override;
    virtual int EnableMTMode(DWORD dwMTOn) override;
    virtual void TurnOffTexuring(BOOL bTexOff) override;
};

#define PF_GENERIC   0
#define PF_RGB       1
#define PF_ALPHA     2
#define PF_MASK      3
#define PF_RGBA      4
#define PF_MAX       4
#define PF_STAGE     64
#define PF_TRANS     128

#define GBitMask15 0x3e0
#define GBitMask16 0x7e0

#define MAX_PALETTES 256

DDPIXELFORMAT* g_FormatList[5];

#define LGD3D_MAX_TEXTURES 1024
sTextureData g_saTextures[LGD3D_MAX_TEXTURES];

DDPIXELFORMAT g_RGBTextureFormat;

DDPIXELFORMAT g_TransRGBTextureFormat, g_PalTextureFormat, g_AlphaTextureFormat, g_8888TexFormat;
BOOL g_bTexSuspended;

ushort* texture_pal_trans[MAX_PALETTES];
IDirectDrawPalette* lpDDPalTexture[MAX_PALETTES];
ushort* texture_pal_opaque[MAX_PALETTES];
ushort default_alpha_pal[256];

BOOL g_bUseDepthBuffer, g_bUseTableFog, g_bUseVertexFog;

void (*chain)(sWinDispDevCallbackInfo*);

extern DevDesc g_sD3DDevDesc;

EXTERN cD6States* pcStates;

int g_b8888supported;
ulong g_dwRSChangeFlags;
grs_bitmap* default_bm;

ushort* alpha_pal;
static int callback_id;

cImStates* cImStates::m_Instance = NULL;
cMSStates* cMSStates::m_Instance = NULL;

int alpha_size_table[81], generic_size_table[81], rgb_size_table[81], trgb_size_table[81], b8888_size_table[81];

sTexBlendArgs sTexBlendArgsProtos[LGD3DTB_NO_STATES][2] =
{
    { { D3DTOP_MODULATE, D3DTA_TEXTURE, D3DTA_DIFFUSE, D3DTOP_SELECTARG2, D3DTA_TEXTURE, D3DTA_DIFFUSE }, { D3DTOP_DISABLE, D3DTA_TEXTURE, D3DTA_CURRENT, D3DTOP_DISABLE, D3DTA_TEXTURE, D3DTA_DIFFUSE } },
    { { D3DTOP_MODULATE, D3DTA_TEXTURE, D3DTA_DIFFUSE, D3DTOP_MODULATE, D3DTA_TEXTURE, D3DTA_DIFFUSE }, { D3DTOP_DISABLE, D3DTA_TEXTURE, D3DTA_CURRENT, D3DTOP_DISABLE, D3DTA_TEXTURE, D3DTA_DIFFUSE } },
    { { D3DTOP_BLENDDIFFUSEALPHA, D3DTA_TEXTURE, D3DTA_DIFFUSE, D3DTOP_SELECTARG2, D3DTA_TEXTURE, D3DTA_DIFFUSE }, { D3DTOP_DISABLE, D3DTA_TEXTURE, D3DTA_CURRENT, D3DTOP_DISABLE, D3DTA_TEXTURE, D3DTA_DIFFUSE } }
};

sTexBlendArgs sMultiTexBlendArgsProtos[LGD3DTB_NO_STATES][2] =
{
    { { D3DTOP_SELECTARG1, D3DTA_TEXTURE, D3DTA_DIFFUSE, D3DTOP_DISABLE, D3DTA_TEXTURE, D3DTA_DIFFUSE }, { D3DTOP_MODULATE, D3DTA_TEXTURE, D3DTA_CURRENT, D3DTOP_DISABLE, D3DTA_TEXTURE, D3DTA_DIFFUSE } },
    { { D3DTOP_SELECTARG1, D3DTA_TEXTURE, D3DTA_DIFFUSE, D3DTOP_SELECTARG2, D3DTA_TEXTURE, D3DTA_DIFFUSE }, { D3DTOP_SELECTARG2, D3DTA_TEXTURE, D3DTA_CURRENT, D3DTOP_MODULATE, D3DTA_TEXTURE, D3DTA_CURRENT } },
    { { D3DTOP_MODULATE, D3DTA_TEXTURE, D3DTA_DIFFUSE, D3DTOP_SELECTARG1, D3DTA_TEXTURE, D3DTA_DIFFUSE }, { D3DTOP_BLENDTEXTUREALPHA, D3DTA_TEXTURE, D3DTA_CURRENT, D3DTOP_SELECTARG2, D3DTA_TEXTURE, D3DTA_CURRENT } }
};

// TODO: initialize in ScrnInit3d
int b_SS2_UseSageTexManager;

ushort** texture_pal_list[PF_MAX] = { texture_pal_opaque, texture_pal_opaque, /* is it working? */ &alpha_pal, texture_pal_trans};
extern "C" BOOL lgd3d_blend_trans;
BOOL lgd3d_blend_trans = TRUE;
static D3DBLEND table[PF_MAX] = { D3DBLEND_ZERO, D3DBLEND_ONE, D3DBLEND_SRCCOLOR, D3DBLEND_DESTCOLOR };

void GetAvailableTexMem(ulong* local, ulong* agp);

void calc_size(tdrv_texture_info *info, d3d_cookie cookie); 
void GetAvailableTexMem(unsigned int *local, unsigned int *agp); 
void blit_8to16(tdrv_texture_info *info, uint16 *dst, int drow, uint16 *pal); 
void blit_8to16_trans(tdrv_texture_info *info, uint16 *dst, int drow, uint16 *pal); 
void blit_8to16_clut(tdrv_texture_info *info, uint16 *dst, int drow, uint16 *pal); 
void blit_8to16_scale(tdrv_texture_info *info, uint16 *dst, int drow, uint16 *pal); 
void blit_8to16_clut_scale(tdrv_texture_info *info, uint16 *dst, int drow, uint16 *pal); 
void blit_8to8(tdrv_texture_info *info, unsigned __int8 *dst, int drow); 
void blit_8to8_clut(tdrv_texture_info *info, unsigned __int8 *dst, int drow); 
void blit_8to8_scale(tdrv_texture_info *info, unsigned __int8 *dst, int drow); 
void blit_8to8_clut_scale(tdrv_texture_info *info, unsigned __int8 *dst, int drow); 
void blit_16to16(tdrv_texture_info *info, uint16 *dst, int drow); 
void blit_16to16_scale(tdrv_texture_info *info, uint16 *dst, int drow); 
void blit_32to32(tdrv_texture_info *info, uint16 *dst, int drow); 
void blit_32to32_scale(tdrv_texture_info *info, uint16 *dst, int drow); 
void blit_32to16(tdrv_texture_info *info, uint16 *dst, int drow);
int STDMETHODCALLTYPE EnumTextureFormatsCallback(DDPIXELFORMAT* lpDDPixFmt, void* lpContext);
void CheckSurfaces(sWinDispDevCallbackInfo *info); 
void InitDefaultTexture(int size); 
int FindClosestColor(float r, float g, float b); 
int init_size_tables(int **p_size_list); 
void init_size_table(int *size_table, unsigned __int8 type); 
int munge_size_tables(int **p_size_list); 
int get_num_sizes(int *size_list, int num_sizes, int *size_table); 

D3DZBUFFERTYPE cD6States::GetDepthBufferState()
{
    return m_psCurrentRS->eZenable;
}

BOOL cD6States::IsZWriteOn()
{
    return m_psCurrentRS->bZWriteEnable;
}

BOOL cD6States::IsZCompareOn()
{
    return m_psCurrentRS->eZCompareFunc == D3DCMP_LESSEQUAL;
}

BOOL cD6States::IsAlphaBlendingOn()
{
    return m_psCurrentRS->bAlphaOn;
}

BOOL cD6States::IsSmoothShadingOn()
{
    return m_psCurrentRS->eShadeMode == D3DSHADE_GOURAUD;
}

BOOL cD6States::IsFogOn()
{
    return m_psCurrentRS->bFogOn;
}

BOOL cD6States::GetDitheringState()
{
    return m_psCurrentRS->bDitheringOn;
}

BOOL cD6States::GetAntialiasingState()
{
    return m_psCurrentRS->bAntialiasingOn;
}

BOOL cD6States::GetTexWrapping(DWORD dwLevel)
{
    return m_psCurrentRS->eWrap[dwLevel] == D3DTADDRESS_WRAP;
}

void* cD6States::GetCurrentStates()
{
    return m_psCurrentRS;
}

BOOL cD6States::IsPaletteOn()
{
    return m_psCurrentRS->bUsePalette;
}

cD6States::cD6States()
{
	m_bTextureListInitialized = 0;
	m_bTexture_RGB = 0;
	m_bUsingLocalMem = 0;
	m_bLocalMem_available = 0;
	m_bAGP_available = 0;
	m_bWBuffer = 0;
	m_bSpecular = 0;
}

int cD6States::Initialize(ulong dwRequestedFlags)
{
    int i;
    IWinDisplayDevice* pWinDisplayDevice;

    m_texture_caps = g_sD3DDevDesc.dpcTriCaps.dwTextureCaps;
    m_bWBuffer = (dwRequestedFlags & LGD3DF_WBUFFER) != 0;

    EnumerateTextureFormats();

    for (i = 0; i < MAX_PALETTES; ++i)
    {
        texture_pal_opaque[i] = 0;
        texture_pal_trans[i] = 0;
        lpDDPalTexture[i] = 0;
    }

    pcStates->SetPalSlotFlags(0, 256, grd_pal, 0, 3);

    alpha_pal = default_alpha_pal;

    pcStates->SetChromaKey(m_psCurrentRS->chroma_r, m_psCurrentRS->chroma_g, m_psCurrentRS->chroma_b);

    memset(g_saTextures, 0, sizeof(g_saTextures));
    m_bTextureListInitialized = TRUE;

    pWinDisplayDevice = AppGetObj(IWinDisplayDevice);
    pWinDisplayDevice->AddTaskSwitchCallback(CheckSurfaces);
    SafeRelease(pWinDisplayDevice);

    InitDefaultTexture(16);

    InitTextureManager();
    return SetDefaultsStates(dwRequestedFlags);
}

#define NONLOCAL_CAPS DDSCAPS_TEXTURE | DDSCAPS_VIDEOMEMORY | DDSCAPS_NONLOCALVIDMEM | DDSCAPS_ALLOCONLOAD
#define LOCAL_CAPS DDSCAPS_TEXTURE | DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM | DDSCAPS_ALLOCONLOAD

void cD6States::EnumerateTextureFormats()
{
    HRESULT hr;
    DevDesc hal;
    DevDesc hel;
    IDirectDrawSurface4* test_surf;
    DDSURFACEDESC2 ddsd;

    memset(&g_RGBTextureFormat, 0, sizeof(g_RGBTextureFormat));
    memset(&g_TransRGBTextureFormat, 0, sizeof(g_TransRGBTextureFormat));
    memset(&g_PalTextureFormat, 0, sizeof(g_PalTextureFormat));
    memset(&g_8888TexFormat, 0, sizeof(g_8888TexFormat));
    g_b8888supported = 0;

    if (hr = g_lpD3Ddevice->EnumTextureFormats((LPD3DENUMPIXELFORMATSCALLBACK)EnumTextureFormatsCallback, 0), hr != S_OK)
    {
        CriticalMsg3(pcDXef, "EnumTextureFormats failed", (ushort)hr, GetDDErrorMsg(hr));
    }

    m_bTexture_RGB = g_bPrefer_RGB;

    g_FormatList[PF_GENERIC] = m_bTexture_RGB ? &g_RGBTextureFormat : &g_PalTextureFormat;

    if (!g_FormatList[PF_GENERIC]->dwFlags)
    {
        m_bTexture_RGB = m_bTexture_RGB == FALSE;
        g_FormatList[PF_GENERIC] = m_bTexture_RGB ? &g_RGBTextureFormat : &g_PalTextureFormat;
        if (!g_FormatList[PF_GENERIC]->dwFlags)
            CriticalMsg("Direct3D device does not support 8 bit palettized or 15 or 16 bit RGB textures");
    }

    g_FormatList[PF_RGB] = &g_RGBTextureFormat;
    g_FormatList[PF_ALPHA] = &g_AlphaTextureFormat;
    g_FormatList[PF_MASK] = &g_TransRGBTextureFormat;
    g_FormatList[PF_RGBA] = &g_8888TexFormat;

    mono_printf(("Using %s textures\n", m_bTexture_RGB ? "16 bit RGB" : "Palettized"));

    if (!g_AlphaTextureFormat.dwFlags)
        mono_printf(("no alpha texture format available.\n"));

    memset(&hal, 0, sizeof(hal));
    hal.dwSize = sizeof(hal);
    memset(&hel, 0, sizeof(hel));
    hel.dwSize = sizeof(hel);

    if (hr = g_lpD3Ddevice->GetCaps(&hal, &hel), hr != S_OK)
    {
        CriticalMsg3(pcDXef, "Failed to obtain device caps", (ushort)hr, GetDDErrorMsg(hr));
    }

    AssertMsg(hal.dwFlags & D3DDD_DEVCAPS, "No HAL device!");

    memset(&ddsd, 0, sizeof(ddsd));
    memcpy(&ddsd.ddpfPixelFormat, g_FormatList[0], sizeof(ddsd.ddpfPixelFormat));
    ddsd.dwHeight = 256;
    ddsd.dwWidth = 256;

    m_bAGP_available = 0;
    m_bLocalMem_available = 0;

    if ((hal.dwDevCaps & D3DDEVCAPS_TEXTURENONLOCALVIDMEM) != 0)
    {
        ddsd.dwSize = sizeof(ddsd);
        ddsd.dwFlags = DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS;
        ddsd.ddsCaps.dwCaps = NONLOCAL_CAPS;
        if (g_lpDD_ext->CreateSurface(&ddsd, &test_surf, 0) == S_OK)
        {
            if (test_surf)
            {
                test_surf->Release();
                test_surf = 0;
                m_bAGP_available = TRUE;
                m_DeviceSurfaceCaps = NONLOCAL_CAPS;
                if (bSpewOn)
                    mprintf("nonlocal videomemory textures available.\n");
            }
        }
    }

    if ((hal.dwDevCaps & D3DDEVCAPS_TEXTUREVIDEOMEMORY) != 0)
    {
        ddsd.dwSize = sizeof(ddsd);
        ddsd.dwFlags = DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS;
        ddsd.ddsCaps.dwCaps = LOCAL_CAPS;
        if (g_lpDD_ext->CreateSurface(&ddsd, &test_surf, 0) == S_OK)
        {
            if (test_surf)
            {
                test_surf->Release();
                test_surf = 0;
                m_bLocalMem_available = TRUE;
                m_DeviceSurfaceCaps = LOCAL_CAPS;
                if (bSpewOn)
                    mprintf("local videomemory textures available.\n");
            }
        }
    }

    if (!m_bAGP_available && !m_bLocalMem_available)
    {
        if (bSpewOn)
            mprintf("No local or nonlocal texture memory! Using system memory textures.\n");
        m_DeviceSurfaceCaps = DDSCAPS_ALLOCONLOAD | DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY;
    }

    m_bUsingLocalMem = m_bLocalMem_available;
}

void cD6States::InitTextureManager()
{
    if (g_tmgr)
    {
        g_tmgr->shutdown();
        g_tmgr = nullptr;
    }

    g_tmgr = get_dopey_texture_manager(pcStates);
    g_tmgr->init(default_bm, LGD3D_MAX_TEXTURES, m_texture_size_list, init_size_tables(&m_texture_size_list), TMGRF_SPEW);
}

ulong cD6States::GetRenderStatesSize()
{
    return sizeof(sRenderStates);
}

void cD6States::SetPointerToCurrentStates(char* p)
{
    m_psCurrentRS = (sRenderStates*)p;
}

void cD6States::SetPointerToSetStates(char* p)
{
    m_psSetRS = (sRenderStates*)p;
}

void cD6States::SetCommonDefaultStates(ulong dwRequestedFlags, int bMultiTexture)
{
    HRESULT hRes, hResult, hR;
    ulong dwPasses;

    memset(m_psCurrentRS, 0, sizeof(sRenderStates));

    m_bCanDither = (dwRequestedFlags & LGD3DF_CAN_DO_DITHER) != 0;
    m_psCurrentRS->bDitheringOn = m_bCanDither != 0 && (dwRequestedFlags & LGD3DF_DITHER) != 0;
    SetRenderStateForGlobal(g_lpD3Ddevice, D3DRENDERSTATE_DITHERENABLE, m_psCurrentRS->bDitheringOn);

    m_bCanAntialias = (dwRequestedFlags & LGD3DF_CAN_DO_ANTIALIAS) != 0;
    m_psCurrentRS->bAntialiasingOn = m_bCanAntialias != 0 && (dwRequestedFlags & LGD3DF_ANTIALIAS) != 0;
    SetRenderStateForGlobal(g_lpD3Ddevice, D3DRENDERSTATE_ANTIALIAS, m_psCurrentRS->bAntialiasingOn != 0 ? D3DANTIALIAS_SORTINDEPENDENT : D3DANTIALIAS_NONE);

    m_psCurrentRS->bUsePalette = TRUE;
    m_psCurrentRS->nAlphaColor = 255;

    // cD6Primitives
    pcRenderBuffer->PassAlphaColor(m_psCurrentRS->nAlphaColor);

    m_psCurrentRS->eShadeMode = D3DSHADE_GOURAUD;
    SetRenderStateForGlobal(g_lpD3Ddevice, D3DRENDERSTATE_SHADEMODE, m_psCurrentRS->eShadeMode);

    if (g_bUseDepthBuffer)
    {
        // choose between z- and w- buffering
        m_psCurrentRS->eZenable = D3DZBUFFERTYPE((m_bWBuffer != FALSE) + 1);
        SetRenderStateForGlobal(g_lpD3Ddevice, D3DRENDERSTATE_ZENABLE, m_psCurrentRS->eZenable);

        m_psCurrentRS->bZWriteEnable = FALSE;
        SetRenderStateForGlobal(g_lpD3Ddevice, D3DRENDERSTATE_ZWRITEENABLE, m_psCurrentRS->bZWriteEnable);

        m_psCurrentRS->eZCompareFunc = D3DCMP_ALWAYS;
        SetRenderStateForGlobal(g_lpD3Ddevice, D3DRENDERSTATE_ZFUNC, m_psCurrentRS->eZCompareFunc);
    }

    m_psCurrentRS->bFogOn = 0;
    m_psCurrentRS->dwFogColor = 0;
    m_psCurrentRS->eFogMode = D3DFOG_EXP;
    m_psCurrentRS->fFogTableDensity = 0.025f;
    m_psCurrentRS->dcFogSpecular = 0xFF000000;

    // primitives
    pcRenderBuffer->PassFogSpecularColor(m_psCurrentRS->dcFogSpecular);

    if (g_bUseTableFog)
    {
        SetRenderStateForGlobal(g_lpD3Ddevice, D3DRENDERSTATE_FOGENABLE, m_psCurrentRS->bFogOn);
        SetRenderStateForGlobal(g_lpD3Ddevice, D3DRENDERSTATE_FOGTABLEMODE, m_psCurrentRS->eFogMode);
        SetRenderStateForGlobal(g_lpD3Ddevice, D3DRENDERSTATE_FOGCOLOR, m_psCurrentRS->dwFogColor);
        SetRenderStateForGlobal(g_lpD3Ddevice, D3DRENDERSTATE_FOGTABLEDENSITY, m_psCurrentRS->fFogTableDensity);
    }
    else if (g_bUseVertexFog)
    {
        SetRenderStateForGlobal(g_lpD3Ddevice, D3DRENDERSTATE_FOGENABLE, m_psCurrentRS->bFogOn);
        SetRenderStateForGlobal(g_lpD3Ddevice, D3DRENDERSTATE_FOGCOLOR, m_psCurrentRS->dwFogColor);
    }

    m_psCurrentRS->naTextureId[0] = -1;
    m_psCurrentRS->naTextureId[1] = -1;
    if (hR = g_lpD3Ddevice->SetTexture(0, NULL), hR != S_OK)
    {
        CriticalMsg3(pcDXef, "SetTexture failed", (ushort)hR, GetDDErrorMsg(hR));
    }

    m_psCurrentRS->bPerspectiveCorr = 1;
    SetRenderStateForGlobal(g_lpD3Ddevice, D3DRENDERSTATE_TEXTUREPERSPECTIVE, m_psCurrentRS->bPerspectiveCorr);

    m_psCurrentRS->baChromaKeying[0] = 0;
    SetRenderStateForGlobal(g_lpD3Ddevice, D3DRENDERSTATE_COLORKEYENABLE, m_psCurrentRS->baChromaKeying[0]);
    

    m_psCurrentRS->eWrap[0] = D3DTADDRESS_WRAP;
    SetTextureStageStateForGlobal(g_lpD3Ddevice, 0, D3DTSS_ADDRESS, m_psCurrentRS->eWrap[0]);

    m_psCurrentRS->eMagTexFilter = D3DTFN_LINEAR;
    m_psCurrentRS->eMinTexFilter = D3DTFN_LINEAR;

    SetTextureStageStateForGlobal(g_lpD3Ddevice, 0, D3DTSS_MAGFILTER, m_psCurrentRS->eMagTexFilter);
    SetTextureStageStateForGlobal(g_lpD3Ddevice, 0, D3DTSS_MINFILTER, m_psCurrentRS->eMinTexFilter);

    m_psCurrentRS->dwTexBlendMode[1] = 0;
    m_psCurrentRS->baTextureWithAlpha[0] = 0;
    m_psCurrentRS->baTextureWithAlpha[1] = 0;

    SetRenderStateForGlobal(g_lpD3Ddevice, D3DRENDERSTATE_SPECULARENABLE, m_bSpecular);
    SetRenderStateForGlobal(g_lpD3Ddevice, D3DRENDERSTATE_STENCILENABLE, 0);

    m_psSetRS->dwNoLightMapLevels = 2;
    memcpy(m_psCurrentRS->saTexBlend, sTexBlendArgsProtos[0], sizeof(m_psCurrentRS->saTexBlend));

    SetTextureStageStateForGlobal(g_lpD3Ddevice, 0, D3DTSS_TEXCOORDINDEX, 0);
    SetTextureStageColors(g_lpD3Ddevice, 0, m_psCurrentRS);

    if (bMultiTexture)
    {
        SetTextureStageStateForGlobal(g_lpD3Ddevice, 1, D3DTSS_TEXCOORDINDEX, 1);
        SetTextureStageColors(g_lpD3Ddevice, 1, m_psCurrentRS);
    }

    m_bCanModulate = g_lpD3Ddevice->ValidateDevice(&dwPasses) == S_OK;
    memcpy(m_psCurrentRS->saTexBlend, sTexBlendArgsProtos[1], sizeof(m_psCurrentRS->saTexBlend));

    SetTextureStageStateForGlobal(g_lpD3Ddevice, 0, D3DTSS_TEXCOORDINDEX, 0);
    SetTextureStageColors(g_lpD3Ddevice, 0, m_psCurrentRS);

    if (bMultiTexture)
    {
        SetTextureStageStateForGlobal(g_lpD3Ddevice, 1, D3DTSS_TEXCOORDINDEX, 1);
        SetTextureStageColors(g_lpD3Ddevice, 1, m_psCurrentRS);
    }

    hR = g_lpD3Ddevice->ValidateDevice(&dwPasses);
    m_bCanModulateAlpha = hR == S_OK;

    if (m_bCanModulate || m_bCanModulateAlpha)
    {
        m_psCurrentRS->dwTexBlendMode[0] = (dwRequestedFlags & LGD3DF_MODULATEALPHA) != 0 ? m_bCanModulateAlpha != FALSE : m_bCanModulate == FALSE;
    }
    else
    {
        SetLGD3DErrorCode(LGD3D_EC_VD_MPASS_MT, hR);
        if (bSpewOn)
        {
            CriticalMsg4(pcLGD3Def, LGD3D_EC_VD_MPASS_MT, GetLgd3dErrorCode(LGD3D_EC_VD_MPASS_MT), (ushort)hR, GetDDErrorMsg(hR));
        }
        else
        {
            Warning((pcLGD3Def, LGD3D_EC_VD_MPASS_MT, GetLgd3dErrorCode(LGD3D_EC_VD_MPASS_MT), (ushort)hR, GetDDErrorMsg(hR)));
        }
        lgd3d_g_bInitialized = 0;
    }
}

int cD6States::SetDefaultsStates(ulong dwRequestedFlags)
{
    HRESULT hRes, hResult, hR;
    ulong dwPasses;

    SetCommonDefaultStates(dwRequestedFlags, FALSE);

    memcpy(m_psCurrentRS->saTexBlend, sTexBlendArgsProtos[m_psCurrentRS->dwTexBlendMode[0]], sizeof(m_psCurrentRS->saTexBlend));

    SetTextureStageStateForGlobal(g_lpD3Ddevice, 0, D3DTSS_TEXCOORDINDEX, 0);
    SetTextureStageColors(g_lpD3Ddevice, 0, m_psCurrentRS);

    SetRenderStateForGlobal(g_lpD3Ddevice, D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);

    if ((dwRequestedFlags & LGD3DF_MULTITEXTURE_COLOR) != 0)
    {
        SetRenderStateForGlobal(g_lpD3Ddevice, D3DRENDERSTATE_SRCBLEND, D3DBLEND_DESTCOLOR);
        SetRenderStateForGlobal(g_lpD3Ddevice, D3DRENDERSTATE_DESTBLEND, D3DBLEND_ZERO);

        if (hR = g_lpD3Ddevice->ValidateDevice(&dwPasses), hR != S_OK)
        {
            SetLGD3DErrorCode(LGD3D_EC_VD_MPASS_MT, hR);
            if (bSpewOn)
            {
                CriticalMsg4(pcLGD3Def, LGD3D_EC_VD_MPASS_MT, GetLgd3dErrorCode(LGD3D_EC_VD_MPASS_MT), (ushort)hR, GetDDErrorMsg(hR));
            }
            else
            {
                Warning((pcLGD3Def, LGD3D_EC_VD_MPASS_MT, GetLgd3dErrorCode(LGD3D_EC_VD_MPASS_MT), (ushort)hR, GetDDErrorMsg(hR)));
            }
            lgd3d_g_bInitialized = 0;

            return FALSE;
        }
    }

    m_psCurrentRS->eSrcAlpha = D3DBLEND_SRCALPHA;
    SetRenderStateForGlobal(g_lpD3Ddevice, D3DRENDERSTATE_SRCBLEND, m_psCurrentRS->eSrcAlpha);
    m_psCurrentRS->eDstAlpha = D3DBLEND_INVSRCALPHA;
    SetRenderStateForGlobal(g_lpD3Ddevice, D3DRENDERSTATE_DESTBLEND, m_psCurrentRS->eDstAlpha);

    if (hR = g_lpD3Ddevice->ValidateDevice(&dwPasses), hR != S_OK)
    {
        SetLGD3DErrorCode(LGD3D_EC_VD_S_DEFAULT, hR);
        if (bSpewOn)
        {
            CriticalMsg4(pcLGD3Def, LGD3D_EC_VD_S_DEFAULT, GetLgd3dErrorCode(LGD3D_EC_VD_S_DEFAULT), (ushort)hR, GetDDErrorMsg(hR));
        }
        else
        {
            Warning((pcLGD3Def, LGD3D_EC_VD_S_DEFAULT, GetLgd3dErrorCode(LGD3D_EC_VD_S_DEFAULT), (ushort)hR, GetDDErrorMsg(hR)));
        }
        lgd3d_g_bInitialized = 0;

        return FALSE;
    }

    memcpy(m_psCurrentRS->saTexBlend, sTexBlendArgsProtos[m_psCurrentRS->dwTexBlendMode[0]], sizeof(m_psCurrentRS->saTexBlend));

    SetTextureStageStateForGlobal(g_lpD3Ddevice, 0, D3DTSS_TEXCOORDINDEX, 0);
    SetTextureStageColors(g_lpD3Ddevice, 0, m_psCurrentRS);

    m_psCurrentRS->bAlphaOn = FALSE;
    SetRenderStateForGlobal(g_lpD3Ddevice, D3DRENDERSTATE_ALPHABLENDENABLE, m_psCurrentRS->bAlphaOn);

    memcpy(m_psSetRS, m_psCurrentRS, sizeof(sRenderStates));

    g_dwRSChangeFlags = 0;

    return TRUE;
}

void cD6States::synchronize()
{
    pcRenderBuffer->FlushPrimitives();
}

void cD6States::start_frame(int n)
{
    pcRenderer->StartFrame(n);
}

void cD6States::end_frame()
{
    pcRenderer->EndFrame();
}

int cD6States::load_texture(tdrv_texture_info* info)
{
    int n = info->id;
    DWORD local_end = 0, agp_end = 0;
    DWORD local_start = 0, agp_start = 0;
    int size;
    int status;

    put_mono('a');

    size = info->size_index < 0 ? 0 : m_texture_size_list[info->size_index];

    if ((m_texture_caps & D3DPTEXTURECAPS_SQUAREONLY) && (info->w != info->h)) {
        put_mono('.');
        return TMGR_FAILURE;
    }

    if (!b_SS2_UseSageTexManager)
    {
        GetAvailableTexMem(m_bLocalMem_available != 0 ? &local_start : 0, m_bAGP_available != 0 ? &agp_start : 0);

        if (local_start < size && m_bUsingLocalMem && m_bAGP_available)
        {
            if (bSpewOn)
                mprintf("using nonlocal vidmem textures...\n");

            m_bUsingLocalMem = 0;
            m_DeviceSurfaceCaps = NONLOCAL_CAPS;
        }

        if ((m_bUsingLocalMem ? local_start : agp_start) < size)
            return TMGR_FAILURE;
    }

    if (n >= LGD3D_MAX_TEXTURES)
        CriticalMsg("Texture id out of range");

    status = reload_texture(info);

    if (status && m_bUsingLocalMem && m_bAGP_available)
    {
        mono_printf(("using nonlocal vidmem textures...\n"));

        m_bUsingLocalMem = 0;
        m_DeviceSurfaceCaps = NONLOCAL_CAPS;

        status = reload_texture(info);
    }

    if (status != TMGR_SUCCESS)
        return status;

    if (b_SS2_UseSageTexManager || size)
        return TMGR_SUCCESS;

    GetAvailableTexMem(m_bLocalMem_available != 0 ? &local_end : 0, m_bAGP_available != 0 ? &agp_end : 0);

    if (m_bLocalMem_available && local_start > local_end)
    {
        info->size_index = local_start - local_end;

        if (m_bAGP_available)
            info->size_index += agp_start - agp_end;

        return TMGR_SUCCESS;
    }

    if (m_bAGP_available && agp_start != agp_end)
    {
        info->size_index = agp_start - agp_end;

        return TMGR_SUCCESS;
    }

    Warning(("Texture load took no space!\n"));

    if (info->w == 1 && info->h == 1)
    {
        info->size_index = 16;

        return TMGR_SUCCESS;
    }
    else
    {
        Error(1, "Direct3d device driver does not accurately report texture memory usage.\n  Contact your 3d accelerator vendor for updated drivers.");
        return TMGR_FAILURE;
    }
}

void cD6States::release_texture(int n)
{
    DDSCAPS2 ddscaps;
    sTextureData* psTexData;

    if (n >= LGD3D_MAX_TEXTURES)
        CriticalMsg("Texture id out of range");

    psTexData = &g_saTextures[n];

    if (n == m_psCurrentRS->naTextureId[0])
        pcRenderBuffer->FlushPrimitives();

    if (!psTexData->lpSurface)
        Warning(("texture %i already released\n", n));

    if (psTexData->lpSurface && m_bLocalMem_available && !m_bUsingLocalMem)
    {
        psTexData->lpSurface->GetCaps(&ddscaps);

        if ((ddscaps.dwCaps & DDSCAPS_LOCALVIDMEM) != 0)
        {
            if (bSpewOn)
                mprintf("Using Local vidmem textures\n");

            m_DeviceSurfaceCaps = LOCAL_CAPS;
            m_bUsingLocalMem = TRUE;
        }
    }

    SafeRelease(psTexData->lpTexture);
    SafeRelease(psTexData->lpSurface);

    psTexData->pTdrvBitmap = 0;
}

int cD6States::reload_texture(tdrv_texture_info* info)
{
    IDirectDrawSurface4* SysmemSurface, ** pDeviceSurface;
    DDSURFACEDESC2 ddsd;
    d3d_cookie cookie;
    LPDDPIXELFORMAT pixel_format;
    HRESULT LastError;
    IDirect3DTexture2* SysmemTexture = NULL, ** pDeviceTexture;
    sTextureData* psTexData;

    psTexData = &g_saTextures[info->id];

    pDeviceTexture = &psTexData->lpTexture;
    pDeviceSurface = &psTexData->lpSurface;
    cookie.value = info->cookie;

    pixel_format = g_FormatList[cookie.flags & (PF_RGBA | PF_RGB | PF_ALPHA)];
    if ((cookie.flags & (PF_RGBA | PF_RGB | PF_ALPHA)) == PF_RGBA && !g_b8888supported)
        pixel_format = g_FormatList[2];

    if (pixel_format->dwFlags == 0)
        return TMGR_FAILURE;

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY;
    ddsd.dwHeight = info->h;
    ddsd.dwWidth = info->w;
    memcpy(&ddsd.ddpfPixelFormat, pixel_format, sizeof(ddsd.ddpfPixelFormat));

    LastError = CreateDDSurface(cookie, &ddsd, &SysmemSurface);
    CheckHResult(LastError, "CreateDDSurface() failed");
    AssertMsg(SysmemSurface != NULL, "NULL SysmemSurface");

    LastError = SysmemSurface->QueryInterface(IID_IDirect3DTexture2, (LPVOID*)&SysmemTexture);
    CheckHResult(LastError, "Failed to obtain D3D texture interface for sysmem surface");

    ddsd.dwSize = sizeof(DDSURFACEDESC2);
    ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;

    if (*pDeviceTexture == NULL) {
        IDirectDrawSurface4* DeviceSurface;

        ddsd.ddsCaps.dwCaps = m_DeviceSurfaceCaps;
        LastError = CreateDDSurface(cookie, &ddsd, pDeviceSurface);
        if ((LastError != DD_OK) && m_bUsingLocalMem && m_bAGP_available)
        {
            mono_printf(("using nonlocal vidmem textures...\n"));
            // try AGP textures...
            m_bUsingLocalMem = FALSE;
            m_DeviceSurfaceCaps = NONLOCAL_CAPS;
            ddsd.ddsCaps.dwCaps = m_DeviceSurfaceCaps;
            LastError = CreateDDSurface(cookie, &ddsd, pDeviceSurface);
        }

        if (LastError != DD_OK) {
            mono_printf(("couldn't load texture: error %i.\n", LastError & 0xffff));
            SafeRelease(SysmemTexture);
            SafeRelease(SysmemSurface);
            *pDeviceSurface = NULL;
            return TMGR_FAILURE;
        }

        DeviceSurface = *pDeviceSurface;

#define MEMORY_TYPE_MASK \
      (DDSCAPS_SYSTEMMEMORY|DDSCAPS_VIDEOMEMORY|DDSCAPS_LOCALVIDMEM|DDSCAPS_NONLOCALVIDMEM)

        if ((m_DeviceSurfaceCaps & MEMORY_TYPE_MASK) != (ddsd.ddsCaps.dwCaps & MEMORY_TYPE_MASK))
            mono_printf((
                "device texture not created in requested memory location\nRequested %x Received %x\n",
                m_DeviceSurfaceCaps & MEMORY_TYPE_MASK, ddsd.ddsCaps.dwCaps & MEMORY_TYPE_MASK));


        // Query our device surface for a texture interface
        LastError = DeviceSurface->QueryInterface(IID_IDirect3DTexture2, (LPVOID*)pDeviceTexture);
        CheckHResult(LastError, "Failed to obtain D3D texture interface for device surface.");
    }

    // Load the bitmap into the sysmem surface
    LastError = SysmemSurface->Lock(NULL, &ddsd, 0, NULL);
    CheckHResult(LastError, "Failed to lock sysmem surface.");

    LoadSurface(info, &ddsd);

    LastError = SysmemSurface->Unlock(NULL);
    CheckHResult(LastError, "Failed to unlock sysmem surface.");

    // Load the sysmem texture into the device texture.  During this call, a
    // driver could compress or reformat the texture surface and put it in
    // video memory.

    LastError = pDeviceTexture[0]->Load(SysmemTexture);
    CheckHResult(LastError, "Failed to load device texture from sysmem texture.");

    // Now we are done with sysmem surface

    SafeRelease(SysmemTexture);
    SafeRelease(SysmemSurface);

    psTexData->pTdrvBitmap = info->bm;
    psTexData->TdrvCookie.value = info->cookie;

    return TMGR_SUCCESS;
}

void cD6States::cook_info(tdrv_texture_info* info)
{
	d3d_cookie cookie;
	int v, w = info->w, h = info->h;

	cookie.palette = 0;

	if (gr_get_fill_type() == FILL_BLEND)
	{
		cookie.flags = PF_ALPHA;
	}
	else
	{
		switch (info->bm->type)
		{
		case BMT_FLAT8:
			cookie.palette = info->bm->align;
			if (info->bm->flags & BMF_TRANS)
			{
				if (texture_pal_trans[cookie.palette] != NULL && g_TransRGBTextureFormat.dwFlags != 0 &&
					(m_psCurrentRS->dwTexBlendMode[0] != FALSE || m_psCurrentRS->nAlphaColor >= 255))
					cookie.flags = PF_MASK;
				else
					cookie.flags = PF_TRANS;
			}
			else
			{
				cookie.flags = PF_GENERIC;
			}
			break;
		case BMT_FLAT16:
			if (gr_test_bitmap_format(info->bm, BMF_RGB_4444))
			{
				cookie.flags = PF_ALPHA;
			}
			else if (info->bm->flags & BMF_TRANS)
			{
				if (g_TransRGBTextureFormat.dwFlags != 0 && (m_psCurrentRS->dwTexBlendMode[0] != FALSE || m_psCurrentRS->nAlphaColor >= 255))
					cookie.flags = PF_MASK;
				else
					cookie.flags = PF_TRANS | PF_RGB;
			}
			else
				cookie.flags = PF_RGB;
			break;
			// BMT_TLUC8+1
		case BMT_FLAT32:
			cookie.flags = PF_RGBA;
			break;
		}
	}

	calc_size(info, cookie);

	cookie.hlog = 0;
	cookie.wlog = 0;
	for (v = 2; v <= w; v += v)
	{
		cookie.wlog++;
	}
	for (v = 2; v <= h; v += v)
	{
		cookie.hlog++;
	}

	AssertMsg((info->h == (1 << cookie.hlog)) && (info->w == (1 << cookie.wlog)),
		"hlog/wlog does not match texture width/height");

	info->cookie = cookie.value;
}

// Disconnect texture from bitmap, but don't release texture yet
void cD6States::unload_texture(int n)
{
	AssertMsg((n >= 0) && (n < LGD3D_MAX_TEXTURES), "Texture id out of range");
	g_saTextures[n].pTdrvBitmap = NULL;
}

/***************************************************************************/
/*                    Loading a grs_bitmap into a system memory surface    */
/***************************************************************************/
/*
 * LoadSurface
 * Loads a grs_bitmap into a texture map DD surface of the given format.  The
 * memory flag specifies DDSCAPS_SYSTEMMEMORY or DDSCAPS_VIDEOMEMORY.
 */

long cD6States::CreateDDSurface(d3d_cookie cookie, DDSURFACEDESC2* pddsd, LPDIRECTDRAWSURFACE4* ppDDS)
{
	HRESULT ddrval;
	LPDIRECTDRAWSURFACE4 pDDS;
	DDCOLORKEY ck;
	DWORD ckey;

	ddrval = g_lpDD_ext->CreateSurface(pddsd, ppDDS, NULL);
	if (ddrval != DD_OK)
		return ddrval;

	pDDS = *ppDDS;

	// Bind the palette, if necessary
	if (pddsd->ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED8) {
		ddrval = pDDS->SetPalette(lpDDPalTexture[cookie.palette]);
		CheckHResult(ddrval, "SetPalette failed while creating surface.");
		ckey = 0;
	}
	else
		ckey = m_psCurrentRS->chroma_key;

	// Set colorkey, if necessary
	if (PF_TRANS & cookie.flags) {
		ck.dwColorSpaceLowValue = ckey;
		ck.dwColorSpaceHighValue = ckey;
		ddrval = pDDS->SetColorKey(DDCKEY_SRCBLT, &ck);
		CheckHResult(ddrval, "SetColorKey failed while creating surface.");
	}

	return DD_OK;
}

void cD6States::LoadSurface(tdrv_texture_info* info, DDSURFACEDESC2* ddsd)
{
	grs_bitmap* bm = info->bm;
	uchar zero_save = 0;
	d3d_cookie cookie;

	cookie.value = info->cookie;

	if ((texture_clut != NULL) && (bm->flags & BMF_TRANS))
	{
		zero_save = texture_clut[0];
		texture_clut[0] = 0;
	}

	if (ddsd->ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED8) {
		uchar* dst = (uchar*)ddsd->lpSurface;
		if ((info->scale_w | info->scale_h) != 0)
			if (texture_clut != NULL)
				blit_8to8_clut_scale(info, dst, ddsd->lPitch);
			else
				blit_8to8_scale(info, dst, ddsd->lPitch);
		else
			if (texture_clut != NULL)
				blit_8to8_clut(info, dst, ddsd->lPitch);
			else
				blit_8to8(info, dst, ddsd->lPitch);
	}
	else {
		ushort* dst = (ushort*)ddsd->lpSurface;
		int drow = ddsd->lPitch / 2;
		char type = cookie.flags & (PF_RGBA | PF_RGB | PF_ALPHA);

		if (bm->type == BMT_FLAT8) {
			ushort* pal;
			ushort zero_save;

			pal = texture_pal_list[cookie.flags & PF_MASK][cookie.palette];
			if (pal == NULL)
				pal = grd_pal16_list[0];

			if (cookie.flags & PF_TRANS) {
				zero_save = pal[0];
				pal[0] = m_psCurrentRS->chroma_key;
			}

			AssertMsg(pal != NULL, "Hey! trying to use NULL 16 bit palette to load texture!");

			if ((info->scale_w | info->scale_h) != 0)
				if ((type & PF_ALPHA) || (texture_clut == NULL))
					blit_8to16_scale(info, dst, drow, pal);
				else
					blit_8to16_clut_scale(info, dst, drow, pal);
			else
				if ((type & PF_ALPHA) || (texture_clut == NULL))
					blit_8to16(info, dst, drow, pal);
				else if (type == PF_MASK && lgd3d_blend_trans)
					blit_8to16_trans(info, dst, drow, pal);
				else
					blit_8to16_clut(info, dst, drow, pal);

			if (cookie.flags & PF_TRANS)
				pal[0] = zero_save;

		}
		else if (bm->type == BMT_FLAT32)
		{
			if (g_b8888supported)
				if ((info->scale_w | info->scale_h) != 0)
					blit_32to32_scale(info, dst, drow);
				else
					blit_32to32(info, dst, drow);
			else
				blit_32to16(info, dst, drow);

		}
		else { // assume straight 16 to 16
			if ((info->scale_w | info->scale_h) != 0)
				blit_16to16_scale(info, dst, drow);
			else
				blit_16to16(info, dst, drow);
		}

	}

	if (zero_save != 0)
		texture_clut[0] = zero_save;
}


void cD6States::release_texture_a(int n)
{
	DDSCAPS2 ddscaps;
	sTextureData* psTexData;

	AssertMsg(n < LGD3D_MAX_TEXTURES, "Texture id out of range");

	psTexData = &g_saTextures[n];

	if (n == m_psCurrentRS->naTextureId[0])
		pcRenderBuffer->FlushPrimitives();

	SafeRelease(psTexData->lpTexture);
	SafeRelease(psTexData->lpSurface);

	psTexData->pTdrvBitmap = 0;
}

int cD6States::load_texture_a(tdrv_texture_info* info)
{
	int n = info->id;
	sTextureData* psTexData;

	put_mono('a');

	if ((m_texture_caps & D3DPTEXTURECAPS_SQUAREONLY) && (info->w != info->h)) {
		put_mono('.');
		return TMGR_FAILURE;
	}

	AssertMsg(info->id < LGD3D_MAX_TEXTURES, "Texture id out of range");

	psTexData = &g_saTextures[n];

	if (n == m_psCurrentRS->naTextureId[0])
		pcRenderBuffer->FlushPrimitives();

	SafeRelease(psTexData->lpTexture);
	SafeRelease(psTexData->lpSurface);

	return reload_texture_a(info);
}

int cD6States::reload_texture_a(tdrv_texture_info* info)
{
	sTextureData* psTexData;
	LPDDPIXELFORMAT pixel_format;
	IDirectDrawSurface4* SysmemSurface, ** pDeviceSurface;
	HRESULT LastError;
	DDSURFACEDESC2 ddsd;
	IDirect3DTexture2** pDeviceTexture;
	d3d_cookie cookie;
	IDirectDrawSurface4* DeviceSurface;

	psTexData = &g_saTextures[info->id];
	pDeviceTexture = &g_saTextures[info->id].lpTexture;
	pDeviceSurface = &g_saTextures[info->id].lpSurface;
	cookie.value = info->cookie;

	pixel_format = g_FormatList[cookie.flags & (PF_RGBA | PF_RGB | PF_ALPHA)];

	if (pixel_format->dwFlags == 0)
		return TMGR_FAILURE;

	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_TEXTURESTAGE | DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS;
	ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY;
	ddsd.dwHeight = info->h;
	ddsd.dwWidth = info->w;
	memcpy(&ddsd.ddpfPixelFormat, pixel_format, sizeof(ddsd.ddpfPixelFormat));

	LastError = CreateDDSurface(cookie, &ddsd, &SysmemSurface);
	CheckHResult(LastError, "CreateDDSurface() failed");
	AssertMsg(SysmemSurface != NULL, "NULL SysmemSurface");

	if (*pDeviceTexture == NULL) {

		ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
		ddsd.ddsCaps.dwCaps2 = DDSCAPS2_TEXTUREMANAGE;
		ddsd.dwTextureStage = (cookie.flags & PF_STAGE);
		LastError = CreateDDSurface(cookie, &ddsd, pDeviceSurface);
		CheckHResult(LastError, "CreateDDSurface() failed");
		AssertMsg(SysmemSurface != NULL, "NULL SysmemSurface");

		DeviceSurface = *pDeviceSurface;

		// Query our device surface for a texture interface
		LastError = DeviceSurface->QueryInterface(IID_IDirect3DTexture2, (LPVOID*)pDeviceTexture);
		CheckHResult(LastError, "Failed to obtain D3D texture interface for device surface.");
	}

	// Load the bitmap into the sysmem surface
	LastError = SysmemSurface->Lock(NULL, &ddsd, 0, NULL);
	CheckHResult(LastError, "Failed to lock sysmem surface.");

	LoadSurface(info, &ddsd);

	LastError = SysmemSurface->Unlock(NULL);
	CheckHResult(LastError, "Failed to unlock sysmem surface.");

	LastError = DeviceSurface->Blt(NULL, SysmemSurface, NULL, DDBLT_WAIT, NULL);
	CheckHResult(LastError, "Failed to blit sysmem surface.");

	SafeRelease(SysmemSurface);

	psTexData->pTdrvBitmap = info->bm;
	psTexData->TdrvCookie.value = info->cookie;

	return TMGR_SUCCESS;
}

int cD6States::get_texture_id()
{
	return m_psCurrentRS->naTextureId[0];
}

void cD6States::TurnOffTexuring(BOOL bTexOff)
{
	HRESULT hResult;

	if (!bTexOff)
	{
		SetTextureNow();
		return;
	}

	hResult = g_lpD3Ddevice->SetTexture(0, NULL);
	CheckHResult(hResult, "SetTexture failed");
	m_psSetRS->naTextureId[0] = TMGR_ID_SOLID;
	g_bTexSuspended = TRUE;
}

void cD6States::SetDithering(BOOL bOn)
{
	HRESULT hRes;

	if (!m_bCanDither)
		return;

	m_psCurrentRS->bDitheringOn = bOn;

	if (m_psSetRS->bDitheringOn != m_psCurrentRS->bDitheringOn)
	{
		pcRenderBuffer->FlushPrimitives();
		SetRenderStateForGlobal(g_lpD3Ddevice, D3DRENDERSTATE_DITHERENABLE, m_psCurrentRS->bDitheringOn);
		m_psSetRS->bDitheringOn = m_psCurrentRS->bDitheringOn;
	}
}

void cD6States::SetAntialiasing(BOOL bOn)
{
	HRESULT hRes;

	if (!m_bCanAntialias)
		return;

	m_psCurrentRS->bAntialiasingOn = bOn;

	if (m_psSetRS->bAntialiasingOn != m_psCurrentRS->bAntialiasingOn)
	{
		pcRenderBuffer->FlushPrimitives();
		SetRenderStateForGlobal(g_lpD3Ddevice, D3DRENDERSTATE_ANTIALIAS, m_psCurrentRS->bAntialiasingOn != 0 ? D3DANTIALIAS_SORTINDEPENDENT : D3DANTIALIAS_NONE);
		m_psSetRS->bAntialiasingOn = m_psCurrentRS->bAntialiasingOn;
	}
}

void cD6States::EnableDepthBuffer(int nFlag)
{
	HRESULT hRes;

	pcRenderBuffer->FlushPrimitives();

	m_psCurrentRS->eZenable = (D3DZBUFFERTYPE)nFlag;
	m_psSetRS->eZenable = (D3DZBUFFERTYPE)nFlag;

	if (!lgd3d_punt_d3d)
		SetRenderStateForGlobal(g_lpD3Ddevice, D3DRENDERSTATE_ZENABLE, m_psCurrentRS->eZenable);
}

void cD6States::SetZWrite(BOOL bZWriteOn)
{
	HRESULT hRes;

	pcRenderBuffer->FlushPrimitives();

	m_psCurrentRS->bZWriteEnable = bZWriteOn;
	m_psSetRS->bZWriteEnable = bZWriteOn;

	if (!lgd3d_punt_d3d)
		SetRenderStateForGlobal(g_lpD3Ddevice, D3DRENDERSTATE_ZWRITEENABLE, m_psCurrentRS->bZWriteEnable);
}

void cD6States::SetZCompare(BOOL bZCompreOn)
{
	HRESULT hRes;

	pcRenderBuffer->FlushPrimitives();

	m_psCurrentRS->eZCompareFunc = bZCompreOn ? D3DCMP_LESSEQUAL : D3DCMP_ALWAYS;
	m_psSetRS->eZCompareFunc = m_psCurrentRS->eZCompareFunc;

	if (!lgd3d_punt_d3d)
		SetRenderStateForGlobal(g_lpD3Ddevice, D3DRENDERSTATE_ZFUNC, m_psCurrentRS->eZCompareFunc);
}

void cD6States::SetFogDensity(float fDensity)
{
	HRESULT hRes;

	if (!g_bUseTableFog && !g_bUseVertexFog)
		return;

	pcRenderBuffer->FlushPrimitives();

	if (!g_bWFog)
		fDensity = fDensity * (0.5 * z_far);

	m_psCurrentRS->fFogTableDensity = fDensity;
	m_psSetRS->fFogTableDensity = fDensity;

	if (!lgd3d_punt_d3d)
		SetRenderStateForGlobal(g_lpD3Ddevice, D3DRENDERSTATE_FOGTABLEDENSITY, m_psCurrentRS->fFogTableDensity);
}

BOOL cD6States::UseLinearTableFog(BOOL bOn)
{
	HRESULT hRes;
	BOOL bWas;

	if (!g_bUseTableFog)
		return FALSE;

	bWas = m_psCurrentRS->eFogMode == D3DFOG_LINEAR;

	if (bWas == bOn)
		return bWas;

	pcRenderBuffer->FlushPrimitives();
	m_psCurrentRS->eFogMode = bOn != FALSE ? D3DFOG_LINEAR : D3DFOG_EXP;
	m_psSetRS->eFogMode = m_psCurrentRS->eFogMode;

	SetRenderStateForGlobal(g_lpD3Ddevice, D3DRENDERSTATE_FOGTABLEMODE, m_psCurrentRS->eFogMode);
}

float cD6States::LinearWorldIntoFogCoef(float fLin)
{
	if (g_bWFog)
		return fLin;

	AssertMsg(z_far > z_near, "Z far and near values are incorrect");

	return (z_far - z_far * z_near / fLin) / (z_far - z_near);
}

void cD6States::SetFogStartAndEnd(float fStart, float fEnd)
{
	HRESULT hRes;
	float fScaledStart, fScaledEnd;

	if (!g_bUseTableFog)
		return;

	fScaledStart = LinearWorldIntoFogCoef(fStart);
	fScaledEnd = LinearWorldIntoFogCoef(fEnd);

	m_psCurrentRS->fFogStart = fScaledStart;
	m_psSetRS->fFogStart = fScaledStart;
	SetRenderStateForGlobal(g_lpD3Ddevice, D3DRENDERSTATE_FOGTABLESTART, m_psCurrentRS->fFogStart);

	m_psCurrentRS->fFogEnd = fScaledEnd;
	m_psSetRS->fFogEnd = fScaledEnd;
	SetRenderStateForGlobal(g_lpD3Ddevice, D3DRENDERSTATE_FOGTABLEEND, m_psCurrentRS->fFogEnd);
}

void cD6States::SetLinearFogDistance(float fDistance)
{
	float fStart, fEnd;

	if (g_bWFog)
		return SetFogStartAndEnd(1.0, fDistance);

	fEnd = fDistance > z_far ? z_far : fDistance;
	fStart = fDistance >= z_near ? (fDistance - z_near) * 0.2 + z_near : z_near;

	SetFogStartAndEnd(fStart, fEnd);
}

void cD6States::SetAlphaColor(float fAlpha)
{
	// convert to integral and clamp
	int iNewValue = int(fAlpha * 255.0);
	if (iNewValue > 255) iNewValue = 255;
	if (iNewValue < 0) iNewValue = 0;

	if (m_psCurrentRS->nAlphaColor != iNewValue)
	{
		pcRenderBuffer->FlushPrimitives();
		m_psCurrentRS->nAlphaColor = iNewValue;
		m_psSetRS->nAlphaColor = iNewValue;
		pcRenderBuffer->PassAlphaColor(iNewValue);
	}
}

void cD6States::EnableAlphaBlending(BOOL bAlphaOn)
{
	HRESULT hRes;
	if (bAlphaOn == m_psCurrentRS->bAlphaOn && bAlphaOn == m_psSetRS->bAlphaOn)
		return;

	pcRenderBuffer->FlushPrimitives();
	m_psCurrentRS->bAlphaOn = bAlphaOn;
	m_psSetRS->bAlphaOn = bAlphaOn;

	if (!lgd3d_punt_d3d)
		SetRenderStateForGlobal(g_lpD3Ddevice, D3DRENDERSTATE_ALPHABLENDENABLE, m_psCurrentRS->bAlphaOn);
}

void cD6States::SetAlphaModulateHack(int iBlendMode)
{
	HRESULT hRes;
	pcRenderBuffer->FlushPrimitives();

	// and 3 <=> mod 4
	m_psCurrentRS->eSrcAlpha = table[iBlendMode & 3];
	m_psSetRS->eSrcAlpha = m_psCurrentRS->eSrcAlpha;
	m_psCurrentRS->eDstAlpha = table[(iBlendMode >> 2) & 3];
	m_psSetRS->eDstAlpha = m_psCurrentRS->eDstAlpha;

	if (!lgd3d_punt_d3d)
	{
		SetRenderStateForGlobal(g_lpD3Ddevice, D3DRENDERSTATE_SRCBLEND, m_psCurrentRS->eSrcAlpha);
		SetRenderStateForGlobal(g_lpD3Ddevice, D3DRENDERSTATE_DESTBLEND, m_psCurrentRS->eDstAlpha);
	}
}

void cD6States::ResetDefaultAlphaModulate()
{
	HRESULT hRes;
	pcRenderBuffer->FlushPrimitives();

	m_psCurrentRS->eSrcAlpha = D3DBLEND_SRCALPHA;
	m_psSetRS->eSrcAlpha = D3DBLEND_SRCALPHA;
	m_psCurrentRS->eDstAlpha = D3DBLEND_INVSRCALPHA;
	m_psSetRS->eDstAlpha = D3DBLEND_INVSRCALPHA;

	if (!lgd3d_punt_d3d)
	{
		SetRenderStateForGlobal(g_lpD3Ddevice, D3DRENDERSTATE_SRCBLEND, m_psCurrentRS->eSrcAlpha);
		SetRenderStateForGlobal(g_lpD3Ddevice, D3DRENDERSTATE_DESTBLEND, m_psCurrentRS->eDstAlpha);
	}
}

void cD6States::SetTextureMapMode(DWORD dwFlag)
{
	DWORD dwFilteredFlag;

	if (dwFlag >= LGD3DTB_NO_STATES)
	{
		Warning(("cD6States::SetTextureMapMode(): mode %i out of range.\n", dwFlag));
		return;
	}

	if (dwFlag == LGD3DTB_MODULATE)
		dwFilteredFlag = m_bCanModulate == FALSE;
	else
	{
		if (dwFlag == LGD3DTB_MODULATEALPHA)
		{
			if (m_bCanModulateAlpha)
				dwFilteredFlag = dwFlag;
			else
				dwFilteredFlag = FALSE;
		}
		else
			dwFilteredFlag = dwFlag;
	}

	m_psCurrentRS->dwTexBlendMode[0] = dwFilteredFlag;
}

void cD6States::GetTexBlendingModes(DWORD* pdw0LevelMode, DWORD* pdw1LevelMode)
{
	if (pdw0LevelMode)
		*pdw0LevelMode = m_psCurrentRS->dwTexBlendMode[0];
	if (pdw1LevelMode)
		*pdw1LevelMode = m_psCurrentRS->dwTexBlendMode[1];
}

BOOL cD6States::SetSmoothShading(BOOL bSmoothOn)
{
	HRESULT hRes;
	BOOL bWasSmooth = IsSmoothShadingOn();

	if (bSmoothOn == bWasSmooth)
		return bWasSmooth;

	pcRenderBuffer->FlushPrimitives();

	m_psCurrentRS->eShadeMode = D3DSHADEMODE(int(bSmoothOn != FALSE) + D3DSHADE_FLAT);
	m_psSetRS->eShadeMode = m_psCurrentRS->eShadeMode;

	if (!lgd3d_punt_d3d)
		SetRenderStateForGlobal(g_lpD3Ddevice, D3DRENDERSTATE_SHADEMODE, m_psCurrentRS->eShadeMode);
}

void cD6States::EnableFog(BOOL bFogOn)
{
	HRESULT hRes;
	if (!g_bUseTableFog && !g_bUseVertexFog)
		return;

	pcRenderBuffer->FlushPrimitives();
	m_psCurrentRS->bFogOn = bFogOn;
	m_psSetRS->bFogOn = bFogOn;

	SetRenderStateForGlobal(g_lpD3Ddevice, D3DRENDERSTATE_FOGENABLE, m_psCurrentRS->bFogOn);
}

void cD6States::SetFogSpecularLevel(float fLevel)
{
	D3DCOLOR dsNewColor = D3DRGBA(0.0, 0.0, 0.0, 1.0 - fLevel);

	if (m_psCurrentRS->dcFogSpecular == dsNewColor)
		return;

	pcRenderBuffer->FlushPrimitives();
	m_psCurrentRS->dcFogSpecular = dsNewColor;
	m_psSetRS->dcFogSpecular = dsNewColor;
	pcRenderBuffer->PassFogSpecularColor(m_psCurrentRS->dcFogSpecular);
}

void cD6States::SetFogColor(int r, int g, int b)
{
	int hRes;
	if (!g_bUseTableFog && !g_bUseVertexFog)
		return;

	if (r > 255)
		r = 255;
	else if (r < 0)
		r = 0;
	if (g > 255)
		g = 255;
	else if (g < 0)
		g = 0;
	if (b > 255)
		b = 255;
	else if (b < 0)
		b = 0;

	m_psCurrentRS->dwFogColor = RGB_MAKE(r, g, b);
	m_psSetRS->dwFogColor = m_psCurrentRS->dwFogColor;

	if (!lgd3d_punt_d3d)
		SetRenderStateForGlobal(g_lpD3Ddevice, D3DRENDERSTATE_FOGCOLOR, m_psCurrentRS->dwFogColor);
}

DWORD cD6States::get_color()
{
	D3DCOLOR color;
	int lgd3d_alpha = m_psCurrentRS->nAlphaColor;

	if (m_psCurrentRS->bUsePalette) {
		int index = grd_gc.fcolor & 0xff;

		switch (grd_gc.fill_type) {
		default:
			Warning(("lgd3d: unsupported fill type: %i\n", grd_gc.fill_type));
		case FILL_NORM:
			break;
		case FILL_CLUT:
			index = ((uchar*)grd_gc.fill_parm)[index];
			break;
		case FILL_SOLID:
			index = grd_gc.fill_parm;
		}

		// This hack effectively simulates the lighting table 
		// hacks done in flight2
		if (lgd3d_clut != NULL)
			index = lgd3d_clut[index];

		index *= 3;
		color = RGBA_MAKE(grd_pal[index], grd_pal[index + 1], grd_pal[index + 2], lgd3d_alpha);
	}
	else {
		switch (grd_gc.fill_type) {
		default:
			Warning(("lgd3d: unsupported fill type: %i\n", grd_gc.fill_type));
		case FILL_NORM:
			color = grd_gc.fcolor;
			break;
		case FILL_SOLID:
			color = grd_gc.fill_parm;
		}
		color = (color & 0xffffff) + (lgd3d_alpha << 24);
	}

	return color;
}

void cD6States::SetTextureNow()
{
	HRESULT hResult;
	g_bTexSuspended = FALSE;
	hResult = g_lpD3Ddevice->SetTexture(0, g_saTextures[m_psCurrentRS->naTextureId[0]].lpTexture);
	CheckHResult(hResult, "SetTexture failed");
}

void cD6States::set_texture_id(int n)
{
	HRESULT hRes, hResult;
	uint8 pixel_flags;

	if (lgd3d_punt_d3d)
		return;

	m_psCurrentRS->naTextureId[0] = n;

	// if current texture already selected
	if (m_psSetRS->naTextureId[0] == m_psCurrentRS->naTextureId[0])
	{
		// update alpha blending mode only
		if (m_psCurrentRS->baTextureWithAlpha[0] && !m_psSetRS->bAlphaOn)
		{
			SetRenderStateForGlobal(g_lpD3Ddevice, D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
			m_psSetRS->bAlphaOn = TRUE;
		}
		else if (m_psSetRS->bAlphaOn != m_psCurrentRS->bAlphaOn)
		{
			SetRenderStateForGlobal(g_lpD3Ddevice, D3DRENDERSTATE_ALPHABLENDENABLE, m_psCurrentRS->bAlphaOn);
			m_psSetRS->bAlphaOn = m_psCurrentRS->bAlphaOn;
		}
	}
	else
	{
		// otherwise, draw the batch and recheck all depend options
		pcRenderBuffer->FlushPrimitives();

		if (m_psCurrentRS->naTextureId[0] == TMGR_ID_CALLBACK)
		{
			g_tmgr->set_texture_callback();
		}
		else
		{
			if (m_psCurrentRS->naTextureId[0] == TMGR_ID_SOLID)
			{
				hResult = g_lpD3Ddevice->SetTexture(0, NULL);
				CheckHResult(hResult, "SetTexture failed");

				m_psSetRS->naTextureId[0] = TMGR_ID_SOLID;

				if (m_psSetRS->bAlphaOn != m_psCurrentRS->bAlphaOn)
				{
					SetRenderStateForGlobal(g_lpD3Ddevice, D3DRENDERSTATE_ALPHABLENDENABLE, m_psCurrentRS->bAlphaOn);
					m_psSetRS->bAlphaOn = m_psCurrentRS->bAlphaOn;
				}
			}
			else // where is TDRV_ID_INVALID check???
			{
				AssertMsg1(m_psCurrentRS->naTextureId[0] < LGD3D_MAX_TEXTURES, "Invalid texture id: %i", m_psCurrentRS->naTextureId[0]);
				pixel_flags = g_saTextures[m_psCurrentRS->naTextureId[0]].TdrvCookie.flags;
				m_psCurrentRS->baChromaKeying[0] = (pixel_flags & PF_TRANS) != 0;

				m_psCurrentRS->eMagTexFilter = m_psCurrentRS->eMinTexFilter = m_psCurrentRS->baChromaKeying[0] ? D3DTFN_POINT : D3DTFN_LINEAR;

				if (m_psSetRS->baChromaKeying[0] != m_psCurrentRS->baChromaKeying[0])
				{
					SetRenderStateForGlobal(g_lpD3Ddevice, D3DRENDERSTATE_COLORKEYENABLE, m_psCurrentRS->baChromaKeying[0]);
					m_psSetRS->baChromaKeying[0] = m_psCurrentRS->baChromaKeying[0];
					SetTextureStageStateForGlobal(g_lpD3Ddevice, 0, D3DTSS_MAGFILTER, m_psCurrentRS->eMagTexFilter);
					m_psSetRS->eMagTexFilter = m_psCurrentRS->eMagTexFilter;
					// @SH_NOTE: original error, must be D3DTSS_MINFILTER
					SetTextureStageStateForGlobal(g_lpD3Ddevice, 0, D3DTSS_MAGFILTER, m_psCurrentRS->eMinTexFilter);
					m_psSetRS->eMinTexFilter = m_psCurrentRS->eMinTexFilter;
				}

				m_psCurrentRS->baTextureWithAlpha[0] = (pixel_flags & (PF_RGBA | PF_MASK)) == PF_ALPHA || (pixel_flags & (PF_RGBA | PF_MASK)) == PF_MASK || (pixel_flags & (PF_RGBA | PF_MASK)) == PF_RGBA;

				if (m_psCurrentRS->baTextureWithAlpha[0] && !m_psSetRS->bAlphaOn)
				{
					SetRenderStateForGlobal(g_lpD3Ddevice, D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
					m_psSetRS->bAlphaOn = TRUE;
				}
				else if (m_psSetRS->bAlphaOn != m_psCurrentRS->bAlphaOn)
				{
					SetRenderStateForGlobal(g_lpD3Ddevice, D3DRENDERSTATE_ALPHABLENDENABLE, m_psCurrentRS->bAlphaOn);
					m_psSetRS->bAlphaOn = m_psCurrentRS->bAlphaOn;
				}

				if (m_psSetRS->dwTexBlendMode[0] != m_psCurrentRS->dwTexBlendMode[0] || m_psSetRS->baTextureWithAlpha[0] != m_psCurrentRS->baTextureWithAlpha[0])
				{
					memcpy(m_psCurrentRS->saTexBlend, sTexBlendArgsProtos[m_psCurrentRS->dwTexBlendMode[0]], sizeof(sTexBlendArgs));

					if (!m_psCurrentRS->dwTexBlendMode[0] && m_psCurrentRS->baTextureWithAlpha[0])
						m_psCurrentRS->saTexBlend[0].eAlphaOperation = D3DTOP_SELECTARG1;

					SetTextureStageStateForGlobal(g_lpD3Ddevice, 0, D3DTSS_TEXCOORDINDEX, 0);
					SetTextureStageColors(g_lpD3Ddevice, 0, m_psCurrentRS);

					m_psSetRS->saTexBlend[0] = m_psCurrentRS->saTexBlend[0];
					m_psSetRS->dwTexBlendMode[0] = m_psCurrentRS->dwTexBlendMode[0];
					m_psSetRS->baTextureWithAlpha[0] = m_psCurrentRS->baTextureWithAlpha[0];
				}

				SetTextureNow();
			}

			m_psSetRS->naTextureId[0] = m_psCurrentRS->naTextureId[0];
		}
	}
}

void cD6States::EnablePalette(BOOL bPalOn)
{
	pcRenderBuffer->FlushPrimitives();
	m_psCurrentRS->bUsePalette = bPalOn;
}

void cD6States::SetPalSlotFlags(int start, int n, uint8* pal_data, int slot, char flags)
{
	int i;
	grs_rgb_bitmask bitmask;

	if (g_lpDD_ext == NULL)
		return;

	if (flags & BMF_TLUC8)
	{
		lgd3d_get_trans_texture_bitmask(&bitmask);

		if (bitmask.red != 0)
		{
			if (texture_pal_trans[slot] == NULL)
				texture_pal_trans[slot] = (ushort*)gr_malloc(512);

			lgd3d_get_trans_texture_bitmask(&bitmask);
			gr_make_pal16(start, n, texture_pal_trans[slot], pal_data, &bitmask);


		}
	}

	if (flags & BMF_TRANS || ((flags & BMF_TLUC8) && bitmask.red == 0))
	{
		if (m_bTexture_RGB)
		{
			if (texture_pal_opaque[slot] == NULL)
				texture_pal_opaque[slot] = (ushort*)gr_malloc(512);

			lgd3d_get_opaque_texture_bitmask(&bitmask);
			gr_make_pal16(start, n, texture_pal_opaque[slot], pal_data, &bitmask);
		}
		else
			SetTexturePalette(start, n, pal_data, slot);
	}
}

void cD6States::SetTexturePalette(int start, int n, uchar* pal, int slot)
{
	PALETTEENTRY         peColorTable[256];
	HRESULT              hRes;
	int i;

	if (lpDDPalTexture[slot] == NULL)
	{
		hRes = g_lpDD_ext->CreatePalette(DDPCAPS_8BIT | DDPCAPS_ALLOW256,
			peColorTable,
			&lpDDPalTexture[slot],
			NULL);
		CheckHResult(hRes, "CreatePalette failed.");
	}

	for (i = 0; i < n; i++) {
		peColorTable[i].peFlags = D3DPAL_READONLY | PC_RESERVED;
		peColorTable[i].peRed = pal[3 * i];
		peColorTable[i].peGreen = pal[3 * i + 1];
		peColorTable[i].peBlue = pal[3 * i + 2];
	}

	hRes = lpDDPalTexture[slot]->SetEntries(0, start, n, peColorTable);

	CheckHResult(hRes, "SetEntries failed.");
}

void cD6States::SetAlphaPalette(ushort* pal)
{
	pcRenderBuffer->FlushPrimitives();
	alpha_pal = pal;
}

void cD6States::SetChromaKey(int r, int g, int b)
{
	pcRenderBuffer->FlushPrimitives();

	m_psCurrentRS->chroma_r = r;
	m_psCurrentRS->chroma_g = g;
	m_psCurrentRS->chroma_b = b;
	if (g_RGBTextureFormat.dwRBitMask == 0x1f) {
		// swap red and blue
		r = m_psCurrentRS->chroma_b;
		b = m_psCurrentRS->chroma_r;
	}
	m_psCurrentRS->chroma_key = b >> 3;
	if (g_RGBTextureFormat.dwGBitMask == GBitMask15) {
		m_psCurrentRS->chroma_key += ((g >> 3) << 5) + ((r >> 3) << 10);
	}
	else {
		m_psCurrentRS->chroma_key += ((g >> 2) << 5) + ((r >> 3) << 11);
	}
}

BOOL cD6States::SetTexWrapping(DWORD dwLevel, BOOL bSetSmooth)
{
	HRESULT hResult;
	BOOL bWasSmooth = GetTexWrapping(dwLevel);

	if (bWasSmooth == bSetSmooth)
		return bWasSmooth;

	pcRenderBuffer->FlushPrimitives();
	m_psCurrentRS->eWrap[dwLevel] = bSetSmooth != 0 ? D3DTADDRESS_WRAP : D3DTADDRESS_CLAMP;
	m_psSetRS->eWrap[dwLevel] = m_psCurrentRS->eWrap[dwLevel];
	if (!lgd3d_punt_d3d)
		SetTextureStageStateForGlobal(g_lpD3Ddevice, dwLevel, D3DTSS_ADDRESS, m_psCurrentRS->eWrap[dwLevel]);

	return bWasSmooth;
}

BOOL cD6States::EnableSpecular(BOOL bUseIt)
{
	HRESULT hRes;

	BOOL bWas = m_bSpecular;
	m_bSpecular = bUseIt;

	SetRenderStateForGlobal(g_lpD3Ddevice, D3DRENDERSTATE_SPECULARENABLE, m_bSpecular);

	return bWas;
}


cD6States* cImStates::Instance()
{
	if (m_Instance == NULL)
		m_Instance = new cImStates;

	return m_Instance;
}

cD6States* cImStates::DeInstance()
{
	if (m_Instance != NULL)
	{
		delete m_Instance;
		m_Instance = NULL;
	}

	return m_Instance;
}

cImStates::cImStates()
{
	// @SH_NOTE: was parent ctr
	// cD6States::cD6States();
}

cImStates::~cImStates()
{
	if (m_bTextureListInitialized)
	{
		if (g_tmgr != nullptr)
		{
			g_tmgr->shutdown();
			g_tmgr = nullptr;
		}

		auto* pWinDisplayDevice = AppGetObj(IWinDisplayDevice);
		pWinDisplayDevice->RemoveTaskSwitchCallback(callback_id);
		SafeRelease(pWinDisplayDevice);

		for (int i = 0; i < LGD3D_MAX_TEXTURES; i++)
		{
			auto* psTD = &g_saTextures[i];
			SafeRelease(psTD->lpTexture);
			SafeRelease(psTD->lpSurface);
		}

		m_bTextureListInitialized = FALSE;
	}

	if (default_bm)
	{
		gr_free(default_bm);
		default_bm = NULL;
	}

	for (int i = 0; i < MAX_PALETTES; i++)
	{
		if (texture_pal_opaque[i])
		{
			gr_free(texture_pal_opaque[i]);
			texture_pal_opaque[i] = NULL;
		}

		if (texture_pal_trans[i])
		{
			gr_free(texture_pal_trans[i]);
			texture_pal_trans[i] = NULL;
		}

		SafeRelease(lpDDPalTexture[i]);
	}

	if (m_texture_size_list)
	{
		free(munge_size_tables);
		m_texture_size_list = NULL;
	}
}

cD6States* cMSStates::Instance()
{
	if (m_Instance == NULL)
		m_Instance = new cMSStates;

	return m_Instance;
}

cD6States* cMSStates::DeInstance()
{
	if (m_Instance != NULL)
	{
		delete m_Instance;
		m_Instance = NULL;
	}

	return m_Instance;
}

cMSStates::cMSStates()
{
	// @SH_NOTE: was parent ctr
	// cD6States::cD6States();
	m_dwCurrentTexLevel = 0;
	m_bTexturePending = 0;
}

cMSStates::~cMSStates()
{
	sTextureData* psTD;
	IWinDisplayDevice* pWinDisplayDevice;
	int i;

	if (m_bTextureListInitialized)
	{
		if (g_tmgr != NULL)
		{
			g_tmgr->shutdown();
			g_tmgr = 0;
		}
		pWinDisplayDevice = AppGetObj(IWinDisplayDevice);
		pWinDisplayDevice->RemoveTaskSwitchCallback(callback_id);
		if (pWinDisplayDevice)
			pWinDisplayDevice->Release();

		for (i = 0; i < LGD3D_MAX_TEXTURES; i++)
		{
			psTD = &g_saTextures[i];
			SafeRelease(psTD->lpTexture);
			SafeRelease(psTD->lpSurface);
		}
		m_bTextureListInitialized = FALSE;
	}

	if (default_bm)
	{
		gr_free(default_bm);
		default_bm = NULL;
	}

	for (i = 0; i < MAX_PALETTES; i++)
	{
		if (texture_pal_opaque[i])
		{
			gr_free(texture_pal_opaque[i]);
			texture_pal_opaque[i] = NULL;
		}

		if (texture_pal_trans[i])
		{
			gr_free(texture_pal_trans[i]);
			texture_pal_trans[i] = NULL;
		}

		SafeRelease(lpDDPalTexture[i]);
	}

	if (m_texture_size_list)
	{
		free(munge_size_tables);
		m_texture_size_list = NULL;
	}
}

int cMSStates::Initialize(DWORD dwRequestedFlags)
{
	g_bPrefer_RGB = TRUE;
	return cD6States::Initialize(dwRequestedFlags);
}

BOOL cMSStates::SetDefaultsStates(DWORD dwRequestedFlags)
{
	HRESULT hRes, hResult, hR;
	ulong dwPasses;

	SetCommonDefaultStates(dwRequestedFlags, TRUE);

	m_psCurrentRS->eSrcAlpha = D3DBLEND_SRCALPHA;
	m_psCurrentRS->eDstAlpha = D3DBLEND_INVSRCALPHA;
	m_psCurrentRS->bAlphaOn = FALSE;


	SetRenderStateForGlobal(g_lpD3Ddevice, D3DRENDERSTATE_SRCBLEND, m_psCurrentRS->eSrcAlpha);
	SetRenderStateForGlobal(g_lpD3Ddevice, D3DRENDERSTATE_DESTBLEND, m_psCurrentRS->eDstAlpha);
	SetRenderStateForGlobal(g_lpD3Ddevice, D3DRENDERSTATE_ALPHABLENDENABLE, m_psCurrentRS->bAlphaOn);
	SetTextureStageStateForGlobal(g_lpD3Ddevice, 1, D3DTSS_MAGFILTER, m_psCurrentRS->eMagTexFilter);
	SetTextureStageStateForGlobal(g_lpD3Ddevice, 1, D3DTSS_MINFILTER, m_psCurrentRS->eMinTexFilter);

	m_psCurrentRS->eWrap[1] = D3DTADDRESS_WRAP;
	// @SH_NOTE: original error, last arg must be m_psCurrentRS->eWrap[1]
	SetTextureStageStateForGlobal(g_lpD3Ddevice, 1, D3DTSS_ADDRESS, m_psCurrentRS->eWrap[0]);

	AssertMsg(dwRequestedFlags & LGD3DF_MULTI_TEXTURING, "Error 666");

	if ((dwRequestedFlags & LGD3DF_MULTITEXTURE_COLOR) != 0)
	{
		memcpy(m_psCurrentRS->saTexBlend, sMultiTexBlendArgsProtos[0], sizeof(m_psCurrentRS->saTexBlend));

		SetTextureStageStateForGlobal(g_lpD3Ddevice, 0, D3DTSS_TEXCOORDINDEX, 0);
		SetTextureStageColors(g_lpD3Ddevice, 0, m_psCurrentRS);
		SetTextureStageStateForGlobal(g_lpD3Ddevice, 1, D3DTSS_TEXCOORDINDEX, 1);
		SetTextureStageColors(g_lpD3Ddevice, 1, m_psCurrentRS);

		if (hR = g_lpD3Ddevice->ValidateDevice(&dwPasses), hR != S_OK)
		{
			SetLGD3DErrorCode(LGD3D_EC_VD_SPASS_MT, hR);
			if (bSpewOn)
			{
				CriticalMsg4(pcLGD3Def, LGD3D_EC_VD_SPASS_MT, GetLgd3dErrorCode(LGD3D_EC_VD_SPASS_MT), (ushort)hR, GetDDErrorMsg(hR));
			}
			else
			{
				Warning((pcLGD3Def, LGD3D_EC_VD_SPASS_MT, GetLgd3dErrorCode(LGD3D_EC_VD_SPASS_MT), (ushort)hR, GetDDErrorMsg(hR)));
			}
			lgd3d_g_bInitialized = FALSE;

			return FALSE;
		}
	}

	if (dwRequestedFlags & LGD3DF_MT_BLENDDIFFUSE)
	{
		memcpy(m_psCurrentRS->saTexBlend, sMultiTexBlendArgsProtos[2], sizeof(m_psCurrentRS->saTexBlend));

		SetTextureStageStateForGlobal(g_lpD3Ddevice, 0, D3DTSS_TEXCOORDINDEX, 0);
		SetTextureStageColors(g_lpD3Ddevice, 0, m_psCurrentRS);
		SetTextureStageStateForGlobal(g_lpD3Ddevice, 1, D3DTSS_TEXCOORDINDEX, 1);
		SetTextureStageColors(g_lpD3Ddevice, 1, m_psCurrentRS);

		if (hR = g_lpD3Ddevice->ValidateDevice(&dwPasses), hR != S_OK)
			return FALSE;

		m_psCurrentRS->dwTexBlendMode[1] = LGD3DTB_BLENDDIFFUSE;
	}

	memcpy(m_psCurrentRS->saTexBlend, sTexBlendArgsProtos[m_psCurrentRS->dwTexBlendMode[1]], sizeof(m_psCurrentRS->saTexBlend));

	SetTextureStageStateForGlobal(g_lpD3Ddevice, 0, D3DTSS_TEXCOORDINDEX, 0);
	SetTextureStageColors(g_lpD3Ddevice, 0, m_psCurrentRS);
	SetTextureStageStateForGlobal(g_lpD3Ddevice, 1, D3DTSS_TEXCOORDINDEX, 1);
	SetTextureStageColors(g_lpD3Ddevice, 1, m_psCurrentRS);

	memcpy(m_psSetRS, m_psCurrentRS, sizeof(sRenderStates));

	g_dwRSChangeFlags = 0;

	return TRUE;
}

void cMSStates::set_texture_id(int n)
{
	m_bTexturePending = TRUE;

	if (m_psSetRS->naTextureId[m_dwCurrentTexLevel] == n)
	{
		m_psCurrentRS->naTextureId[m_dwCurrentTexLevel] = n;
	}
	else
	{
		pcRenderBuffer->FlushPrimitives();
		m_psCurrentRS->naTextureId[m_dwCurrentTexLevel] = n;
		// TODO: find definition to number
		if (m_dwCurrentTexLevel == 1)
			m_LastLightMapBm = g_saTextures[n].pTdrvBitmap;

		if (m_psCurrentRS->naTextureId[m_dwCurrentTexLevel] == TMGR_ID_CALLBACK)
			g_tmgr->set_texture_callback();
	}
}

int cMSStates::EnableMTMode(DWORD dwMTOn)
{
	HRESULT hResult, hRes, hR;
	DWORD dwLevel;
	sTexBlendArgs(*sTexArgs)[2];
	BOOL bSetTexture;
	DWORD dwNoLMLevels;
	sTextureData* psTD;
	uint8 pixel_flags;

	if (!m_bTexturePending)
	{
		if (m_psCurrentRS->naTextureId[0] != -1)
		{
			if ((m_psCurrentRS->baTextureWithAlpha[0] || m_psCurrentRS->baTextureWithAlpha[1]) && (!m_psSetRS->bAlphaOn))
			{
				SetRenderStateForGlobal(g_lpD3Ddevice, D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
				m_psSetRS->bAlphaOn = TRUE;
			}
			else if (m_psSetRS->bAlphaOn != m_psCurrentRS->bAlphaOn)
			{
				SetRenderStateForGlobal(g_lpD3Ddevice, D3DRENDERSTATE_ALPHABLENDENABLE, m_psCurrentRS->bAlphaOn);
				m_psSetRS->bAlphaOn = m_psCurrentRS->bAlphaOn;
			}
		}

		return TRUE;
	}

	m_bTexturePending = FALSE;
	if (dwMTOn)
	{
		dwNoLMLevels = TRUE;
		sTexArgs = sMultiTexBlendArgsProtos;
	}
	else
	{
		dwNoLMLevels = FALSE;
		sTexArgs = sTexBlendArgsProtos;
	}

	m_psCurrentRS->dwNoLightMapLevels = dwNoLMLevels;
	bSetTexture = FALSE;

	if (dwMTOn)
	{
		if (m_psCurrentRS->naTextureId[0] == -1)
		{
			Warning(("\nMulti texturing with no textures?!\n\n"));
			return FALSE;
		}

		if (m_psCurrentRS->naTextureId[1] == -1)
		{
			lgd3d_set_texture_level(1);
			if (g_tmgr)
				g_tmgr->set_texture(m_LastLightMapBm);
			lgd3d_set_texture_level(0);
			Warning(("\nSwap-out inside texure pair!, Trying to reload bitmap %p\n\n", m_LastLightMapBm));
		}

		if (m_psSetRS->naTextureId[0] != m_psCurrentRS->naTextureId[0] || m_psSetRS->naTextureId[1] != m_psCurrentRS->naTextureId[1])
		{
			bSetTexture = TRUE;
			for (dwLevel = 0; dwLevel < 2; dwLevel++)
			{
				if (m_psSetRS->naTextureId[dwLevel] != m_psCurrentRS->naTextureId[dwLevel])
				{
					AssertMsg1(m_psCurrentRS->naTextureId[dwLevel] < LGD3D_MAX_TEXTURES, "Invalid texture id: %i", m_psCurrentRS->naTextureId[dwLevel]);
					m_psCurrentRS->baChromaKeying[dwLevel] = (g_saTextures[m_psCurrentRS->naTextureId[dwLevel]].TdrvCookie.flags & PF_TRANS) != 0;

					if (m_psSetRS->baChromaKeying[dwLevel] != m_psCurrentRS->baChromaKeying[dwLevel])
					{
						SetRenderStateForGlobal(g_lpD3Ddevice, D3DRENDERSTATE_COLORKEYENABLE, m_psCurrentRS->baChromaKeying[dwLevel]);
						m_psSetRS->baChromaKeying[dwLevel] = m_psCurrentRS->baChromaKeying[dwLevel];
					}
				}
			}
		}

		if (m_psSetRS->dwNoLightMapLevels != m_psCurrentRS->dwNoLightMapLevels || m_psSetRS->dwTexBlendMode[0] != m_psCurrentRS->dwTexBlendMode[0])
		{
			bSetTexture = TRUE;
			memcpy(m_psCurrentRS->saTexBlend, sTexArgs[m_psCurrentRS->dwTexBlendMode[dwNoLMLevels]], sizeof(m_psCurrentRS->saTexBlend));
			m_psSetRS->saTexBlend[0] = m_psCurrentRS->saTexBlend[0];
			m_psSetRS->saTexBlend[1] = m_psCurrentRS->saTexBlend[1];
		}

		if (bSetTexture)
		{
			if (m_psSetRS->bAlphaOn != m_psCurrentRS->bAlphaOn)
			{
				SetRenderStateForGlobal(g_lpD3Ddevice, D3DRENDERSTATE_ALPHABLENDENABLE, m_psCurrentRS->bAlphaOn);
				m_psSetRS->bAlphaOn = m_psCurrentRS->bAlphaOn;
			}

			AssertMsg1(m_psCurrentRS->naTextureId[0] < LGD3D_MAX_TEXTURES, "Invalid texture id: %i", m_psCurrentRS->naTextureId[0]);
			psTD = &g_saTextures[m_psCurrentRS->naTextureId[0]];
			SetTextureStageStateForGlobal(g_lpD3Ddevice, 0, D3DTSS_TEXCOORDINDEX, 0);
			SetTextureStageColors(g_lpD3Ddevice, 0, m_psCurrentRS);
			if (hR = g_lpD3Ddevice->SetTexture(0, psTD->lpTexture), hR != S_OK)
			{
				CriticalMsg3(pcDXef, "SetTexture failed", (ushort)hR, GetDDErrorMsg(hR));
			}
			m_psSetRS->naTextureId[0] = m_psCurrentRS->naTextureId[0];

			AssertMsg1(m_psCurrentRS->naTextureId[1] < LGD3D_MAX_TEXTURES, "Invalid texture id: %i", m_psCurrentRS->naTextureId[1]);
			psTD = &g_saTextures[m_psCurrentRS->naTextureId[1]];
			SetTextureStageStateForGlobal(g_lpD3Ddevice, 1, D3DTSS_TEXCOORDINDEX, 1);
			SetTextureStageColors(g_lpD3Ddevice, 1, m_psCurrentRS);
			if (hR = g_lpD3Ddevice->SetTexture(1, psTD->lpTexture), hR != S_OK)
			{
				CriticalMsg3(pcDXef, "SetTexture failed", (ushort)hR, GetDDErrorMsg(hR));
			}
			m_psSetRS->naTextureId[1] = m_psCurrentRS->naTextureId[1];

			m_psSetRS->dwNoLightMapLevels = m_psCurrentRS->dwNoLightMapLevels;
		}
	}
	else
	{
		if (m_psSetRS->naTextureId[0] == m_psCurrentRS->naTextureId[0])
		{
			if (m_psCurrentRS->baTextureWithAlpha[0] && !m_psSetRS->bAlphaOn)
			{
				SetRenderStateForGlobal(g_lpD3Ddevice, D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
				m_psSetRS->bAlphaOn = TRUE;
			}
			else if (m_psSetRS->bAlphaOn != m_psCurrentRS->bAlphaOn)
			{
				SetRenderStateForGlobal(g_lpD3Ddevice, D3DRENDERSTATE_ALPHABLENDENABLE, m_psCurrentRS->bAlphaOn);
				m_psSetRS->bAlphaOn = m_psCurrentRS->bAlphaOn;
			}
		}
		else if (m_psCurrentRS->naTextureId[0] == -1)
		{
			if (hR = g_lpD3Ddevice->SetTexture(0, NULL), hR != S_OK)
			{
				CriticalMsg3(pcDXef, "SetTexture failed", (ushort)hR, GetDDErrorMsg(hR));
			}
			m_psSetRS->naTextureId[0] = -1;
			m_psCurrentRS->baTextureWithAlpha[0] = FALSE;
			if (m_psSetRS->bAlphaOn != m_psCurrentRS->bAlphaOn)
			{
				SetRenderStateForGlobal(g_lpD3Ddevice, D3DRENDERSTATE_ALPHABLENDENABLE, m_psCurrentRS->bAlphaOn);
				m_psSetRS->bAlphaOn = m_psCurrentRS->bAlphaOn;
			}
		}
		else
		{
			bSetTexture = TRUE;
			AssertMsg1(m_psCurrentRS->naTextureId[0] < LGD3D_MAX_TEXTURES, "Invalid texture id: %i", m_psCurrentRS->naTextureId[0]);
			pixel_flags = g_saTextures[m_psCurrentRS->naTextureId[0]].TdrvCookie.flags;

			m_psCurrentRS->baChromaKeying[0] = (pixel_flags & PF_TRANS) != 0;
			m_psCurrentRS->eMagTexFilter = m_psCurrentRS->eMinTexFilter = m_psCurrentRS->baChromaKeying[0] ? D3DTFN_POINT : D3DTFN_LINEAR;

			if (m_psSetRS->baChromaKeying[0] != m_psCurrentRS->baChromaKeying[0])
			{
				SetRenderStateForGlobal(g_lpD3Ddevice, D3DRENDERSTATE_COLORKEYENABLE, m_psCurrentRS->baChromaKeying[0]);
				m_psSetRS->baChromaKeying[0] = m_psCurrentRS->baChromaKeying[0];
				SetTextureStageStateForGlobal(g_lpD3Ddevice, 0, D3DTSS_MAGFILTER, m_psCurrentRS->eMagTexFilter);
				m_psSetRS->eMagTexFilter = m_psCurrentRS->eMagTexFilter;
				// @SH_NOTE: second arg must be D3DTSS_MINFILTER
				SetTextureStageStateForGlobal(g_lpD3Ddevice, 0, D3DTSS_MAGFILTER, m_psCurrentRS->eMinTexFilter);
				m_psSetRS->eMinTexFilter = m_psCurrentRS->eMinTexFilter;
			}

			pixel_flags &= (PF_RGBA | PF_MASK);
			m_psCurrentRS->baTextureWithAlpha[0] = pixel_flags == PF_ALPHA || pixel_flags == PF_MASK || pixel_flags == PF_RGBA;

			if (m_psCurrentRS->baTextureWithAlpha[0] && !m_psSetRS->bAlphaOn)
			{
				SetRenderStateForGlobal(g_lpD3Ddevice, D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
				m_psSetRS->bAlphaOn = TRUE;
			}
			else if (m_psSetRS->bAlphaOn != m_psCurrentRS->bAlphaOn)
			{
				SetRenderStateForGlobal(g_lpD3Ddevice, D3DRENDERSTATE_ALPHABLENDENABLE, m_psCurrentRS->bAlphaOn);
				m_psSetRS->bAlphaOn = m_psCurrentRS->bAlphaOn;
			}
		}

		if (m_psSetRS->dwNoLightMapLevels != m_psCurrentRS->dwNoLightMapLevels || m_psSetRS->dwTexBlendMode[0] != m_psCurrentRS->dwTexBlendMode[0] || m_psSetRS->baTextureWithAlpha[0] != m_psCurrentRS->baTextureWithAlpha[0])
		{
			bSetTexture = TRUE;
			memcpy(m_psCurrentRS->saTexBlend, sTexArgs[m_psCurrentRS->dwTexBlendMode[dwNoLMLevels]], sizeof(m_psCurrentRS->saTexBlend));
			if (!m_psCurrentRS->dwTexBlendMode[0] && m_psCurrentRS->baTextureWithAlpha[0])
				m_psCurrentRS->saTexBlend[0].eAlphaOperation = D3DTOP_SELECTARG1;
			m_psSetRS->saTexBlend[1] = m_psCurrentRS->saTexBlend[1];
			m_psSetRS->baTextureWithAlpha[0] = m_psCurrentRS->baTextureWithAlpha[0];
		}

		if (bSetTexture)
		{
			SetTextureStageStateForGlobal(g_lpD3Ddevice, 0, D3DTSS_TEXCOORDINDEX, 0);
			SetTextureStageColors(g_lpD3Ddevice, 0, m_psCurrentRS);
			SetTextureStageStateForGlobal(g_lpD3Ddevice, 1, D3DTSS_TEXCOORDINDEX, 1);
			SetTextureStageColors(g_lpD3Ddevice, 1, m_psCurrentRS);

			m_psSetRS->dwNoLightMapLevels = m_psCurrentRS->dwNoLightMapLevels;

			if (m_psCurrentRS->naTextureId[0] != -1)
			{
				AssertMsg1(m_psCurrentRS->naTextureId[0] < LGD3D_MAX_TEXTURES, "Invalid texture id: %i", m_psCurrentRS->naTextureId[0]);
				if (hR = g_lpD3Ddevice->SetTexture(0, g_saTextures[m_psCurrentRS->naTextureId[0]].lpTexture), hR != S_OK)
				{
					CriticalMsg3(pcDXef, "SetTexture failed", (ushort)hR, GetDDErrorMsg(hR));
				}
				m_psSetRS->naTextureId[0] = m_psCurrentRS->naTextureId[0];

				m_psCurrentRS->naTextureId[1] = -1;
				if (hR = g_lpD3Ddevice->SetTexture(1, NULL), hR != S_OK)
				{
					CriticalMsg3(pcDXef, "SetTexture failed", (ushort)hR, GetDDErrorMsg(hR));
				}
				m_psSetRS->naTextureId[1] = m_psCurrentRS->naTextureId[1];
			}
		}
	}

	return TRUE;
}

void cMSStates::TurnOffTexuring(BOOL bTexOff)
{
	HRESULT hResult, hRes, hR;

	if (bTexOff)
	{
		g_bTexSuspended = FALSE;
		m_bTexturePending = TRUE;
		return;
	}

	m_psCurrentRS->dwNoLightMapLevels = 0;
	if (hR = g_lpD3Ddevice->SetTexture(0, NULL), hR != S_OK)
	{
		CriticalMsg3(pcDXef, "SetTexture failed", (ushort)hR, GetDDErrorMsg(hR));
	}
	m_psSetRS->naTextureId[0] = -1;
	m_psCurrentRS->baTextureWithAlpha[0] = 0;

	if (m_psSetRS->bAlphaOn != m_psCurrentRS->bAlphaOn)
	{
		SetRenderStateForGlobal(g_lpD3Ddevice, D3DRENDERSTATE_ALPHABLENDENABLE, m_psCurrentRS->bAlphaOn);
		m_psSetRS->bAlphaOn = m_psCurrentRS->bAlphaOn;
	}

	if (m_psSetRS->dwNoLightMapLevels != m_psCurrentRS->dwNoLightMapLevels || m_psSetRS->dwTexBlendMode[0] != m_psCurrentRS->dwTexBlendMode[0] || m_psSetRS->baTextureWithAlpha[0] != m_psCurrentRS->baTextureWithAlpha[0])
	{
		memcpy(m_psCurrentRS->saTexBlend, sTexBlendArgsProtos[m_psCurrentRS->dwTexBlendMode[0]], sizeof(m_psCurrentRS->saTexBlend));
		if (!m_psCurrentRS->dwTexBlendMode[0] && m_psCurrentRS->baTextureWithAlpha[0])
			m_psCurrentRS->saTexBlend[0].eAlphaOperation = D3DTOP_SELECTARG1;
		m_psSetRS->saTexBlend[0] = m_psCurrentRS->saTexBlend[0];
		m_psSetRS->saTexBlend[1] = m_psCurrentRS->saTexBlend[1];
		m_psSetRS->baTextureWithAlpha[0] = m_psCurrentRS->baTextureWithAlpha[0];

		SetTextureStageStateForGlobal(g_lpD3Ddevice, 0, D3DTSS_TEXCOORDINDEX, 0);
		SetTextureStageColors(g_lpD3Ddevice, 0, m_psCurrentRS);
		SetTextureStageStateForGlobal(g_lpD3Ddevice, 1, D3DTSS_TEXCOORDINDEX, 1);
		SetTextureStageColors(g_lpD3Ddevice, 1, m_psCurrentRS);

		m_psSetRS->dwNoLightMapLevels = m_psCurrentRS->dwNoLightMapLevels;
	}

	m_bTexturePending = TRUE;
	g_bTexSuspended = TRUE;
}

BOOL cMSStates::reload_texture(tdrv_texture_info* info)
{
	IDirectDrawSurface4* SysmemSurface, ** pDeviceSurface;
	DDSURFACEDESC2 ddsd;
	d3d_cookie cookie;
	LPDDPIXELFORMAT pixel_format;
	HRESULT LastError;
	IDirect3DTexture2* SysmemTexture = NULL, ** pDeviceTexture;
	sTextureData* psTexData;

	psTexData = &g_saTextures[info->id];

	pDeviceTexture = &psTexData->lpTexture;
	pDeviceSurface = &psTexData->lpSurface;
	cookie.value = info->cookie;

	pixel_format = g_FormatList[cookie.flags & (PF_RGBA | PF_RGB | PF_ALPHA)];
	if ((cookie.flags & (PF_RGBA | PF_RGB | PF_ALPHA)) == PF_RGBA && !g_b8888supported)
		pixel_format = g_FormatList[2];

	if (pixel_format->dwFlags == 0)
		return TMGR_FAILURE;

	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS;
	ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY;
	ddsd.dwHeight = info->h;
	ddsd.dwWidth = info->w;
	memcpy(&ddsd.ddpfPixelFormat, pixel_format, sizeof(ddsd.ddpfPixelFormat));

	LastError = CreateDDSurface(cookie, &ddsd, &SysmemSurface);
	CheckHResult(LastError, "CreateDDSurface() failed");
	AssertMsg(SysmemSurface != NULL, "NULL SysmemSurface");

	LastError = SysmemSurface->QueryInterface(IID_IDirect3DTexture2, (LPVOID*)&SysmemTexture);
	CheckHResult(LastError, "Failed to obtain D3D texture interface for sysmem surface");

	ddsd.dwSize = sizeof(DDSURFACEDESC2);
	ddsd.dwFlags = DDSD_TEXTURESTAGE | DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
	ddsd.dwTextureStage = (cookie.flags & PF_STAGE);

	if (*pDeviceTexture == NULL) {
		IDirectDrawSurface4* DeviceSurface;

		ddsd.ddsCaps.dwCaps = m_DeviceSurfaceCaps;
		LastError = CreateDDSurface(cookie, &ddsd, pDeviceSurface);
		if ((LastError != DD_OK) && m_bUsingLocalMem && m_bAGP_available)
		{
			mono_printf(("using nonlocal vidmem textures...\n"));
			// try AGP textures...
			m_bUsingLocalMem = FALSE;
			m_DeviceSurfaceCaps = NONLOCAL_CAPS;
			ddsd.ddsCaps.dwCaps = m_DeviceSurfaceCaps;
			LastError = CreateDDSurface(cookie, &ddsd, pDeviceSurface);
		}

		if (LastError != DD_OK) {
			mono_printf(("couldn't load texture: error %i.\n", LastError & 0xffff));
			SafeRelease(SysmemTexture);
			SafeRelease(SysmemSurface);
			*pDeviceSurface = NULL;
			return TMGR_FAILURE;
		}

		DeviceSurface = *pDeviceSurface;

#define MEMORY_TYPE_MASK \
      (DDSCAPS_SYSTEMMEMORY|DDSCAPS_VIDEOMEMORY|DDSCAPS_LOCALVIDMEM|DDSCAPS_NONLOCALVIDMEM)

		if ((m_DeviceSurfaceCaps & MEMORY_TYPE_MASK) != (ddsd.ddsCaps.dwCaps & MEMORY_TYPE_MASK))
			mono_printf((
				"device texture not created in requested memory location\nRequested %x Received %x\n",
				m_DeviceSurfaceCaps & MEMORY_TYPE_MASK, ddsd.ddsCaps.dwCaps & MEMORY_TYPE_MASK));


		// Query our device surface for a texture interface
		LastError = DeviceSurface->QueryInterface(IID_IDirect3DTexture2, (LPVOID*)pDeviceTexture);
		CheckHResult(LastError, "Failed to obtain D3D texture interface for device surface.");
	}

	// Load the bitmap into the sysmem surface
	LastError = SysmemSurface->Lock(NULL, &ddsd, 0, NULL);
	CheckHResult(LastError, "Failed to lock sysmem surface.");

	LoadSurface(info, &ddsd);

	LastError = SysmemSurface->Unlock(NULL);
	CheckHResult(LastError, "Failed to unlock sysmem surface.");

	// Load the sysmem texture into the device texture.  During this call, a
	// driver could compress or reformat the texture surface and put it in
	// video memory.

	LastError = pDeviceTexture[0]->Load(SysmemTexture);
	CheckHResult(LastError, "Failed to load device texture from sysmem texture.");

	// Now we are done with sysmem surface

	SafeRelease(SysmemTexture);
	SafeRelease(SysmemSurface);

	psTexData->pTdrvBitmap = info->bm;
	psTexData->TdrvCookie.value = info->cookie;

	return TMGR_SUCCESS;
}

void cMSStates::cook_info(tdrv_texture_info* info)
{
	d3d_cookie cookie;
	int v, w = info->w, h = info->h;

	if (!m_dwCurrentTexLevel)
	{
		return cD6States::cook_info(info);
	}

	cookie.palette = 0;

	if (gr_get_fill_type() == FILL_BLEND)
	{
		cookie.flags = PF_ALPHA;
	}
	else
	{
		switch (info->bm->type)
		{
		case BMT_FLAT8:
			cookie.palette = info->bm->align;
			if (info->bm->flags & BMF_TRANS)
			{
				if (texture_pal_trans[cookie.palette] != NULL && g_TransRGBTextureFormat.dwFlags != 0)
					cookie.flags = PF_MASK;
				else
					cookie.flags = PF_TRANS;
			}
			else
			{
				cookie.flags = PF_GENERIC;
			}
			break;
		case BMT_FLAT16:
			if (gr_test_bitmap_format(info->bm, BMF_RGB_4444))
			{
				cookie.flags = PF_ALPHA;
			}
			else if (info->bm->flags & BMF_TRANS)
			{
				if (g_TransRGBTextureFormat.dwFlags != 0)
					cookie.flags = PF_MASK;
				else
					cookie.flags = PF_TRANS | PF_RGB;
			}
			else
				cookie.flags = PF_RGB;
			break;
		case BMT_FLAT32:
			cookie.flags = PF_RGBA;
			break;
		}
	}

	cookie.flags |= PF_STAGE;
	calc_size(info, cookie);

	cookie.hlog = 0;
	cookie.wlog = 0;
	for (v = 2; v <= w; v += v)
	{
		cookie.wlog++;
	}
	for (v = 2; v <= h; v += v)
	{
		cookie.hlog++;
	}

	AssertMsg((info->h == (1 << cookie.hlog)) && (info->w == (1 << cookie.wlog)),
		"hlog/wlog does not match texture width/height");

	info->cookie = cookie.value;
}

void cMSStates::SetLightMapMode(DWORD dwFlag)
{
	if (dwFlag < LGD3DTB_NO_STATES)
		m_psCurrentRS->dwTexBlendMode[1] = dwFlag;
	else
		Warning(("cMSStates::SetLightMapMode(): mode %i out of range.\n", dwFlag));

}

void calc_size(tdrv_texture_info* info, d3d_cookie cookie)
{
	grs_bitmap* bm = info->bm;
	int size_index;
	int* size_table;
	int i, j;
	int w, h;

	info->scale_h = info->scale_w = 0;

	switch (cookie.flags & (PF_RGBA | PF_RGB | PF_ALPHA))
	{
	case PF_GENERIC:
		size_table = generic_size_table;
		break;
	case PF_RGB:
		size_table = rgb_size_table;
		break;
	case PF_ALPHA:
		size_table = alpha_size_table;
		break;
	case PF_MASK:
		size_table = trgb_size_table;
		break;
	case PF_RGBA:
		size_table = b8888_size_table;
		break;
	default:
		break;
	}

	// compute log2 to width
	for (i = 0, w = 1; i < 9 && w != bm->w; w *= 2)
		i++;

	// and for height
	for (j = 0, h = 1; j < 9 && h != bm->h; h *= 2)
		j++;

	// max texture size = 256x256
	if (i < 9 && j < 9)
	{
		for (size_index = size_table[9 * i + j]; size_index < 0; )
		{
			if (i < j)
			{
				i++;
				info->scale_w++;
			}
			else
			{
				j++;
				info->scale_h++;
				if (j >= 9)
				{
					mono_printf(("Unsupported texture size: w=%i h=%i\n", bm->w, bm->h));
					break;
				}
			}
		}
	}
	else
	{
		mono_printf(("Unsupported texture size: w=%i h=%i\n", bm->w, bm->h));
		size_index = -1;
	}

	info->w = bm->w << info->scale_w;
	info->h = bm->h << info->scale_h;
	info->size_index = size_index;
}

// OK, here are our various specialized blitters

static void blit_8to16(tdrv_texture_info* info, ushort* dst, int drow, ushort* pal)
{
	int i, j;
	grs_bitmap* bm = info->bm;
	uchar* src = info->bits;

	for (i = 0; i < bm->h; i++) {
		for (j = 0; j < bm->w; j++)
			dst[j] = pal[src[j]];
		src += bm->row;
		dst += drow;
	}
}

static void blit_8to16_trans(tdrv_texture_info* info, ushort* dst, int drow, ushort* pal)
{
	int i, j, k;
	int w, h;
	grs_bitmap* bm = info->bm;
	uchar* src = info->bits;

	for (i = 0; i < bm->h; i++) {
		for (j = 0; j < bm->w; j++)
		{
			k = src[j];
			if (src[j])
			{
				dst[j] = pal[k];
			}
			else
			{
				// pick previous pixel
				if (j > 0)
					k = src[j - 1];
				// if zero, try to find valid
				if (!k)
				{
					if (i > 0)
						k = src[j - bm->row];

					if (!k)
					{
						if (j < w - 1)
							k = src[j + 1];

						if (!k && i < h - 1)
							k = src[bm->row + j];
					}
				}
				dst[j] = pal[k] & 0x7FFF;
			}

			dst[j] = pal[texture_clut[src[j]]];
		}
		src += bm->row;
		dst += drow;
	}
}

static void blit_8to16_clut(tdrv_texture_info* info, ushort* dst, int drow, ushort* pal)
{
	int i, j;
	grs_bitmap* bm = info->bm;
	uchar* src = info->bits;

	for (i = 0; i < bm->h; i++) {
		for (j = 0; j < bm->w; j++)
			dst[j] = pal[texture_clut[src[j]]];
		src += bm->row;
		dst += drow;
	}
}

static void blit_8to16_scale(tdrv_texture_info* info, ushort* dst, int drow, ushort* pal)
{
	int i, j, k;
	int scale_w = info->scale_w;
	int scale_h = info->scale_h;
	int step_w = 1 << (scale_w - 1);
	int step_h = (1 << scale_h) - 1;
	grs_bitmap* bm = info->bm;
	uchar* src = info->bits;

	for (i = 0; i < bm->h; i++) {
		if (scale_w > 0)
		{
			ulong* base = (ulong*)dst;
			for (j = 0; j < bm->w; j++)
			{
				ulong c32 = pal[src[j]];
				c32 += (c32 << 16);
				for (k = 0; k < step_w; k++)
					base[k] = c32;
				base += step_w;
			}
		}
		else {
			for (j = 0; j < bm->w; j++)
				dst[j] = pal[src[j]];
		}
		for (j = 0; j < step_h; j++) {
			memcpy(dst + drow, dst, 2 * (bm->w << scale_w));
			dst += drow;
		}
		src += bm->row;
		dst += drow;
	}
}

static void blit_8to16_clut_scale(tdrv_texture_info* info, ushort* dst, int drow, ushort* pal)
{
	int i, j, k;
	int scale_w = info->scale_w;
	int scale_h = info->scale_h;
	int step_w = 1 << (scale_w - 1);
	int step_h = (1 << scale_h) - 1;
	grs_bitmap* bm = info->bm;
	uchar* src = info->bits;

	for (i = 0; i < bm->h; i++) {
		if (scale_w > 0)
		{
			ulong* base = (ulong*)dst;
			for (j = 0; j < bm->w; j++)
			{
				ulong c32 = pal[texture_clut[src[j]]];
				c32 += (c32 << 16);
				for (k = 0; k < step_w; k++)
					base[k] = c32;
				base += step_w;
			}
		}
		else {
			for (j = 0; j < bm->w; j++)
				dst[j] = pal[texture_clut[src[j]]];
		}
		for (j = 0; j < step_h; j++) {
			memcpy(dst + drow, dst, 2 * (bm->w << scale_w));
			dst += drow;
		}
		src += bm->row;
		dst += drow;
	}
}

static void blit_8to8(tdrv_texture_info* info, uchar* dst, int drow)
{
	int i;
	grs_bitmap* bm = info->bm;
	uchar* src = info->bits;

	for (i = 0; i < bm->h; i++) {
		memcpy(dst, src, bm->w);
		src += bm->row;
		dst += drow;
	}
}

static void blit_8to8_clut(tdrv_texture_info* info, uchar* dst, int drow)
{
	int i, j;
	grs_bitmap* bm = info->bm;
	uchar* src = info->bits;

	for (i = 0; i < bm->h; i++) {
		for (j = 0; j < bm->w; j++)
			dst[j] = texture_clut[src[j]];
		src += bm->row;
		dst += drow;
	}
}

static void blit_8to8_scale(tdrv_texture_info* info, uchar* dst, int drow)
{
	int i, j, k;
	int scale_w = info->scale_w;
	int scale_h = info->scale_h;
	int step_w = 1 << scale_w;
	int step_h = (1 << scale_h) - 1;
	grs_bitmap* bm = info->bm;
	uchar* src = info->bits;

	for (i = 0; i < bm->h; i++) {
		if (scale_w > 0) {
			uchar* base = dst;
			for (j = 0; j < bm->w; j++) {
				uchar c = src[j];
				for (k = 0; k < step_w; k++)
					base[k] = c;
				base += step_w;
			}
		}
		else
			memcpy(dst, src, bm->w);
		for (j = 0; j < step_h; j++) {
			memcpy(dst + drow, dst, bm->w);
			dst += drow;
		}
		src += bm->row;
		dst += drow;
	}
}

static void blit_8to8_clut_scale(tdrv_texture_info* info, uchar* dst, int drow)
{
	int i, j, k;
	int scale_w = info->scale_w;
	int scale_h = info->scale_h;
	int step_w = 1 << scale_w;
	int step_h = (1 << scale_h) - 1;
	grs_bitmap* bm = info->bm;
	uchar* src = info->bits;

	for (i = 0; i < bm->h; i++) {
		uchar* base = dst;
		for (j = 0; j < bm->w; j++) {
			uchar c = texture_clut[src[j]];
			for (k = 0; k < step_w; k++)
				base[k] = c;
			base += step_w;
		}
		for (j = 0; j < step_h; j++) {
			memcpy(dst + drow, dst, bm->w);
			dst += drow;
		}
		src += bm->row;
		dst += drow;
	}
}

static void blit_16to16(tdrv_texture_info* info, ushort* dst, int drow)
{
	int i;
	grs_bitmap* bm = info->bm;
	uchar* src = info->bits;

	for (i = 0; i < bm->h; i++) {
		memcpy(dst, src, 2 * bm->w);
		src += bm->row;
		dst += drow;
	}
}

static void blit_16to16_scale(tdrv_texture_info* info, ushort* dst, int drow)
{
	int i, j, k;
	int scale_w = info->scale_w;
	int scale_h = info->scale_h;
	int step_w = 1 << scale_w;
	int step_h = (1 << scale_h) - 1;
	grs_bitmap* bm = info->bm;
	int srow = bm->row >> 1;
	ushort* src = (ushort*)info->bits;

	for (i = 0; i < bm->h; i++) {
		if (step_w > 1) {
			ushort* base = dst;
			for (j = 0; j < bm->w; j++) {
				ushort c = src[j];
				for (k = 0; k < step_w; k++)
					base[k] = c;
				base += step_w;
			}
		}
		else
			memcpy(dst, src, 2 * bm->w);
		for (j = 0; j < step_h; j++) {
			memcpy(dst + drow, dst, 2 * bm->w);
			dst += drow;
		}
		src += srow;
		dst += drow;
	}
}

static void blit_32to32(tdrv_texture_info* info, ushort* dst, int drow)
{
	int i;
	grs_bitmap* bm = info->bm;
	uchar* src = info->bits;

	for (i = 0; i < bm->h; i++) {
		memcpy(dst, src, 4 * bm->w);
		src += bm->row;
		dst += drow;
	}
}

static void blit_32to32_scale(tdrv_texture_info* info, ushort* dst, int drow)
{
	int i, j, k;
	int scale_w = info->scale_w;
	int scale_h = info->scale_h;
	int step_w = 1 << scale_w;
	int step_h = (1 << scale_h) - 1;
	grs_bitmap* bm = info->bm;
	int DstRow = drow >> 1;
	int srow = bm->row >> 2;
	uint* src = (uint*)info->bits;

	for (i = 0; i < bm->h; i++) {
		if (step_w > 1) {
			uint* base = (uint*)dst;
			for (j = 0; j < bm->w; j++) {
				uint c = src[j];
				for (k = 0; k < step_w; k++)
					base[k] = c;
				base += step_w;
			}
		}
		else
			memcpy(dst, src, 4 * bm->w);
		for (j = 0; j < step_h; j++) {
			memcpy(dst + drow, dst, 4 * bm->w);
			dst += drow;
		}
		src += srow;
		dst += drow;
	}
}

static void blit_32to16(tdrv_texture_info* info, ushort* dst, int drow)
{
	int x, y;
	grs_bitmap* bm = info->bm;
	uint* pS = (uint*)info->bits;
	int w = bm->w, h = bm->h;

	for (y = 0; y < h; y++) {
		for (x = 0; x < w; x++)
		{
			*dst = ((*pS & 0xF0) >> 4) | ((*pS & 0xF000) >> 8) | ((*pS & 0xF00000) >> 12) | ((*pS & 0xF0000000) >> 16);
			pS++;
			dst++;
		}

		if (drow > w)
			dst += drow - w;
	}
}

void GetAvailableTexMem(DWORD* local, DWORD* agp)
{
    DDSCAPS2 ddscaps;
    ulong total;

	AssertMsg(NULL != g_lpDD_ext, "No DirectDraw object!");

    memset(&ddscaps, 0, sizeof(ddscaps));

	if (local != NULL) {
		ddscaps.dwCaps = LOCAL_CAPS;
		g_lpDD_ext->GetAvailableVidMem(&ddscaps, &total, local);
	}
	if (agp != NULL) {
		ddscaps.dwCaps = NONLOCAL_CAPS;
		g_lpDD_ext->GetAvailableVidMem(&ddscaps, &total, agp);
	}
}


int STDMETHODCALLTYPE EnumTextureFormatsCallback(DDPIXELFORMAT* lpDDPixFmt, void* lpContext)
{
    if ((lpDDPixFmt->dwFlags & DDPF_PALETTEINDEXED8) != 0)
    {
        mono_printf(("8 bit palette indexed format\n"));
    }
    else if ((lpDDPixFmt->dwFlags & DDPF_ALPHA) != 0)
    {
        mono_printf(("Alpha format: bitdepth %i\n", lpDDPixFmt->dwRGBBitCount));
    }
    else if ((lpDDPixFmt->dwFlags & DDPF_RGB) != 0)
    {
        mono_printf(("RGB format: bitdepth %i\n", lpDDPixFmt->dwRGBBitCount));
        mono_printf(("bitmasks: A %x, R %x G %x B %x\n", lpDDPixFmt->dwRGBAlphaBitMask, lpDDPixFmt->dwRBitMask, lpDDPixFmt->dwGBitMask, lpDDPixFmt->dwBBitMask));
    }
    else
    {
        mono_printf(("other format: %i\n", lpDDPixFmt->dwFlags));
    }

    if ((lpDDPixFmt->dwFlags & DDPF_RGB) != 0 && lpDDPixFmt->dwRGBBitCount == 16)
    {
        if (lpDDPixFmt->dwRGBAlphaBitMask == 0xF000 && lpDDPixFmt->dwRBitMask == 0xF00 && lpDDPixFmt->dwGBitMask == 0xF0 && lpDDPixFmt->dwBBitMask == 0xF)
        {
            memcpy(&g_AlphaTextureFormat, lpDDPixFmt, sizeof(g_AlphaTextureFormat));
        }
        else if (lpDDPixFmt->dwRGBAlphaBitMask == 0x8000 && lpDDPixFmt->dwGBitMask == GBitMask15)
        {
            memcpy(&g_TransRGBTextureFormat, lpDDPixFmt, sizeof(g_TransRGBTextureFormat));
        }
        else if (lpDDPixFmt->dwGBitMask == GBitMask16 || lpDDPixFmt->dwGBitMask == GBitMask15 && !g_RGBTextureFormat.dwFlags)
        {
            memcpy(&g_RGBTextureFormat, lpDDPixFmt, sizeof(g_RGBTextureFormat));
        }
    }
    else if ((lpDDPixFmt->dwFlags & DDPF_RGB) != 0 && lpDDPixFmt->dwRGBBitCount == 32)
    {
        if (lpDDPixFmt->dwRGBAlphaBitMask == 0xFF000000 && lpDDPixFmt->dwRBitMask == 0xff0000 && lpDDPixFmt->dwBBitMask == 255)
        {
            memcpy(&g_8888TexFormat, lpDDPixFmt, sizeof(g_8888TexFormat));
            g_b8888supported = TRUE;
        }
    }
    else if ((lpDDPixFmt->dwFlags & DDPF_PALETTEINDEXED8) != 0)
    {
        memcpy(&g_PalTextureFormat, lpDDPixFmt, sizeof(g_PalTextureFormat));
    }

    return TRUE;
}

void CheckSurfaces(sWinDispDevCallbackInfo* info)
{
    int message;
    IDirectDrawSurface4* lpTS;
    int i;

    message = info->message;

    if (message == kCallbackChainAddFunc)
    {
        GenericCallbackChainHandler(&callback_id, (void(**)(callback_chain_info*)) & chain, &info->chain_info);
    }
    else if (message == kCallbackChainRemoveFunc)
    {
        for (i = 0; i < LGD3D_MAX_TEXTURES; i++)
        {
            lpTS = g_saTextures[i].lpSurface;
            if (lpTS && lpTS->IsLost() == DDERR_SURFACELOST)
            {
                if (lpTS->Restore())
                    Warning(("Could not restore lost surface %i!\n", i));
                if (g_saTextures[i].pTdrvBitmap)
                {
                    if (!g_tmgr)
                        CriticalMsg("Hmmm.  Should have a non-NULL texture manager here.");
                    g_tmgr->unload_texture(g_saTextures[i].pTdrvBitmap);
                }
            }
        }
    }

    if (chain)
        chain(info);
}

void InitDefaultTexture(int size)
{
    grs_bitmap* bm;
    int i, j;
    byte c0, c1;

    bm = gr_alloc_bitmap(2, 0, size, size);

    c0 = FindClosestColor(180, 10, 10);
    c1 = FindClosestColor(10, 180, 10);

    for (i = 0; i < size; ++i)
    {
        for (j = 0; j < size; ++j)
        {
            bm->bits[j + i * size] = ((i + j) & 1) != 0 ? c0 : c1;
        }
    }

    default_bm = bm;
}

static int FindClosestColor(float r, float g, float b)
{
    float best = 3 * 256 * 256;
    uchar* pal = grd_pal;
    int i, color = -1;

    for (i = 0; i < 256; i++) {
        float test, dr, dg, db;
        dr = r - pal[0];
        dg = g - pal[1];
        db = b - pal[2];
        pal += 3;
        test = dr * dr + db * db + dg * dg;
        if (test < best) {
            best = test;
            color = i;
        }
    }
    AssertMsg(color >= 0, "Couldn't fit color.");
    return color;
}

int init_size_tables(int** p_size_list)
{
    init_size_table(alpha_size_table, TF_RGB);
    init_size_table(generic_size_table, 0);
    init_size_table(rgb_size_table, TF_ALPHA);
    init_size_table(trgb_size_table, TF_ALPHA | TF_RGB);
    init_size_table(b8888_size_table, TF_TRANS);
    return munge_size_tables(p_size_list);
}

void init_size_table(int* size_table, byte type)
{
    int i, j;
    tdrv_texture_info info;
    int w, h;
    d3d_cookie cookie;

    info.id = 0;
    cookie.flags = type;

    for (i = 0, w = 1; i < 81; i += 9, w *= 2)
    {
        for (j = 0, h = 1; j < 9; j++, h *= 2)
        {
            info.bm = gr_alloc_bitmap((type != 0 && type != TF_RGB) ? (type == TF_TRANS ? BMT_FLAT32 : BMT_FLAT16) : BMT_FLAT8, 0, w, h);
            info.scale_h = 0;
            info.scale_w = 0;
            info.w = info.bm->w;
            info.h = info.bm->h;
            info.size_index = -1;
            info.bits = info.bm->bits;
            cookie.wlog = info.bm->wlog;
            cookie.hlog = info.bm->hlog;
            info.cookie = cookie.value;

            if (pcStates->load_texture(&info) == -1)
            {
                size_table[j + i] = -1;
            }
            else
            {
                if (b_SS2_UseSageTexManager)
                    size_table[j + i] = type + 5 * (j + i);
                else
                    size_table[j + i] = info.size_index;
                pcStates->release_texture(info.id);
            }

            gr_free(info.bm);
        }
    }
}

int munge_size_tables(int** p_size_list)
{
    int* size_list;
    int num_sizes;

    size_list = (int*)malloc(1620);
    num_sizes = get_num_sizes(size_list,
        get_num_sizes(size_list, get_num_sizes(size_list, get_num_sizes(size_list, get_num_sizes(size_list, 0, alpha_size_table),
            generic_size_table), rgb_size_table), trgb_size_table), b8888_size_table);
    *p_size_list = (int*)Realloc(size_list, 4 * num_sizes);
    return num_sizes;
}

int get_num_sizes(int* size_list, int num_sizes, int* size_table)
{
    int i, j;

    for (i = 0; i < 81; ++i)
    {
        if (size_table[i] != -1)
        {
            for (j = 0; j < num_sizes && size_list[j] != size_table[i]; ++j)
                ;
            if (j == num_sizes)
                size_list[num_sizes++] = size_table[i];
            size_table[i] = j;
        }
    }
    return num_sizes;
}
