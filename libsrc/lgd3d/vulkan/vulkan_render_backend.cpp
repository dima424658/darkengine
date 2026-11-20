#include "vulkan_render_backend.h"
#include "vulkan_backend_bridge.h"
#include <render_backend.h>
#include "runtime.h"

#include <algorithm>
#include <cstdarg>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <exception>

namespace lgd3d
{
cVulkanRenderBackend::cVulkanRenderBackend()
{
   StartTrace();
}

cVulkanRenderBackend::~cVulkanRenderBackend()
{
   ResetTrace();
}

void cVulkanRenderBackend::SetRenderer(cVulkanSceneRenderer * renderer)
{
   m_renderer = renderer;
   if (m_renderer)
      m_renderer->SetGamma(m_gamma);
   m_commandBuffer = nullptr;
   m_width = m_height = 0;
   m_drawableWidth = m_drawableHeight = 0;
   m_sceneActive = false;
   m_canvasOverlayEnabled = true;
}

void cVulkanRenderBackend::Attach(vk::CommandBuffer commandBuffer,
                                  int logicalWidth, int logicalHeight,
                                  int clientWidth, int clientHeight,
                                  int drawableWidth, int drawableHeight)
{
   m_commandBuffer = commandBuffer;
   m_width = logicalWidth;
   m_height = logicalHeight;
   m_clientWidth = clientWidth;
   m_clientHeight = clientHeight;
   m_drawableWidth = drawableWidth;
   m_drawableHeight = drawableHeight;
}

void cVulkanRenderBackend::Detach()
{
   m_commandBuffer = nullptr;
}

bool cVulkanRenderBackend::SceneActive() const
{
   return m_sceneActive;
}

void cVulkanRenderBackend::SetWindowMetrics(int clientWidth, int clientHeight,
                                            int drawableWidth,
                                            int drawableHeight)
{
   m_clientWidth = clientWidth;
   m_clientHeight = clientHeight;
   m_drawableWidth = drawableWidth;
   m_drawableHeight = drawableHeight;
}

void cVulkanRenderBackend::GetViewport(int logicalWidth, int logicalHeight,
                                       int drawableWidth, int drawableHeight,
                                       bool overlay, int * x, int * y,
                                       int * width, int * height) const
{
   if (!x || !y || !width || !height)
      return;
   *x = *y = 0;
   *width = std::max(std::min(logicalWidth, drawableWidth), 0);
   *height = std::max(std::min(logicalHeight, drawableHeight), 0);
   if (!logicalWidth || !logicalHeight || !drawableWidth || !drawableHeight)
      return;
   if (!m_scaleToWindow)
   {
      const double pixelScaleX =
         m_clientWidth > 0 ? static_cast<double>(drawableWidth) / m_clientWidth
                           : 1.0;
      const double pixelScaleY =
         m_clientHeight > 0
            ? static_cast<double>(drawableHeight) / m_clientHeight
            : 1.0;
      *width =
         std::min(drawableWidth,
                  std::max(static_cast<int>(logicalWidth * pixelScaleX), 0));
      *height =
         std::min(drawableHeight,
                  std::max(static_cast<int>(logicalHeight * pixelScaleY), 0));
      return;
   }
   if (m_sceneActive && m_fitToViewport && !overlay)
   {
      *width = drawableWidth;
      *height = drawableHeight;
      return;
   }
   const double scale =
      std::min(static_cast<double>(drawableWidth) / logicalWidth,
               static_cast<double>(drawableHeight) / logicalHeight);
   *width = std::max(static_cast<int>(logicalWidth * scale), 1);
   *height = std::max(static_cast<int>(logicalHeight * scale), 1);
   *x = (drawableWidth - *width) / 2;
   *y = (drawableHeight - *height) / 2;
}

int cVulkanRenderBackend::UseLinearCanvasSampling() const
{
   return m_scaleToWindow ? 1 : 0;
}

int cVulkanRenderBackend::Available() const
{
   return m_renderer != nullptr;
}

void cVulkanRenderBackend::SetGamma(float gamma)
{
   if (!std::isfinite(gamma) || gamma <= 0.0f)
      return;
   m_gamma = gamma;
   if (m_renderer)
      m_renderer->SetGamma(gamma);
}

void cVulkanRenderBackend::SetScaleToWindow(bool enabled)
{
   m_scaleToWindow = enabled;
}

void cVulkanRenderBackend::SetFitToViewport(bool enabled)
{
   m_fitToViewport = enabled;
}

void cVulkanRenderBackend::SetPreserveCanvas(bool enabled)
{
   m_preserve = enabled;
}

int cVulkanRenderBackend::PreserveCanvas() const
{
   return m_preserve;
}

void cVulkanRenderBackend::SetCanvasOverlayEnabled(bool enabled)
{
   m_canvasOverlayEnabled = enabled;
}

int cVulkanRenderBackend::CanvasOverlayEnabled() const
{
   return m_canvasOverlayEnabled;
}

int cVulkanRenderBackend::GetClientSize(int * width, int * height) const
{
   if (!width || !height || !m_renderer || m_clientWidth <= 0 ||
       m_clientHeight <= 0)
      return 0;
   *width = m_clientWidth;
   *height = m_clientHeight;
   return 1;
}

int cVulkanRenderBackend::GetLogicalSize(int * width, int * height) const
{
   if (!width || !height || m_width <= 0 || m_height <= 0)
      return 0;
   *width = m_width;
   *height = m_height;
   return 1;
}

int cVulkanRenderBackend::ClientToLogicalPoint(int * x, int * y) const
{
   if (!x || !y || !m_renderer || m_width <= 0 || m_height <= 0 ||
       m_clientWidth <= 0 || m_clientHeight <= 0 || m_drawableWidth <= 0 ||
       m_drawableHeight <= 0)
      return 0;
   const int drawableX = *x * m_drawableWidth / m_clientWidth;
   const int drawableY = *y * m_drawableHeight / m_clientHeight;
   int viewportX, viewportY, viewportWidth, viewportHeight;
   GetViewport(m_width, m_height, m_drawableWidth, m_drawableHeight, true,
               &viewportX, &viewportY, &viewportWidth, &viewportHeight);
   if (viewportWidth <= 0 || viewportHeight <= 0)
      return 0;
   const int localX = drawableX - viewportX;
   const int localY = drawableY - viewportY;
   *x = localX == viewportWidth / 2 ? m_width / 2
                                    : localX * m_width / viewportWidth;
   *y = localY == viewportHeight / 2 ? m_height / 2
                                     : localY * m_height / viewportHeight;
   return 1;
}

int cVulkanRenderBackend::LogicalToClientPoint(int * x, int * y) const
{
   if (!x || !y || !m_renderer || m_width <= 0 || m_height <= 0 ||
       m_clientWidth <= 0 || m_clientHeight <= 0 || m_drawableWidth <= 0 ||
       m_drawableHeight <= 0)
      return 0;
   int viewportX, viewportY, viewportWidth, viewportHeight;
   GetViewport(m_width, m_height, m_drawableWidth, m_drawableHeight, true,
               &viewportX, &viewportY, &viewportWidth, &viewportHeight);
   if (viewportWidth <= 0 || viewportHeight <= 0)
      return 0;
   const int drawableX =
      viewportX +
      (*x == m_width / 2 ? viewportWidth / 2 : *x * viewportWidth / m_width);
   const int drawableY =
      viewportY + (*y == m_height / 2 ? viewportHeight / 2
                                      : *y * viewportHeight / m_height);
   *x = drawableX * m_clientWidth / m_drawableWidth;
   *y = drawableY * m_clientHeight / m_drawableHeight;
   return 1;
}

int cVulkanRenderBackend::BeginScene()
{
   if (!m_renderer || !m_commandBuffer)
      return 0;
   if (!m_preserve)
   {
      ClearCanvasRegion(0, 0, m_width, m_height);
      m_renderer->ClearDepth(m_commandBuffer);
   }
   m_sceneActive = true;
   return 1;
}

void cVulkanRenderBackend::PresentProgress()
{
   DarkRenderPresentProgress();
}

void cVulkanRenderBackend::DeactivateScene()
{
   m_sceneActive = false;
}

void cVulkanRenderBackend::ClearDepth()
{
   if (m_renderer && m_commandBuffer)
      m_renderer->ClearDepth(m_commandBuffer);
}

void cVulkanRenderBackend::ClearDepthRect(int x0, int y0, int x1, int y1)
{
   if (!m_renderer || !m_commandBuffer || m_width <= 0 || m_height <= 0)
      return;
   const int left = std::clamp(x0, 0, m_width);
   const int top = std::clamp(y0, 0, m_height);
   const int right = std::clamp(x1, 0, m_width);
   const int bottom = std::clamp(y1, 0, m_height);
   if (right <= left || bottom <= top)
      return;
   int viewportX, viewportY, viewportWidth, viewportHeight;
   GetViewport(m_width, m_height, m_drawableWidth, m_drawableHeight, false,
               &viewportX, &viewportY, &viewportWidth, &viewportHeight);
   m_renderer->SetViewport(viewportX, viewportY, viewportWidth, viewportHeight);
   m_renderer->ClearDepthRect(m_commandBuffer, left, top, right - left,
                              bottom - top, m_width, m_height);
}

void cVulkanRenderBackend::ClearCanvasRegion(int x, int y, int width,
                                             int height)
{
   if (!m_renderer || !m_commandBuffer)
      return;
   int viewportX, viewportY, viewportWidth, viewportHeight;
   GetViewport(m_width, m_height, m_drawableWidth, m_drawableHeight, false,
               &viewportX, &viewportY, &viewportWidth, &viewportHeight);
   m_renderer->SetViewport(viewportX, viewportY, viewportWidth, viewportHeight);
   m_renderer->ClearCanvasRegion(m_commandBuffer, x, y, width, height, m_width,
                                 m_height);
}

void cVulkanRenderBackend::PushDrawState()
{
   if (m_renderer)
      m_renderer->PushDrawState();
}

void cVulkanRenderBackend::PopDrawState()
{
   if (m_renderer)
      m_renderer->PopDrawState();
}

void cVulkanRenderBackend::SetDepth(bool compare, bool write)
{
   if (m_renderer)
      m_renderer->SetDepth(compare, write);
}

void cVulkanRenderBackend::SetBlend(int mode)
{
   if (m_renderer)
      m_renderer->SetBlend(mode);
}

void cVulkanRenderBackend::SetAlphaTest(bool enabled)
{
   if (m_renderer)
      m_renderer->SetAlphaTest(enabled);
}

void cVulkanRenderBackend::SetSampler(int level, bool wrap, bool smooth)
{
   if (m_renderer)
      m_renderer->SetSampler(level, wrap, smooth);
}

void cVulkanRenderBackend::SetFog(bool enabled, uint32_t rgb)
{
   if (m_renderer)
      m_renderer->SetFog(enabled, rgb);
}

int cVulkanRenderBackend::UpdateTexture(void * texture, int width, int height,
                                        const void * rgba, int rowBytes)
{
   return m_renderer &&
          m_renderer->UpdateTexture(texture, width, height, rgba, rowBytes);
}

void cVulkanRenderBackend::DestroyTexture(void * texture)
{
   if (m_renderer)
      m_renderer->DestroyTexture(texture);
}

void cVulkanRenderBackend::BindTexture(int level, void * texture)
{
   if (m_renderer)
      m_renderer->BindTexture(level, texture);
}

int cVulkanRenderBackend::Draw(int primitive,
                               const sRenderBackendVertex * vertices,
                               int vertexCount, bool useSecondTexture)
{
   if (!m_renderer || !m_commandBuffer)
      return 0;
   int x, y, width, height;
   GetViewport(m_width, m_height, m_drawableWidth, m_drawableHeight, false, &x,
               &y, &width, &height);
   m_renderer->SetViewport(x, y, width, height);
   try
   {
      return m_renderer->Draw(m_commandBuffer, vertices, vertexCount, primitive,
                              useSecondTexture, m_width, m_height);
   }
   catch (const std::exception &)
   {
      return 0;
   }
   catch (...)
   {
      return 0;
   }
}

void cVulkanRenderBackend::ResetTrace()
{
   if (m_trace)
   {
      std::fclose(m_trace);
      m_trace = nullptr;
   }
}

void cVulkanRenderBackend::StartTrace()
{
   ResetTrace();
   const char * path = std::getenv("DARK_RENDER_TRACE");
   if (path && *path)
      m_trace = fopen(path, "w");
}

void cVulkanRenderBackend::Trace(const char * format, va_list args)
{
   if (!m_trace || !format)
      return;
   std::vfprintf(m_trace, format, args);
   std::fputc('\n', m_trace);
   std::fflush(m_trace);
}

cVulkanRenderBackend & Backend()
{
   static cVulkanRenderBackend backend;
   return backend;
}

template <typename TOperation>
void IgnoreOperationFailure(TOperation && operation) noexcept
{
   try
   {
      operation();
   }
   catch (...)
   {
   }
}

template <typename TOperation>
int ReturnZeroOnFailure(TOperation && operation) noexcept
{
   try
   {
      return operation();
   }
   catch (...)
   {
      return 0;
   }
}
} // namespace lgd3d

using namespace lgd3d;

void RenderBackendVulkanSetRenderer(cVulkanSceneRenderer * renderer)
{
   Backend().SetRenderer(renderer);
}

void RenderBackendVulkanAttach(vk::CommandBuffer commandBuffer,
                               int logicalWidth, int logicalHeight,
                               int clientWidth, int clientHeight,
                               int drawableWidth, int drawableHeight)
{
   Backend().Attach(commandBuffer, logicalWidth, logicalHeight, clientWidth,
                    clientHeight, drawableWidth, drawableHeight);
}

void RenderBackendVulkanDetach(void)
{
   Backend().Detach();
}
int RenderBackendVulkanSceneActive(void)
{
   return Backend().SceneActive();
}
int RenderBackendVulkanCanvasOverlayEnabled(void)
{
   return Backend().CanvasOverlayEnabled();
}

void RenderBackendVulkanSetWindowMetrics(int clientWidth, int clientHeight,
                                         int drawableWidth, int drawableHeight)
{
   Backend().SetWindowMetrics(clientWidth, clientHeight, drawableWidth,
                              drawableHeight);
}

void RenderBackendVulkanGetViewport(int logicalWidth, int logicalHeight,
                                    int drawableWidth, int drawableHeight,
                                    int overlay, int * x, int * y, int * width,
                                    int * height)
{
   Backend().GetViewport(logicalWidth, logicalHeight, drawableWidth,
                         drawableHeight, overlay != 0, x, y, width, height);
}

int RenderBackendVulkanUseLinearCanvasSampling(void)
{
   return Backend().UseLinearCanvasSampling();
}

extern "C"
{
int RenderBackendAvailable(void)
{
   return Backend().Available();
}
void RenderBackendSetGamma(float gamma)
{
   Backend().SetGamma(gamma);
}
void RenderBackendSetScaleToWindow(int enabled)
{
   Backend().SetScaleToWindow(enabled != 0);
}
void RenderBackendSetFitToViewport(int enabled)
{
   Backend().SetFitToViewport(enabled != 0);
}
void RenderBackendSetPreserveCanvas(int enabled)
{
   Backend().SetPreserveCanvas(enabled != 0);
}
int RenderBackendPreserveCanvas(void)
{
   return Backend().PreserveCanvas();
}
void RenderBackendSetCanvasOverlayEnabled(int enabled)
{
   Backend().SetCanvasOverlayEnabled(enabled != 0);
}
int RenderBackendCanvasOverlayEnabled(void)
{
   return Backend().CanvasOverlayEnabled();
}
int RenderBackendGetClientSize(int * width, int * height)
{
   return Backend().GetClientSize(width, height);
}

int RenderBackendGetLogicalSize(int * width, int * height)
{
   return Backend().GetLogicalSize(width, height);
}
int RenderBackendClientToLogicalPoint(int * x, int * y)
{
   return Backend().ClientToLogicalPoint(x, y);
}
int RenderBackendLogicalToClientPoint(int * x, int * y)
{
   return Backend().LogicalToClientPoint(x, y);
}
int RenderBackendBeginScene(void)
{
   return ReturnZeroOnFailure([] { return Backend().BeginScene(); });
}
void RenderBackendPresentProgress(void)
{
   IgnoreOperationFailure([] { Backend().PresentProgress(); });
}
void RenderBackendDeactivateScene(void)
{
   Backend().DeactivateScene();
}
void RenderBackendClearDepth(void)
{
   IgnoreOperationFailure([] { Backend().ClearDepth(); });
}
void RenderBackendClearDepthRect(int x0, int y0, int x1, int y1)
{
   IgnoreOperationFailure([=] { Backend().ClearDepthRect(x0, y0, x1, y1); });
}

void RenderBackendSetDepth(int compare, int write)
{
   Backend().SetDepth(compare != 0, write != 0);
}
void RenderBackendSetBlend(int mode)
{
   Backend().SetBlend(mode);
}
void RenderBackendSetAlphaTest(int enabled)
{
   Backend().SetAlphaTest(enabled != 0);
}
void RenderBackendSetSampler(int level, int wrap, int smooth)
{
   Backend().SetSampler(level, wrap != 0, smooth != 0);
}
void RenderBackendSetFog(int enabled, uint32_t rgb)
{
   Backend().SetFog(enabled != 0, rgb);
}
void * RenderBackendCreateTexture(int width, int height, const void * rgba,
                                  int rowBytes)
{
   try
   {
      return Backend().CreateTexture(width, height, rgba, rowBytes);
   }
   catch (...)
   {
      return nullptr;
   }
}
int RenderBackendUpdateTexture(void * texture, int width, int height,
                               const void * rgba, int rowBytes)
{
   try
   {
      return Backend().UpdateTexture(texture, width, height, rgba, rowBytes);
   }
   catch (...)
   {
      return 0;
   }
}
void RenderBackendDestroyTexture(void * texture)
{
   try
   {
      Backend().DestroyTexture(texture);
   }
   catch (...)
   {
   }
}
void RenderBackendBindTexture(int level, void * texture)
{
   Backend().BindTexture(level, texture);
}
int RenderBackendDraw(int primitive, const sRenderBackendVertex * vertices,
                      int vertexCount, int useSecondTexture)
{
   try
   {
      return Backend().Draw(primitive, vertices, vertexCount,
                            useSecondTexture != 0);
   }
   catch (...)
   {
      return 0;
   }
}
void RenderBackendTraceReset(void)
{
   Backend().StartTrace();
}
void RenderBackendTrace(const char * format, ...)
{
   va_list args;
   va_start(args, format);
   Backend().Trace(format, args);
   va_end(args);
}
}

void RenderBackendPushDrawState(void)
{
   Backend().PushDrawState();
}

void RenderBackendPopDrawState(void)
{
   Backend().PopDrawState();
}

namespace lgd3d
{
void * cVulkanRenderBackend::CreateTexture(int width, int height,
                                           const void * rgba, int rowBytes)
{
   return m_renderer ? m_renderer->CreateTexture(width, height, rgba, rowBytes)
                     : nullptr;
}
} // namespace lgd3d
