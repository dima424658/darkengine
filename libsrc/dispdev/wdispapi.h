///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/dispdev/RCS/wdispapi.h $
// $Author: ZARKO $
// $Date: 1998/09/18 16:15:54 $
// $Revision: 1.15 $
//
//

#ifndef __WDISPAPI_H
#define __WDISPAPI_H

#ifdef _WIN32
#include <wdspguid.h>

F_DECLARE_INTERFACE(IDirectDraw);
F_DECLARE_INTERFACE(IDirectDrawSurface);

//zb
F_DECLARE_INTERFACE(IDirectDraw4);
F_DECLARE_INTERFACE(IDirectDrawSurface4);

typedef struct grs_bitmap grs_bitmap;
typedef grs_bitmap        sGrBitmap;
typedef struct sWinDispDevCallbackInfo sWinDispDevCallbackInfo;

typedef void (WinDispDevCallbackFunc)(sWinDispDevCallbackInfo *);

///////////////////////////////////////////////////////////////////////////////
//
// INTERFACE: IWinDisplayDevice
//
// Windows-specific elements of display device
//
#undef INTERFACE
#define INTERFACE IWinDisplayDevice

DECLARE_INTERFACE_(IWinDisplayDevice, IUnknown)
{
    //
    // IUnknown methods
    //
    DECLARE_UNKNOWN_PURE();

    //
    // Handle a message from owning window management code.  Return TRUE indicate
    // message is entirely handled. pRetVal to specify return value from window
    // procedure, if handled.
    //
    STDMETHOD_(BOOL, ProcessMessage)(THIS_ uint msg, uint wParam, long lParam, long * pRetVal) PURE;

    //
    // Get the dimensions of the current mode
    //
    STDMETHOD_(void, GetWidthHeight)(THIS_ unsigned * pWidth, unsigned * pHeight) PURE;

    //
    // Signal a task switch
    //
    STDMETHOD_(void, OnTaskSwitch)(THIS_ BOOL) PURE;

    //
    // Access the instance of DirectDraw in use, if any (use at own risk)
    //
    STDMETHOD_(BOOL, GetDirectDraw)(THIS_ IDirectDraw4 **) PURE;

    //
    // Access the device lock/unlock mutex
    //
    STDMETHOD_(void, WaitForMutex)(THIS) PURE;
    STDMETHOD_(void, ReleaseMutex)(THIS) PURE;

    //
    // Crude special accessord for Golf cursor
    //
    STDMETHOD_(void, CursorBltFromGDIScreen)(THIS_ sGrBitmap *, int x, int y) PURE;
    STDMETHOD_(void, CursorBltToGDIScreen)(THIS_ sGrBitmap *, int x, int y) PURE;

    //
    // Access the DirectDraw surface associated with the bitmap, 
    // if any (use at own risk)
    //
    STDMETHOD_(BOOL, GetBitmapSurface)(THIS_ sGrBitmap *, 
                                             IDirectDrawSurface4 **) PURE;

    //
    // Add application callback for task switch message.
    // Callback _must_ be able to chain and handle the kWinDispDevAddFunc and 
    // RemoveFunc messages appropriately.
    // (see x:\prj\tech\libsrc\d3d\texture.c for example)
    //
    STDMETHOD_(void, AddTaskSwitchCallback)(THIS_ WinDispDevCallbackFunc *) PURE;

    //
    // Remove application callback
    // 
    //
    STDMETHOD_(void, RemoveTaskSwitchCallback)(THIS_ int callback_id) PURE;

};

#define IWinDisplayDevice_CursorBltFromGDIScreen(p, bm, x, y)   COMCall3(p, CursorBltFromGDIScreen, bm, x, y)
#define IWinDisplayDevice_CursorBltToGDIScreen(p, bm, x, y)     COMCall3(p, CursorBltToGDIScreen, bm, x, y)
#define IWinDisplayDevice_GetDirectDraw(p, ppDD)                COMCall1(p, GetDirectDraw, ppDD)
#define IWinDisplayDevice_Release(p)                            COMRelease(p)
#define IWinDisplayDevice_GetBitmapSurface(p, bm, ppDDS)        COMCall2(p, GetBitmapSurface, bm, ppDDS)

///////////////////////////////////////////////////////////////////////////////

#endif /* _WIN32 */
#endif /* !__WDISPAPI_H */
