#pragma once

#include <d3dtypes.h>

#include <types.h>
#include <r3ds.h>
#include <g2spoint.h>
#include <lgd3d.h>

class cD6Primitives {
protected:
    cD6Primitives(const class cD6Primitives &);
    cD6Primitives();
    ~cD6Primitives();

public:
    cD6Primitives * DeInstance();

private:
    int m_bPrimitivesPending;
    int m_bFlushingOn;
    int m_nAlpha;
    unsigned long m_dcFogSpecular;
    int m_bPointMode;
    int m_iSavedTexId;
    enum ePolyMode m_ePolyMode;

public:
    ePolyMode GetPolyMode();
    int SetPolyMode(ePolyMode eNewMode);

protected:
    void DrawStandardEdges(void *pVertera, unsigned int dwNoVeriteces);

public:
    virtual void FlushPrimitives();
    void PassFogSpecularColor(unsigned int dcFogColor);
    void PassAlphaColor(int nAlapha);
    virtual void StartFrame();
    virtual void EndFrame();
    virtual void Clear(int c);
    virtual void StartNonTexMode();
    virtual void EndNonTexMode();

private:
    unsigned long m_dwNoCashedPoints;
    unsigned long m_dwPointBufferSize;
    D3DTLVERTEX m_saPointBuffer[50];

    D3DTLVERTEX m_saVertexBuffer[50];
    unsigned long m_dwNoCashedVertices;
    unsigned long m_dwVertexBufferSize;

public:
    virtual D3DTLVERTEX * ReservePointSlots(int n);
    virtual void FlushPoints();
    void SetPointBufferSize(unsigned long); // static?
    unsigned long GetPointBufferSize(); // static?
    virtual int DrawPoint(r3s_point *p);

    D3DTLVERTEX * ReservePolySlots(int n);
    virtual void DrawPoly(bool bSuspendTexturing);
    virtual int Poly(int n, r3s_point * * ppl);
    virtual int SPoly(int n, r3s_point * * ppl);
    virtual int RGB_Poly(int n, r3s_point * * ppl);
    virtual int RGBA_Poly(int n, r3s_point * * ppl);
    virtual int Trifan(int n, r3s_point * * ppl);
    virtual int LitTrifan(int n, r3s_point * * ppl);
    virtual int RGBlitTrifan(int n, r3s_point * * ppl);
    virtual int RGBAlitTrifan(int n, r3s_point * * ppl);
    virtual int RGBAFogLitTrifan(int n, r3s_point * * ppl);
    virtual int DiffuseSpecularLitTrifan(int n, r3s_point * * ppl);
    virtual int g2UPoly(int n, g2s_point * * ppl);
    virtual int g2Poly(int n, g2s_point * * ppl);
    virtual int g2UTrifan(int n, g2s_point * * ppl);
    virtual int g2Trifan(int n, g2s_point * * ppl);
    void DrawLine(r3s_point * p0, r3s_point * p1);

protected:
    virtual void InitializeVIBCounters();
    virtual int CreateVertIndBuffer(DWORD dwInitialNoEntries);
    virtual void DeleteVertIndBuffer();
    virtual int ResizeVertIndBuffer(DWORD dwNewNoEntries);

private:
    D3DTLVERTEX * m_pVIB;
    DWORD m_dwNoVIBEntries;
    DWORD m_dwVIBSizeInEntries;
    unsigned short m_waIndices[256];
    DWORD m_dwVIBmax;
    DWORD m_dwNoIndices;
    unsigned short m_waTempIndices[50];
    DWORD m_dwMinVIndex;
    DWORD m_dwMaxVIndex;
    DWORD m_dwTempIndCounter;
    grs_bitmap * m_hack_light_bm;

public:
    virtual void EndIndexedRun();
    virtual void FlushIfNoFit(int nIndicesToAdd, bool bSuspendTexturing);
    virtual D3DTLVERTEX * GetIndPolySlot(int nPolySize, r3ixs_info *psIndInfo);
    virtual void DrawIndPolies();
    virtual void FlushIndPolies();
    virtual int PolyInd(int n, r3s_point **ppl, r3ixs_info *psIndInfo);
    virtual int SPolyInd(int n, r3s_point **ppl, r3ixs_info *psIndInfo);
    virtual int RGB_PolyInd(int n, r3s_point **ppl, r3ixs_info *psIndInfo);
    virtual int RGBA_PolyInd(int n, r3s_point **ppl, r3ixs_info *psIndInfo);
    virtual int TrifanInd(int n, r3s_point **ppl, r3ixs_info *psIndInfo);
    virtual int LitTrifanInd(int n, r3s_point **ppl, r3ixs_info *psIndInfo);
    virtual int RGBlitTrifanInd(int n, r3s_point **ppl, r3ixs_info *psIndInfo);
    virtual int RGBAlitTrifanInd(int n, r3s_point **ppl, r3ixs_info *psIndInfo);
    int RGBAFogLitTrifanInd(int n, r3s_point **ppl, r3ixs_info *psIndInfo);
    int DiffuseSpecularLitTrifanInd(int n, r3s_point **ppl, r3ixs_info *psIndInfo);
    virtual int TrifanMTD(int n, r3s_point **ppl, LGD3D_tex_coord **pptc);
    virtual int LitTrifanMTD(int n, r3s_point **ppl, LGD3D_tex_coord **pptc);
    virtual int RGBlitTrifanMTD(int n, r3s_point **ppl, LGD3D_tex_coord **pptc);
    virtual int RGBAlitTrifanMTD(int n, r3s_point **ppl, LGD3D_tex_coord **pptc);
    virtual int RGBAFogLitTrifanMTD(int n, r3s_point **ppl, LGD3D_tex_coord **pptc);
    virtual int DiffuseSpecularLitTrifanMTD(int n, r3s_point **ppl, LGD3D_tex_coord **pptc);
    virtual int g2UTrifanMTD(int n, g2s_point **vpl, LGD3D_tex_coord **pptc);
    virtual int g2TrifanMTD(int n, g2s_point **vpl, LGD3D_tex_coord **pptc);

    void init_hack_light_bm();
    void do_quad_light(r3s_point *p, float r, grs_bitmap *bm);
    void HackLight(r3s_point *p, float r);
    void HackLightExtra(r3s_point *p, float r, grs_bitmap *bm);
};