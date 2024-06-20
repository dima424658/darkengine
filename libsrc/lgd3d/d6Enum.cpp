#include <types.h>
#include <ddraw.h>
#include <d3dcaps.h>
#include <d3d.h>

IDirect3DDevice3* g_lpD3Ddevice;
int bSpewOn;

int lgd3d_enumerate_devices();
int lgd3d_enumerate_devices_capable_of(unsigned long flags);
void lgd3d_unenumerate_devices();
lgd3ds_device_info *lgd3d_get_device_info(int device_number);

int c_DDEnumCallback(GUID *lpGUID, const char *lpDriverDescription, const char *lpDriverName, LGD3D_sEnumerationInfo *lpContext);
void GetDevices(LGD3D_sEnumerationInfo *info);
int c_EnumDisplayModesCallback(DDSURFACEDESC2 *pSD, char *data);
int c_EnumDevicesCallback(GUID *lpGuid, const char *lpDeviceDescription, const char *lpDeviceName, D3DDEVICEDESC *pDeviceDesc, D3DDEVICEDESC *lpD3DHELDeviceDesc, void *lpUserArg);
