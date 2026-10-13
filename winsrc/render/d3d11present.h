#ifndef __D3D11PRESENT_H
#define __D3D11PRESENT_H

#include <win32_platform.h>

struct IDirectDrawSurface;
struct sRenderBackendVertex;

// Games scale their fixed render canvas to the application window. DromEd
// disables that policy because its canvas follows the editor client area and
// must remain pixel-aligned while the window is being resized.
void D3D11SetScaleToWindow(BOOL enabled);

// DromEd incrementally redraws a shared 2D canvas around its hardware
// viewport. Games instead rebuild their overlay every hardware frame and
// must discard stale software pixels before compositing the D3D11 scene.
void D3D11SetPreserveLegacyCanvas(BOOL enabled);

// Presents the engine's legacy CPU canvas through a native D3D11 swap chain.
// Keeping this at the display boundary lets the 2D UI, movies, and software
// rasterizer continue to use their existing 16-bit canvas while Windows sees
// only a normal 32-bit borderless window.
class cD3D11Presenter {
public:
  cD3D11Presenter();
  ~cD3D11Presenter();

  BOOL Start(HWND hwnd, DWORD sourceWidth, DWORD sourceHeight,
             IDirectDrawSurface *surface);
  void Stop();
  BOOL SetGamma(double gamma);
  BOOL Present(IDirectDrawSurface *surface, int x0, int y0, int x1, int y1);
  BOOL GetClientSize(int *width, int *height) const;
  BOOL ClientToLogicalPoint(int *x, int *y) const;
  BOOL LogicalToClientPoint(int *x, int *y) const;

  BOOL BeginHardwareFrame();
  void EndHardwareFrame();
  void DeactivateScene();
  void ClearDepth();
  void SetDepth(BOOL compareEnabled, BOOL writeEnabled);
  void SetBlend(int blendMode);
  void SetAlphaTest(BOOL enabled);
  void SetSampler(int level, BOOL wrap, BOOL smooth);
  void SetFog(BOOL enabled, DWORD rgb);
  void *CreateTexture(int width, int height, const void *rgba, int rowBytes);
  BOOL UpdateTexture(void *texture, int width, int height, const void *rgba,
                     int rowBytes);
  void DestroyTexture(void *texture);
  void BindTexture(int level, void *texture);
  BOOL Draw(int primitive, const sRenderBackendVertex *vertices, int vertexCount,
            BOOL useSecondTexture);

private:
  struct sImpl;
  static HRESULT CreateRenderTarget(sImpl *impl);
  static HRESULT CreateSceneResources(sImpl *impl, DWORD width, DWORD height);
  BOOL EnsureSourceCapacity(DWORD width, DWORD height);
  BOOL ConfigureLogicalSource(DWORD sourceWidth, DWORD sourceHeight,
                              IDirectDrawSurface *surface);
  BOOL EnsureSceneSize();
  BOOL EnsureOutputSize();
  BOOL ResizeOutput(DWORD width, DWORD height);
  BOOL FlushHardwareCommands();
  sImpl *m_pImpl;

  cD3D11Presenter(const cD3D11Presenter &);
  cD3D11Presenter &operator=(const cD3D11Presenter &);
};

#endif
