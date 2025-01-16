
#include <d3d.h>
#include <d3dcaps.h>
#include <ddraw.h>
#include <types.h>

#include <comtools.h>
#include <dbg.h>
#include <lgd3d.h>
#include <lgassert.h>
#include <dddynf.h>
#include <mprintf.h>
#include <emode.h>
#include <mode.h>

static bool bWBuffer;
static int nNoDevices = -2;
#define MAX_DEVICE_NUMBER 10
static lgd3ds_device_info* psDeviceList[MAX_DEVICE_NUMBER];
static bool bNoD3d = true;
static bool bDepthBuffer = false;
static bool bWindowed = false;

struct LGD3D_sEnumerationInfo
{
	GUID* p_ddraw_guid;
	int device_number;
	char* p_ddraw_desc;
	uint16 supported_modes[GRD_MODES];
	int num_supported;
	DWORD requested_flags;
};


BOOL CALLBACK c_DDEnumCallback(GUID* lpGUID, LPSTR lpDriverDescription, LPSTR lpDriverName, LPVOID lpContext);
HRESULT CALLBACK c_EnumDisplayModesCallback(LPDDSURFACEDESC2 pSD, LPVOID data);
HRESULT CALLBACK c_EnumDevicesCallback(GUID* lpGuid, LPSTR lpDeviceDescription, LPSTR lpDeviceName, LPD3DDEVICEDESC pDeviceDesc, LPD3DDEVICEDESC lpD3DHELDeviceDesc, LPVOID lpUserArg);

int lgd3d_enumerate_devices()
{
	return lgd3d_enumerate_devices_capable_of(0);
}

int lgd3d_enumerate_devices_capable_of(unsigned long flags)
{
	LGD3D_sEnumerationInfo sDEinfo = {};

	if (nNoDevices >= -1)
		return nNoDevices;

	if (!LoadDirectDraw())
	{
		nNoDevices = -1;
		return nNoDevices;
	}

	sDEinfo.device_number = 0;
	sDEinfo.requested_flags = flags;
	DynDirectDrawEnumerate(c_DDEnumCallback, &sDEinfo);
	if (!bNoD3d || sDEinfo.device_number)
		nNoDevices = sDEinfo.device_number;
	else
		nNoDevices = -1;

	return nNoDevices;
}

void lgd3d_unenumerate_devices()
{
	for (int i = 0; i < nNoDevices; ++i)
	{
		delete psDeviceList[i]; // FIXME char[]
		psDeviceList[i] = nullptr;
	}

	nNoDevices = -2;
}

lgd3ds_device_info* lgd3d_get_device_info(int device_number)
{
	if (nNoDevices <= device_number)
		CriticalMsg1("Invalid device number: %d", device_number);

	return psDeviceList[device_number];
}

void GetDevices(LGD3D_sEnumerationInfo* info)
{
	GUID* p_ddraw_guid = info->p_ddraw_guid;
	IDirectDraw* lpdd = nullptr;
	if (FAILED(DynDirectDrawCreate(p_ddraw_guid, &lpdd, nullptr)))
	{
		Warning(("Can't create ddraw object.\n"));
		return;
	}

	IDirectDraw4* lpdd4 = nullptr;
	if (lpdd->QueryInterface(IID_IDirectDraw4, reinterpret_cast<void**>(&lpdd4)) != S_OK)
	{
		Warning(("Can't obtain DDraw4 interface.\n"));
		SafeRelease(lpdd);
		return;
	}

	DDCAPS ddcaps = {};
	ddcaps.dwSize = sizeof(ddcaps);
	if (lpdd4->GetCaps(&ddcaps, nullptr) != DD_OK || (ddcaps.dwCaps & DDCAPS_3D) == 0)
	{
		Warning(("Not a 3d accelerator.\n"));
		SafeRelease(lpdd4);
		SafeRelease(lpdd);
		return;
	}

	bDepthBuffer = ddcaps.ddsCaps.dwCaps & DDSCAPS_3DDEVICE;
	bWindowed = ddcaps.dwCaps2 & DDCAPS2_CANRENDERWINDOWED;

	IDirect3D3* lpd3d = nullptr;
	if (lpdd4->QueryInterface(IID_IDirect3D3, reinterpret_cast<void**>(&lpd3d)) != S_OK)
	{
		Warning(("Can't obtain D3D interface.\n"));
		SafeRelease(lpdd4);
		SafeRelease(lpdd);
		return;
	}

	bNoD3d = false;
	info->num_supported = 0;
	lpdd4->EnumDisplayModes(0, nullptr, info, c_EnumDisplayModesCallback);
	if (info->num_supported <= 0)
	{
		Warning(("Couldn't find any display modes.\n"));
	}
	else if (lpd3d->EnumDevices(c_EnumDevicesCallback, info))
	{
		Warning(("EnumDevices failed.\n"));
	}

	SafeRelease(lpd3d);
	SafeRelease(lpdd4);
	SafeRelease(lpdd);
}

BOOL CALLBACK c_DDEnumCallback(GUID* lpGUID, LPSTR lpDriverDescription, LPSTR lpDriverName, LPVOID lpContext)
{
	reinterpret_cast<LGD3D_sEnumerationInfo*>(lpContext)->p_ddraw_guid = lpGUID;
	reinterpret_cast<LGD3D_sEnumerationInfo*>(lpContext)->p_ddraw_desc = lpDriverDescription;
	GetDevices(reinterpret_cast<LGD3D_sEnumerationInfo*>(lpContext));

	return TRUE;
}

HRESULT CALLBACK c_EnumDisplayModesCallback(LPDDSURFACEDESC2 pSD, LPVOID data)
{
	auto* info = reinterpret_cast<LGD3D_sEnumerationInfo*>(data);

	auto mode = gr_mode_from_info(pSD->dwWidth, pSD->dwHeight, pSD->ddpfPixelFormat.dwRGBBitCount);
	if (mode < 0)
		return 1;

	for (int i = 0; i < info->num_supported; ++i)
		if (info->supported_modes[i] == mode)
			return 1;

	info->supported_modes[info->num_supported++] = mode;

	return 1;
}

HRESULT CALLBACK c_EnumDevicesCallback(GUID* lpGuid, LPSTR lpDeviceDescription, LPSTR lpDeviceName, LPD3DDEVICEDESC pDeviceDesc, LPD3DDEVICEDESC lpD3DHELDeviceDesc, LPVOID lpUserArg)
{
	auto* info = reinterpret_cast<LGD3D_sEnumerationInfo*>(lpUserArg);

	if (!pDeviceDesc->dcmColorModel)
		return 1;

	DWORD capabilities_flags = 0;
	if ((pDeviceDesc->dwDeviceRenderBitDepth & DDBD_16) == 0)
	{
		mprintf("No 16 bit.\n");
		return 1;
	}

	if ((pDeviceDesc->dcmColorModel & D3DCOLOR_RGB) == 0)
	{
		mprintf("device not RGB.\n");
		return 1;
	}

	if ((pDeviceDesc->dpcTriCaps.dwShadeCaps & D3DPSHADECAPS_COLORGOURAUDRGB) == 0)
	{
		mprintf("no color gouraud shading.\n");
		return 1;
	}

	if ((pDeviceDesc->dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_ALPHA) == 0)
	{
		mprintf("no alpha blending.\n");
		return 1;
	}

	if ((pDeviceDesc->dwDevCaps & (D3DDEVCAPS_TEXTURENONLOCALVIDMEM | D3DDEVCAPS_TEXTUREVIDEOMEMORY | D3DDEVCAPS_TEXTURESYSTEMMEMORY)) == 0)
	{
		mprintf("no texture mapping.\n");
		return 1;
	}

	if (pDeviceDesc->dpcTriCaps.dwRasterCaps & D3DPRASTERCAPS_ZBUFFERLESSHSR)
	{
		pDeviceDesc->dpcTriCaps.dwRasterCaps &= ~D3DPRASTERCAPS_DITHER;
	}

	if (pDeviceDesc->dpcTriCaps.dwRasterCaps & D3DPRASTERCAPS_FOGTABLE)
	{
		capabilities_flags = LGD3DF_CAN_DO_TABLE_FOG;
	}
	else if (info->requested_flags & LGD3DF_TABLE_FOG)
	{
		mprintf("no table fog\n");
		return 1;
	}

	if (pDeviceDesc->dpcTriCaps.dwRasterCaps & D3DPRASTERCAPS_FOGVERTEX)
	{
		capabilities_flags |= LGD3DF_CAN_DO_VERTEX_FOG;
	}
	else if (info->requested_flags & LGD3DF_VERTEX_FOG)
	{
		mprintf("no vertex fog\n");
		return 1;
	}

	if (pDeviceDesc->dpcTriCaps.dwShadeCaps & D3DPRASTERCAPS_ZBIAS)
		capabilities_flags |= LGD3DF_CAN_DO_ITERATE_ALPHA;

	if (bDepthBuffer)
	{
		capabilities_flags |= LGD3DF_CAN_DO_ZBUFFER;
		if (pDeviceDesc->dpcTriCaps.dwRasterCaps & D3DPRASTERCAPS_WBUFFER)
			capabilities_flags |= LGD3DF_CAN_DO_WBUFFER;
	}

	if (bWindowed)
		capabilities_flags |= LGD3DF_CAN_DO_WINDOWED;

	if (pDeviceDesc->wMaxTextureBlendStages >= 2 && pDeviceDesc->wMaxSimultaneousTextures >= 2 && pDeviceDesc->dwFVFCaps >= 2)
		capabilities_flags |= LGD3DF_CAN_DO_SINGLE_PASS_MT;

	if (pDeviceDesc->dpcTriCaps.dwRasterCaps & D3DPRASTERCAPS_DITHER)
		capabilities_flags |= LGD3DF_CAN_DO_SINGLE_PASS_MT;

	if (pDeviceDesc->dpcTriCaps.dwRasterCaps & D3DPRASTERCAPS_ANTIALIASSORTINDEPENDENT)
		capabilities_flags |= LGD3DF_CAN_DO_ANTIALIAS;

	AssertMsg(info->device_number < MAX_DEVICE_NUMBER, "Too many d3d devices available.");

	lgd3ds_device_info* device_info;
	auto malloc_size = strlen(info->p_ddraw_desc) + sizeof(short) * info->num_supported + 291;
	if (info->p_ddraw_guid)
	{
		device_info = reinterpret_cast<lgd3ds_device_info*>(new char[malloc_size + sizeof(GUID)]);
		device_info->p_ddraw_guid = reinterpret_cast<GUID*>((reinterpret_cast<char*>(device_info) + sizeof(malloc_size)));
		memcpy(device_info->p_ddraw_guid, info->p_ddraw_guid, sizeof(GUID));
	}
	else
	{
		device_info = reinterpret_cast<lgd3ds_device_info*>(new char[malloc_size]);
		device_info->p_ddraw_guid = nullptr;
	}

	memcpy(&device_info->device_guid, lpGuid, sizeof(GUID));

	device_info->supported_modes = (short*)&device_info[1];
	if (info->num_supported > 0)
		memcpy(device_info->supported_modes, info->supported_modes, 2 * info->num_supported);
	device_info->supported_modes[info->num_supported] = -1;

	device_info->device_desc = (DevDesc*)&device_info->supported_modes[info->num_supported + 1];
	memcpy(device_info->device_desc, pDeviceDesc, sizeof(DevDesc));

	device_info->p_ddraw_desc = (char*)&device_info->device_desc[1];
	strcpy(device_info->p_ddraw_desc, info->p_ddraw_desc);

	device_info->flags = capabilities_flags;

	psDeviceList[info->device_number++] = device_info;

	return 1;
}
