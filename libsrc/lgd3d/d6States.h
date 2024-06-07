#pragma once

#include <ddraw.h>
#include <d3dtypes.h>

#include <types.h>
#include <grs.h>


struct d3d_cookie {
 uint8 wlog;
 uint8 hlog;
 uint8 flags;
 uint8 palette;
 unsigned long value;
};

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

class cD6States
{
public:
    virtual int Initialize(DWORD dwRequestedFlags);

private:
    unsigned long m_DeviceSurfaceCaps;
    int *m_texture_size_list;
    unsigned long m_texture_caps;
    bool m_bTextureListInitialized;
    sRenderStates *m_psCurrentRS;
    sRenderStates *m_psSetRS;
    bool m_bTexture_RGB;
    bool m_bUsingLocalMem;
    bool m_bLocalMem_available;
    bool m_bAGP_available;
    bool m_bWBuffer;
    bool m_bCanDither;
    bool m_bCanAntialias;
    bool m_bSpecular;
    bool m_bCanModulate;
    bool m_bCanModulateAlpha;

protected:
    void EnumerateTextureFormats();
    void InitTextureManager();
    virtual int SetDefaultsStates(DWORD dwRequestedFlags);
    void SetCommonDefaultStates(DWORD dwRequestedFlags, bool bMultiTexture);

    cD6States(const cD6States &);
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
    virtual void TurnOffTexuring(bool bTexOff);
    void SetTextureNow();
    virtual cD6States *DeInstance();
    void SetDithering(bool bOn);
    int GetDitheringState();
    void SetAntialiasing(bool bOn);
    int GetAntialiasingState();
    virtual void EnableDepthBuffer(int nFlag);
    int GetDepthBufferState();
    virtual void SetZWrite(bool bZWriteOn);
    bool IsZWriteOn();
    virtual void SetZCompare(bool bZCompreOn);
    bool IsZCompareOn();
    virtual void SetFogDensity(float fDensity);
    int UseLinearTableFog(bool bOn);
    void SetFogStartAndEnd(float fStart, float fEnd);
    void SetLinearFogDistance(float fDistance);
    void SetAlphaColor(float fAlpha);
    void EnableAlphaBlending(bool bAlphaOn);
    bool IsAlphaBlendingOn();
    void SetAlphaModulateHack(int iBlendMode);
    void ResetDefaultAlphaModulate();
    void SetTextureMapMode(DWORD dwFlag);
    void GetTexBlendingModes(DWORD *pdw0LevelMode, DWORD *pdw1LevelMode);
    int SetSmoothShading(bool bSmoothOn);
    bool IsSmoothShadingOn();
    void EnableFog(bool bFogOn);
    bool IsFogOn();
    void SetFogSpecularLevel(float fLevel);
    void SetFogColor(int r, int g, int b);
    unsigned long get_color();
    void SetTexturePalette(int start, int n, uint8 *pal, int slot);
    void EnablePalette(bool bPalOn);
    bool IsPaletteOn();
    void SetPalSlotFlags(int start, int n, uint8 *pal_data, int slot, char flags);
    void SetAlphaPalette(uint16 *pal);
    virtual void SetChromaKey(int r, int g, int b);
    int GetTexWrapping(DWORD dwLevel);
    int SetTexWrapping(DWORD dwLevel, bool bSetSmooth);
    int EnableSpecular(bool bUseIt);

private:
    float LinearWorldIntoFogCoef(float fLin);
};