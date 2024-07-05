#pragma once

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
    friend class cMSStates;

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
    // also abstract
    virtual ~cD6States() = 0;

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
    D3DZBUFFERTYPE GetDepthBufferState();
    virtual void SetZWrite(BOOL bZWriteOn);
    BOOL IsZWriteOn();
    virtual void SetZCompare(BOOL bZCompreOn);
    BOOL IsZCompareOn();
    virtual void SetFogDensity(float fDensity);
    BOOL UseLinearTableFog(BOOL bOn);
    void SetFogStartAndEnd(float fStart, float fEnd);
    void SetLinearFogDistance(float fDistance);
    void SetAlphaColor(float fAlpha);
    void EnableAlphaBlending(BOOL bAlphaOn);
    BOOL IsAlphaBlendingOn();
    void SetAlphaModulateHack(int iBlendMode);
    void ResetDefaultAlphaModulate();
    void SetTextureMapMode(DWORD dwFlag);
    void GetTexBlendingModes(DWORD *pdw0LevelMode, DWORD *pdw1LevelMode);
    BOOL SetSmoothShading(BOOL bSmoothOn);
    BOOL IsSmoothShadingOn();
    void EnableFog(BOOL bFogOn);
    BOOL IsFogOn();
    void SetFogSpecularLevel(float fLevel);
    void SetFogColor(int r, int g, int b);
    unsigned long get_color();
    void SetTexturePalette(int start, int n, uchar *pal, int slot);
    void EnablePalette(BOOL bPalOn);
    BOOL IsPaletteOn();
    void SetPalSlotFlags(int start, int n, uint8 *pal_data, int slot, char flags);
    void SetAlphaPalette(uint16 *pal);
    virtual void SetChromaKey(int r, int g, int b);
    int GetTexWrapping(DWORD dwLevel);
    BOOL SetTexWrapping(DWORD dwLevel, BOOL bSetSmooth);
    BOOL EnableSpecular(BOOL bUseIt);

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
extern BOOL g_bTexSuspended;

extern BOOL bSpewOn;
extern BOOL g_bPrefer_RGB;

extern BOOL lgd3d_g_bInitialized;
