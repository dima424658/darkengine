#pragma once

#include <d3dtypes.h>

#include <lgd3d.h>
#include <r3ds.h>
#include <g2spoint.h>

struct MTVERTEX
{
    float sx, sy, sz, rhw;
    unsigned int color, specular;
    float tu, tv, tu2, tv2;
};

class cD6Primitives {
protected:
    cD6Primitives(const class cD6Primitives &);
    cD6Primitives();
    ~cD6Primitives();

public:
    virtual cD6Primitives * DeInstance();

protected:
    BOOL m_bPrimitivesPending;
    BOOL m_bFlushingOn;
    int m_nAlpha;
    ulong m_dcFogSpecular;
    BOOL m_bPointMode;
    int m_iSavedTexId;
    enum ePolyMode m_ePolyMode;

public:
    ePolyMode GetPolyMode();
    BOOL SetPolyMode(ePolyMode eNewMode);

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

protected:
    DWORD m_dwNoCashedPoints;
    DWORD m_dwPointBufferSize;
    D3DTLVERTEX m_saPointBuffer[50];

    D3DTLVERTEX m_saVertexBuffer[50];
    DWORD m_dwNoCashedVertices;
    DWORD m_dwVertexBufferSize;

public:
    virtual D3DTLVERTEX* ReservePointSlots(int n);
    virtual void FlushPoints();
    void SetPointBufferSize(DWORD); // static?
    DWORD GetPointBufferSize(); // static?
    virtual int DrawPoint(r3s_point *p);

    D3DTLVERTEX* ReservePolySlots(unsigned int n);
    virtual void DrawPoly(BOOL bSuspendTexturing);
    virtual BOOL Poly(int n, r3s_point * * ppl);
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
    virtual BOOL CreateVertIndBuffer(DWORD dwInitialNoEntries);
    virtual void DeleteVertIndBuffer();
    virtual BOOL ResizeVertIndBuffer(DWORD dwNewNoEntries);

private:
    D3DTLVERTEX * m_pVIB;
    DWORD m_dwNoVIBEntries;
    DWORD m_dwVIBSizeInEntries;
    ushort m_waIndices[256];
    DWORD m_dwVIBmax;
    DWORD m_dwNoIndices;
    ushort m_waTempIndices[50];
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

extern cD6Primitives* pcRenderBuffer;

class cImBuffer : public cD6Primitives
{
private:
    static cImBuffer* m_Instance;

    cImBuffer(const class cImBuffer&);
    cImBuffer();
    ~cImBuffer();

public:
    static cD6Primitives* Instance();
    virtual cD6Primitives* DeInstance() override;
};

class cMSBuffer : public cD6Primitives
{
private:
    static cMSBuffer* m_Instance;

    cMSBuffer(const class cMSBuffer&);
    cMSBuffer();
    ~cMSBuffer();

public:
    static cD6Primitives* Instance();
    virtual cD6Primitives* DeInstance() override;

private:
    MTVERTEX m_saMTVertices[0x32];
    DWORD m_dwNoMTVertices;
    DWORD m_dwMaxNoMTVertices;

private:
    virtual void DrawPoly(BOOL bSuspendTexturing) override;
    virtual void FlushIndPolies() override;
    virtual void DrawIndPolies() override;

    MTVERTEX* ReserveMTPolySlots(int n);
    void DrawMTPoly();

public:
    virtual int TrifanMTD(int n, r3s_point** ppl, LGD3D_tex_coord** pptc) override;
    virtual int LitTrifanMTD(int n, r3s_point** ppl, LGD3D_tex_coord** pptc) override;
    virtual int RGBlitTrifanMTD(int n, r3s_point** ppl, LGD3D_tex_coord** pptc) override;
    virtual int RGBAlitTrifanMTD(int n, r3s_point** ppl, LGD3D_tex_coord** pptc) override;
    virtual int RGBAFogLitTrifanMTD(int n, r3s_point** ppl, LGD3D_tex_coord** pptc) override;
    virtual int DiffuseSpecularLitTrifanMTD(int n, r3s_point** ppl, LGD3D_tex_coord** pptc) override;
    virtual int g2UTrifanMTD(int n, g2s_point** vpl, LGD3D_tex_coord** pptc) override;
    virtual int g2TrifanMTD(int n, g2s_point** vpl, LGD3D_tex_coord** pptc) override;
};


