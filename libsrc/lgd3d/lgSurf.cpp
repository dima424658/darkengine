#include <initguid.h>
#include <lgSurf_i.h>

#include <wdispapi.h>
#include <dispapi.h>
#include <lgd3d.h>
#include <lgassert.h>
#include <appagg.h>
#include <dddynf.h>
#include <dbg.h>

extern BOOL lgd3d_g_bInitialized;

#define MAKE_LGSHRESULT( code )  MAKE_HRESULT( 1, 0x404, code )

#define LGSERR_NOTINITIALIZED MAKE_LGSHRESULT( 1 )
#define LGSERR_D3DNOTINITIALIZED MAKE_LGSHRESULT( 2 )
#define LGSERR_DDNOTINITIALIZED MAKE_LGSHRESULT( 4 )
#define LGSERR_GENERIC MAKE_LGSHRESULT( 5 )
#define LGSERR_SURFACELOCKED MAKE_LGSHRESULT( 7 )
#define LGSERR_SURFACEUNLOCKED MAKE_LGSHRESULT( 8 )
#define LGSERR_INVALIDDESC MAKE_LGSHRESULT( 9 )

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
	virtual HRESULT STDMETHODCALLTYPE ChangeClipper(HWND hMainWindow) override;
	virtual HRESULT STDMETHODCALLTYPE SetAs3dHdwTarget(DWORD dwRequestedFlags, DWORD dwWidth, DWORD dwHeight, DWORD dwBitDepth, DWORD* pdwCapabilityFlags, grs_canvas** ppsCanvas) override;
	virtual HRESULT STDMETHODCALLTYPE Resize3dHdw(DWORD dwRequestedFlags, DWORD dwWidth, DWORD dwHeight, DWORD dwBitDepth, DWORD* pdwCapabilityFlags, grs_canvas** ppsCanvas) override;

	virtual HRESULT STDMETHODCALLTYPE BlitToScreen(DWORD dwXScreen, DWORD dwYScreen, DWORD dwWidth, DWORD dwHeight) override;

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
	virtual BOOL STDMETHODCALLTYPE GetDirectDraw(IDirectDraw4** ppDD) override;
	virtual BOOL STDMETHODCALLTYPE GetRenderSurface(IDirectDrawSurface4** ppRS) override;

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

static inline void WarnSufraceError(const char* what, DWORD hResult)
{
	Warning(("%s: error %d\n%s ", what, hResult, GetDDErrorMsg(hResult)));
}

static inline void RaiseSufraceError(const char* what, DWORD hResult)
{
	CriticalMsg3("%s: error %d\n%s ", what, hResult, GetDDErrorMsg(hResult));
}

IMPLEMENT_UNAGGREGATABLE2_SELF_DELETE(cLGSurface, ILGSurface, ILGDD4Surface);

cLGSurface::cLGSurface()
	: m_nFrameCount{ 0 }, m_lpDD4{ nullptr }, m_lpScreen{ nullptr }, m_lpTheSurface{ nullptr }, m_nD3DDevID{ -1 },
	m_eSurfState{ kLGSSUninitialized }, m_bZbuffer{ FALSE }, m_b3DAttached{ FALSE }, m_sDescription{}, m_sCanvas{}
{
}

cLGSurface::~cLGSurface()
{
	ReleaseContent();
}

void cLGSurface::ReleaseContent()
{
	if (m_b3DAttached)
		lgd3d_shutdown();

	SafeRelease(m_lpTheSurface);
	SafeRelease(m_lpScreen);
	SafeRelease(m_lpDD4);

	m_sDescription = {};
	m_sCanvas = {};
}

void cLGSurface::RecordSurfaceDescription(DDSURFACEDESC2* lpDDSD)
{
	m_sDescription = {};

	m_sDescription.dwWidth = lpDDSD->dwWidth;
	m_sDescription.dwHeight = lpDDSD->dwHeight;
	m_sDescription.dwPitch = lpDDSD->dwLinearSize;
	m_sDescription.dwBitDepth = lpDDSD->ddpfPixelFormat.dwRGBBitCount;
	m_sDescription.sBitmask.red = lpDDSD->ddpfPixelFormat.dwRBitMask;
	m_sDescription.sBitmask.green = lpDDSD->ddpfPixelFormat.dwGBitMask;
	m_sDescription.sBitmask.blue = lpDDSD->ddpfPixelFormat.dwBBitMask;
	m_sDescription.sBitmask.alpha = lpDDSD->ddpfPixelFormat.dwRGBAlphaBitMask;
}

void cLGSurface::FixThePitchInCanvas()
{
	m_sCanvas.bm.row = m_sDescription.dwPitch;
}

void cLGSurface::SetBitsInCanvas(void* pvData)
{
	m_sCanvas.bm.bits = static_cast<uchar*>(pvData);
}

HRESULT STDMETHODCALLTYPE cLGSurface::CreateInternalScreenSurface(HWND hMainWindow)
{
	if (m_eSurfState != eLGSurfState::kLGSSUninitialized)
		return LGSERR_NOTINITIALIZED;

	if (!LoadDirectDraw())
		return LGSERR_GENERIC;

	IDirectDraw* lpDD = nullptr;
	auto hResult = DynDirectDrawCreate(nullptr, &lpDD, nullptr);
	if (hResult)
	{
		RaiseSufraceError("DirectDrawCreation", hResult);
		return LGSERR_GENERIC;
	}

	hResult = lpDD->QueryInterface(IID_IDirectDraw4, reinterpret_cast<void**>(&m_lpDD4));
	if (hResult)
	{
		RaiseSufraceError("QueryInterface for DD4", hResult);

		SafeRelease(lpDD);
		return LGSERR_GENERIC;
	}

	hResult = m_lpDD4->SetCooperativeLevel(nullptr, DDSCL_NORMAL);
	if (hResult)
	{
		RaiseSufraceError("SetCooperativeLevel for DD4", hResult);

		SafeRelease(lpDD);
		SafeRelease(m_lpDD4);
		return LGSERR_GENERIC;
	}

	DDSURFACEDESC2 sDDSurfDesc = {};
	sDDSurfDesc.dwSize = sizeof(sDDSurfDesc);
	sDDSurfDesc.dwFlags = DDSD_CAPS;
	sDDSurfDesc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

	hResult = m_lpDD4->CreateSurface(&sDDSurfDesc, &m_lpScreen, nullptr);
	if (hResult)
	{
		RaiseSufraceError("CreateSurface: screen", hResult);

		SafeRelease(lpDD);
		SafeRelease(m_lpDD4);
		return LGSERR_GENERIC;
	}

	ChangeClipper(hMainWindow);
	SafeRelease(lpDD);
	m_eSurfState = eLGSurfState::kLGSSInitialized;

	return S_OK;
}

HRESULT STDMETHODCALLTYPE cLGSurface::ChangeClipper(HWND hMainWindow)
{
	IDirectDrawClipper* pIClipper = nullptr;
	auto hResult = m_lpScreen->GetClipper(&pIClipper);
	if (pIClipper)
	{
		HWND hClipWindow = nullptr;
		pIClipper->GetHWnd(&hClipWindow);
		if (hClipWindow != nullptr && hClipWindow == hMainWindow)
			return S_OK;

		m_lpScreen->SetClipper(nullptr);
		SafeRelease(pIClipper);
	}

	if (!hMainWindow)
		return S_OK;

	hResult = m_lpDD4->CreateClipper(0, &pIClipper, nullptr);
	if (hResult)
	{
		RaiseSufraceError("CreateClipper", hResult);
		SafeRelease(pIClipper);

		return LGSERR_GENERIC;
	}

	hResult = pIClipper->SetHWnd(0, hMainWindow);
	if (hResult)
	{
		RaiseSufraceError("SetHWnd(clipper)", hResult);
		SafeRelease(pIClipper);

		return LGSERR_GENERIC;
	}

	hResult = m_lpScreen->SetClipper(pIClipper);
	if (hResult)
	{
		RaiseSufraceError("SetClipper", hResult);
		SafeRelease(pIClipper);

		return LGSERR_GENERIC;
	}

	SafeRelease(pIClipper);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE cLGSurface::SetAs3dHdwTarget(DWORD dwRequestedFlags, DWORD dwWidth, DWORD dwHeight, DWORD dwBitDepth, DWORD* pdwCapabilityFlags, grs_canvas** ppsCanvas)
{
	if (lgd3d_g_bInitialized)
		return LGSERR_D3DNOTINITIALIZED;

	*pdwCapabilityFlags = 0;
	GUID* pCurGuid = nullptr;

	if (m_eSurfState == eLGSurfState::kLGSSUninitialized)
	{
		auto* pIWinDispDevice = AppGetObj(IWinDisplayDevice);
		pIWinDispDevice->GetDirectDraw(&m_lpDD4);
		if (!m_lpDD4)
			return LGSERR_DDNOTINITIALIZED;

		auto bReturn = pIWinDispDevice->GetBitmapSurface(nullptr, &m_lpScreen);
		SafeRelease(pIWinDispDevice);

		if (!bReturn)
		{
			ReleaseContent();
			return LGSERR_DDNOTINITIALIZED;
		}

		if (!m_lpScreen)
			CriticalMsg("No render surface available.");

		auto* pIDisplayDevice = AppGetObj(IDisplayDevice);
		pIDisplayDevice->GetKind2(nullptr, nullptr, &pCurGuid);
		SafeRelease(pIDisplayDevice);
	}

	m_nD3DDevID = -1;
	auto nNoDevices = lgd3d_enumerate_devices();
	for (int i = 0; i < nNoDevices; ++i)
	{
		auto* pDevGuid = lgd3d_get_device_info(i)->p_ddraw_guid;

		if (pCurGuid && pDevGuid && IsEqualGUID(*pCurGuid, *pDevGuid))
		{
			m_nD3DDevID = i;
			break;
		}

		if (pCurGuid == nullptr && pDevGuid == nullptr)
		{
			m_nD3DDevID = i;
			break;
		}
	}

	if (m_nD3DDevID == -1)
		return LGSERR_DDNOTINITIALIZED;

	auto* pD3DInfo = lgd3d_get_device_info(m_nD3DDevID);
	pD3DInfo->flags |= dwRequestedFlags;

	DDSURFACEDESC2 sSurfaceDesc = {};
	sSurfaceDesc.dwSize = sizeof(sSurfaceDesc);
	sSurfaceDesc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
	sSurfaceDesc.ddsCaps.dwCaps = DDSCAPS_LOCALVIDMEM | DDSCAPS_VIDEOMEMORY | DDSCAPS_3DDEVICE | DDSCAPS_OFFSCREENPLAIN;
	sSurfaceDesc.dwWidth = dwWidth;
	sSurfaceDesc.dwHeight = dwHeight;

	auto hResult = m_lpDD4->CreateSurface(&sSurfaceDesc, &m_lpTheSurface, nullptr);
	if (hResult)
	{
		cLGSurface::ReleaseContent();
		WarnSufraceError("cLGSurface::CreateSurface", hResult);

		return hResult == DDERR_OUTOFVIDEOMEMORY ? 0x80040406 : LGSERR_GENERIC;
	}

	sSurfaceDesc = {};
	sSurfaceDesc.dwSize = sizeof(sSurfaceDesc);

	hResult = m_lpTheSurface->GetSurfaceDesc(&sSurfaceDesc);
	if (hResult)
	{
		RaiseSufraceError("cLGSurface::GetSurfaceDesc", hResult);
		return LGSERR_GENERIC;
	}

	RecordSurfaceDescription(&sSurfaceDesc);
	if (!lgd3d_attach_to_lgsurface(this))
	{
		ReleaseContent();
		return LGSERR_GENERIC;
	}

	m_b3DAttached = TRUE;
	*pdwCapabilityFlags = pD3DInfo->flags;
	m_bZbuffer = *pdwCapabilityFlags & (LGD3DF_ZBUFFER | LGD3DF_WBUFFER);
	gr_init_canvas(&m_sCanvas, nullptr, BMT_FLAT16, dwWidth, dwHeight);

	FixThePitchInCanvas();
	SetBitsInCanvas(nullptr);
	*ppsCanvas = &m_sCanvas;
	m_eSurfState = eLGSurfState::kLGSSUnlocked;

	return S_OK;
}

HRESULT STDMETHODCALLTYPE cLGSurface::Resize3dHdw(DWORD dwRequestedFlags, DWORD dwWidth, DWORD dwHeight, DWORD dwBitDepth, DWORD* pdwCapabilityFlags, grs_canvas** ppsCanvas)
{
	ReleaseContent();

	return SetAs3dHdwTarget(dwRequestedFlags, dwWidth, dwHeight, dwBitDepth, pdwCapabilityFlags, ppsCanvas);
}

HRESULT STDMETHODCALLTYPE cLGSurface::BlitToScreen(DWORD dwXScreen, DWORD dwYScreen, DWORD dwWidth, DWORD dwHeight)
{
	if (!m_b3DAttached)
		CriticalMsg("cLGSurface::BlitToScreen 3dhardware not attached");

	if (m_eSurfState != eLGSurfState::kLGSSUnlocked)
		return LGSERR_SURFACELOCKED;

	if (dwHeight > m_sDescription.dwHeight || dwWidth > m_sDescription.dwWidth)
		return LGSERR_INVALIDDESC;

	RECT sRect = {};
	sRect.left = 0;
	sRect.top = 0;
	sRect.right = dwWidth;
	sRect.bottom = dwHeight;

	RECT dRect = {};
	dRect.left = dwXScreen;
	dRect.top = dwYScreen;
	dRect.right = dwWidth + dwXScreen;
	dRect.bottom = dwHeight + dwYScreen;

	auto* pIDisplayDevice = AppGetObj(IDisplayDevice);
	auto nLockCount = pIDisplayDevice->BreakLock();

	auto hResult = m_lpScreen->Blt(&dRect, m_lpTheSurface, &sRect, DDBLT_WAIT, nullptr);
	if (hResult)
	{
		WarnSufraceError("cLGSurface::BlitToScreen", hResult);
		return LGSERR_GENERIC;
	}

	pIDisplayDevice->RestoreLock(nLockCount);
	SafeRelease(pIDisplayDevice);

	return S_OK;
}

void STDMETHODCALLTYPE cLGSurface::CleanSurface()
{
	if (!m_b3DAttached)
		CriticalMsg("cLGSurface::CleanSurface 3dhardware not attached");

	if (m_eSurfState != eLGSurfState::kLGSSUnlocked)
		CriticalMsg("cLGSurface::CleanSurface surface was locked!");

	lgd3d_clean_render_surface(m_bZbuffer);
}

void STDMETHODCALLTYPE cLGSurface::Start3D()
{
	if (!m_b3DAttached)
		CriticalMsg("cLGSurface::Start3D 3dhardware not attached");

	if (m_eSurfState != eLGSurfState::kLGSSUnlocked)
		CriticalMsg("cLGSurface::Start3D surface was locked!");

	lgd3d_start_frame(m_nFrameCount++);

	m_eSurfState = eLGSurfState::kLGSS3DRenderining;
}

void STDMETHODCALLTYPE cLGSurface::End3D()
{
	if (!m_b3DAttached)
		CriticalMsg("cLGSurface::End3D 3dhardware not attached");

	if (m_eSurfState != kLGSS3DRenderining)
		CriticalMsg("cLGSurface::End3D surface was not in 3d state!");

	lgd3d_end_frame();

	m_eSurfState = eLGSurfState::kLGSSUnlocked;
}

HRESULT STDMETHODCALLTYPE cLGSurface::LockFor2D(grs_canvas** ppsCanvas, eLGSBlit eBlitFlags)
{
	if (m_eSurfState != eLGSurfState::kLGSSUnlocked)
		return LGSERR_SURFACELOCKED;

	DWORD dwFlags = DDLOCK_NOSYSLOCK | DDLOCK_WAIT;
	if (eBlitFlags & eLGSBlit::kLGSBRead)
		dwFlags |= DDLOCK_READONLY;
	else if (eBlitFlags & eLGSBlit::kLGSBWrite)
		dwFlags |= DDLOCK_WRITEONLY;

	DDSURFACEDESC2 sDDSD = {};
	sDDSD.dwSize = sizeof(sDDSD);

	auto hResult = m_lpTheSurface->Lock(nullptr, &sDDSD, dwFlags, nullptr);
	if (hResult)
	{
		WarnSufraceError("cLGSurface::LockFor2D", hResult);
		return LGSERR_GENERIC;
	}

	m_eSurfState = eLGSurfState::kLGSS2DLocked;
	RecordSurfaceDescription(&sDDSD);
	FixThePitchInCanvas();
	SetBitsInCanvas(sDDSD.lpSurface);

	*ppsCanvas = &m_sCanvas;

	return S_OK;
}

HRESULT STDMETHODCALLTYPE cLGSurface::UnlockFor2D()
{
	if (m_eSurfState != eLGSurfState::kLGSS2DLocked)
		return LGSERR_SURFACEUNLOCKED;

	auto hResult = m_lpTheSurface->Unlock(nullptr);
	if (hResult)
	{
		WarnSufraceError("cLGSurface::UnlockFor2D", hResult);
		return hResult == DDERR_NOTLOCKED ? LGSERR_SURFACEUNLOCKED : LGSERR_GENERIC;
	}

	m_eSurfState = eLGSurfState::kLGSSUnlocked;
	SetBitsInCanvas(nullptr);

	return S_OK;
}

DWORD STDMETHODCALLTYPE cLGSurface::GetWidth()
{
	return m_sDescription.dwWidth;
}

DWORD STDMETHODCALLTYPE cLGSurface::GetHeight()
{
	return m_sDescription.dwHeight;
}

DWORD STDMETHODCALLTYPE cLGSurface::GetPitch()
{
	return m_sDescription.dwPitch;
}

DWORD STDMETHODCALLTYPE cLGSurface::GetBitDepth()
{
	return m_sDescription.dwBitDepth;
}

eLGSurfState STDMETHODCALLTYPE cLGSurface::GetSurfaceState()
{
	return m_eSurfState;
}

HRESULT STDMETHODCALLTYPE cLGSurface::GetSurfaceDescription(sLGSurfaceDescription* psDsc)
{
	if (!psDsc)
		return LGSERR_INVALIDDESC;

	*psDsc = m_sDescription;

	return S_OK;
}

int STDMETHODCALLTYPE cLGSurface::GetDeviceInfoIndex()
{
	return m_nD3DDevID;
}

int STDMETHODCALLTYPE cLGSurface::GetDirectDraw(IDirectDraw4** ppDD)
{
	*ppDD = m_lpDD4;
	if (!m_lpDD4)
		return FALSE;

	m_lpDD4->AddRef();

	return TRUE;
}

BOOL STDMETHODCALLTYPE cLGSurface::GetRenderSurface(IDirectDrawSurface4** ppRS)
{
	*ppRS = m_lpScreen;
	if (!m_lpScreen)
		return FALSE;

	m_lpScreen->AddRef();

	return TRUE;
}

BOOL CreateLGSurface(ILGSurface** ppILGSurf)
{
	*ppILGSurf = new cLGSurface{};
	return *ppILGSurf != nullptr;
}