#ifndef __VKREND_H
#define __VKREND_H

#include <stdint.h>
#include <comtools.h>
#include <dispguid.h>

union SDL_Event;
struct IVulkanOverlay;
F_DECLARE_INTERFACE(IVulkanRenderer);

#undef INTERFACE
#define INTERFACE IVulkanRenderer
DECLARE_INTERFACE_(IVulkanRenderer,IUnknown)
{
   DECLARE_UNKNOWN_PURE();

   STDMETHOD_(BOOL, Open)(THIS_ int width, int height, const char *title) PURE;
   STDMETHOD_(void, Close)(THIS) PURE;
   STDMETHOD_(BOOL, TakeResize)(THIS_ int *width, int *height) PURE;
   STDMETHOD_(BOOL, BeginEventPump)(THIS) PURE;
   STDMETHOD_(void, EndEventPump)(THIS) PURE;
   STDMETHOD_(void, WindowResized)(THIS) PURE;
   STDMETHOD_(void, WindowMinimized)(THIS) PURE;
   STDMETHOD_(void, WindowRestored)(THIS) PURE;
   STDMETHOD_(void, SetPhysicalDeviceIndex)(THIS_ int index) PURE;
   // The caller keeps the overlay alive for as long as it is installed.
   STDMETHOD_(void, SetOverlay)(THIS_ struct IVulkanOverlay *overlay) PURE;
   STDMETHOD_(void, SetTopInset)(THIS_ int points) PURE;
   STDMETHOD_(BOOL, ProcessOverlayEvent)(THIS_ const union SDL_Event *event) PURE;
   STDMETHOD_(void, BeginApplicationFrame)(THIS) PURE;
   STDMETHOD_(void, EndApplicationFrame)(THIS) PURE;
   STDMETHOD_(void, PresentProgress)(THIS) PURE;
   STDMETHOD_(BOOL, PresentCanvasRect)(THIS_ const void *pixels, int width, int height,
      int rowBytes, int depth, const uint8_t *palette, int x0, int y0, int x1, int y1) PURE;
   STDMETHOD_(BOOL, ClearOverlay)(THIS_ void *pixels, int width, int height,
      int rowBytes, int depth) PURE;
   STDMETHOD_(BOOL, HasDirectCanvas)(THIS) PURE;
   STDMETHOD_(void, InvalidateCanvas)(THIS) PURE;
   STDMETHOD_(void, PresentRect)(THIS_ const void *pixels, int width, int height,
      int rowBytes, int depth, const uint8_t *palette, int x0, int y0, int x1, int y1) PURE;
   STDMETHOD_(void, Present)(THIS_ const void *pixels, int width, int height,
      int rowBytes, int bitDepth, const uint8_t *palette) PURE;
   STDMETHOD_(BOOL, PresentMovieFrame)(THIS_ const void *rgba, int width, int height,
      int rowBytes) PURE;
};
#undef INTERFACE

EXTERN tResult LGAPI VulkanRendererCreate(IUnknown *pOuter);

#endif
