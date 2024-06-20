#include <lgd3d.h>
#include <d3d.h>
#include <wdispapi.h>
#include <wdispcb.h>
#include <appagg.h>
#include <texture.h>
#include <mprintf.h>

BOOL lgd3d_blend_trans = TRUE;

#ifndef SHIP
#define mono_printf(x) \
   if (bSpewOn)      \
      mprintf x;
#else
#define mono_printf(x)
#endif

// #include "d3dmacs.h"
#include "d6States.h"

#include <lgassert.h>
#include <dbg.h>


#ifndef SHIP
static const char* hResErrorMsg = "%s:\nFacility %i, Error %i";
#endif
static char* pcDXef = (char*)"%s: error %d\n%s";
static char* pcLGD3Def = (char*)"LGD3D error no %d : %s : message: %d\n%s";

#define CheckHResult(hRes, msg) \
AssertMsg3(hRes==0, hResErrorMsg, msg, (hRes>>16)&0x7fff, hRes&0xffff)

class cImStates : public cD6States
{
public:
    cD6States *Instance();
    virtual cD6States *DeInstance() override;

protected:
    cImStates(const class cImStates &);
    cImStates();
    virtual ~cImStates();

private:
    cImStates *m_Instance;
};

class cMSStates : public cD6States {
public:
    cD6States * Instance();
    virtual cD6States * DeInstance() override;
    virtual int Initialize(DWORD dwRequestedFlags) override;
    virtual int SetDefaultsStates(DWORD dwRequestedFlags) override;

protected:
    cMSStates(const cMSStates &);
    cMSStates();
    virtual ~cMSStates();

private:
    cMSStates * m_Instance;
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
    virtual void TurnOffTexuring(bool bTexOff) override;
};

DDPIXELFORMAT* g_FormatList[5];

sTextureData g_saTextures[LGD3D_MAX_TEXTURES];

DDPIXELFORMAT g_RGBTextureFormat;

DDPIXELFORMAT g_TransRGBTextureFormat, g_PalTextureFormat, g_AlphaTextureFormat, g_8888TexFormat;

ushort* texture_pal_trans[256];
IDirectDrawPalette* lpDDPalTexture[256];
ushort* texture_pal_opaque[256];
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

int alpha_size_table[81], generic_size_table[81], rgb_size_table[81], trgb_size_table[81], b8888_size_table[81];

sTexBlendArgs sTexBlendArgsProtos[3][2] =
{
    { { D3DTOP_MODULATE, D3DTA_TEXTURE, D3DTA_DIFFUSE, D3DTOP_SELECTARG2, D3DTA_TEXTURE, D3DTA_DIFFUSE }, { D3DTOP_DISABLE, D3DTA_TEXTURE, D3DTA_CURRENT, D3DTOP_DISABLE, D3DTA_TEXTURE, D3DTA_DIFFUSE } },
    { { D3DTOP_MODULATE, D3DTA_TEXTURE, D3DTA_DIFFUSE, D3DTOP_MODULATE, D3DTA_TEXTURE, D3DTA_DIFFUSE }, { D3DTOP_DISABLE, D3DTA_TEXTURE, D3DTA_CURRENT, D3DTOP_DISABLE, D3DTA_TEXTURE, D3DTA_DIFFUSE } },
    { { D3DTOP_BLENDDIFFUSEALPHA, D3DTA_TEXTURE, D3DTA_DIFFUSE, D3DTOP_SELECTARG2, D3DTA_TEXTURE, D3DTA_DIFFUSE }, { D3DTOP_DISABLE, D3DTA_TEXTURE, D3DTA_CURRENT, D3DTOP_DISABLE, D3DTA_TEXTURE, D3DTA_DIFFUSE } }
};

sTexBlendArgs sMultiTexBlendArgsProtos[3][2] =
{
    { { D3DTOP_SELECTARG1, D3DTA_TEXTURE, D3DTA_DIFFUSE, D3DTOP_DISABLE, D3DTA_TEXTURE, D3DTA_DIFFUSE }, { D3DTOP_MODULATE, D3DTA_TEXTURE, D3DTA_CURRENT, D3DTOP_DISABLE, D3DTA_TEXTURE, D3DTA_DIFFUSE } },
    { { D3DTOP_SELECTARG1, D3DTA_TEXTURE, D3DTA_DIFFUSE, D3DTOP_SELECTARG2, D3DTA_TEXTURE, D3DTA_DIFFUSE }, { D3DTOP_SELECTARG2, D3DTA_TEXTURE, D3DTA_CURRENT, D3DTOP_MODULATE, D3DTA_TEXTURE, D3DTA_CURRENT } },
    { { D3DTOP_MODULATE, D3DTA_TEXTURE, D3DTA_DIFFUSE, D3DTOP_SELECTARG1, D3DTA_TEXTURE, D3DTA_DIFFUSE }, { D3DTOP_BLENDTEXTUREALPHA, D3DTA_TEXTURE, D3DTA_CURRENT, D3DTOP_SELECTARG2, D3DTA_TEXTURE, D3DTA_CURRENT } }
};

// TODO: initialize in ScrnInit3d
int b_SS2_UseSageTexManager;

ushort** texture_pal_list[4] = { texture_pal_opaque, texture_pal_opaque, /* is it working? */ &alpha_pal, texture_pal_trans};


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
void blit_32to16(tdrv_texture_info *info, uint16 *dst, unsigned int drow);
int EnumTextureFormatsCallback(_DDPIXELFORMAT *lpDDPixFmt, void *lpContext); 
void CheckSurfaces(sWinDispDevCallbackInfo *info); 
void InitDefaultTexture(int size); 
int FindClosestColor(float r, float g, float b); 
int init_size_tables(int **p_size_list); 
void init_size_table(int *size_table, unsigned __int8 type); 
int munge_size_tables(int **p_size_list); 
int get_num_sizes(int *size_list, int num_sizes, int *size_table); 


void CheckSurfaces(sWinDispDevCallbackInfo* info)
{
    int message;
    IDirectDrawSurface4* lpTS;
    int i;

    message = info->message;

    if (message == kCallbackChainAddFunc)
    {
        GenericCallbackChainHandler(&callback_id, (void(**)(callback_chain_info*))&chain, &info->chain_info);
    }
    else if (message == kCallbackChainRemoveFunc)
    {
        for (i = 0; i < 1024; i++)
        {
            lpTS = g_saTextures[i].lpSurface;
            if (lpTS && lpTS->IsLost() == DDERR_SURFACELOST)
            {
                if (lpTS->Restore())
                    DbgReportWarning("Could not restore lost surface %i!\n", i);
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

int STDMETHODCALLTYPE EnumTextureFormatsCallback(DDPIXELFORMAT* lpDDPixFmt, void* lpContext)
{
    if ((lpDDPixFmt->dwFlags & DDPF_PALETTEINDEXED8) != 0)
    {
        if (bSpewOn)
            mprintf("8 bit palette indexed format\n");
    }
    else if ((lpDDPixFmt->dwFlags & DDPF_ALPHA) != 0)
    {
        if (bSpewOn)
            mprintf("Alpha format: bitdepth %i\n", lpDDPixFmt->dwRGBBitCount);
    }
    else if ((lpDDPixFmt->dwFlags & DDPF_RGB) != 0)
    {
        if (bSpewOn)
            mprintf("RGB format: bitdepth %i\n", lpDDPixFmt->dwRGBBitCount);
        if (bSpewOn)
            mprintf("bitmasks: A %x, R %x G %x B %x\n", lpDDPixFmt->dwRGBAlphaBitMask, lpDDPixFmt->dwRBitMask, lpDDPixFmt->dwGBitMask, lpDDPixFmt->dwBBitMask);
    }
    else if (bSpewOn)
    {
        mprintf("other format: %i\n", lpDDPixFmt->dwFlags);
    }

    if ((lpDDPixFmt->dwFlags & DDPF_RGB) != 0 && lpDDPixFmt->dwRGBBitCount == 16)
    {
        if (lpDDPixFmt->dwRGBAlphaBitMask == 0xF000 && lpDDPixFmt->dwRBitMask == 0xF00 && lpDDPixFmt->dwGBitMask == 0xF0 && lpDDPixFmt->dwBBitMask == 0xF)
        {
            memcpy(&g_AlphaTextureFormat, lpDDPixFmt, sizeof(g_AlphaTextureFormat));
        }
        else if (lpDDPixFmt->dwRGBAlphaBitMask == 0x8000 && lpDDPixFmt->dwGBitMask == 0x3E0)
        {
            memcpy(&g_TransRGBTextureFormat, lpDDPixFmt, sizeof(g_TransRGBTextureFormat));
        }
        else if (lpDDPixFmt->dwGBitMask == 0x7E0 || lpDDPixFmt->dwGBitMask == 0x3E0 && !g_RGBTextureFormat.dwFlags)
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

int FindClosestColor(float r, float g, float b)
{
    float dg, dr, db;
    float test;
    byte* pal;
    int i;
    int color;
    float best;

    best = 196608.0;
    pal = grd_pal;
    color = -1;

    for (i = 0; i < 256; i++)
    {
        dr = r - (float)pal[i * 3 + 0];
        dg = g - (float)pal[i * 3 + 1];
        db = b - (float)pal[i * 3 + 2];
        test = dr * dr + db * db + dg * dg;
        if (test < (float)best)
        {
            best = dr * dr + db * db + dg * dg;
            color = i;
        }
    }
    if (color < 0)
        CriticalMsg("Couldn't fit color.");
    return color;
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

int init_size_tables(int** p_size_list)
{
    init_size_table(alpha_size_table, TF_RGB);
    init_size_table(generic_size_table, 0);
    init_size_table(rgb_size_table, TF_ALPHA);
    init_size_table(trgb_size_table, TF_ALPHA | TF_RGB);
    init_size_table(b8888_size_table, TF_TRANS);
    munge_size_tables(p_size_list);
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

    for (i = 0; i < 256; ++i)
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
    if (pWinDisplayDevice)
        pWinDisplayDevice->Release();

    InitDefaultTexture(16);

    InitTextureManager();
    SetDefaultsStates(dwRequestedFlags);
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

    g_FormatList[0] = m_bTexture_RGB ? &g_RGBTextureFormat : &g_PalTextureFormat;

    if (!g_FormatList[0]->dwFlags)
    {
        m_bTexture_RGB = m_bTexture_RGB == FALSE;
        g_FormatList[0] = m_bTexture_RGB ? &g_RGBTextureFormat : &g_PalTextureFormat;
        if (!g_FormatList[0]->dwFlags)
            CriticalMsg("Direct3D device does not support 8 bit palettized or 15 or 16 bit RGB textures");
    }

    g_FormatList[1] = &g_RGBTextureFormat;
    g_FormatList[2] = &g_AlphaTextureFormat;
    g_FormatList[3] = &g_TransRGBTextureFormat;
    g_FormatList[4] = &g_8888TexFormat;

    if (bSpewOn)
        mprintf("Using %s textures\n", m_bTexture_RGB ? "16 bit RGB" : "Palettized");

    if (!g_AlphaTextureFormat.dwFlags && bSpewOn)
        mprintf("no alpha texture format available.\n");

    memset(&hal, 0, sizeof(hal));
    hal.dwSize = sizeof(hal);
    memset(&hel, 0, sizeof(hel));
    hel.dwSize = sizeof(hel);

    if (hr = g_lpD3Ddevice->GetCaps(&hal, &hel), hr != S_OK)
    {
        CriticalMsg3(pcDXef, "Failed to obtain device caps", (ushort)hr, GetDDErrorMsg(hr));
    }

    if ((hal.dwFlags & 2) == 0)
        CriticalMsg("No HAL device!");

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
        g_tmgr = 0;
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
    this->m_psCurrentRS = (sRenderStates*)p;
}

void cD6States::SetPointerToSetStates(char* p)
{
    this->m_psSetRS = (sRenderStates*)p;
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
            DbgReportWarning(pcLGD3Def, LGD3D_EC_VD_MPASS_MT, GetLgd3dErrorCode(LGD3D_EC_VD_MPASS_MT), (ushort)hR, GetDDErrorMsg(hR));
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
                DbgReportWarning(pcLGD3Def, LGD3D_EC_VD_MPASS_MT, GetLgd3dErrorCode(LGD3D_EC_VD_MPASS_MT), (ushort)hR, GetDDErrorMsg(hR));
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
            DbgReportWarning(pcLGD3Def, LGD3D_EC_VD_S_DEFAULT, GetLgd3dErrorCode(LGD3D_EC_VD_S_DEFAULT), (ushort)hR, GetDDErrorMsg(hR));
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
    ulong local_end, agp_end;
    ulong local_start, agp_start;
    int size;
    int status;

    local_start = 0;
    agp_start = 0;
    size = info->size_index < 0 ? 0 : m_texture_size_list[info->size_index];

    if ((m_texture_caps & 0x20) != 0 && info->w != info->h)
        return TMGR_FAILURE;

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

    if (info->id >= LGD3D_MAX_TEXTURES)
        CriticalMsg("Texture id out of range");

    status = reload_texture(info);

    if (status && m_bUsingLocalMem && m_bAGP_available)
    {
        if (bSpewOn)
            mprintf("using nonlocal vidmem textures...\n");

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

    DbgReportWarning("Texture load took no space!\n");

    if (info->w == 1 && info->h == 1)
    {
        info->size_index = 16;

        return TMGR_SUCCESS;
    }
    else
    {
        DbgReportError(1, "Direct3d device driver does not accurately report texture memory usage.\n  Contact your 3d accelerator vendor for updated drivers.");
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
        DbgReportWarning("texture %i already released\n", n);

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

    RELEASE(psTexData->lpTexture);
    RELEASE(psTexData->lpSurface);

    psTexData->pTdrvBitmap = 0;
}

int cD6States::reload_texture(tdrv_texture_info* info)
{
    IDirectDrawSurface4* SysmemSurface, **pDeviceSurface;
    DDSURFACEDESC2 pddsd;
    d3d_cookie cookie;
    LPDDPIXELFORMAT pixel_format;
    HRESULT LastError;
    IDirect3DTexture2 *SysmemTexture = NULL, **pDeviceTexture;

    pDeviceTexture = &g_saTextures[info->id].lpTexture;
    pDeviceSurface = &g_saTextures[info->id].lpSurface;
    cookie.value = info->cookie;

    pixel_format = g_FormatList[cookie.flags & (TF_TRANS | TF_ALPHA | TF_RGB)];
    if ((cookie.flags & (TF_TRANS | TF_ALPHA | TF_RGB)) == TF_TRANS && !g_b8888supported)
        pixel_format = g_FormatList[2];

    if (pixel_format->dwFlags == 0)
        return TDRV_FAILURE;

    memset(&pddsd, 0, sizeof(pddsd));
    pddsd.dwSize = sizeof(pddsd);
    pddsd.dwFlags = DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS;
    pddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY;
    pddsd.dwHeight = info->h;
    pddsd.dwWidth = info->w;
    memcpy(&pddsd.ddpfPixelFormat, pixel_format, sizeof(pddsd.ddpfPixelFormat));

    LastError = CreateDDSurface(cookie, &pddsd, &SysmemSurface);
    CheckHResult(LastError, "CreateSurface() failed");
    AssertMsg(SysmemSurface != NULL, "NULL SysmemSurface");

    LastError = SysmemSurface->QueryInterface(IID_IDirect3DTexture2, (LPVOID*)&SysmemTexture);
    CheckHResult(LastError, "Failed to obtain D3D texture interface for sysmem surface");

    pddsd.dwSize = sizeof(DDSURFACEDESC2);
    pddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;

    if (*pDeviceTexture == NULL) {
        IDirectDrawSurface4* DeviceSurface;

        pddsd.ddsCaps.dwCaps = m_DeviceSurfaceCaps;
        LastError = CreateDDSurface(cookie, &pddsd, pDeviceSurface);
        if ((LastError != DD_OK) && m_bUsingLocalMem && m_bAGP_available)
        {
            mono_printf(("using nonlocal vidmem textures...\n"));
            // try AGP textures...
            m_bUsingLocalMem = FALSE;
            m_DeviceSurfaceCaps = NONLOCAL_CAPS;
            pddsd.ddsCaps.dwCaps = m_DeviceSurfaceCaps;
            LastError = CreateDDSurface(cookie, &pddsd, pDeviceSurface);
        }

        if (LastError != DD_OK) {
            mono_printf(("couldn't load texture: error %i.\n", LastError & 0xffff));
            SafeRelease(SysmemTexture);
            SafeRelease(SysmemSurface);
            *pDeviceSurface = NULL;
            return TDRV_FAILURE;
        }

        DeviceSurface = *pDeviceSurface;

#define MEMORY_TYPE_MASK \
      (DDSCAPS_SYSTEMMEMORY|DDSCAPS_VIDEOMEMORY|DDSCAPS_LOCALVIDMEM|DDSCAPS_NONLOCALVIDMEM)

        if ((m_DeviceSurfaceCaps & MEMORY_TYPE_MASK) != (pddsd.ddsCaps.dwCaps & MEMORY_TYPE_MASK))
            mono_printf((
                "device texture not created in requested memory location\nRequested %x Received %x\n",
                m_DeviceSurfaceCaps & MEMORY_TYPE_MASK, pddsd.ddsCaps.dwCaps & MEMORY_TYPE_MASK));


        // Query our device surface for a texture interface
        LastError = DeviceSurface->QueryInterface(IID_IDirect3DTexture2, (LPVOID*)pDeviceTexture);
        CheckHResult(LastError, "Failed to obtain D3D texture interface for device surface.");
    }
   
    // Load the bitmap into the sysmem surface
    LastError = SysmemSurface->Lock(NULL, &pddsd, 0, NULL);
    CheckHResult(LastError, "Failed to lock sysmem surface.");

    LoadSurface(info, &pddsd);

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

    g_saTextures[info->id].pTdrvBitmap = info->bm;
    g_saTextures[info->id].TdrvCookie.value = info->cookie;

    return TDRV_SUCCESS;
}



void GetAvailableTexMem(ulong* local, ulong* agp)
{
    DDSCAPS2 ddscaps;
    ulong total;

    if (!g_lpDD_ext)
        CriticalMsg("No DirectDraw object!");

    memset(&ddscaps, 0, sizeof(ddscaps));

    if (local)
    {
        ddscaps.dwCaps = NONLOCAL_CAPS;
        g_lpDD_ext->GetAvailableVidMem(&ddscaps, &total, local);
    }

    if (agp)
    {
        ddscaps.dwCaps = NONLOCAL_CAPS;
        g_lpDD_ext->GetAvailableVidMem(&ddscaps, &total, agp);
    }
}

