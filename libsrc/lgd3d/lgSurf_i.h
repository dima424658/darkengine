/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

/*=========================================================

  Created:  2/23/99 8:58:55 AM

  File:  lgSurf.h

  Description:  ILGSurface interface
                is interface encapsulating video memory
                used as a target for 2/3D rendering

=========================================================*/

#pragma once


//_____INCLUDES_AND_DEFINITIONS___________________________

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <comtools.h>
#include <d3d.h>
#include <dev2d.h>

DEFINE_LG_GUID(IID_ILGSurface, 0x25e);
DEFINE_LG_GUID(IID_ILGDD4Surface, 0x269);

//_______TYPEDEFS_________________________________________

// internal states of a LGSurface
typedef enum eLGSurfState{
   kLGSSUninitialized = 0,
   kLGSSInitialized,
   kLGSSUnlocked,
   kLGSS3DRenderining,
   kLGSS2DLocked,
   kLGSS2DBlitting
} eLGSurfState;


// generic surface description for our surface
// NOTE: a pointer to the surface's surface is not included
typedef struct sLGSurfaceDescription {
   DWORD               dwWidth;
   DWORD               dwHeight;
   DWORD               dwBitDepth;
   DWORD               dwPitch;
   grs_rgb_bitmask     sBitmask;
} sLGSurfaceDescription;


// types of 2d surface lock
typedef enum eLGSBlit{
   kLGSBRead               = 0x00000001L,
   kLGSBWrite              = 0x00000002L,
   kLGSBReadWrite          = 0x00000004L
} eLGSBlit;


////////////////////////////////////////////

#undef INTERFACE
#define INTERFACE ILGSurface

DECLARE_INTERFACE_(ILGSurface, IUnknown)
{
   DECLARE_UNKNOWN_PURE();
   
   
   /////////////////////////////
   //
   // CreateInternalScreenSurface
   //
   
   // Creates an internal structure that represents the main window
   // initial state is required to be kLGSSUninitialized
   // state is set to kLGSSInitialized upon the completion
   // If hMainWindow is non-NULL, sets the clipper to clip at that window
   // if NULL, no clipper is attached

   // E_LGSURF_HARDWARE_ERROR    API specific failure
   //   
   STDMETHOD ( CreateInternalScreenSurface ) (
      THIS_
      /*[in]*/    HWND hMainWindow
      ) PURE;
   
   /////////////////////////////
   //
   // CreateClipper
   //
   // Creates or changes the clipper attached to the blit surface
   // to a windows handle.  If handle is NULL, removes the clipper

   // E_LGSURF_HARDWARE_ERROR    API specific failure
   //   
   STDMETHOD ( ChangeClipper ) (
      THIS_
      /*[in]*/    HWND hMainWindow
      ) PURE;
   

   /////////////////////////////
   //
   // SetAs3dHdwTarget
   //
   
   // IF the initial state is kLGSSUninitialized:
   // grab the existing hardvare rendering device.  At this point global
   // rendering target was obtained from the 2d environment
   // ELSE
   // it is assumed that the internal representation of the screen was allready
   // set.
   // Upon the completion the state is set to kLGSSUnlocked
   
   // E_3DHDW_INUSE:           the/a renderer could not have been committed
   // E_2D_DRIVER_ERROR:       failed to obtain data from 2d environment
   // E_HARDWARE_ERROR:        renderer specific error
   // E_OUT_OF_VIDEO_MEMORY:   not enough video memory **or** hardware 
   //                          incapable of rendering to of-screen surface
   STDMETHOD_( HRESULT, SetAs3dHdwTarget ) ( 
      THIS_ 
      /*[in]*/    DWORD dwRequestedFlags,
      /*[in]*/    DWORD dwWidth, DWORD pdwHeight, DWORD dwBitDepth,
      /*[out]*/   DWORD* pdwCapabilityFlags,
      /*[out]*/   grs_canvas** ppsCanvas 
      ) PURE;
   
  

   
   
   ///////////////////////////////
   //
   // Resize3dHdw
   //
   
   // resize the surface and retool the hardware renderer
   // (error returns are the same as for "SetAs3dHdwTarget")
   STDMETHOD_( HRESULT, Resize3dHdw) ( 
      THIS_ 
      /*[in]*/    DWORD dwRequestedFlags,
      /*[in]*/    DWORD dwWidth, DWORD pdwHeight, DWORD dwBitDepth,
      /*[out]*/   DWORD* pdwCapabilityFlags,
      /*[out]*/   grs_canvas** ppsCanvas
      ) PURE;
   
   
   //////////////////////////////
   //
   // BlitToScreen
   //
   
   // blit the surface to the rendering target determined by the 
   // 2d environment. The input parameters specify the target rectangle.
   // NOTE: the screen coordinate are assumed for dwXScreen and dwYScreen.
   //       If the rectangle is smaller than the surface upper left rectangle
   //       of the LGSurface is blitted.  Target rectangle larger than the
   //       surface results in an error.
   
   // E_SURFACE_LOCKED:            surface was not in "unlocked" state
   // E_INPUT_PARAMETERS_INVALID:  the dimensions passed are too big
   // E_HARDWARE_ERROR:            blit failed
   
   STDMETHOD_( HRESULT, BlitToScreen ) (THIS_
      /// target rectangele. All coordintes are relative to the screen
      DWORD   dwXScreen, 
      DWORD   dwYScreen,
      DWORD   dwWidth,
      DWORD   dwHeight
      )
      PURE;
   
   ///////////////////////////////
   //
   // CleanSurface
   //
   
   // cleans the surface and the depth buffer if it were attached
   
   STDMETHOD_( void, CleanSurface) (THIS) PURE;
   
   
   ///////////////////////////////
   //
   // Start3D
   //
   
   // starts a 3d frame, uses the current 3d API to initialize rendering
   
   STDMETHOD_( void, Start3D) (THIS) PURE;
   
   
   
   //////////////////////////////
   // 
   // End3D
   //
   
   // ends the frame by calling the 3d API's end of frame method
   
   STDMETHOD_( void, End3D ) (THIS) PURE;
   
   
   //////////////////////////////////
   //
   // LockFor2D
   //
   
   // locks the memory of the surface and provides a correct pointer
   // to the video memory inside the canvas structure.  The pointer should
   // be used for writing and/or reading only according to the passed
   // flag in eBlitFlags.
   
   // E_SURFACE_LOCKED:        surface is not in the "unlocked" state
   // E_HARDWARE_ERROR:        API specific failure to lock the surface 
   
   STDMETHOD_( HRESULT, LockFor2D ) (
      THIS_ 
      /*[out]*/   grs_canvas** ppsCanvas,  
      /*[in]*/    eLGSBlit eBlitFlags
      ) PURE;
   
   
   
   /////////////////////////////
   //
   // UnlockFor2D
   //
   
   // unlocks the videomemory. 
   // NOTE:  the memory pointer in the canvas structure is set to NULL
   
   // E_SURFACE_WASNOT_LOCKED: surface was not locked for 2d rendering
   // E_HARDWARE_ERROR:        API specific failure to unlock the surface     
   
   STDMETHOD_( HRESULT, UnlockFor2D ) (THIS) PURE;
   
   
   
   // State queries
   
   STDMETHOD_( DWORD, GetWidth ) (THIS) PURE;
   STDMETHOD_( DWORD, GetHeight ) (THIS) PURE;
   STDMETHOD_( DWORD, GetPitch ) (THIS) PURE;
   STDMETHOD_( DWORD, GetBitDepth ) (THIS) PURE;
   STDMETHOD_( eLGSurfState, GetSurfaceState ) (THIS) PURE;
   
   /////////////////////////
   //
   // GetSurfaceDescription
   //
   
   // copies sLGSurfaceDescription into the provided structure, *psDsc
   
   // E_INPUT_PARAMETERS_INVALID:      psDsc is NULL
   
   STDMETHOD_( HRESULT, GetSurfaceDescription) (THIS_ 
      /*[out]*/ sLGSurfaceDescription* psDsc ) PURE;
};

#undef INTERFACE
#define INTERFACE ILGDD4Surface

DECLARE_INTERFACE_(ILGDD4Surface, IUnknown)
{
   DECLARE_UNKNOWN_PURE();

    STDMETHOD_( int, GetDeviceInfoIndex ) (THIS);

    STDMETHOD_( int, GetDirectDraw ) (
        THIS_
        /*[out]*/   IDirectDraw4 **ppDD
    );

    STDMETHOD_( int, GetRenderSurface ) (
        THIS_
        /*[out]*/   IDirectDrawSurface4 **ppRS
    );
};


//______EXPORTED_DATA_____________________________________


/// creates an empty surface
EXTERN BOOL CreateLGSurface( ILGSurface** ppILGSurf );



//______ERROR_MESSAGES_____________________________________


#define LGSURF_SUBSYS       4


#define E_LGSURF_ALREADY_INITIALIZED          MakeResult( kError, LGSURF_SUBSYS, 1 )

#define E_LGSURF_3DHDW_INUSE                  MakeResult( kError, LGSURF_SUBSYS, 2 )

#define S_LGSURF_3DHDW_ATTACHED               MakeResult( kSuccess, LGSURF_SUBSYS, 3 )

#define E_LGSURF_2D_DRIVER_ERROR              MakeResult( kError, LGSURF_SUBSYS, 4 )

#define E_LGSURF_HARDWARE_ERROR               MakeResult( kError, LGSURF_SUBSYS, 5 )

#define E_LGSURF_OUT_OF_VIDEO_MEMORY          MakeResult( kError, LGSURF_SUBSYS, 6 )

#define E_LGSURF_SURFACE_LOCKED               MakeResult( kError, LGSURF_SUBSYS, 7 )

#define E_LGSURF_SURFACE_WASNOT_LOCKED        MakeResult( kError, LGSURF_SUBSYS, 8 )

#define E_LGSURF_INPUT_PARAMETERS_INVALID     MakeResult( kError, LGSURF_SUBSYS, 9 )
