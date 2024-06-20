#pragma once

#include <tdrv.h>

#include <ddraw.h>
#include <d3d.h>
#include <d3dtypes.h>

#include <types.h>
#include <grs.h>


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

struct tdrv_texture_info
{
    grs_bitmap *bm;
    int id;
    int flags;
    int scale_w;
    int scale_h;
    int w;
    int h;
    int size_index;
    uint8 *bits;
    unsigned long cookie;
};

struct sTexBlendArgs
{
    D3DTEXTUREOP eColorOperation;
    DWORD dwColorArg1;
    DWORD dwColorArg2;
    D3DTEXTUREOP eAlphaOperation;
    DWORD dwAlphaArg1;
    DWORD dwAlphaArg2;
};

struct sRenderStates
{
    int bUsePalette;
    int nAlphaColor;
    D3DSHADEMODE eShadeMode;
    int bDitheringOn;
    int bAntialiasingOn;
    int bAlphaOn;
    D3DBLEND eSrcAlpha;
    D3DBLEND eDstAlpha;
    D3DZBUFFERTYPE eZenable;
    int bZWriteEnable;
    D3DCMPFUNC eZCompareFunc;
    int bFogOn;
    DWORD dwFogColor;
    D3DFOGMODE eFogMode;
    float fFogTableDensity;
    float fFogStart;
    float fFogEnd;
    unsigned long dcFogSpecular;
    int bPerspectiveCorr;
    DWORD dwNoLightMapLevels;
    DWORD dwTexBlendMode[2];
    int naTextureId[2];
    int baTextureWithAlpha[2];
    int baChromaKeying[2];
    sTexBlendArgs saTexBlend[2];
    D3DTEXTUREADDRESS eWrap[2];
    int chroma_r;
    int chroma_g;
    int chroma_b;
    unsigned long chroma_key;
    D3DTEXTUREMINFILTER eMagTexFilter;
    D3DTEXTUREMINFILTER eMinTexFilter;
};

typedef struct
{
    IDirect3DTexture2* lpTexture;
    IDirectDrawSurface4* lpSurface;
    grs_bitmap* pTdrvBitmap;
    d3d_cookie TdrvCookie;
} sTextureData;

class cD6States
{
public:
    virtual int Initialize(DWORD dwRequestedFlags);

protected:
    unsigned long m_DeviceSurfaceCaps;
    int *m_texture_size_list;
    unsigned long m_texture_caps;
    BOOL m_bTextureListInitialized;
    sRenderStates *m_psCurrentRS;
    sRenderStates *m_psSetRS;
private:
    BOOL m_bTexture_RGB;
    BOOL m_bUsingLocalMem;
    BOOL m_bLocalMem_available;
    BOOL m_bAGP_available;
    BOOL m_bWBuffer;
    BOOL m_bCanDither;
    BOOL m_bCanAntialias;
    BOOL m_bSpecular;
    BOOL m_bCanModulate;
    BOOL m_bCanModulateAlpha;

protected:
    void EnumerateTextureFormats();
    void InitTextureManager();
    virtual int SetDefaultsStates(DWORD dwRequestedFlags);
    void SetCommonDefaultStates(DWORD dwRequestedFlags, BOOL bMultiTexture);

    // copy ctr was created by compiler
    // cD6States(const cD6States &);
    cD6States();
    virtual ~cD6States();

public:
    int load_texture(tdrv_texture_info *info);
    void release_texture(int n);
    void unload_texture(int n);
    void synchronize();
    void start_frame(int n);
    void end_frame();
    virtual void set_texture_id(int n);
    virtual int reload_texture(tdrv_texture_info *info);
    virtual void cook_info(tdrv_texture_info *info);
    int reload_texture_a(tdrv_texture_info *info);
    int load_texture_a(tdrv_texture_info *info);
    void release_texture_a(int n); // maybe unsigned
    virtual void SetLightMapMode(DWORD dwFlag);
    virtual void SetTextureLevel(int n);
    virtual int EnableMTMode(DWORD dwMTOn);
    unsigned long GetRenderStatesSize();
    void SetPointerToCurrentStates(char *p);
    void SetPointerToSetStates(char *p);
    void *GetCurrentStates();
    long CreateDDSurface(d3d_cookie cookie, DDSURFACEDESC2 *pddsd, IDirectDrawSurface4 **ppDDS);
    void LoadSurface(tdrv_texture_info *info, DDSURFACEDESC2 *pddsd);
    int get_texture_id();
    virtual void TurnOffTexuring(BOOL bTexOff);
    void SetTextureNow();
    virtual cD6States *DeInstance() = 0; // abstract for cD6States
    void SetDithering(BOOL bOn);
    int GetDitheringState();
    void SetAntialiasing(BOOL bOn);
    int GetAntialiasingState();
    virtual void EnableDepthBuffer(int nFlag);
    int GetDepthBufferState();
    virtual void SetZWrite(BOOL bZWriteOn);
    bool IsZWriteOn();
    virtual void SetZCompare(BOOL bZCompreOn);
    bool IsZCompareOn();
    virtual void SetFogDensity(float fDensity);
    int UseLinearTableFog(BOOL bOn);
    void SetFogStartAndEnd(float fStart, float fEnd);
    void SetLinearFogDistance(float fDistance);
    void SetAlphaColor(float fAlpha);
    void EnableAlphaBlending(BOOL bAlphaOn);
    BOOL IsAlphaBlendingOn();
    void SetAlphaModulateHack(int iBlendMode);
    void ResetDefaultAlphaModulate();
    void SetTextureMapMode(DWORD dwFlag);
    void GetTexBlendingModes(DWORD *pdw0LevelMode, DWORD *pdw1LevelMode);
    int SetSmoothShading(BOOL bSmoothOn);
    BOOL IsSmoothShadingOn();
    void EnableFog(BOOL bFogOn);
    BOOL IsFogOn();
    void SetFogSpecularLevel(float fLevel);
    void SetFogColor(int r, int g, int b);
    unsigned long get_color();
    void SetTexturePalette(int start, int n, uint8 *pal, int slot);
    void EnablePalette(BOOL bPalOn);
    BOOL IsPaletteOn();
    void SetPalSlotFlags(int start, int n, uint8 *pal_data, int slot, char flags);
    void SetAlphaPalette(uint16 *pal);
    virtual void SetChromaKey(int r, int g, int b);
    int GetTexWrapping(DWORD dwLevel);
    int SetTexWrapping(DWORD dwLevel, bool bSetSmooth);
    int EnableSpecular(bool bUseIt);

private:
    float LinearWorldIntoFogCoef(float fLin);
public:
    // cD6States& operator=(const cD6States&);
};


#define SetRenderStateForGlobal(dev, key, value) \
    if (hRes = dev->SetRenderState(key, value), hRes != S_OK) \
    { \
        CriticalMsg3(pcDXef, "SetRenderStateForGlobal failed", (ushort)hRes, GetDDErrorMsg(hRes)); \
    }

#define SetTextureStageStateForGlobal(dev, stage, key, value) \
    if (hResult = dev->SetTextureStageState(stage, key, value), hResult != S_OK) \
    { \
        CriticalMsg3(pcDXef, "SetTextureStageState failed", (ushort)hResult, GetDDErrorMsg(hResult)); \
    }

#define SetTextureStageColors(dev, stage, rs) \
    SetTextureStageStateForGlobal(dev, stage, D3DTSS_COLOROP, rs->saTexBlend[stage].eColorOperation); \
    SetTextureStageStateForGlobal(dev, stage, D3DTSS_COLORARG1, rs->saTexBlend[stage].dwColorArg1); \
    SetTextureStageStateForGlobal(dev, stage, D3DTSS_COLORARG2, rs->saTexBlend[stage].dwColorArg2); \
    SetTextureStageStateForGlobal(dev, stage, D3DTSS_ALPHAOP, rs->saTexBlend[stage].eAlphaOperation); \
    SetTextureStageStateForGlobal(dev, stage, D3DTSS_ALPHAARG1, rs->saTexBlend[stage].dwAlphaArg1); \
    SetTextureStageStateForGlobal(dev, stage, D3DTSS_ALPHAARG2, rs->saTexBlend[stage].dwAlphaArg2);

extern cD6States* pcStates;
extern IDirect3DDevice3* g_lpD3Ddevice;
extern IDirectDraw4* g_lpDD_ext;
extern BOOL g_bUseDepthBuffer, g_bUseTableFog, g_bUseVertexFog;

/* code from lgd3d.h */

#define LGRT_SINGLE_TEXTURE_SINGLE_PASS     0L
#define LGRT_MULTI_TEXTURE_SINGLE_PASS      1L

    //NOTE: multipass multitexturing is not supported!!!

    // Capabilities that can be requested:

#define LGD3DF_SPEW                    0x00000002L

//depth buffer:
#define LGD3DF_ZBUFFER                 0x00000001L // Z-buffer 
#define LGD3DF_WBUFFER                 0x00000004L // W-buffer 
#define LGD3DF_DEPTH_BUFFER_REQUIRED   0x00000008L // the 
// selected (Z_ or W-) depth buffer REQUIRED!
//fog:
#define LGD3DF_TABLE_FOG               0x00000010L // table based pixel fog
#define LGD3DF_VERTEX_FOG              0x00000020L 

//dithering
#define LGD3DF_DITHER                  0x00000040L // use dithering 

//antialiasing
#define LGD3DF_ANTIALIAS				   0x00000080L // use (sort independent) antialiasing 

#define LGD3DF_MULTI_TEXTURING         0x00000100L // we support ONLY two sets of textures and
// and texture coordinates

#define LGD3DF_MODULATEALPHA			   0x00000200L // we want to use modulate alpha for single texture mode
#define LGD3DF_BLENDDIFFUSE				0x00000400L // we want to use blend diffuse for single texture mode

#define LGD3DF_MULTITEXTURE_COLOR		0x00000800L // we want to use color light maps for single or multi texture mode
#define LGD3DF_MULTITEXTURE_ALPHA		0x00001000L // we want to use alpha light maps for single or multi texture mode

#define LGD3DF_DO_WINDOWED             0x00002000L

#define LGD3DF_MT_BLENDDIFFUSE         0x00004000L // we want to use alpha light maps for single or multi texture mode

// Supported capabilities: (returned by the enumeration)

//depth buffer
#define LGD3DF_CAN_DO_ZBUFFER				0x00010000L
#define LGD3DF_CAN_DO_WBUFFER				0x00040000L // W-buffer 
//fog:
#define LGD3DF_CAN_DO_TABLE_FOG			0x00080000L // table based pixel fog
#define LGD3DF_CAN_DO_VERTEX_FOG			0x00100000L // vertex fog
//dithering
#define LGD3DF_CAN_DO_DITHER				0x00200000L // can dither
#define LGD3DF_CAN_DO_ANTIALIAS			0x00400000L // can use (sort independent) antialiasing 

#define LGD3DF_CAN_DO_SINGLE_PASS_MT	0x02000000L // we can do single pass double texturing
#define LGD3DF_CAN_DO_WINDOWED         0x04000000L // we can play the game in lil' window

#define LGD3DF_CAN_DO_ITERATE_ALPHA    0x08000000L // can do Gouraud interpolation between vetices alpha color


///////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////

    // Multi texturing

    //used for single level texturing
#define LGD3DTB_MODULATE            0L    //default        
#define LGD3DTB_MODULATEALPHA       1L
#define LGD3DTB_BLENDDIFFUSE        2L

#define LGD3DTB_NO_STATES           3L

    //2 levels texturing:
#define LGD3D_MULTITEXTURE_COLOR             0L  //default
#define LGD3D_MULTITEXTURE_ALPHA             1L  
#define LGD3D_MULTITEXTURE_BLEND_TEX_ALPHA   2L  

#define LGD3D_MULTITEXTURE_NO_STATES         3L

    // additional sets of texture coordinates are added
    typedef struct {
        float   u, v;
    } LGD3D_tex_coord;

    // error codes:( the first argument of "lgd3d_get_error" )
#define LGD3D_EC_OK                             0L
#define LGD3D_EC_DD_KAPUT                       1L
#define LGD3D_EC_RESTORE_ALL_SURFS              2L
#define LGD3D_EC_QUERY_D3D                      3L
#define LGD3D_EC_GET_DD_CAPS                    4L
#define LGD3D_EC_ZBUFF_ENUMERATION              5L
#define LGD3D_EC_CREATE_3D_DEVICE               6L
#define LGD3D_EC_CREATE_VIEWPORT                7L
#define LGD3D_EC_ADD_VIEWPORT                   8L
#define LGD3D_EC_SET_VIEWPORT                   9L
#define LGD3D_EC_SET_CURR_VP                    10L
#define LGD3D_EC_CREATE_BK_MATERIAL             11L
#define LGD3D_EC_SET_BK_MATERIAL                12L
#define LGD3D_EC_GET_BK_MAT_HANDLE              13L
#define LGD3D_EC_GET_SURF_DESC                  14L
#define LGD3D_EC_GET_3D_CAPS                    15L
#define LGD3D_EC_VD_MPASS_MT                    16L
#define LGD3D_EC_VD_S_DEFAULT                   17L
#define LGD3D_EC_VD_SPASS_MT                    18L
#define LGD3D_EC_VD_M_DEFAULT                   19L
#define LGD3D_EC_VD_SPASS_BLENDDIFFUSE          20L
#define LGD3D_EC_VD_MPASS_BLENDDIFFUSE          21L

BOOL lgd3d_get_error(DWORD* pdwCode, DWORD* phResult);
extern int bSpewOn;
extern char* GetDDErrorMsg(int hRes);
extern int g_bPrefer_RGB;

extern void SetLGD3DErrorCode(ulong dwCode, long hRes);
extern char* GetLgd3dErrorCode(ulong dwErrorCode);

extern BOOL lgd3d_g_bInitialized;
