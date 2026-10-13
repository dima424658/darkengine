#include "d3d11present.h"
#include "render_backend.h"
#include "d3d11scene.h"
#pragma pack(push, 8)
#include <d3d11.h>
#include <d3dcompiler.h>
#include <ddraw.h>
#include <win32_platform.h>
#pragma pack(pop)
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

template <class T> static void ReleaseInterface(T *&p) {
  if (p) {
    p->Release();
    p = NULL;
  }
}

struct sCompositeConstants {
  float gamma, hasScene, pointSample, padding0;
  float sourceScale[2], padding[2];
  float overlayScale[2], overlayOffset[2];
};
struct sSceneConstants {
  float width, height, fogEnabled, useTexture1;
  float fogColor[4];
  float alphaTest, padding[3];
};
struct sRenderBackendTexture {
  ID3D11Texture2D *texture;
  ID3D11ShaderResourceView *view;
  int width, height;
  sRenderBackendTexture() : texture(NULL), view(NULL), width(0), height(0) {}
};

struct sRenderBackendCommand {
  UINT firstVertex, vertexCount;
  int primitive, blendMode, depthState;
  BOOL useSecondTexture, fogEnabled, alphaTest;
  DWORD fogColor;
  ID3D11ShaderResourceView *textures[2];
  ID3D11SamplerState *samplers[2];
};

static BOOL g_D3D11ScaleToWindow = TRUE;
static BOOL g_D3D11FitToViewport = FALSE;
static BOOL g_D3D11InteractiveResize = FALSE;
static BOOL g_D3D11PreserveLegacyCanvas = FALSE;
static char g_D3D11CapturePath[MAX_PATH] = "";
static int g_D3D11CaptureFrame = 1;
static int g_D3D11HardwareFrameCount = 0;
static BOOL g_D3D11CaptureConfigured = FALSE;
static BOOL g_D3D11CaptureComplete = FALSE;

static RECT ComputePresentationViewport(DWORD sourceWidth, DWORD sourceHeight,
                                        DWORD outputWidth, DWORD outputHeight,
                                        BOOL fitToOutput) {
  RECT viewport = {0, 0, (LONG)outputWidth, (LONG)outputHeight};
  LONG width, height;

  if (!sourceWidth || !sourceHeight || !outputWidth || !outputHeight)
    return viewport;

  if (g_D3D11ScaleToWindow) {
    // Fit is the dynamic-resolution choice: use every available client pixel
    // for both the hardware scene and the final composite. Fixed logical
    // resolutions retain their aspect-preserving presentation below.
    if (fitToOutput)
      return viewport;
    if ((__int64)outputWidth * sourceHeight >
        (__int64)outputHeight * sourceWidth) {
      height = (LONG)outputHeight;
      width = max(1, MulDiv(height, (int)sourceWidth, (int)sourceHeight));
      viewport.left = ((LONG)outputWidth - width) / 2;
      viewport.right = viewport.left + width;
    } else {
      width = (LONG)outputWidth;
      height = max(1, MulDiv(width, (int)sourceHeight, (int)sourceWidth));
      viewport.top = ((LONG)outputHeight - height) / 2;
      viewport.bottom = viewport.top + height;
    }
  } else {
    viewport.right = min((LONG)sourceWidth, (LONG)outputWidth);
    viewport.bottom = min((LONG)sourceHeight, (LONG)outputHeight);
  }
  return viewport;
}

static BOOL TraceEnabled(void) {
  static int enabled = -1;
  if (enabled < 0)
    enabled = GetEnvironmentVariableA("DARK_RENDER_TRACE", NULL, 0) > 0;
  return enabled != 0;
}

static void ConfigureFrameCapture(void) {
  char frame[32];
  if (g_D3D11CaptureConfigured)
    return;
  g_D3D11CaptureConfigured = TRUE;
  GetEnvironmentVariableA("DARK_RENDER_CAPTURE", g_D3D11CapturePath,
                          ARRAYSIZE(g_D3D11CapturePath));
  if (GetEnvironmentVariableA("DARK_RENDER_CAPTURE_FRAME", frame,
                              ARRAYSIZE(frame))) {
    g_D3D11CaptureFrame = atoi(frame);
    if (g_D3D11CaptureFrame < 1)
      g_D3D11CaptureFrame = 1;
  }
}

static BOOL CaptureTextureBmp(ID3D11Device *device,
                              ID3D11DeviceContext *context,
                              ID3D11Texture2D *source, const char *path) {
  D3D11_TEXTURE2D_DESC desc;
  D3D11_MAPPED_SUBRESOURCE mapped;
  ID3D11Texture2D *staging = NULL;
  BITMAPFILEHEADER fileHeader;
  BITMAPINFOHEADER infoHeader;
  FILE *file = NULL;
  UINT x, y;
  BOOL result = FALSE;
  if (!device || !context || !source || !path || !*path)
    return FALSE;
  source->GetDesc(&desc);
  desc.Usage = D3D11_USAGE_STAGING;
  desc.BindFlags = 0;
  desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
  desc.MiscFlags = 0;
  if (FAILED(device->CreateTexture2D(&desc, NULL, &staging)))
    return FALSE;
  context->CopyResource(staging, source);
  if (FAILED(context->Map(staging, 0, D3D11_MAP_READ, 0, &mapped)))
    goto done;
  ZeroMemory(&fileHeader, sizeof(fileHeader));
  ZeroMemory(&infoHeader, sizeof(infoHeader));
  fileHeader.bfType = 0x4d42;
  fileHeader.bfOffBits = sizeof(fileHeader) + sizeof(infoHeader);
  fileHeader.bfSize = fileHeader.bfOffBits + desc.Width * desc.Height * 4;
  infoHeader.biSize = sizeof(infoHeader);
  infoHeader.biWidth = desc.Width;
  infoHeader.biHeight = -(LONG)desc.Height;
  infoHeader.biPlanes = 1;
  infoHeader.biBitCount = 32;
  infoHeader.biCompression = BI_RGB;
  infoHeader.biSizeImage = desc.Width * desc.Height * 4;
  fopen_s(&file, path, "wb");
  if (!file)
    goto unmap;
  fwrite(&fileHeader, sizeof(fileHeader), 1, file);
  fwrite(&infoHeader, sizeof(infoHeader), 1, file);
  for (y = 0; y < desc.Height; ++y) {
    const BYTE *row = (const BYTE *)mapped.pData + y * mapped.RowPitch;
    for (x = 0; x < desc.Width; ++x) {
      BYTE bgra[4] = {row[x * 4 + 2], row[x * 4 + 1], row[x * 4],
                      row[x * 4 + 3]};
      fwrite(bgra, sizeof(bgra), 1, file);
    }
  }
  result = TRUE;
  fclose(file);
  file = NULL;
unmap:
  context->Unmap(staging, 0);
done:
  if (file)
    fclose(file);
  ReleaseInterface(staging);
  return result;
}

extern "C" void RenderBackendTraceReset(void) {
  if (TraceEnabled())
    DeleteFileA("modern_render_trace.log");
}

extern "C" void RenderBackendTrace(const char *format, ...) {
  FILE *file;
  va_list args;
  if (!TraceEnabled())
    return;
  fopen_s(&file, "modern_render_trace.log", "a");
  if (!file)
    return;
  va_start(args, format);
  vfprintf(file, format, args);
  va_end(args);
  fputc('\n', file);
  fclose(file);
}

extern "C" BOOL RenderBackendCaptureComplete(void) {
  return g_D3D11CaptureComplete;
}

struct cD3D11Presenter::sImpl {
  HWND hwnd;
  DWORD sourceWidth, sourceHeight, targetWidth, targetHeight;
  DWORD backBufferWidth, backBufferHeight;
  DWORD sceneWidth, sceneHeight;
  DWORD sourceTextureWidth, sourceTextureHeight;
  IDirectDrawSurface *legacySurface;
  ID3D11Device *device;
  ID3D11DeviceContext *context;
  IDXGISwapChain *swapChain;
  ID3D11RenderTargetView *renderTarget;
  ID3D11Texture2D *sourceTexture, *sceneTexture, *depthTexture;
  ID3D11ShaderResourceView *sourceView, *sceneView;
  ID3D11RenderTargetView *sceneTarget;
  ID3D11DepthStencilView *depthView;
  ID3D11VertexShader *compositeVS, *sceneVS;
  ID3D11PixelShader *compositePS, *scenePS;
  ID3D11SamplerState *compositeSampler, *samplers[4];
  ID3D11Buffer *compositeConstants, *sceneConstants, *sceneVertices;
  ID3D11InputLayout *sceneLayout;
  UINT sceneVertexCapacity;
  ID3D11BlendState *blendStates[4];
  ID3D11DepthStencilState *depthStates[4];
  ID3D11RasterizerState *rasterizer;
  ID3D11ShaderResourceView *boundTextures[2];
  ID3D11SamplerState *boundSamplers[2];
  std::vector<sRenderBackendVertex> queuedVertices;
  std::vector<sRenderBackendCommand> queuedCommands;
  std::vector<DWORD> overlayPixels;
  float gamma;
  BOOL hardwareFrame, sceneActive, sceneOpen, depthCompare, depthWrite,
      fogEnabled, alphaTest, sourceNeedsFullUpload;
  int blendMode;
  DWORD fogColor;
  sImpl()
      : hwnd(NULL), sourceWidth(0), sourceHeight(0), targetWidth(0),
        targetHeight(0), backBufferWidth(0), backBufferHeight(0),
        sceneWidth(0), sceneHeight(0),
        sourceTextureWidth(0), sourceTextureHeight(0), legacySurface(NULL),
        device(NULL), context(NULL), swapChain(NULL), renderTarget(NULL),
        sourceTexture(NULL), sceneTexture(NULL), depthTexture(NULL),
        sourceView(NULL), sceneView(NULL), sceneTarget(NULL), depthView(NULL),
        compositeVS(NULL), sceneVS(NULL), compositePS(NULL), scenePS(NULL),
        compositeSampler(NULL), compositeConstants(NULL), sceneConstants(NULL),
        sceneVertices(NULL), sceneLayout(NULL), sceneVertexCapacity(0),
        rasterizer(NULL), gamma(1), hardwareFrame(FALSE), sceneActive(FALSE),
        sceneOpen(FALSE), depthCompare(FALSE), depthWrite(FALSE),
        fogEnabled(FALSE), alphaTest(FALSE), sourceNeedsFullUpload(FALSE),
        blendMode(kRenderBackendBlendOpaque), fogColor(0) {
    ZeroMemory(samplers, sizeof(samplers));
    ZeroMemory(blendStates, sizeof(blendStates));
    ZeroMemory(depthStates, sizeof(depthStates));
    ZeroMemory(boundTextures, sizeof(boundTextures));
    ZeroMemory(boundSamplers, sizeof(boundSamplers));
  }
  void ReleaseQueuedCommands() {
    size_t i;
    for (i = 0; i < queuedCommands.size(); ++i) {
      ReleaseInterface(queuedCommands[i].textures[0]);
      ReleaseInterface(queuedCommands[i].textures[1]);
    }
    queuedCommands.clear();
    queuedVertices.clear();
  }
};

static cD3D11Presenter *g_pPresenter = NULL;
static HRESULT CompileShader(const char *s, const char *entry,
                             const char *target, ID3DBlob **blob) {
  ID3DBlob *errors = NULL;
  HRESULT hr = D3DCompile(s, strlen(s), NULL, NULL, NULL, entry, target,
                          D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, blob, &errors);
  if (FAILED(hr) && errors)
    OutputDebugStringA((const char *)errors->GetBufferPointer());
  ReleaseInterface(errors);
  return hr;
}
static HRESULT CreateDynamicBuffer(ID3D11Device *d, UINT bytes, UINT bind,
                                   ID3D11Buffer **out) {
  D3D11_BUFFER_DESC x;
  ZeroMemory(&x, sizeof(x));
  x.ByteWidth = bytes;
  x.Usage = D3D11_USAGE_DYNAMIC;
  x.BindFlags = bind;
  x.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
  return d->CreateBuffer(&x, NULL, out);
}

cD3D11Presenter::cD3D11Presenter() : m_pImpl(new sImpl) {}
cD3D11Presenter::~cD3D11Presenter() {
  Stop();
  delete m_pImpl;
}
void cD3D11Presenter::Stop() {
  int i;
  if (!m_pImpl)
    return;
  if (g_pPresenter == this)
    g_pPresenter = NULL;
  m_pImpl->ReleaseQueuedCommands();
  if (m_pImpl->context)
    m_pImpl->context->ClearState();
  for (i = 0; i < 4; ++i) {
    ReleaseInterface(m_pImpl->blendStates[i]);
    ReleaseInterface(m_pImpl->depthStates[i]);
    ReleaseInterface(m_pImpl->samplers[i]);
  }
  ReleaseInterface(m_pImpl->rasterizer);
  ReleaseInterface(m_pImpl->sceneConstants);
  ReleaseInterface(m_pImpl->sceneVertices);
  ReleaseInterface(m_pImpl->sceneLayout);
  ReleaseInterface(m_pImpl->scenePS);
  ReleaseInterface(m_pImpl->sceneVS);
  ReleaseInterface(m_pImpl->compositeConstants);
  ReleaseInterface(m_pImpl->compositeSampler);
  ReleaseInterface(m_pImpl->compositePS);
  ReleaseInterface(m_pImpl->compositeVS);
  ReleaseInterface(m_pImpl->depthView);
  ReleaseInterface(m_pImpl->depthTexture);
  ReleaseInterface(m_pImpl->sceneTarget);
  ReleaseInterface(m_pImpl->sceneView);
  ReleaseInterface(m_pImpl->sceneTexture);
  ReleaseInterface(m_pImpl->sourceView);
  ReleaseInterface(m_pImpl->sourceTexture);
  ReleaseInterface(m_pImpl->renderTarget);
  ReleaseInterface(m_pImpl->swapChain);
  ReleaseInterface(m_pImpl->context);
  ReleaseInterface(m_pImpl->device);
  ReleaseInterface(m_pImpl->legacySurface);
  m_pImpl->hwnd = NULL;
  m_pImpl->sourceWidth = m_pImpl->sourceHeight = 0;
  m_pImpl->targetWidth = m_pImpl->targetHeight = 0;
  m_pImpl->backBufferWidth = m_pImpl->backBufferHeight = 0;
  m_pImpl->sceneWidth = m_pImpl->sceneHeight = 0;
  m_pImpl->sourceTextureWidth = m_pImpl->sourceTextureHeight = 0;
  m_pImpl->hardwareFrame = m_pImpl->sceneActive = m_pImpl->sceneOpen = FALSE;
  m_pImpl->sourceNeedsFullUpload = FALSE;
  m_pImpl->sceneVertexCapacity = 0;
  m_pImpl->overlayPixels.clear();
  m_pImpl->depthCompare = m_pImpl->depthWrite = FALSE;
  m_pImpl->fogEnabled = m_pImpl->alphaTest = FALSE;
  m_pImpl->blendMode = kRenderBackendBlendOpaque;
  m_pImpl->fogColor = 0;
  ZeroMemory(m_pImpl->boundTextures, sizeof(m_pImpl->boundTextures));
  ZeroMemory(m_pImpl->boundSamplers, sizeof(m_pImpl->boundSamplers));
}
HRESULT cD3D11Presenter::CreateRenderTarget(sImpl *p) {
  ID3D11Texture2D *b = NULL;
  D3D11_TEXTURE2D_DESC desc;
  HRESULT hr =
      p->swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void **)&b);
  if (SUCCEEDED(hr)) {
    b->GetDesc(&desc);
    p->backBufferWidth = desc.Width;
    p->backBufferHeight = desc.Height;
    hr = p->device->CreateRenderTargetView(b, NULL, &p->renderTarget);
  }
  ReleaseInterface(b);
  return hr;
}

HRESULT cD3D11Presenter::CreateSceneResources(sImpl *p, DWORD width,
                                              DWORD height) {
  ID3D11Texture2D *sceneTexture = NULL;
  ID3D11RenderTargetView *sceneTarget = NULL;
  ID3D11ShaderResourceView *sceneView = NULL;
  ID3D11Texture2D *depthTexture = NULL;
  ID3D11DepthStencilView *depthView = NULL;
  D3D11_TEXTURE2D_DESC td;
  HRESULT hr = E_FAIL;

  if (!p || !p->device || !width || !height)
    return E_INVALIDARG;

  ZeroMemory(&td, sizeof(td));
  td.Width = width;
  td.Height = height;
  td.MipLevels = td.ArraySize = 1;
  td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  td.SampleDesc.Count = 1;
  td.Usage = D3D11_USAGE_DEFAULT;
  td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
  if (FAILED(hr = p->device->CreateTexture2D(&td, NULL, &sceneTexture)) ||
      FAILED(hr = p->device->CreateRenderTargetView(sceneTexture, NULL,
                                                    &sceneTarget)) ||
      FAILED(hr = p->device->CreateShaderResourceView(sceneTexture, NULL,
                                                      &sceneView)))
    goto fail;

  td.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
  td.BindFlags = D3D11_BIND_DEPTH_STENCIL;
  if (FAILED(hr = p->device->CreateTexture2D(&td, NULL, &depthTexture)) ||
      FAILED(hr = p->device->CreateDepthStencilView(depthTexture, NULL,
                                                    &depthView)))
    goto fail;

  // The old scene may still be bound by either the hardware pass or the
  // composite pass. Unbind it before replacing the logical-resolution
  // resources, but leave the independent swap-chain target alone.
  {
    ID3D11ShaderResourceView *nulls[2] = {NULL, NULL};
    p->context->PSSetShaderResources(0, 2, nulls);
    p->context->OMSetRenderTargets(0, NULL, NULL);
  }
  ReleaseInterface(p->depthView);
  ReleaseInterface(p->depthTexture);
  ReleaseInterface(p->sceneTarget);
  ReleaseInterface(p->sceneView);
  ReleaseInterface(p->sceneTexture);

  p->sceneTexture = sceneTexture;
  p->sceneTarget = sceneTarget;
  p->sceneView = sceneView;
  p->depthTexture = depthTexture;
  p->depthView = depthView;
  p->sceneWidth = width;
  p->sceneHeight = height;
  return S_OK;

fail:
  ReleaseInterface(depthView);
  ReleaseInterface(depthTexture);
  ReleaseInterface(sceneView);
  ReleaseInterface(sceneTarget);
  ReleaseInterface(sceneTexture);
  return hr;
}

BOOL cD3D11Presenter::EnsureSourceCapacity(DWORD width, DWORD height) {
  ID3D11Texture2D *sourceTexture = NULL;
  ID3D11ShaderResourceView *sourceView = NULL;
  ID3D11ShaderResourceView *nulls[2] = {NULL, NULL};
  D3D11_TEXTURE2D_DESC td;

  if (!m_pImpl || !m_pImpl->device || !width || !height)
    return FALSE;
  width = max(width, m_pImpl->sourceTextureWidth);
  height = max(height, m_pImpl->sourceTextureHeight);
  if (m_pImpl->sourceTexture && width == m_pImpl->sourceTextureWidth &&
      height == m_pImpl->sourceTextureHeight)
    return TRUE;

  ZeroMemory(&td, sizeof(td));
  td.Width = width;
  td.Height = height;
  td.MipLevels = td.ArraySize = 1;
  td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  td.SampleDesc.Count = 1;
  td.Usage = D3D11_USAGE_DEFAULT;
  td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
  if (FAILED(m_pImpl->device->CreateTexture2D(&td, NULL, &sourceTexture)) ||
      FAILED(m_pImpl->device->CreateShaderResourceView(sourceTexture, NULL,
                                                       &sourceView)))
    goto fail;

  m_pImpl->context->PSSetShaderResources(0, 2, nulls);
  ReleaseInterface(m_pImpl->sourceView);
  ReleaseInterface(m_pImpl->sourceTexture);
  m_pImpl->sourceTexture = sourceTexture;
  m_pImpl->sourceView = sourceView;
  m_pImpl->sourceTextureWidth = width;
  m_pImpl->sourceTextureHeight = height;
  m_pImpl->sourceNeedsFullUpload = TRUE;
  return TRUE;

fail:
  ReleaseInterface(sourceView);
  ReleaseInterface(sourceTexture);
  return FALSE;
}

BOOL cD3D11Presenter::ConfigureLogicalSource(DWORD sw, DWORD sh,
                                             IDirectDrawSurface *surface) {
  std::vector<DWORD> overlayPixels;

  if (!m_pImpl || !m_pImpl->device || !surface || !sw || !sh)
    return FALSE;
  m_pImpl->ReleaseQueuedCommands();
  if (!EnsureSourceCapacity(sw, sh))
    return FALSE;

  overlayPixels.assign(sw * sh, 0);
  ReleaseInterface(m_pImpl->legacySurface);
  m_pImpl->legacySurface = surface;
  surface->AddRef();
  m_pImpl->sourceWidth = sw;
  m_pImpl->sourceHeight = sh;
  m_pImpl->overlayPixels.swap(overlayPixels);
  if (!EnsureSceneSize())
    return FALSE;
  m_pImpl->hardwareFrame = FALSE;
  m_pImpl->sceneActive = FALSE;
  m_pImpl->sceneOpen = FALSE;
  m_pImpl->sourceNeedsFullUpload = TRUE;
  RenderBackendTrace("logical-source %lux%lu output=%lux%lu scale=%d fit=%d preserve=%d",
                   sw, sh, m_pImpl->backBufferWidth,
                   m_pImpl->backBufferHeight, g_D3D11ScaleToWindow,
                   g_D3D11FitToViewport,
                   g_D3D11PreserveLegacyCanvas);
  // Logical canvas changes do not invalidate model textures.  Keep the
  // desired bindings in sync with lgd3d's texture-id cache; clearing them
  // here makes the first objects after a menu-to-game transition sample a
  // null SRV when lgd3d correctly decides the texture ID has not changed.
  return TRUE;
}

BOOL cD3D11Presenter::EnsureSceneSize() {
  DWORD width, height;

  if (!m_pImpl || !m_pImpl->device || !m_pImpl->sourceWidth ||
      !m_pImpl->sourceHeight)
    return FALSE;

  width = m_pImpl->sourceWidth;
  height = m_pImpl->sourceHeight;
  if (g_D3D11ScaleToWindow && g_D3D11FitToViewport &&
      m_pImpl->backBufferWidth && m_pImpl->backBufferHeight) {
    RECT viewport = ComputePresentationViewport(
        m_pImpl->sourceWidth, m_pImpl->sourceHeight,
        m_pImpl->backBufferWidth, m_pImpl->backBufferHeight, TRUE);
    width = viewport.right - viewport.left;
    height = viewport.bottom - viewport.top;
  }

  if (width == m_pImpl->sceneWidth && height == m_pImpl->sceneHeight)
    return TRUE;
  // While the user drags a window edge, scale the last completed scene.
  // Reallocate the single scene/depth pair once WM_EXITSIZEMOVE arrives.
  if (g_D3D11InteractiveResize && m_pImpl->sceneTexture)
    return TRUE;
  m_pImpl->ReleaseQueuedCommands();
  if (FAILED(CreateSceneResources(m_pImpl, width, height)))
    return FALSE;
  RenderBackendTrace(
      "scene-size source=%lux%lu scene=%lux%lu output=%lux%lu scale=%d fit=%d",
      m_pImpl->sourceWidth, m_pImpl->sourceHeight, width, height,
      m_pImpl->backBufferWidth, m_pImpl->backBufferHeight,
      g_D3D11ScaleToWindow, g_D3D11FitToViewport);
  m_pImpl->hardwareFrame = FALSE;
  m_pImpl->sceneActive = FALSE;
  m_pImpl->sceneOpen = FALSE;
  return TRUE;
}

BOOL cD3D11Presenter::ResizeOutput(DWORD width, DWORD height) {
  ID3D11ShaderResourceView *nulls[2] = {NULL, NULL};
  HRESULT hr;

  if (!m_pImpl || !m_pImpl->swapChain || !width || !height)
    return FALSE;
  if (width == m_pImpl->targetWidth && height == m_pImpl->targetHeight)
    return TRUE;

  m_pImpl->ReleaseQueuedCommands();
  m_pImpl->context->PSSetShaderResources(0, 2, nulls);
  m_pImpl->context->OMSetRenderTargets(0, NULL, NULL);
  ReleaseInterface(m_pImpl->renderTarget);

  hr = m_pImpl->swapChain->ResizeBuffers(1, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
  if (FAILED(hr) || FAILED(CreateRenderTarget(m_pImpl)))
  {
    m_pImpl->targetWidth = m_pImpl->targetHeight = 0;
    return FALSE;
  }

  m_pImpl->targetWidth = width;
  m_pImpl->targetHeight = height;
  m_pImpl->hardwareFrame = FALSE;
  m_pImpl->sceneOpen = FALSE;
  return EnsureSceneSize();
}

BOOL cD3D11Presenter::EnsureOutputSize() {
  RECT rc;
  DWORD width, height;
  if (!m_pImpl || !m_pImpl->hwnd)
    return FALSE;
  GetClientRect(m_pImpl->hwnd, &rc);
  width = rc.right - rc.left;
  height = rc.bottom - rc.top;
  if (!width || !height)
    return FALSE;
  if (width != m_pImpl->targetWidth || height != m_pImpl->targetHeight)
    return ResizeOutput(width, height);
  return EnsureSceneSize();
}

BOOL cD3D11Presenter::Start(HWND hwnd, DWORD sw, DWORD sh,
                            IDirectDrawSurface *surface) {
  static const char cvs[] =
      "struct O{float4 p:SV_Position;float2 u:TEXCOORD0;};O main(uint "
      "i:SV_VertexID){O o;float2 "
      "u=float2((i<<1)&2,i&2);o.p=float4(u.x*2-1,1-u.y*2,0,1);o.u=u;return o;}";
  static const char cps[] =
      "cbuffer C:register(b0){float g;float hs;float pp;float z;float2 us;"
      "float2 zz;float2 os;float2 oo;}Texture2D "
      "ui:register(t0);Texture2D sc:register(t1);SamplerState "
      "s:register(s0);float4 main(float4 p:SV_Position,float2 "
      "u:TEXCOORD0):SV_Target{float4 a;float3 b;if(pp>.5){int3 "
      "q=int3(int2(p.xy),0);a=ui.Load(q);b=sc.Load(q).rgb;}else{"
      "float2 au=(u-oo)*os;float ai=step(0,au.x)*step(au.x,1)*"
      "step(0,au.y)*step(au.y,1);a=ui.Sample(s,saturate(au)*us)*ai;"
      "b=sc.Sample(s,u).rgb;}float3 "
      "c=a.rgb;if(hs>.5)c=lerp(b,a.rgb,a.a);return "
      "float4(pow(saturate(c),g),1);}";
  RECT rc;
  DXGI_SWAP_CHAIN_DESC sd;
  D3D_FEATURE_LEVEL levels[] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1,
                                D3D_FEATURE_LEVEL_10_0},
                    got;
  D3D11_SAMPLER_DESC samp;
  D3D11_BUFFER_DESC bd;
  D3D11_BLEND_DESC bl;
  D3D11_DEPTH_STENCIL_DESC ds;
  D3D11_RASTERIZER_DESC rs;
  ID3DBlob *vb = NULL, *pb = NULL;
  HRESULT hr;
  DWORD clientWidth, clientHeight;
  DWORD sceneWidth, sceneHeight;
  RECT sceneViewport;
  int i;
  D3D11_INPUT_ELEMENT_DESC il[] = {
      {"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0,
       D3D11_INPUT_PER_VERTEX_DATA, 0},
      {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16,
       D3D11_INPUT_PER_VERTEX_DATA, 0},
      {"FOG", 0, DXGI_FORMAT_R32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0},
      {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 36,
       D3D11_INPUT_PER_VERTEX_DATA, 0},
      {"TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 0, 44,
       D3D11_INPUT_PER_VERTEX_DATA, 0}};
  if (!hwnd || !sw || !sh || !surface)
    return FALSE;
  // A menu/movie/game resolution is a logical canvas and scene-size change.
  // Keep the device and swap chain alive; only logical resources are replaced.
  if (m_pImpl->device && m_pImpl->swapChain && m_pImpl->hwnd == hwnd) {
    if (ConfigureLogicalSource(sw, sh, surface))
      return TRUE;
  }
  RenderBackendTrace("presenter-start requested=%lux%lu scale=%d preserve=%d",
                   sw, sh, g_D3D11ScaleToWindow,
                   g_D3D11PreserveLegacyCanvas);
  Stop();
  m_pImpl->overlayPixels.assign(sw * sh, 0);
  // A typical mission produces tens of thousands of expanded triangle
  // vertices.  Retain that storage between frames instead of repeatedly
  // growing the command streams during the first frames of play.
  m_pImpl->queuedVertices.reserve(65536);
  m_pImpl->queuedCommands.reserve(2048);
  GetClientRect(hwnd, &rc);
  clientWidth = rc.right - rc.left;
  clientHeight = rc.bottom - rc.top;
  ZeroMemory(&sd, sizeof(sd));
  // Zero dimensions tell DXGI to create the swap-chain buffers at the HWND's
  // actual client-pixel size.  Passing legacy/logical dimensions here can
  // create an undersized render target on a scaled desktop.
  sd.BufferDesc.Width = 0;
  sd.BufferDesc.Height = 0;
  sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  sd.SampleDesc.Count = 1;
  sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  sd.BufferCount = 1;
  sd.OutputWindow = hwnd;
  sd.Windowed = TRUE;
  sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
  hr = D3D11CreateDeviceAndSwapChain(
      NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, D3D11_CREATE_DEVICE_BGRA_SUPPORT,
      levels, 3, D3D11_SDK_VERSION, &sd, &m_pImpl->swapChain, &m_pImpl->device,
      &got, &m_pImpl->context);
  if (FAILED(hr))
    hr = D3D11CreateDeviceAndSwapChain(
        NULL, D3D_DRIVER_TYPE_WARP, NULL, D3D11_CREATE_DEVICE_BGRA_SUPPORT,
        levels, 3, D3D11_SDK_VERSION, &sd, &m_pImpl->swapChain,
        &m_pImpl->device, &got, &m_pImpl->context);
  if (FAILED(hr) || FAILED(CreateRenderTarget(m_pImpl)) ||
      !EnsureSourceCapacity(sw, sh))
    goto fail;
  sceneWidth = sw;
  sceneHeight = sh;
  if (g_D3D11ScaleToWindow && g_D3D11FitToViewport) {
    sceneViewport = ComputePresentationViewport(
        sw, sh, m_pImpl->backBufferWidth, m_pImpl->backBufferHeight, TRUE);
    sceneWidth = sceneViewport.right - sceneViewport.left;
    sceneHeight = sceneViewport.bottom - sceneViewport.top;
  }
  if (FAILED(CreateSceneResources(m_pImpl, sceneWidth, sceneHeight)))
    goto fail;
  if (FAILED(CompileShader(cvs, "main", "vs_4_0", &vb)) ||
      FAILED(m_pImpl->device->CreateVertexShader(vb->GetBufferPointer(),
                                                 vb->GetBufferSize(), NULL,
                                                 &m_pImpl->compositeVS)))
    goto fail;
  ReleaseInterface(vb);
  if (FAILED(CompileShader(cps, "main", "ps_4_0", &pb)) ||
      FAILED(m_pImpl->device->CreatePixelShader(pb->GetBufferPointer(),
                                                pb->GetBufferSize(), NULL,
                                                &m_pImpl->compositePS)))
    goto fail;
  ReleaseInterface(pb);
  if (FAILED(CompileShader(kD3D11SceneVS, "main", "vs_4_0", &vb)) ||
      FAILED(m_pImpl->device->CreateVertexShader(vb->GetBufferPointer(),
                                                 vb->GetBufferSize(), NULL,
                                                 &m_pImpl->sceneVS)) ||
      FAILED(m_pImpl->device->CreateInputLayout(il, 5, vb->GetBufferPointer(),
                                                vb->GetBufferSize(),
                                                &m_pImpl->sceneLayout)))
    goto fail;
  ReleaseInterface(vb);
  if (FAILED(CompileShader(kD3D11ScenePS, "main", "ps_4_0", &pb)) ||
      FAILED(m_pImpl->device->CreatePixelShader(pb->GetBufferPointer(),
                                                pb->GetBufferSize(), NULL,
                                                &m_pImpl->scenePS)))
    goto fail;
  ReleaseInterface(pb);
  ZeroMemory(&samp, sizeof(samp));
  samp.MaxLOD = D3D11_FLOAT32_MAX;
  for (i = 0; i < 4; ++i) {
    samp.Filter = (i & 1) ? D3D11_FILTER_MIN_MAG_MIP_LINEAR
                          : D3D11_FILTER_MIN_MAG_MIP_POINT;
    samp.AddressU = samp.AddressV = samp.AddressW =
        (i & 2) ? D3D11_TEXTURE_ADDRESS_WRAP : D3D11_TEXTURE_ADDRESS_CLAMP;
    if (FAILED(
            m_pImpl->device->CreateSamplerState(&samp, &m_pImpl->samplers[i])))
      goto fail;
  }
  samp.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
  samp.AddressU = samp.AddressV = samp.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
  if (FAILED(m_pImpl->device->CreateSamplerState(&samp,
                                                 &m_pImpl->compositeSampler)))
    goto fail;
  m_pImpl->boundSamplers[0] = m_pImpl->samplers[1];
  m_pImpl->boundSamplers[1] = m_pImpl->samplers[1];
  for (i = 0; i < 4; ++i) {
    D3D11DescribeBlend(i, &bl);
    if (FAILED(m_pImpl->device->CreateBlendState(&bl,
                                                 &m_pImpl->blendStates[i])))
      goto fail;
  }
  for (i = 0; i < 4; ++i) {
    ZeroMemory(&ds, sizeof(ds));
    ds.DepthEnable = i != 0;
    ds.DepthWriteMask =
        (i & 2) ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
    ds.DepthFunc =
        (i & 1) ? D3D11_COMPARISON_LESS_EQUAL : D3D11_COMPARISON_ALWAYS;
    if (FAILED(m_pImpl->device->CreateDepthStencilState(
            &ds, &m_pImpl->depthStates[i])))
      goto fail;
  }
  ZeroMemory(&rs, sizeof(rs));
  rs.FillMode = D3D11_FILL_SOLID;
  rs.CullMode = D3D11_CULL_NONE;
  rs.DepthClipEnable = TRUE;
  if (FAILED(m_pImpl->device->CreateRasterizerState(&rs, &m_pImpl->rasterizer)))
    goto fail;
  ZeroMemory(&bd, sizeof(bd));
  bd.Usage = D3D11_USAGE_DEFAULT;
  bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
  bd.ByteWidth = sizeof(sCompositeConstants);
  if (FAILED(m_pImpl->device->CreateBuffer(&bd, NULL,
                                           &m_pImpl->compositeConstants)))
    goto fail;
  bd.ByteWidth = sizeof(sSceneConstants);
  if (FAILED(
          m_pImpl->device->CreateBuffer(&bd, NULL, &m_pImpl->sceneConstants)))
    goto fail;
  m_pImpl->sceneVertexCapacity = 4096;
  if (FAILED(CreateDynamicBuffer(
          m_pImpl->device,
          m_pImpl->sceneVertexCapacity * sizeof(sRenderBackendVertex),
          D3D11_BIND_VERTEX_BUFFER, &m_pImpl->sceneVertices)))
    goto fail;
  m_pImpl->hwnd = hwnd;
  m_pImpl->sourceWidth = sw;
  m_pImpl->sourceHeight = sh;
  m_pImpl->targetWidth = clientWidth;
  m_pImpl->targetHeight = clientHeight;
  m_pImpl->legacySurface = surface;
  surface->AddRef();
  // The main window is still hidden on initial startup.  Give DWM a defined
  // black swap-chain image before the display provider reveals the HWND, so
  // no class/default surface can appear between desktop and the first movie.
  {
    float black[4] = {0, 0, 0, 1};
    m_pImpl->context->OMSetRenderTargets(1, &m_pImpl->renderTarget, NULL);
    m_pImpl->context->ClearRenderTargetView(m_pImpl->renderTarget, black);
    m_pImpl->swapChain->Present(0, 0);
  }
  g_pPresenter = this;
  return TRUE;
fail:
  ReleaseInterface(vb);
  ReleaseInterface(pb);
  Stop();
  return FALSE;
}

BOOL cD3D11Presenter::SetGamma(double g) {
  if (!m_pImpl || g <= 0)
    return FALSE;
  m_pImpl->gamma = (float)g;
  return TRUE;
}

BOOL cD3D11Presenter::GetClientSize(int *width, int *height) const {
  RECT client;

  if (!m_pImpl || !m_pImpl->hwnd || !width || !height ||
      !GetClientRect(m_pImpl->hwnd, &client))
    return FALSE;
  *width = client.right - client.left;
  *height = client.bottom - client.top;
  return *width > 0 && *height > 0;
}

BOOL cD3D11Presenter::ClientToLogicalPoint(int *x, int *y) const {
  RECT client, viewport;
  int width, height, localX, localY;

  if (!m_pImpl || !m_pImpl->hwnd || !m_pImpl->sourceWidth ||
      !m_pImpl->sourceHeight || !x || !y ||
      !GetClientRect(m_pImpl->hwnd, &client))
    return FALSE;
  viewport = ComputePresentationViewport(
      m_pImpl->sourceWidth, m_pImpl->sourceHeight,
      client.right - client.left, client.bottom - client.top, FALSE);
  width = viewport.right - viewport.left;
  height = viewport.bottom - viewport.top;
  if (!width || !height)
    return FALSE;

  localX = *x - viewport.left;
  localY = *y - viewport.top;
  // Mouselook warps to the logical center. Preserve that exact fixed point
  // even when an odd-sized client viewport cannot round-trip by division.
  *x = localX == width / 2
           ? (int)m_pImpl->sourceWidth / 2
           : MulDiv(localX, (int)m_pImpl->sourceWidth, width);
  *y = localY == height / 2
           ? (int)m_pImpl->sourceHeight / 2
           : MulDiv(localY, (int)m_pImpl->sourceHeight, height);
  return TRUE;
}

BOOL cD3D11Presenter::LogicalToClientPoint(int *x, int *y) const {
  RECT client, viewport;
  int width, height;

  if (!m_pImpl || !m_pImpl->hwnd || !m_pImpl->sourceWidth ||
      !m_pImpl->sourceHeight || !x || !y ||
      !GetClientRect(m_pImpl->hwnd, &client))
    return FALSE;
  viewport = ComputePresentationViewport(
      m_pImpl->sourceWidth, m_pImpl->sourceHeight,
      client.right - client.left, client.bottom - client.top, FALSE);
  width = viewport.right - viewport.left;
  height = viewport.bottom - viewport.top;
  if (!width || !height)
    return FALSE;

  *x = viewport.left +
       (*x == (int)m_pImpl->sourceWidth / 2
            ? width / 2
            : MulDiv(*x, width, (int)m_pImpl->sourceWidth));
  *y = viewport.top +
       (*y == (int)m_pImpl->sourceHeight / 2
            ? height / 2
            : MulDiv(*y, height, (int)m_pImpl->sourceHeight));
  return TRUE;
}

static BYTE ExpandChannel(DWORD p, DWORD m) {
  DWORD s = 0, n;
  if (!m)
    return 0;
  while (!(m & 1)) {
    m >>= 1;
    ++s;
  }
  n = m;
  return (BYTE)((((p >> s) & n) * 255 + n / 2) / n);
}
BOOL cD3D11Presenter::BeginHardwareFrame() {
  D3D11_VIEWPORT v;
  ID3D11ShaderResourceView *nulls[2] = {NULL, NULL};
  float c[4] = {0, 0, 0, 1};
  if (!m_pImpl || !m_pImpl->device || !EnsureOutputSize())
    return FALSE;
  if (!m_pImpl->hardwareFrame) {
    m_pImpl->ReleaseQueuedCommands();
    if (!g_D3D11PreserveLegacyCanvas && m_pImpl->legacySurface) {
      DDBLTFX f;
      ZeroMemory(&f, sizeof(f));
      f.dwSize = sizeof(f);
      m_pImpl->legacySurface->Blt(NULL, NULL, NULL,
                                  DDBLT_COLORFILL | DDBLT_WAIT, &f);
    }
    m_pImpl->context->PSSetShaderResources(0, 2, nulls);
    m_pImpl->context->ClearRenderTargetView(m_pImpl->sceneTarget, c);
    m_pImpl->context->ClearDepthStencilView(
        m_pImpl->depthView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1, 0);
    m_pImpl->hardwareFrame = TRUE;
    m_pImpl->sceneActive = TRUE;
  }
  ZeroMemory(&v, sizeof(v));
  // Fixed modes follow the selected logical resolution. Fit mode follows the
  // available client viewport, and the composite pass uses that same area.
  v.Width = (float)m_pImpl->sceneWidth;
  v.Height = (float)m_pImpl->sceneHeight;
  v.MaxDepth = 1;
  m_pImpl->context->OMSetRenderTargets(1, &m_pImpl->sceneTarget,
                                       m_pImpl->depthView);
  m_pImpl->context->RSSetViewports(1, &v);
  m_pImpl->context->RSSetState(m_pImpl->rasterizer);
  m_pImpl->sceneOpen = TRUE;
  return TRUE;
}
void cD3D11Presenter::EndHardwareFrame() {
  if (m_pImpl) {
    FlushHardwareCommands();
    m_pImpl->sceneOpen = FALSE;
  }
}
void cD3D11Presenter::DeactivateScene() {
  if (!m_pImpl)
    return;
  m_pImpl->ReleaseQueuedCommands();
  m_pImpl->hardwareFrame = FALSE;
  m_pImpl->sceneActive = FALSE;
  m_pImpl->sceneOpen = FALSE;
}
void cD3D11Presenter::ClearDepth() {
  if (m_pImpl && m_pImpl->depthView) {
    FlushHardwareCommands();
    m_pImpl->context->ClearDepthStencilView(m_pImpl->depthView,
                                            D3D11_CLEAR_DEPTH, 1, 0);
  }
}
void cD3D11Presenter::SetDepth(BOOL c, BOOL w) {
  m_pImpl->depthCompare = c;
  m_pImpl->depthWrite = w;
}
void cD3D11Presenter::SetBlend(int m) {
  if (m >= 0 && m < 4)
    m_pImpl->blendMode = m;
}
void cD3D11Presenter::SetAlphaTest(BOOL enabled) {
  if (m_pImpl)
    m_pImpl->alphaTest = enabled;
}
void cD3D11Presenter::SetSampler(int l, BOOL w, BOOL s) {
  if (l >= 0 && l < 2)
    m_pImpl->boundSamplers[l] = m_pImpl->samplers[(w ? 2 : 0) | (s ? 1 : 0)];
}
void cD3D11Presenter::SetFog(BOOL e, DWORD c) {
  m_pImpl->fogEnabled = e;
  m_pImpl->fogColor = c;
}
void *cD3D11Presenter::CreateTexture(int w, int h, const void *p, int row) {
  sRenderBackendTexture *t = new sRenderBackendTexture;
  D3D11_TEXTURE2D_DESC d;
  D3D11_SUBRESOURCE_DATA x;
  ZeroMemory(&d, sizeof(d));
  d.Width = w;
  d.Height = h;
  d.MipLevels = d.ArraySize = 1;
  d.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  d.SampleDesc.Count = 1;
  d.Usage = D3D11_USAGE_DEFAULT;
  d.BindFlags = D3D11_BIND_SHADER_RESOURCE;
  ZeroMemory(&x, sizeof(x));
  x.pSysMem = p;
  x.SysMemPitch = row;
  if (FAILED(m_pImpl->device->CreateTexture2D(&d, &x, &t->texture)) ||
      FAILED(m_pImpl->device->CreateShaderResourceView(t->texture, NULL,
                                                       &t->view))) {
    ReleaseInterface(t->view);
    ReleaseInterface(t->texture);
    delete t;
    return NULL;
  }
  t->width = w;
  t->height = h;
  return t;
}
BOOL cD3D11Presenter::UpdateTexture(void *h, int w, int he, const void *p,
                                    int row) {
  sRenderBackendTexture *t = (sRenderBackendTexture *)h;
  if (!t || t->width != w || t->height != he)
    return FALSE;
  FlushHardwareCommands();
  m_pImpl->context->UpdateSubresource(t->texture, 0, NULL, p, row, 0);
  return TRUE;
}
void cD3D11Presenter::DestroyTexture(void *h) {
  sRenderBackendTexture *t = (sRenderBackendTexture *)h;
  int i;
  if (!t)
    return;
  FlushHardwareCommands();
  for (i = 0; i < 2; ++i)
    if (m_pImpl->boundTextures[i] == t->view)
      m_pImpl->boundTextures[i] = NULL;
  ReleaseInterface(t->view);
  ReleaseInterface(t->texture);
  delete t;
}
void cD3D11Presenter::BindTexture(int l, void *h) {
  sRenderBackendTexture *t = (sRenderBackendTexture *)h;
  if (l >= 0 && l < 2)
    m_pImpl->boundTextures[l] = t ? t->view : NULL;
}
BOOL cD3D11Presenter::Draw(int prim, const sRenderBackendVertex *v, int n,
                           BOOL second) {
  sRenderBackendCommand cmd;
  int di = (m_pImpl->depthCompare ? 1 : 0) | (m_pImpl->depthWrite ? 2 : 0);
  if (!m_pImpl->sceneOpen || !v || n <= 0)
    return FALSE;
  ZeroMemory(&cmd, sizeof(cmd));
  cmd.firstVertex = (UINT)m_pImpl->queuedVertices.size();
  cmd.vertexCount = n;
  cmd.primitive = prim;
  cmd.blendMode = m_pImpl->blendMode;
  cmd.depthState = di;
  cmd.useSecondTexture = second;
  cmd.fogEnabled = m_pImpl->fogEnabled;
  cmd.alphaTest = m_pImpl->alphaTest;
  cmd.fogColor = m_pImpl->fogColor;
  cmd.textures[0] = m_pImpl->boundTextures[0];
  cmd.textures[1] = m_pImpl->boundTextures[1];
  cmd.samplers[0] = m_pImpl->boundSamplers[0];
  cmd.samplers[1] = m_pImpl->boundSamplers[1];
  m_pImpl->queuedVertices.insert(m_pImpl->queuedVertices.end(), v, v + n);
  if (!m_pImpl->queuedCommands.empty()) {
    sRenderBackendCommand &last = m_pImpl->queuedCommands.back();
    if (last.firstVertex + last.vertexCount == cmd.firstVertex &&
        last.primitive == cmd.primitive && last.blendMode == cmd.blendMode &&
        last.depthState == cmd.depthState &&
        last.useSecondTexture == cmd.useSecondTexture &&
        last.fogEnabled == cmd.fogEnabled && last.alphaTest == cmd.alphaTest &&
        last.fogColor == cmd.fogColor && last.textures[0] == cmd.textures[0] &&
        last.textures[1] == cmd.textures[1] &&
        last.samplers[0] == cmd.samplers[0] &&
        last.samplers[1] == cmd.samplers[1]) {
      last.vertexCount += n;
      return TRUE;
    }
  }
  if (cmd.textures[0])
    cmd.textures[0]->AddRef();
  if (cmd.textures[1])
    cmd.textures[1]->AddRef();
  m_pImpl->queuedCommands.push_back(cmd);
  return TRUE;
}

BOOL cD3D11Presenter::FlushHardwareCommands() {
  D3D11_MAPPED_SUBRESOURCE map;
  UINT stride = sizeof(sRenderBackendVertex), offset = 0;
  size_t i;
  int oldBlend = -1, oldDepth = -1, oldPrimitive = -1;
  ID3D11ShaderResourceView *oldTextures[2] = {NULL, NULL};
  ID3D11SamplerState *oldSamplers[2] = {NULL, NULL};
  BOOL constantsValid = FALSE, oldSecond = FALSE, oldFog = FALSE,
       oldAlphaTest = FALSE;
  DWORD oldFogColor = 0;
  float blendFactor[4] = {0, 0, 0, 0};
  if (!m_pImpl || m_pImpl->queuedCommands.empty())
    return TRUE;
  if (m_pImpl->queuedVertices.size() > m_pImpl->sceneVertexCapacity) {
    ReleaseInterface(m_pImpl->sceneVertices);
    m_pImpl->sceneVertexCapacity = (UINT)m_pImpl->queuedVertices.size() + 4096;
    if (FAILED(CreateDynamicBuffer(
            m_pImpl->device,
            m_pImpl->sceneVertexCapacity * sizeof(sRenderBackendVertex),
            D3D11_BIND_VERTEX_BUFFER, &m_pImpl->sceneVertices))) {
      m_pImpl->ReleaseQueuedCommands();
      return FALSE;
    }
  }
  if (FAILED(m_pImpl->context->Map(m_pImpl->sceneVertices, 0,
                                   D3D11_MAP_WRITE_DISCARD, 0, &map))) {
    m_pImpl->ReleaseQueuedCommands();
    return FALSE;
  }
  memcpy(map.pData, &m_pImpl->queuedVertices[0],
         m_pImpl->queuedVertices.size() * sizeof(sRenderBackendVertex));
  m_pImpl->context->Unmap(m_pImpl->sceneVertices, 0);
  m_pImpl->context->IASetInputLayout(m_pImpl->sceneLayout);
  m_pImpl->context->IASetVertexBuffers(0, 1, &m_pImpl->sceneVertices, &stride,
                                       &offset);
  m_pImpl->context->VSSetShader(m_pImpl->sceneVS, NULL, 0);
  m_pImpl->context->VSSetConstantBuffers(0, 1, &m_pImpl->sceneConstants);
  m_pImpl->context->PSSetShader(m_pImpl->scenePS, NULL, 0);
  m_pImpl->context->PSSetConstantBuffers(0, 1, &m_pImpl->sceneConstants);
  for (i = 0; i < m_pImpl->queuedCommands.size(); ++i) {
    const sRenderBackendCommand &cmd = m_pImpl->queuedCommands[i];
    if (!constantsValid || oldSecond != cmd.useSecondTexture ||
        oldFog != cmd.fogEnabled || oldAlphaTest != cmd.alphaTest ||
        oldFogColor != cmd.fogColor) {
      sSceneConstants c;
      ZeroMemory(&c, sizeof(c));
      c.width = (float)m_pImpl->sourceWidth;
      c.height = (float)m_pImpl->sourceHeight;
      c.fogEnabled = cmd.fogEnabled ? 1.0f : 0.0f;
      c.useTexture1 = cmd.useSecondTexture ? 1.0f : 0.0f;
      c.fogColor[0] = ((cmd.fogColor >> 16) & 255) / 255.0f;
      c.fogColor[1] = ((cmd.fogColor >> 8) & 255) / 255.0f;
      c.fogColor[2] = (cmd.fogColor & 255) / 255.0f;
      c.fogColor[3] = 1;
      c.alphaTest = cmd.alphaTest ? 1.0f : 0.0f;
      m_pImpl->context->UpdateSubresource(m_pImpl->sceneConstants, 0, NULL, &c,
                                          0, 0);
      constantsValid = TRUE;
      oldSecond = cmd.useSecondTexture;
      oldFog = cmd.fogEnabled;
      oldAlphaTest = cmd.alphaTest;
      oldFogColor = cmd.fogColor;
    }
    if (oldTextures[0] != cmd.textures[0] ||
        oldTextures[1] != cmd.textures[1]) {
      m_pImpl->context->PSSetShaderResources(0, 2, cmd.textures);
      oldTextures[0] = cmd.textures[0];
      oldTextures[1] = cmd.textures[1];
    }
    if (oldSamplers[0] != cmd.samplers[0] ||
        oldSamplers[1] != cmd.samplers[1]) {
      m_pImpl->context->PSSetSamplers(0, 2, cmd.samplers);
      oldSamplers[0] = cmd.samplers[0];
      oldSamplers[1] = cmd.samplers[1];
    }
    if (oldBlend != cmd.blendMode) {
      m_pImpl->context->OMSetBlendState(m_pImpl->blendStates[cmd.blendMode],
                                        blendFactor, ~0u);
      oldBlend = cmd.blendMode;
    }
    if (oldDepth != cmd.depthState) {
      m_pImpl->context->OMSetDepthStencilState(
          m_pImpl->depthStates[cmd.depthState], 0);
      oldDepth = cmd.depthState;
    }
    if (oldPrimitive != cmd.primitive) {
      m_pImpl->context->IASetPrimitiveTopology(
          cmd.primitive == kRenderBackendLines ? D3D11_PRIMITIVE_TOPOLOGY_LINELIST
          : cmd.primitive == kRenderBackendPoints
              ? D3D11_PRIMITIVE_TOPOLOGY_POINTLIST
              : D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
      oldPrimitive = cmd.primitive;
    }
    m_pImpl->context->Draw(cmd.vertexCount, cmd.firstVertex);
  }
  m_pImpl->ReleaseQueuedCommands();
  return TRUE;
}

BOOL cD3D11Presenter::Present(IDirectDrawSurface *surface, int x0, int y0,
                              int x1, int y1) {
  static DWORD opaque16[65536];
  static DWORD overlay16[65536];
  static DWORD cachedRedMask = 0, cachedGreenMask = 0, cachedBlueMask = 0;
  DDSURFACEDESC sd;
  D3D11_BOX uploadBox;
  D3D11_VIEWPORT v;
  RECT viewport;
  DWORD cw, ch, bw, bh;
  int x, y;
  BOOL transparentOverlay;
  BOOL tracePresentation;
  HRESULT hr;
  float black[4] = {0, 0, 0, 1};
  sCompositeConstants c;
  ID3D11ShaderResourceView *views[2];
  if (!m_pImpl || !m_pImpl->device || !surface)
    return FALSE;
  ConfigureFrameCapture();
  tracePresentation = m_pImpl->sourceNeedsFullUpload;
  // Dark's menu panels animate across the retained canvas even when the
  // legacy caller submits only a dirty rectangle.  Always refresh the whole
  // source for non-3D presentation so stale/cleared regions cannot flicker or
  // make button feedback disappear. Hardware gameplay keeps its single
  // frame-boundary upload path.
  if (tracePresentation || !m_pImpl->sceneActive) {
    x0 = 0;
    y0 = 0;
    x1 = (int)m_pImpl->sourceWidth;
    y1 = (int)m_pImpl->sourceHeight;
  }
  x0 = max(0, min(x0, (int)m_pImpl->sourceWidth));
  y0 = max(0, min(y0, (int)m_pImpl->sourceHeight));
  x1 = max(x0, min(x1, (int)m_pImpl->sourceWidth));
  y1 = max(y0, min(y1, (int)m_pImpl->sourceHeight));
  if (x0 == x1 || y0 == y1)
    return TRUE;
  transparentOverlay = m_pImpl->hardwareFrame || m_pImpl->sceneActive;
  ZeroMemory(&sd, sizeof(sd));
  sd.dwSize = sizeof(sd);
  if (FAILED(surface->Lock(NULL, &sd, DDLOCK_READONLY | DDLOCK_WAIT, NULL)))
    return FALSE;
  if (sd.ddpfPixelFormat.dwRGBBitCount != 15 &&
      sd.ddpfPixelFormat.dwRGBBitCount != 16 &&
      sd.ddpfPixelFormat.dwRGBBitCount != 32) {
    surface->Unlock(NULL);
    return FALSE;
  }
  if (sd.ddpfPixelFormat.dwRGBBitCount != 32) {
    DWORD redMask = sd.ddpfPixelFormat.dwRBitMask;
    DWORD greenMask = sd.ddpfPixelFormat.dwGBitMask;
    DWORD blueMask = sd.ddpfPixelFormat.dwBBitMask;
    if (redMask != cachedRedMask || greenMask != cachedGreenMask ||
        blueMask != cachedBlueMask) {
      DWORD pixel;
      for (pixel = 0; pixel < 65536; ++pixel) {
        DWORD rgba = ExpandChannel(pixel, redMask) |
                     ((DWORD)ExpandChannel(pixel, greenMask) << 8) |
                     ((DWORD)ExpandChannel(pixel, blueMask) << 16) | 0xff000000;
        opaque16[pixel] = rgba;
        overlay16[pixel] = pixel ? rgba : 0;
      }
      cachedRedMask = redMask;
      cachedGreenMask = greenMask;
      cachedBlueMask = blueMask;
    }
    for (y = y0; y < y1; ++y) {
      const WORD *src =
          (const WORD *)((const BYTE *)sd.lpSurface + y * sd.lPitch);
      DWORD *dst = &m_pImpl->overlayPixels[y * m_pImpl->sourceWidth];
      const DWORD *table = transparentOverlay ? overlay16 : opaque16;
      for (x = x0; x < x1; ++x)
        dst[x] = table[src[x]];
    }
  } else {
    for (y = y0; y < y1; ++y) {
      const DWORD *src =
          (const DWORD *)((const BYTE *)sd.lpSurface + y * sd.lPitch);
      DWORD *dst = &m_pImpl->overlayPixels[y * m_pImpl->sourceWidth];
      for (x = x0; x < x1; ++x) {
        DWORD pixel = src[x];
        dst[x] = ((pixel >> 16) & 255) | (pixel & 0xff00) |
                 ((pixel & 255) << 16) |
                 ((transparentOverlay && !(pixel & 0xffffff)) ? 0 : 0xff000000);
      }
    }
  }
  surface->Unlock(NULL);
  uploadBox.left = x0;
  uploadBox.top = y0;
  uploadBox.front = 0;
  uploadBox.right = x1;
  uploadBox.bottom = y1;
  uploadBox.back = 1;
  m_pImpl->context->UpdateSubresource(
      m_pImpl->sourceTexture, 0, &uploadBox,
      &m_pImpl->overlayPixels[y0 * m_pImpl->sourceWidth + x0],
      m_pImpl->sourceWidth * sizeof(DWORD), 0);
  m_pImpl->sourceNeedsFullUpload = FALSE;
  if (!EnsureOutputSize())
    return FALSE;
  cw = m_pImpl->targetWidth;
  ch = m_pImpl->targetHeight;
  bw = m_pImpl->backBufferWidth ? m_pImpl->backBufferWidth : cw;
  bh = m_pImpl->backBufferHeight ? m_pImpl->backBufferHeight : ch;
  viewport = ComputePresentationViewport(m_pImpl->sourceWidth,
                                         m_pImpl->sourceHeight, bw, bh,
                                         g_D3D11FitToViewport &&
                                             m_pImpl->sceneActive);
  ZeroMemory(&v, sizeof(v));
  v.TopLeftX = (float)viewport.left;
  v.TopLeftY = (float)viewport.top;
  v.Width = (float)(viewport.right - viewport.left);
  v.Height = (float)(viewport.bottom - viewport.top);
  v.MaxDepth = 1;
  if (tracePresentation)
    RenderBackendTrace(
        "present source=%lux%lu texture=%lux%lu scene=%lux%lu output=%lux%lu viewport=%.1f,%.1f %.1fx%.1f scale=%d fit=%d scene-active=%d gamma=%.4f",
        m_pImpl->sourceWidth, m_pImpl->sourceHeight,
        m_pImpl->sourceTextureWidth, m_pImpl->sourceTextureHeight,
        m_pImpl->sceneWidth, m_pImpl->sceneHeight, bw, bh,
        v.TopLeftX, v.TopLeftY, v.Width, v.Height, g_D3D11ScaleToWindow,
        g_D3D11FitToViewport, m_pImpl->sceneActive, m_pImpl->gamma);
  m_pImpl->context->OMSetRenderTargets(1, &m_pImpl->renderTarget, NULL);
  m_pImpl->context->ClearRenderTargetView(m_pImpl->renderTarget, black);
  m_pImpl->context->RSSetViewports(1, &v);
  c.gamma = m_pImpl->gamma;
  c.hasScene = m_pImpl->sceneActive ? 1.0f : 0.0f;
  c.pointSample = g_D3D11ScaleToWindow ? 0.0f : 1.0f;
  c.padding0 = 0;
  c.sourceScale[0] = (float)m_pImpl->sourceWidth /
                     m_pImpl->sourceTextureWidth;
  c.sourceScale[1] = (float)m_pImpl->sourceHeight /
                     m_pImpl->sourceTextureHeight;
  c.padding[0] = c.padding[1] = 0;
  c.overlayScale[0] = c.overlayScale[1] = 1.0f;
  c.overlayOffset[0] = c.overlayOffset[1] = 0.0f;
  if (m_pImpl->sceneActive && g_D3D11ScaleToWindow &&
      g_D3D11FitToViewport && v.Width > 0 && v.Height > 0) {
    const double sourceAspect = (double)m_pImpl->sourceWidth /
                                m_pImpl->sourceHeight;
    const double viewportAspect = (double)v.Width / v.Height;
    // Keep the legacy HUD/menu canvas undistorted and centered over the
    // full-width scene. Areas outside its original aspect stay transparent.
    if (viewportAspect > sourceAspect) {
      const double fraction = sourceAspect / viewportAspect;
      c.overlayScale[0] = (float)(1.0 / fraction);
      c.overlayOffset[0] = (float)((1.0 - fraction) * 0.5);
    } else if (viewportAspect < sourceAspect) {
      const double fraction = viewportAspect / sourceAspect;
      c.overlayScale[1] = (float)(1.0 / fraction);
      c.overlayOffset[1] = (float)((1.0 - fraction) * 0.5);
    }
  }
  m_pImpl->context->UpdateSubresource(m_pImpl->compositeConstants, 0, NULL, &c,
                                      0, 0);
  views[0] = m_pImpl->sourceView;
  views[1] = m_pImpl->sceneView;
  m_pImpl->context->IASetInputLayout(NULL);
  m_pImpl->context->IASetPrimitiveTopology(
      D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  m_pImpl->context->VSSetShader(m_pImpl->compositeVS, NULL, 0);
  m_pImpl->context->PSSetShader(m_pImpl->compositePS, NULL, 0);
  m_pImpl->context->PSSetConstantBuffers(0, 1, &m_pImpl->compositeConstants);
  m_pImpl->context->PSSetShaderResources(0, 2, views);
  m_pImpl->context->PSSetSamplers(0, 1, &m_pImpl->compositeSampler);
  m_pImpl->context->Draw(3, 0);
  if (!g_D3D11CaptureComplete && g_D3D11CapturePath[0] &&
      ++g_D3D11HardwareFrameCount >= g_D3D11CaptureFrame) {
    ID3D11Texture2D *backBuffer = NULL;
    if (SUCCEEDED(m_pImpl->swapChain->GetBuffer(
            0, __uuidof(ID3D11Texture2D), (void **)&backBuffer))) {
      g_D3D11CaptureComplete = CaptureTextureBmp(
          m_pImpl->device, m_pImpl->context, backBuffer, g_D3D11CapturePath);
      ReleaseInterface(backBuffer);
      RenderBackendTrace("frame-capture frame=%d result=%d path=%s",
                       g_D3D11HardwareFrameCount, g_D3D11CaptureComplete,
                       g_D3D11CapturePath);
    }
  }
  // The engine owns frame pacing. Do not block each legacy display flush on
  // the desktop refresh interval.
  hr = m_pImpl->swapChain->Present(0, 0);
  m_pImpl->hardwareFrame = FALSE;
  return SUCCEEDED(hr);
}

void D3D11SetScaleToWindow(BOOL enabled) {
  g_D3D11ScaleToWindow = enabled;
}

void D3D11SetPreserveLegacyCanvas(BOOL enabled) {
  g_D3D11PreserveLegacyCanvas = enabled;
}

extern "C" int RenderBackendAvailable(void) { return g_pPresenter != NULL; }
extern "C" void RenderBackendSetScaleToWindow(int enabled) {
  D3D11SetScaleToWindow(enabled);
}
extern "C" void RenderBackendSetFitToViewport(int enabled) {
  g_D3D11FitToViewport = enabled;
}
extern "C" void RenderBackendSetInteractiveResize(int resizing) {
  g_D3D11InteractiveResize = resizing;
}
extern "C" void RenderBackendSetPreserveCanvas(int enabled) {
  g_D3D11PreserveLegacyCanvas = enabled;
}
extern "C" int RenderBackendPreserveCanvas(void) {
  return g_D3D11PreserveLegacyCanvas;
}
extern "C" int RenderBackendGetSettledClientSize(int *width, int *height) {
  return !g_D3D11InteractiveResize && g_pPresenter &&
         g_pPresenter->GetClientSize(width, height);
}
extern "C" int RenderBackendClientToLogicalPoint(int *x, int *y) {
  return g_pPresenter && g_pPresenter->ClientToLogicalPoint(x, y);
}
extern "C" int RenderBackendLogicalToClientPoint(int *x, int *y) {
  return g_pPresenter && g_pPresenter->LogicalToClientPoint(x, y);
}
extern "C" int RenderBackendBeginFrame(void) {
  return g_pPresenter && g_pPresenter->BeginHardwareFrame();
}
extern "C" void RenderBackendEndFrame(void) {
  if (g_pPresenter)
    g_pPresenter->EndHardwareFrame();
}
extern "C" void RenderBackendDeactivateScene(void) {
  if (g_pPresenter)
    g_pPresenter->DeactivateScene();
}
extern "C" void RenderBackendClearDepth(void) {
  if (g_pPresenter)
    g_pPresenter->ClearDepth();
}
extern "C" void RenderBackendSetDepth(int c, int w) {
  if (g_pPresenter)
    g_pPresenter->SetDepth(c, w);
}
extern "C" void RenderBackendSetBlend(int m) {
  if (g_pPresenter)
    g_pPresenter->SetBlend(m);
}
extern "C" void RenderBackendSetAlphaTest(int e) {
  if (g_pPresenter)
    g_pPresenter->SetAlphaTest(e);
}
extern "C" void RenderBackendSetSampler(int l, int w, int s) {
  if (g_pPresenter)
    g_pPresenter->SetSampler(l, w, s);
}
extern "C" void RenderBackendSetFog(int e, uint32_t c) {
  if (g_pPresenter)
    g_pPresenter->SetFog(e, c);
}
extern "C" void *RenderBackendCreateTexture(int w, int h, const void *p, int r) {
  return g_pPresenter ? g_pPresenter->CreateTexture(w, h, p, r) : NULL;
}
extern "C" int RenderBackendUpdateTexture(void *t, int w, int h, const void *p,
                                          int r) {
  return g_pPresenter && g_pPresenter->UpdateTexture(t, w, h, p, r);
}
extern "C" void RenderBackendDestroyTexture(void *t) {
  if (g_pPresenter)
    g_pPresenter->DestroyTexture(t);
}
extern "C" void RenderBackendBindTexture(int l, void *t) {
  if (g_pPresenter)
    g_pPresenter->BindTexture(l, t);
}
extern "C" int RenderBackendDraw(int p, const sRenderBackendVertex *v, int n,
                                 int t) {
  return g_pPresenter && g_pPresenter->Draw(p, v, n, t);
}
