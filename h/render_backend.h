#ifndef __RENDER_BACKEND_H
#define __RENDER_BACKEND_H

#include <stdint.h>

// Platform-neutral rendering boundary used by the legacy lgd3d facade and
// higher-level engine code.  A platform renderer owns the window/surface and
// implements these operations; the current Windows provider is D3D11.  An
// OpenGL provider can implement the same contract without exposing GL types
// to the engine.

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
void RenderBackendSetScaleToWindow(int enabled);
void RenderBackendSetFitToViewport(int enabled);
void RenderBackendSetInteractiveResize(int resizing);
void RenderBackendSetPreserveCanvas(int enabled);
int RenderBackendPreserveCanvas(void);
void RenderBackendSetWindowedClientSizeHint(int width, int height);
// Returns the drawable client size in physical pixels. While an interactive
// resize is in progress the size is intentionally withheld so callers can
// rebuild expensive engine-owned canvases once, when the drag finishes.
int RenderBackendGetSettledClientSize(int *width, int *height);
int RenderBackendClientToLogicalPoint(int *x, int *y);
int RenderBackendLogicalToClientPoint(int *x, int *y);
int RenderBackendBeginFrame(void);
void RenderBackendEndFrame(void);
void RenderBackendDeactivateScene(void);
void RenderBackendClearDepth(void);
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
int RenderBackendCaptureComplete(void);

#ifdef __cplusplus
}
#endif

#endif
