#pragma once

#include <lgd3d.h>
#include <comtools.h>
#include <wdispapi.h>

class cD6Frame
{
public:
	cD6Frame(ILGSurface* pILGSurface);
	cD6Frame(DWORD dwWidth, DWORD dwHeight, lgd3ds_device_info* psDeviceInfo);
	~cD6Frame();

private:
	void InitializeGlobals(DWORD dwWidth, DWORD dwHeight, DWORD dwRequestedFlags);
	void InitializeEnvironment(lgd3ds_device_info* psDeviceInfo);
	HRESULT GetDDstuffFromDisplay();
	HRESULT CreateDepthBuffer();
	int CreateD3D(const GUID& sDeviceGUID);
	void ExamineRenderingCapabilities();

private:
	DWORD m_dwRequestedFlags;
	bool m_bDepthBuffer;
	DWORD m_dwTextureOpCaps;
	IWinDisplayDevice* m_pWinDisplayDevice;
};
