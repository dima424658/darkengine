// $Header: x:/prj/tech/libsrc/lgd3d/RCS/setup.h 1.7 1997/10/28 15:29:27 KEVIN Exp $

#ifndef __SETUP_H
#define __SETUP_H

#include <ddraw.h>
#include <d3d.h>

#ifdef USE_D3D2_API
#define IID_ID3D              IID_IDirect3D2
#define IID_ID3DTexture       IID_IDirect3DTexture2
#define LPD3DI                LPDIRECT3D2
#define LPD3DIDEVICE          LPDIRECT3DDEVICE2
#define LPD3DIVIEWPORT        LPDIRECT3DVIEWPORT2
#define LPD3DIMATERIAL        LPDIRECT3DMATERIAL2
#define LPD3DITEXTURE         LPDIRECT3DTEXTURE2
#define LPD3DIMATERIAL        LPDIRECT3DMATERIAL2
#define D3DSVIEWPORT          D3DVIEWPORT2
#else
#define IID_ID3D              IID_IDirect3D
#define IID_ID3DTexture       IID_IDirect3DTexture
#define LPD3DI                LPDIRECT3D
#define LPD3DIDEVICE          LPDIRECT3DDEVICE
#define LPD3DIVIEWPORT        LPDIRECT3DVIEWPORT
#define LPD3DIMATERIAL        LPDIRECT3DMATERIAL
#define LPD3DITEXTURE         LPDIRECT3DTEXTURE
#define LPD3DIMATERIAL        LPDIRECT3DMATERIAL
#define D3DSVIEWPORT          D3DVIEWPORT
#endif
#define LPD3DIEXECUTEBUFFER   LPDIRECT3DEXECUTEBUFFER 

extern LPD3DIEXECUTEBUFFER    lpd3dExecuteBuffer;
extern LPD3DIEXECUTEBUFFER    lpd3dExecuteBuffer2;
extern LPDIRECTDRAWSURFACE    lpddDevice;
extern LPD3DIDEVICE           lpd3dDevice;                                  
extern LPD3DIVIEWPORT         lpd3dViewport;                                  
extern LPD3DI                 lpd3d;
extern D3DMATERIALHANDLE      hPolyMaterial;
extern LPD3DIMATERIAL         lpd3dBackgroundMaterial;
extern uchar *lgd3d_clut;
#endif
