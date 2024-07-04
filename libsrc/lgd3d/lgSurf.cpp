#define INITGUID
#include <lgSurf_i.h>

#include <comtools.h>

class cLGSurface final : public ILGSurface, public ILGDD4Surface
{
public:
	DECLARE_UNAGGREGATABLE();

public:
	cLGSurface();
	~cLGSurface();

	void ReleaseContent();
	void RecordSurfaceDescription(DDSURFACEDESC2* lpDDSD);
	void FixThePitchInCanvas();
	void SetBitsInCanvas(void* pvData);

public:
	virtual HRESULT STDMETHODCALLTYPE CreateInternalScreenSurface(HWND hMainWindow) override;
	virtual long STDMETHODCALLTYPE ChangeClipper(HWND hMainWindow) override;
	virtual HRESULT STDMETHODCALLTYPE SetAs3dHdwTarget(DWORD dwRequestedFlags, DWORD dwWidth, DWORD pdwHeight, DWORD dwBitDepth, DWORD* pdwCapabilityFlags, grs_canvas** ppsCanvas) override;
	virtual HRESULT STDMETHODCALLTYPE Resize3dHdw(DWORD dwRequestedFlags, DWORD dwWidth, DWORD pdwHeight, DWORD dwBitDepth, DWORD* pdwCapabilityFlags, grs_canvas** ppsCanvas) override;

	virtual HRESULT STDMETHODCALLTYPE BlitToScreen(DWORD   dwXScreen, DWORD   dwYScreen, DWORD   dwWidth, DWORD   dwHeight) override;

	virtual void STDMETHODCALLTYPE CleanSurface() override;
	virtual void STDMETHODCALLTYPE Start3D() override;
	virtual void STDMETHODCALLTYPE End3D() override;

	virtual HRESULT STDMETHODCALLTYPE LockFor2D(grs_canvas** ppsCanvas, eLGSBlit eBlitFlags) override;
	virtual HRESULT STDMETHODCALLTYPE UnlockFor2D() override;

	virtual DWORD STDMETHODCALLTYPE GetWidth() override;
	virtual DWORD STDMETHODCALLTYPE GetHeight() override;
	virtual DWORD STDMETHODCALLTYPE GetPitch() override;
	virtual DWORD STDMETHODCALLTYPE GetBitDepth() override;
	virtual eLGSurfState STDMETHODCALLTYPE GetSurfaceState() override;

	virtual HRESULT STDMETHODCALLTYPE GetSurfaceDescription(sLGSurfaceDescription* psDsc) override;

	virtual int STDMETHODCALLTYPE GetDeviceInfoIndex() override;
	virtual int STDMETHODCALLTYPE GetDirectDraw(IDirectDraw4** ppDD) override;
	virtual int STDMETHODCALLTYPE GetRenderSurface(IDirectDrawSurface4** ppRS) override;

private:
	int m_nFrameCount;
	IDirectDraw4* m_lpDD4;
	IDirectDrawSurface4* m_lpScreen;
	IDirectDrawSurface4* m_lpTheSurface;
	int m_nD3DDevID;
	eLGSurfState m_eSurfState;
	BOOL m_bZbuffer;
	BOOL m_b3DAttached;
	sLGSurfaceDescription m_sDescription;
	grs_canvas m_sCanvas;
};

IMPLEMENT_UNAGGREGATABLE2_SELF_DELETE(cLGSurface, ILGSurface, ILGDD4Surface);


BOOL CreateLGSurface(ILGSurface** ppILGSurf)
{
	*ppILGSurf = new cLGSurface();
	return *ppILGSurf != nullptr;
}