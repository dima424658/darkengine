// $Header: x:/prj/tech/libsrc/lgd3d/RCS/setup.c 1.31 1998/04/22 16:51:25 KEVIN Exp $
#include <windows.h>
#include <ddraw.h>
#include <d3d.h>

#include <comtools.h>
#include <wdispapi.h>
#include <appagg.h>
#include <lgassert.h>
#include <dev2d.h>
#include <mprintf.h>
#include <dbg.h>

#include <lgd3d.h>
#include <texture.h>
#include <tmgr.h>
#include <setup.h>

#define global
global LPD3DI                 lpd3d                      = NULL;
global LPD3DIDEVICE           lpd3dDevice                = NULL;
global LPDIRECTDRAWSURFACE    lpddDevice                 = NULL;
global LPD3DIVIEWPORT         lpd3dViewport              = NULL;
global LPD3DIMATERIAL         lpd3dBackgroundMaterial    = NULL;
#ifndef USE_D3D2_API
global LPD3DIEXECUTEBUFFER    lpd3dExecuteBuffer         = NULL;
global LPD3DIEXECUTEBUFFER    lpd3dExecuteBuffer2        = NULL;
#endif
global D3DMATERIALHANDLE      hPolyMaterial              = 0UL;
global texture_manager        *g_tmgr                    = NULL;

static LPDIRECTDRAWSURFACE    lpddZBuffer                = NULL;
static LPD3DIMATERIAL         PolyMaterial               = NULL;
static D3DMATERIALHANDLE      hd3dBackgroundMaterial     = 0UL;

void lgd3d_set_hardware()
{
}

void lgd3d_set_software()
{
}

void lgd3d_set_RGB()
{
}

BOOL lgd3d_is_RGB()
{
   return TRUE;
}

BOOL lgd3d_is_hardware()
{
   return TRUE;
}
                            
                            
static HRESULT InitMaterial(void)
{
   D3DMATERIAL d3dMaterial;
   HRESULT     hRes;

   hRes = IDirect3D_CreateMaterial(
      lpd3d,
      &lpd3dBackgroundMaterial,
      NULL);

   if (FAILED(hRes))
      return hRes;

   hRes = IDirect3D_CreateMaterial(
      lpd3d,
      &PolyMaterial,
      NULL);

   if (FAILED(hRes))
      return hRes;

   ZeroMemory(&d3dMaterial, sizeof(d3dMaterial));
   d3dMaterial.dwSize = sizeof(d3dMaterial);

   d3dMaterial.dcvDiffuse.r  = D3DVAL(0.0);
   d3dMaterial.dcvDiffuse.g  = D3DVAL(0.0);
   d3dMaterial.dcvDiffuse.b  = D3DVAL(0.0);
   d3dMaterial.dcvAmbient.r  = D3DVAL(0.0);
   d3dMaterial.dcvAmbient.g  = D3DVAL(0.0);
   d3dMaterial.dcvAmbient.b  = D3DVAL(0.0);
   d3dMaterial.dcvSpecular.r = D3DVAL(0.0);
   d3dMaterial.dcvSpecular.g = D3DVAL(0.0);
   d3dMaterial.dcvSpecular.b = D3DVAL(0.0);
   d3dMaterial.dvPower       = D3DVAL(0.0);

   /*
   * As this is the background material we don't want a ramp allocated (we
   * are not going to be smooth shading the background).
   */
   d3dMaterial.dwRampSize    = 1UL;
   
   hRes = lpd3dBackgroundMaterial->lpVtbl->SetMaterial(lpd3dBackgroundMaterial,
                                                      &d3dMaterial);
   if (FAILED(hRes))
      return hRes;
   hRes = lpd3dBackgroundMaterial->lpVtbl->GetHandle(lpd3dBackgroundMaterial,
                                                   lpd3dDevice,
                                                   &hd3dBackgroundMaterial);
   if (FAILED(hRes))
      return hRes;

   hRes = lpd3dViewport->lpVtbl->SetBackground(lpd3dViewport, hd3dBackgroundMaterial);
   if (FAILED(hRes))
      return hRes;

   d3dMaterial.diffuse.r  = D3DVAL(1.0);
   d3dMaterial.diffuse.g  = D3DVAL(1.0);
   d3dMaterial.diffuse.b  = D3DVAL(1.0);
   d3dMaterial.ambient.r  = D3DVAL(1.0);
   d3dMaterial.ambient.g  = D3DVAL(1.0);
   d3dMaterial.ambient.b  = D3DVAL(1.0);

   hRes = PolyMaterial->lpVtbl->SetMaterial(PolyMaterial,
                                                      &d3dMaterial);
   if (FAILED(hRes))
      return hRes;
   hRes = PolyMaterial->lpVtbl->GetHandle(PolyMaterial,
                                                   lpd3dDevice,
                                                   &hPolyMaterial);
   if (FAILED(hRes))
      return hRes;

   return DD_OK;
}

//static LPDIRECTDRAWCOLORCONTROL lpcc=NULL;

static DWORD FlagsToBitdepth(DWORD dwFlags)
{
    if (dwFlags & DDBD_1)
        return 1UL;
    else if (dwFlags & DDBD_2)
        return 2UL;
    else if (dwFlags & DDBD_4)
        return 4UL;
    else if (dwFlags & DDBD_8)
        return 8UL;
    else if (dwFlags & DDBD_16)
        return 16UL;
    else if (dwFlags & DDBD_24)
        return 24UL;
    else if (dwFlags & DDBD_32)
        return 32UL;
    else
        return 0UL; /* Oh, please... */
}

static HRESULT CreateZBuffer(lgd3ds_device_info *info, LPDIRECTDRAW lpdd)
{
   DWORD bitDepth = info->device_desc->dwDeviceZBufferBitDepth;
   DDSURFACEDESC ddsd;
   HRESULT hRes;

   if (0UL == bitDepth)
      return ~DD_OK;

   bitDepth = FlagsToBitdepth(bitDepth);
   ZeroMemory(&ddsd, sizeof(ddsd));
   ddsd.dwSize           = sizeof(ddsd);
   ddsd.dwFlags          = DDSD_CAPS   |
                           DDSD_WIDTH  |
                           DDSD_HEIGHT |
                           DDSD_ZBUFFERBITDEPTH;
   ddsd.ddsCaps.dwCaps   = DDSCAPS_ZBUFFER | DDSCAPS_VIDEOMEMORY;
   ddsd.dwWidth          = grd_cap->w;
   ddsd.dwHeight         = grd_cap->h;
   ddsd.dwZBufferBitDepth= bitDepth;
   hRes = lpdd->lpVtbl->CreateSurface(lpdd, &ddsd, &lpddZBuffer, NULL);

   if (FAILED(hRes)) {
      info->flags &= ~LGD3DF_ZBUFFER;
      return hRes;
   }

   hRes = lpddDevice->lpVtbl->AddAttachedSurface(lpddDevice, lpddZBuffer);

   if (FAILED(hRes))
      return hRes;

   return DD_OK;
}

// sadly, this really doesn't work.  Drivers get really upset when you
// try to lock the z buffer
#if 0
void lgd3d_blit_zbuffer(void)
{
   DDSURFACEDESC ddsd;
   HRESULT hres;
   grs_bitmap bm;

   hres = lpddZBuffer->lpVtbl->Lock(lpddZBuffer, NULL, &ddsd, DDLOCK_SURFACEMEMORYPTR|DDLOCK_WAIT, NULL);
   AssertMsg1 (hres == DD_OK, "Lock failed: %x", hres);

   gr_init_bitmap(&bm, ddsd.lpSurface, BMT_FLAT16, 0, grd_cap->w, grd_cap->h);
   bm.row = ddsd.lPitch;
   gr_bitmap(&bm, 0, 0);
}
#endif

static void CreateDevice(DWORD dwWidth, DWORD dwHeight, lgd3ds_device_info *info)
{
   HRESULT              hRes;
   LPDIRECTDRAW         lpdd;
   DDSURFACEDESC        desc;
   IWinDisplayDevice *  pWinDisplayDevice;

   // @TBD: Set display device kind _and_ set screen mode
   // For now, assume already done.

   pWinDisplayDevice = AppGetObj (IWinDisplayDevice);

   // get direct draw object from WinDisplayDevice
   VerifyMsg(IWinDisplayDevice_GetDirectDraw(pWinDisplayDevice, &lpdd),
               "Couldn't get direct draw object");

   AssertMsg(NULL != lpdd, "NULL ddraw object");

   hRes = lpdd->lpVtbl->QueryInterface(lpdd, &IID_ID3D, &lpd3d);

   AssertMsg(!(FAILED(hRes)), "Couldn't get d3d object from direct draw.");
   AssertMsg(NULL != lpd3d, "NULL d3d object");

   AssertMsg(NULL == lpddDevice, "ddraw device already present");
   AssertMsg(NULL == lpd3dDevice, "d3d device already present");


   hRes = IWinDisplayDevice_GetBitmapSurface(pWinDisplayDevice, NULL, &lpddDevice);
   AssertMsg(!FAILED(hRes), "GetBitmapSurface failed.");

   AssertMsg((lpddDevice != NULL), "No render surface available.");

   if (lpddDevice->lpVtbl->IsLost(lpddDevice))
      lpddDevice->lpVtbl->Restore(lpddDevice);

//   hRes = lpddDevice->lpVtbl->QueryInterface(lpddDevice, &IID_IDirectDrawColorControl,
//      (LPVOID *)&lpcc);
//   AssertMsg1(!FAILED(hRes), "Query for ColorControl failed. Error %i", hRes&0xffff);

   ZeroMemory(&desc, sizeof(desc));
   desc.dwSize = sizeof(desc);

   hRes = lpddDevice->lpVtbl->GetSurfaceDesc(lpddDevice, &desc);

#ifdef DEBUG
   if (FAILED(hRes))
      Warning(("CreateDevice(): GetSurfaceDesc surface failed: %x\n", hRes));

   if (!(desc.dwFlags & DDSD_CAPS))
      Warning(("CreateDevice(): flags indicate caps field not valid!\n"));

   if (desc.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
      Warning(("CreateDevice(): Rendering to primary surface!\n"));
#endif

   pWinDisplayDevice->lpVtbl->Release(pWinDisplayDevice);

   if (info->flags & LGD3DF_ZBUFFER) {
      hRes = CreateZBuffer(info, lpdd);
      if (hRes != DD_OK)
         info->flags &= ~LGD3DF_ZBUFFER;
   }

#ifdef USE_D3D2_API
   hRes = lpd3d->lpVtbl->CreateDevice(lpd3d, &info->device_guid, lpddDevice, &lpd3dDevice);
#else
   hRes = lpddDevice->lpVtbl->QueryInterface(lpddDevice, info->device_guid, &lpd3dDevice);
#endif

   AssertMsg1(!FAILED(hRes), "CreateDevice failed. hres = %i",hRes&0xffff);

   hRes = lpd3d->lpVtbl->CreateViewport(lpd3d, &lpd3dViewport, NULL);

   AssertMsg(!FAILED(hRes), "CreateViewport failed.");

   if (lpd3dDevice == NULL)
      Error(1, "Failed to create direct3D device");

   hRes = lpd3dDevice->lpVtbl->AddViewport(lpd3dDevice, lpd3dViewport);

   AssertMsg(!FAILED(hRes), "AddViewport failed.");

#ifdef USE_D3D2_API
   hRes = lpd3dDevice->lpVtbl->SetCurrentViewport(lpd3dDevice, lpd3dViewport);

   AssertMsg(!FAILED(hRes), "SetCurrentViewport failed.");
#endif

   lpdd->lpVtbl->Release(lpdd);
}

#ifdef USE_D3D2_API

#define CreateExecuteBuffer()
#define ReleaseExecuteBuffer()

#else

static void ReleaseExecuteBuffer()
{
   SafeRelease(lpd3dExecuteBuffer);
   SafeRelease(lpd3dExecuteBuffer2);
}

static void CreateExecuteBuffer()
{
   D3DEXECUTEBUFFERDESC d3dExecuteBufferDesc;
   HRESULT hRes;

   ZeroMemory(&d3dExecuteBufferDesc, sizeof(d3dExecuteBufferDesc));
   d3dExecuteBufferDesc.dwSize       = sizeof(d3dExecuteBufferDesc);
   d3dExecuteBufferDesc.dwFlags      = D3DDEB_BUFSIZE;
   d3dExecuteBufferDesc.dwBufferSize = 1024;
   hRes = lpd3dDevice->lpVtbl->CreateExecuteBuffer(lpd3dDevice,
                                                   &d3dExecuteBufferDesc,
                                                   &lpd3dExecuteBuffer,
                                                   NULL);

   AssertMsg(!FAILED(hRes), "CreateExecuteBuffer failed.");
   ZeroMemory(&d3dExecuteBufferDesc, sizeof(d3dExecuteBufferDesc));
   d3dExecuteBufferDesc.dwSize       = sizeof(d3dExecuteBufferDesc);
   d3dExecuteBufferDesc.dwFlags      = D3DDEB_BUFSIZE;
   d3dExecuteBufferDesc.dwBufferSize = 1024;
   hRes = lpd3dDevice->lpVtbl->CreateExecuteBuffer(lpd3dDevice,
                                                   &d3dExecuteBufferDesc,
                                                   &lpd3dExecuteBuffer2,
                                                   NULL);

   AssertMsg(!FAILED(hRes), "CreateExecuteBuffer failed.");
}

#endif


void lgd3d_shutdown(void)
{
   extern void lgd3d_render_shutdown(void);

   lgd3d_render_shutdown();
   ShutdownTextureSys();
   SafeRelease(lpd3dBackgroundMaterial);
   SafeRelease(PolyMaterial);
   ReleaseExecuteBuffer();
   SafeRelease(lpd3dViewport);
   SafeRelease(lpd3dDevice);
   SafeRelease(lpddDevice);
   SafeRelease(lpddZBuffer);
   SafeRelease(lpd3d);
}

void lgd3d_init(lgd3ds_device_info *device_info)
{
   extern void lgd3d_render_init(lgd3ds_device_info *info);

   // @TBD: get w and h from app; set mode in CreateDevice()
   CreateDevice(grd_cap->w, grd_cap->h, device_info);

   CreateExecuteBuffer();

   VerifyMsg(InitMaterial()==DD_OK, "InitMaterial failed");

   InitTextureSys(device_info);

   lgd3d_start_frame(0);
   lgd3d_render_init(device_info);
   lgd3d_end_frame();
}

static HRESULT InitViewport(void)
{
   D3DSVIEWPORT d3dViewport;

   AssertMsg(NULL != lpd3dViewport,"hey, no viewport!");

   ZeroMemory(&d3dViewport, sizeof(d3dViewport));
   d3dViewport.dwSize   = sizeof(d3dViewport);
   d3dViewport.dwX      = 0UL;
   d3dViewport.dwY      = 0UL;
   d3dViewport.dwWidth  = (DWORD)grd_cap->w; // width in pixels
   d3dViewport.dwHeight = (DWORD)grd_cap->h; // height in pixels
#ifdef USE_D3D2_API
   d3dViewport.dvClipX  = D3DVAL(-1.0);
   d3dViewport.dvClipY  = D3DVAL(-1.0);
   d3dViewport.dvClipWidth  = D3DVAL(2.0);
   d3dViewport.dvClipHeight = D3DVAL(2.0);
   d3dViewport.dvMinZ = D3DVAL(0.0);
   d3dViewport.dvMaxZ = D3DVAL(1.0);
   return lpd3dViewport->lpVtbl->SetViewport2(lpd3dViewport, &d3dViewport);
#else
   d3dViewport.dvScaleX = D3DVAL((float)d3dViewport.dwWidth / 2.0);
   d3dViewport.dvScaleY = D3DVAL((float)d3dViewport.dwHeight / 2.0);
   d3dViewport.dvMaxX   = D3DVAL(1.0);
   d3dViewport.dvMaxY   = D3DVAL(1.0);
   return lpd3dViewport->lpVtbl->SetViewport(lpd3dViewport, &d3dViewport);
#endif
}

uchar *lgd3d_clut = NULL;
uchar *lgd3d_set_clut(uchar *clut)
{
   uchar *old_clut = lgd3d_clut;
   lgd3d_clut = clut;
   lgd3d_set_texture_clut(clut);
   return old_clut;
}

void lgd3d_start_frame(int frame)
{
   HRESULT        hRes;
   extern void lgd3d_render_start_frame(void);

   hRes = lpd3dDevice->lpVtbl->BeginScene(lpd3dDevice);
   AssertMsg(!(FAILED(hRes)), "BeginScene failed");

   hRes = InitViewport();
   AssertMsg1(!(FAILED(hRes)), "InitViewport failed: Error %d", hRes&0xffff);

   lgd3d_render_start_frame();

   if (g_tmgr!=NULL)
      g_tmgr->start_frame(frame); // set current frame and use default texture

}

void lgd3d_end_frame(void)
{
   HRESULT hRes;
   extern void lgd3d_render_end_frame(void);

   lgd3d_render_end_frame();
   hRes = lpd3dDevice->lpVtbl->EndScene(lpd3dDevice);
   AssertMsg(!(FAILED(hRes)), "EndScene failed");

   if (g_tmgr!=NULL)
      g_tmgr->end_frame();
}
