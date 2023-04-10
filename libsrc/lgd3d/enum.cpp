// $Header: x:/prj/tech/libsrc/lgd3d/RCS/enum.cpp 1.7 1998/02/16 15:08:00 KEVIN Exp $
#include <windows.h>
#include <ddraw.h>
#include <d3d.h>

#include <comtools.h>
#include <dddynf.h>
#include <lgassert.h>
#include <memall.h>
#include <dev2d.h>

#include <lgd3d.h>
#include <setup.h>

#ifndef SHIP
#include <mprintf.h>
#define spew(x) mprint(x)
#else
#define spew(x)
#pragma off(unreferenced)
#endif


#define MAX_DEVICES 10
#define TEXTURE_FLAGS (              \
   D3DDEVCAPS_TEXTURENONLOCALVIDMEM| \
   D3DDEVCAPS_TEXTURESYSTEMMEMORY|   \
   D3DDEVCAPS_TEXTUREVIDEOMEMORY)

typedef struct lgd3ds_device_enum_info {
   LPGUID   p_ddraw_guid;
   int      device_number;
   char *   p_ddraw_desc;
   short    supported_modes[GRD_MODES];
   int      num_supported;
} lgd3ds_device_enum_info;

static int num_devices = -2;
static lgd3ds_device_info *device_list[MAX_DEVICES];
static BOOL no_d3d = TRUE;

#pragma off (unreferenced)
static HRESULT WINAPI
EnumDeviceCallback(LPGUID       p_device_guid, 
                   LPSTR           description,
                   LPSTR           name,
                   LPD3DDEVICEDESC device_desc,
                   LPD3DDEVICEDESC ,
                   LPVOID          user_arg)
{
   lgd3ds_device_enum_info *info = (lgd3ds_device_enum_info *)user_arg;
   lgd3ds_device_info *device_info;

// If there is no hardware support then the color model is zero.

   if (0UL == device_desc->dcmColorModel)
      return D3DENUMRET_OK;

#ifdef PRINT_ENUM
   mprintf("Device Name: %s\n", name);
   mprintf("Device Description: %s\n", description);
#endif

// Does the device render at the depth we want?

   if (0UL == (device_desc->dwDeviceRenderBitDepth & DDBD_16)) {
      spew("No 16 bit.\n");
      return D3DENUMRET_OK;
   }

   if ((D3DCOLOR_RGB & device_desc->dcmColorModel)==0) {
      spew("device not RGB.\n");
      return D3DENUMRET_OK;
   }

   if ((device_desc->dpcTriCaps.dwShadeCaps & D3DPSHADECAPS_COLORGOURAUDRGB)==0) {
      spew("no color gouraud shading.\n");
      return D3DENUMRET_OK;
   }

   if ((device_desc->dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_ALPHA) == 0) {
      spew("no alpha blending.\n");
      return D3DENUMRET_OK;
   }


   if ((device_desc->dwDevCaps & TEXTURE_FLAGS) == 0) {
      spew("no texture mapping.\n");
      return D3DENUMRET_OK;
   }

   if ((device_desc->dpcTriCaps.dwRasterCaps &
        (D3DPRASTERCAPS_FOGTABLE|D3DPRASTERCAPS_FOGVERTEX)) == 0) {
      spew("no fog available\n");
      return D3DENUMRET_OK;
   }

#if 0
   if ((device_desc->dwMaxTextureWidth < 256) ||
       (device_desc->dwMaxTextureHeight < 256) ||
       (device_desc->dwMinTextureWidth > 256) ||
       (device_desc->dwMinTextureHeight > 256)) {
       spew("no 256x256 texture support\n");
      return D3DENUMRET_OK;
   }
#endif

   // tbd: check for colorkey, etc.

   AssertMsg(info->device_number < MAX_DEVICES, "Too many d3d devices available.");

   int malloc_size = 
      sizeof (*device_info) +                      // info structure
      (info->num_supported + 1) * sizeof(short) +  // mode list
      sizeof(DevDesc) +                            // D3D Device description
      strlen(info->p_ddraw_desc) + 1;              // ddraw driver description string

   if (info->p_ddraw_guid != NULL) {
      device_info = (lgd3ds_device_info *)Malloc (malloc_size + sizeof(GUID));
      device_info->p_ddraw_guid = (GUID *)(((uchar *)device_info) + malloc_size);
      CopyMemory(device_info->p_ddraw_guid, info->p_ddraw_guid, sizeof(GUID));
   } else {
      device_info = (lgd3ds_device_info *)Malloc (malloc_size);
      device_info->p_ddraw_guid = NULL;
   }

   CopyMemory(&device_info->device_guid, p_device_guid, sizeof(GUID));

   device_info->supported_modes = (short *)(device_info+1);
   if (info->num_supported>0)
      CopyMemory(device_info->supported_modes, &info->supported_modes[0], info->num_supported * sizeof(short));
   device_info->supported_modes[info->num_supported] = -1;

   device_info->device_desc = (DevDesc *)(&device_info->supported_modes[info->num_supported+1]);
   *(device_info->device_desc) = *device_desc;

   device_info->p_ddraw_desc = (char *)(device_info->device_desc + 1);
   strcpy(device_info->p_ddraw_desc, info->p_ddraw_desc);

   device_info->flags = 0;

   device_list[info->device_number++] = device_info;

   return D3DENUMRET_OK;
}
#pragma on (unreferenced)


static HRESULT WINAPI
  EnumDisplayModesCallback(LPDDSURFACEDESC pSD, LPVOID data)
{
   lgd3ds_device_enum_info *info = (lgd3ds_device_enum_info *)data;
   int mode = gr_mode_from_info(pSD->dwWidth, pSD->dwHeight, 
      pSD->ddpfPixelFormat.dwRGBBitCount);

   if (mode < 0)
      return D3DENUMRET_OK;

   // first make sure we haven't already listed this mode as supported
   int i;
   for (i=0; i<info->num_supported; i++)
      if (info->supported_modes[i] == mode) break;

   if (i<info->num_supported)
      return D3DENUMRET_OK;

   info->supported_modes[info->num_supported++] = (short )mode;
   return D3DENUMRET_OK;
}

static void GetDevices(lgd3ds_device_enum_info *info)
{
   HRESULT        hRes;
   LPD3DI         lpd3d;
   LPDIRECTDRAW   lpdd;

// get directdraw object

   hRes = DynDirectDrawCreate(info->p_ddraw_guid, &lpdd, NULL);
   AssertMsg(!FAILED(hRes), "Can't create ddraw object.");

// get direct3d object

   hRes = lpdd->QueryInterface(IID_ID3D, (void **)&lpd3d);
   if ((FAILED(hRes)) || (NULL == lpd3d)) {
      SafeRelease(lpdd);
      return;
   }
   no_d3d = FALSE;

   info->num_supported = 0;
   lpdd->EnumDisplayModes(0, NULL, info, EnumDisplayModesCallback);

   if (info->num_supported > 0) {
      hRes = lpd3d->EnumDevices(EnumDeviceCallback, info);
      AssertMsg(!FAILED(hRes), "EnumDevices failed.");
   }

   SafeRelease(lpd3d);
   SafeRelease(lpdd);
}

#pragma off (unreferenced)
static BOOL PASCAL DDEnumCallback(LPGUID guid, char *desc, char *name, void *user_arg)
{
   lgd3ds_device_enum_info *info = (lgd3ds_device_enum_info *)user_arg;

#ifdef PRINT_ENUM
   mprintf("ddraw name: %s\n", name);
   mprintf("ddraw description: %s\n", desc);
#endif

   info->p_ddraw_guid = guid;
   info->p_ddraw_desc = desc;
   GetDevices(info);
    
   return DDENUMRET_OK;
}
#pragma on (unreferenced)

// returns -1 if direct draw is unavailable,
// number of suitable devices available otherwise
int lgd3d_enumerate_devices(void)
{
   lgd3ds_device_enum_info info;

   if (num_devices >= -1)
      return num_devices;

   if (!LoadDirectDraw())
      return num_devices = -1;

   info.device_number = 0;
   DynDirectDrawEnumerate(DDEnumCallback, &info);
   if (no_d3d && (info.device_number == 0))
      // we couldn't get any d3d object
      num_devices = -1;
   else
      num_devices = info.device_number;
   return num_devices;
}

void lgd3d_unenumerate_devices(void)
{
   for (int i=0; i<num_devices; i++) {
      Free(device_list[i]);
      device_list[i] = NULL;
   }
   num_devices = -2;
}


lgd3ds_device_info *lgd3d_get_device_info(int device_number)
{
   AssertMsg(num_devices > device_number, "Invalid device number");
   return device_list[device_number];
}


