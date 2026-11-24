#ifndef __DISPWND_H
#define __DISPWND_H

#include <comtools.h>
#include <dispguid.h>

struct SDL_Window;
F_DECLARE_INTERFACE(IDisplayWindow);

#undef INTERFACE
#define INTERFACE IDisplayWindow
DECLARE_INTERFACE_(IDisplayWindow,IUnknown)
{
   DECLARE_UNKNOWN_PURE();

   STDMETHOD_(BOOL, Open)(THIS_ int width, int height, const char *title) PURE;
   STDMETHOD_(void, Close)(THIS) PURE;
   STDMETHOD_(struct SDL_Window *, GetSDLWindow)(THIS) PURE;
   STDMETHOD_(void, GetMetrics)(THIS_ int *clientWidth, int *clientHeight,
      int *drawableWidth, int *drawableHeight) PURE;
   STDMETHOD_(BOOL, IsMinimized)(THIS) PURE;
   STDMETHOD_(BOOL, StartupWindowPending)(THIS) PURE;
   STDMETHOD_(BOOL, ShowForPresentation)(THIS) PURE;
   STDMETHOD_(void, RevealAfterPresent)(THIS) PURE;
   STDMETHOD_(BOOL, SetFullscreen)(THIS_ BOOL enabled) PURE;
   STDMETHOD_(void, SetTitle)(THIS_ const char *title) PURE;
   STDMETHOD_(void, SetNativeCursorVisible)(THIS_ BOOL visible) PURE;
   STDMETHOD_(void, SetMousePositionLogical)(THIS_ int x, int y) PURE;
   STDMETHOD_(void, SetRelativeMouse)(THIS_ BOOL enabled) PURE;
   STDMETHOD_(void, SetFocused)(THIS_ BOOL focused) PURE;
   STDMETHOD_(BOOL, IsRelativeMouseActive)(THIS) PURE;
   STDMETHOD_(void, SetWindowedClientSizeHint)(THIS_ int width, int height) PURE;
};
#undef INTERFACE

EXTERN tResult LGAPI DisplayWindowCreate(IUnknown *pOuter);

#endif
