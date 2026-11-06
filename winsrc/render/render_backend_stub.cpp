#include <render_backend.h>

// The legacy Visual Studio DirectDraw build has no hardware renderer. The
// supported Windows Vulkan build is the SDL3/CMake target; these definitions
// preserve the old engine-facing ABI for the software-only legacy target.
extern "C" {
int RenderBackendAvailable(void) { return 0; }
void RenderBackendSetGamma(float) {}
void RenderBackendSetScaleToWindow(int) {}
void RenderBackendSetFitToViewport(int) {}
void RenderBackendSetInteractiveResize(int) {}
void RenderBackendSetPreserveCanvas(int) {}
int RenderBackendPreserveCanvas(void) { return 0; }
int RenderBackendGetSettledClientSize(int *, int *) { return 0; }
int RenderBackendClientToLogicalPoint(int *, int *) { return 0; }
int RenderBackendLogicalToClientPoint(int *, int *) { return 0; }
int RenderBackendBeginFrame(void) { return 0; }
void RenderBackendEndFrame(void) {}
void RenderBackendDeactivateScene(void) {}
void RenderBackendClearDepth(void) {}
void RenderBackendClearCanvasRegion(int, int, int, int) {}
void RenderBackendSetDepth(int, int) {}
void RenderBackendSetBlend(int) {}
void RenderBackendSetAlphaTest(int) {}
void RenderBackendSetSampler(int, int, int) {}
void RenderBackendSetFog(int, uint32_t) {}
void *RenderBackendCreateTexture(int, int, const void *, int) { return nullptr; }
int RenderBackendUpdateTexture(void *, int, int, const void *, int) { return 0; }
void RenderBackendDestroyTexture(void *) {}
void RenderBackendBindTexture(int, void *) {}
int RenderBackendDraw(int, const sRenderBackendVertex *, int, int) { return 0; }
void RenderBackendTraceReset(void) {}
void RenderBackendTrace(const char *, ...) {}
int RenderBackendCaptureComplete(void) { return 0; }
}
