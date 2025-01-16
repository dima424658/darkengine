#include <d6Frame.h>

#include <types.h>
#include <lgSurf_i.h>
#include <lgassert.h>
#include <ddraw.h>
#include <dbg.h>
#include <d3d.h>
#include <appagg.h>
#include <d6States.h>
#include <d6Render.h>

IDirectDraw4* g_lpDD_ext = nullptr;
IDirectDrawSurface4* g_lpRenderBuffer = nullptr;
IDirectDrawSurface4* g_lpDepthBuffer = nullptr;
IDirect3DDevice3* g_lpD3Ddevice = nullptr;

extern BOOL lgd3d_g_bInitialized;
BOOL bSpewOn;
int g_bWFog;

DWORD g_dwScreenWidth;
DWORD g_dwScreenHeight;
D3DMATERIALHANDLE g_hBackgroundMaterial;
extern DDPIXELFORMAT g_RGBTextureFormat;
DDSURFACEDESC2 g_sDescOfRenderBuffer;
D3DDEVICEDESC g_sD3DDevDesc;
DDPIXELFORMAT g_sDDPFDepth;
extern DDPIXELFORMAT* g_FormatList[5];

float g_XOffset;
float g_YOffset;
extern BOOL g_bUseDepthBuffer, g_bUseTableFog, g_bUseVertexFog;

cD6Renderer* pcRenderer;
IDirect3D3* g_lpD3D;
IDirect3DViewport3* g_lpViewport;
IDirect3DMaterial3* g_lpBackgroundMaterial;
extern BOOL g_b8888supported;

HRESULT CALLBACK c_EnumZBufferFormats(LPDDPIXELFORMAT lpDDPixFmt, LPVOID lpContext);

static inline void RaiseLGD3DErrorCode(DWORD dwCode, DWORD hResult)
{
	SetLGD3DErrorCode(dwCode, hResult);
	if (bSpewOn)
		CriticalMsg4("LGD3D error no %d : %s : message: %d\n%s ", dwCode, GetLgd3dErrorCode(dwCode), hResult, GetDDErrorMsg(hResult));
	else
		DbgReportWarning("LGD3D error no %d : %s : message: %d\n%s ", dwCode, GetLgd3dErrorCode(dwCode), hResult, GetDDErrorMsg(hResult));

	lgd3d_g_bInitialized = false;
}

cD6Frame::cD6Frame(ILGSurface* pILGSurface)
{
	auto dwWidth = pILGSurface->GetWidth();
	auto dwHeight = pILGSurface->GetHeight();

	ILGDD4Surface* pIlgdd4surface = nullptr;
	if (pILGSurface->QueryInterface(IID_ILGDD4Surface, reinterpret_cast<void**>(&pIlgdd4surface)) < 0)
		CriticalMsg("Failed to query ILGDD4Surface interface");

	auto nDeviceIndex = pIlgdd4surface->GetDeviceInfoIndex();
	auto* psDeviceInfo = lgd3d_get_device_info(nDeviceIndex);
	InitializeGlobals(dwWidth, dwHeight, psDeviceInfo->flags);

	pIlgdd4surface->GetDirectDraw(&g_lpDD_ext);
	if (!g_lpDD_ext)
		CriticalMsg("cD6Frame: GetDirectDraw failed");

	pIlgdd4surface->GetRenderSurface(&g_lpRenderBuffer);
	if (!g_lpRenderBuffer)
		CriticalMsg("cD6Frame: GetRenderSurface failed");

	SafeRelease(pIlgdd4surface);

	InitializeEnvironment(psDeviceInfo);
}

cD6Frame::cD6Frame(DWORD dwWidth, DWORD dwHeight, lgd3ds_device_info* psDeviceInfo)
{
	InitializeGlobals(dwWidth, dwHeight, psDeviceInfo->flags);
	if (FAILED(GetDDstuffFromDisplay()))
	{
		RaiseLGD3DErrorCode(LGD3D_EC_DD_KAPUT, S_OK);
		return;
	}

	InitializeEnvironment(psDeviceInfo);
}

cD6Frame::~cD6Frame()
{
	pcRenderer = cD6Renderer::DeInstance();

	SafeRelease(g_lpBackgroundMaterial);
	SafeRelease(g_lpViewport);
	SafeRelease(g_lpD3Ddevice);

	g_lpRenderBuffer->DeleteAttachedSurface(0, g_lpDepthBuffer);
	SafeRelease(g_lpRenderBuffer);

	SafeRelease(g_lpDepthBuffer);
	SafeRelease(g_lpD3D);
	SafeRelease(g_lpDD_ext);
	SafeRelease(m_pWinDisplayDevice);
}

void cD6Frame::InitializeGlobals(DWORD dwWidth, DWORD dwHeight, DWORD dwRequestedFlags)
{
	g_dwScreenWidth = dwWidth;
	g_dwScreenHeight = dwHeight;

	m_pWinDisplayDevice = nullptr;
	g_XOffset = 0.0;
	g_YOffset = 0.0;

	g_lpDD_ext = nullptr;
	g_lpRenderBuffer = nullptr;
	g_lpDepthBuffer = nullptr;
	g_lpD3D = nullptr;
	g_lpD3Ddevice = nullptr;
	g_lpViewport = nullptr;
	g_lpBackgroundMaterial = nullptr;
	g_hBackgroundMaterial = 0;
	g_b8888supported = false;
	g_bUseDepthBuffer = dwRequestedFlags & 0x5;
	bSpewOn = dwRequestedFlags & 0x2;
	g_bUseTableFog = false;
	g_bUseVertexFog = false;

	if (dwRequestedFlags & 0x10)
		g_bUseTableFog = dwRequestedFlags & 0x80000;

	if (dwRequestedFlags & 0x20)
		g_bUseVertexFog = dwRequestedFlags & 0x100000;
}

void cD6Frame::InitializeEnvironment(lgd3ds_device_info* psDeviceInfo)
{
	if (g_lpRenderBuffer->IsLost() == -2005532222)
	{
		auto hResult = g_lpDD_ext->RestoreAllSurfaces();
		if (FAILED(hResult))
		{
			RaiseLGD3DErrorCode(LGD3D_EC_RESTORE_ALL_SURFS, hResult);
			return;
		}
	}

	auto hResult = g_lpDD_ext->QueryInterface(IID_IDirect3D3, reinterpret_cast<void**>(&g_lpD3D));
	if (FAILED(hResult))
	{
		RaiseLGD3DErrorCode(LGD3D_EC_QUERY_D3D, hResult);
		return;
	}

	DDCAPS sDDChdw = {};
	sDDChdw.dwSize = sizeof(sDDChdw);

	DDCAPS sDDChel = {};
	sDDChel.dwSize = sizeof(sDDChel);

	hResult = g_lpDD_ext->GetCaps(&sDDChdw, &sDDChel);
	if (hResult)
	{
		RaiseLGD3DErrorCode(LGD3D_EC_GET_DD_CAPS, hResult);
		return;
	}

	if ((sDDChdw.dwCaps & DDCAPS_3D) == 0)
		CriticalMsg("Not an accelerator.");

	memset(&g_sDescOfRenderBuffer, 0, sizeof(g_sDescOfRenderBuffer));
	g_sDescOfRenderBuffer.dwSize = sizeof(DDSURFACEDESC2);

	hResult = g_lpRenderBuffer->GetSurfaceDesc(&g_sDescOfRenderBuffer);
	if (hResult)
	{
		RaiseLGD3DErrorCode(LGD3D_EC_GET_SURF_DESC, hResult);
		return;
	}
	
	if ((g_sDescOfRenderBuffer.dwFlags & DDSD_CAPS) == 0)
		Warning(("cD6Frame: flags indicate caps field not valid!\n"));

	if ((g_sDescOfRenderBuffer.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) != 0)
		Warning(("cD6Frame: Rendering to primary surface!\n"));

	m_bDepthBuffer = sDDChdw.ddsCaps.dwCaps & DDSCAPS_ZBUFFER;
	if (g_bUseDepthBuffer)
	{
		if (m_bDepthBuffer)
		{
			hResult = g_lpD3D->EnumZBufferFormats(psDeviceInfo->device_guid, c_EnumZBufferFormats, &g_sDDPFDepth);
			if (hResult)
			{
				RaiseLGD3DErrorCode(LGD3D_EC_ZBUFF_ENUMERATION, hResult);
				return;
			}

			if (CreateDepthBuffer())
			{
				Warning(("Could not create Zbuffer\n"));
				psDeviceInfo->flags &= ~1u;
				psDeviceInfo->flags &= ~4u;
				if (psDeviceInfo->flags & 8)
				{
					lgd3d_g_bInitialized = false;
					return;
				}

				g_bUseDepthBuffer = false;
			}
		}
		else
		{
			Warning(("Z-buffer not present\n"));
			g_bUseDepthBuffer = false;
			psDeviceInfo->flags &= ~1u;
			psDeviceInfo->flags &= ~4u;
			if (psDeviceInfo->flags & 8)
			{
				lgd3d_g_bInitialized = false;
				return;
			}
		}
	}

	if (CreateD3D(psDeviceInfo->device_guid))
	{
		m_dwRequestedFlags = psDeviceInfo->flags;
		ExamineRenderingCapabilities();
		psDeviceInfo->flags = m_dwRequestedFlags;
	}
}

HRESULT cD6Frame::GetDDstuffFromDisplay()
{
	m_pWinDisplayDevice = AppGetObj(IWinDisplayDevice);
	m_pWinDisplayDevice->GetDirectDraw(&g_lpDD_ext);
	if (!g_lpDD_ext)
		CriticalMsg("cD6Frame: GetDirectDraw failed");

	auto hResult = m_pWinDisplayDevice->GetBitmapSurface(nullptr, &g_lpRenderBuffer);
	if (FAILED(hResult))
		CriticalMsg("GetBitmapSurface failed.");

	if (!g_lpRenderBuffer)
		CriticalMsg("No render surface available.");

	return hResult;
}

HRESULT cD6Frame::CreateDepthBuffer()
{
	DDSURFACEDESC2 sDDSD;
	memcpy(&sDDSD, &g_sDescOfRenderBuffer, sizeof(sDDSD));
	sDDSD.dwFlags = 4103;
	sDDSD.ddsCaps.dwCaps = 147456;

	memcpy(&sDDSD.ddpfPixelFormat, &g_sDDPFDepth, sizeof(sDDSD.ddpfPixelFormat));

	auto hResult = g_lpDD_ext->CreateSurface(&sDDSD, &g_lpDepthBuffer, nullptr);
	if (FAILED(hResult))
	{
		Warning(("%s: error %d\n%s ", "Failed to create depth buffer\n", hResult, GetDDErrorMsg(hResult)));
		return hResult;
	}

	hResult = g_lpRenderBuffer->AddAttachedSurface(g_lpDepthBuffer);
	if (FAILED(hResult))
		Warning(("%s: error %d\n%s ", "Failed to attach depth buffer\n", hResult, GetDDErrorMsg(hResult)));

	return hResult;
}

int cD6Frame::CreateD3D(const GUID& sDeviceGUID)
{
	if (!g_lpDD_ext)
		CriticalMsg("cD6Frame::CreateD3D: DirectDraw Not Initialized.");

	if (!g_lpD3D)
		CriticalMsg("cD6Frame::CreateD3D: D3D Not Initialized.");

	auto hResult = g_lpD3D->CreateDevice(sDeviceGUID, g_lpRenderBuffer, &g_lpD3Ddevice, nullptr);
	if (hResult)
	{
		RaiseLGD3DErrorCode(LGD3D_EC_CREATE_3D_DEVICE, hResult);
		return 0;
	}

	hResult = g_lpD3D->CreateViewport(&g_lpViewport, nullptr);
	if (hResult)
	{
		RaiseLGD3DErrorCode(LGD3D_EC_CREATE_VIEWPORT, hResult);
		return 0;
	}

	hResult = g_lpD3Ddevice->AddViewport(g_lpViewport);
	if (hResult)
	{
		RaiseLGD3DErrorCode(LGD3D_EC_ADD_VIEWPORT, hResult);
		return 0;
	}

	D3DVIEWPORT2 vdData = {};
	vdData.dwSize = sizeof(vdData);
	vdData.dwWidth = g_dwScreenWidth;
	vdData.dwHeight = g_dwScreenHeight;
	vdData.dvClipX = -1.0;
	vdData.dvClipY = -1.0;
	vdData.dvClipWidth = 2.0;
	vdData.dvClipHeight = 2.0;
	vdData.dvMaxZ = 1.0;

	hResult = g_lpViewport->SetViewport2(&vdData);
	if (hResult)
	{
		RaiseLGD3DErrorCode(LGD3D_EC_SET_VIEWPORT, hResult);
		return 0;
	}

	hResult = g_lpD3Ddevice->SetCurrentViewport(g_lpViewport);
	if (hResult)
	{
		RaiseLGD3DErrorCode(LGD3D_EC_SET_CURR_VP, hResult);
		return 0;
	}

	hResult = g_lpD3D->CreateMaterial(&g_lpBackgroundMaterial, nullptr);
	if (hResult)
	{
		RaiseLGD3DErrorCode(LGD3D_EC_CREATE_BK_MATERIAL, hResult);
		return 0;
	}

	D3DMATERIAL sD3DMaterial = {};
	sD3DMaterial.dwSize = sizeof(sD3DMaterial);
	sD3DMaterial.dwRampSize = 1;

	hResult = g_lpBackgroundMaterial->SetMaterial(&sD3DMaterial);
	if (hResult)
	{
		RaiseLGD3DErrorCode(LGD3D_EC_SET_BK_MATERIAL, hResult);
		return 0;
	}

	hResult = g_lpBackgroundMaterial->GetHandle(g_lpD3Ddevice, &g_hBackgroundMaterial);
	if (hResult)
	{
		RaiseLGD3DErrorCode(LGD3D_EC_GET_BK_MAT_HANDLE, hResult);
		return 0;
	}

	return 1;
}

void cD6Frame::ExamineRenderingCapabilities()
{
	if (!g_lpDD_ext)
		CriticalMsg("cD6Frame::CreateD3D: DirectDraw Not Initialized.");
	if (!g_lpRenderBuffer)
		CriticalMsg("cD6Frame::CreateD3D: Render surface Not Initialized.");
	if (!g_lpD3D)
		CriticalMsg("cD6Frame::CreateD3D: D3D Not Initialized.");
	if (!g_lpD3Ddevice)
		CriticalMsg("cD6Frame::CreateD3D: D3D device Not Initialized.");

	g_sD3DDevDesc = {};
	g_sD3DDevDesc.dwSize = sizeof(g_sD3DDevDesc);

	DevDesc sD3DHELDevDesc = {};
	sD3DHELDevDesc.dwSize = sizeof(sD3DHELDevDesc);
	auto hResult = g_lpD3Ddevice->GetCaps(&g_sD3DDevDesc, &sD3DHELDevDesc);
	if (hResult)
	{
		RaiseLGD3DErrorCode(LGD3D_EC_GET_3D_CAPS, hResult);
		return;
	}

	if ((g_sD3DDevDesc.dwFlags & 2) == 0)
		CriticalMsg("Device Description: invalid caps");

	if ((g_sD3DDevDesc.dwFlags & 0x40) == 0)
		CriticalMsg("Device Description: invalid caps");

	g_bWFog = (g_sD3DDevDesc.dpcTriCaps.dwRasterCaps & 0x100000) != 0;
	if ((g_sD3DDevDesc.dpcTriCaps.dwRasterCaps & 0x40000) == 0)
		m_dwRequestedFlags &= ~4u;

	m_dwTextureOpCaps = g_sD3DDevDesc.dwTextureOpCaps;


	unsigned int dwLGMode = 0;
	if (m_dwRequestedFlags & 0x100)
	{
		if (g_sD3DDevDesc.wMaxTextureBlendStages < 2 || g_sD3DDevDesc.wMaxSimultaneousTextures < 2 || LOWORD(g_sD3DDevDesc.dwFVFCaps) < 2)
		{
			m_dwRequestedFlags &= ~1u;
		}
		else
		{
			dwLGMode = 1;
		}
	}

	pcRenderer = cD6Renderer::Instance(dwLGMode, &m_dwRequestedFlags);
}

HRESULT CALLBACK c_EnumZBufferFormats(LPDDPIXELFORMAT lpDDPixFmt, LPVOID lpContext)
{
	if (lpDDPixFmt->dwRGBBitCount != 16)
		return S_FALSE;

	memcpy(lpContext, lpDDPixFmt, sizeof(DDPIXELFORMAT));

	return D3D_OK;
}
