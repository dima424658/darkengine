#include <types.h>
#include <wdispapi.h>
#include <d3d.h>

DevDesc g_sD3DDevDesc;
IDirectDraw4* g_lpDD_ext;

class cD6Frame {
public:
    cD6Frame(ILGSurface *pILGSurface);
    cD6Frame(DWORD dwWidth, DWORD dwHeight, lgd3ds_device_info *psDeviceInfo);
    ~cD6Frame();

private:
    void InitializeGlobals(DWORD dwWidth, DWORD dwHeight, DWORD dwRequestedFlags);
    void InitializeEnvironment(lgd3ds_device_info *psDeviceInfo);
    long GetDDstuffFromDisplay();
    long CreateDepthBuffer();
    int CreateD3D(const GUID & sDeviceGUID);
    void ExamineRenderingCapabilities();

private:
    DWORD m_dwRequestedFlags;
    int m_bDepthBuffer;
    DWORD m_dwTextureOpCaps;
    IWinDisplayDevice * m_pWinDisplayDevice;
};

cD6Frame::cD6Frame(ILGSurface *pILGSurface)
{
}

cD6Frame::cD6Frame(DWORD dwWidth, DWORD dwHeight, lgd3ds_device_info *psDeviceInfo)
{
}

cD6Frame::~cD6Frame()
{
}

void cD6Frame::InitializeGlobals(DWORD dwWidth, DWORD dwHeight, DWORD dwRequestedFlags)
{
}

void cD6Frame::InitializeEnvironment(lgd3ds_device_info *psDeviceInfo)
{
}

long cD6Frame::GetDDstuffFromDisplay()
{
    return 0;
}

long cD6Frame::CreateDepthBuffer()
{
    return 0;
}

int cD6Frame::CreateD3D(const GUID &sDeviceGUID)
{
    return 0;
}

void cD6Frame::ExamineRenderingCapabilities()
{
}

int c_EnumZBufferFormats(DDPIXELFORMAT *lpDDPixFmt, void *lpContext);
