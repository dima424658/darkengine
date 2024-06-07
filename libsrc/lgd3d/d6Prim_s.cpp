#include <d6Prim.h>

struct MTVERTEX
{
    float sx, sy, sz, rhw;
    unsigned int color, specular;
    float tu, tv, tu2, tv2;
};

class cImBuffer : public cD6Primitives
{
private:
    static cImBuffer *m_Instance;

    cImBuffer(const class cImBuffer &);
    cImBuffer();
    ~cImBuffer();

public:
    cD6Primitives *Instance();
    virtual cD6Primitives *DeInstance() override;
};

class cMSBuffer : public cD6Primitives
{
private:
    static cMSBuffer *m_Instance;

    cMSBuffer(const class cMSBuffer &);
    cMSBuffer();
    ~cMSBuffer();

public:
    cD6Primitives *Instance();
    virtual cD6Primitives *DeInstance() override;

private:
    MTVERTEX m_saMTVertices[0x32];
    DWORD m_dwNoMTVertices;
    DWORD m_dwMaxNoMTVertices;

private:
    virtual void DrawPoly(bool bSuspendTexturing) override;
    virtual void FlushIndPolies() override;
    virtual void DrawIndPolies() override;

    MTVERTEX *ReserveMTPolySlots(int n);
    void DrawMTPoly();

public:
    virtual int TrifanMTD(int n, r3s_point **ppl, LGD3D_tex_coord **pptc) override;
    virtual int LitTrifanMTD(int n, r3s_point **ppl, LGD3D_tex_coord **pptc) override;
    virtual int RGBlitTrifanMTD(int n, r3s_point **ppl, LGD3D_tex_coord **pptc) override;
    virtual int RGBAlitTrifanMTD(int n, r3s_point **ppl, LGD3D_tex_coord **pptc) override;
    virtual int RGBAFogLitTrifanMTD(int n, r3s_point **ppl, LGD3D_tex_coord **pptc) override;
    virtual int DiffuseSpecularLitTrifanMTD(int n, r3s_point **ppl, LGD3D_tex_coord **pptc) override;
    virtual int g2UTrifanMTD(int n, g2s_point **vpl, LGD3D_tex_coord **pptc) override;
    virtual int g2TrifanMTD(int n, g2s_point **vpl, LGD3D_tex_coord **pptc) override;
};