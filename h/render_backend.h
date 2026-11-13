#ifndef __RENDER_BACKEND_H
#define __RENDER_BACKEND_H

#include <stdint.h>

// Platform-neutral rendering boundary used by the legacy lgd3d facade and
// higher-level engine code. The display provider owns the window and surface;
// the backend implements these operations without exposing graphics API
// types to engine callers.

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sRenderBackendVertex {
  float x, y, z, rhw;
  float r, g, b, a;
  float fog;
  float u0, v0;
  float u1, v1;
} sRenderBackendVertex;

enum eRenderBackendPrimitive {
  kRenderBackendTriangles = 0,
  kRenderBackendLines = 1,
  kRenderBackendPoints = 2
};

enum eRenderBackendBlend {
  kRenderBackendBlendOpaque = 0,
  kRenderBackendBlendAlpha = 1,
  kRenderBackendBlendMultiply = 2,
  kRenderBackendBlendAdd = 3
};

int RenderBackendAvailable(void);
void RenderBackendSetGamma(float gamma);
// Select a physical device by the index reported through the legacy lgd3d
// device enumeration. Negative values restore automatic device selection.
void RenderBackendSetPhysicalDeviceIndex(int index);
void RenderBackendSetScaleToWindow(int enabled);
void RenderBackendSetFitToViewport(int enabled);
void RenderBackendSetPreserveCanvas(int enabled);
int RenderBackendPreserveCanvas(void);
void RenderBackendSetCanvasOverlayEnabled(int enabled);
int RenderBackendCanvasOverlayEnabled(void);
void RenderBackendSetWindowedClientSizeHint(int width, int height);
// Returns the current client size in logical points. Use the window metrics
// passed to the backend for the drawable size in physical pixels.
int RenderBackendGetClientSize(int *width, int *height);
// Kept for existing callers that used the former resize-settle API. This now
// returns the same current size as RenderBackendGetClientSize.
int RenderBackendGetSettledClientSize(int *width, int *height);
int RenderBackendGetLogicalSize(int *width, int *height);
int RenderBackendClientToLogicalPoint(int *x, int *y);
int RenderBackendLogicalToClientPoint(int *x, int *y);
// Presents a legacy-canvas update immediately when the platform normally
// defers those updates until the enclosing application frame ends.  Intended
// for throttled progress UI inside synchronous loading work.
void RenderBackendPresentProgress(void);
int RenderBackendBeginFrame(void);
void RenderBackendEndFrame(void);
void RenderBackendDeactivateScene(void);
void RenderBackendClearDepth(void);
void RenderBackendClearDepthRect(int x0, int y0, int x1, int y1);
// Clear a logical canvas rectangle in the active hardware scene.
void RenderBackendClearCanvasRegion(int x, int y, int width, int height);
void RenderBackendSetDepth(int compareEnabled, int writeEnabled);
void RenderBackendSetBlend(int blendMode);
void RenderBackendSetAlphaTest(int enabled);
void RenderBackendSetSampler(int level, int wrap, int smooth);
void RenderBackendSetFog(int enabled, uint32_t rgb);

void *RenderBackendCreateTexture(int width, int height, const void *rgba,
                                 int rowBytes);
int RenderBackendUpdateTexture(void *texture, int width, int height,
                               const void *rgba, int rowBytes);
void RenderBackendDestroyTexture(void *texture);
void RenderBackendBindTexture(int level, void *texture);

int RenderBackendDraw(int primitive, const sRenderBackendVertex *vertices,
                      int vertexCount, int useSecondTexture);

// Optional diagnostics for automated render checks. Set DARK_RENDER_TRACE to
// opt in to the plain-text trace; normal game and viewer runs create no log.
void RenderBackendTraceReset(void);
void RenderBackendTrace(const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif
