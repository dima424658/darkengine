#include "vulkan_render_backend.hpp"
#include <render_backend.h>
#include "runtime.h"

#include <algorithm>
#include <cstdarg>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <exception>

namespace
{
class cVulkanRenderBackend
{
public:
   ~cVulkanRenderBackend()
   {
      ResetTrace();
   }

   void SetRenderer(cVulkanSceneRenderer *renderer)
   {
      m_renderer = renderer;
      if (m_renderer)
         m_renderer->SetGamma(m_gamma);
      m_commandBuffer = nullptr;
      m_width = m_height = 0;
      m_drawableWidth = m_drawableHeight = 0;
      m_sceneActive = false;
   }

   void Attach(vk::CommandBuffer commandBuffer, int logicalWidth,
      int logicalHeight, int clientWidth, int clientHeight,
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

   void Detach() { m_commandBuffer = nullptr; }
   bool SceneActive() const { return m_sceneActive; }

   void SetWindowMetrics(int clientWidth, int clientHeight,
      int drawableWidth, int drawableHeight)
   {
      m_clientWidth = clientWidth;
      m_clientHeight = clientHeight;
      m_drawableWidth = drawableWidth;
      m_drawableHeight = drawableHeight;
   }

   void GetViewport(int logicalWidth, int logicalHeight, int drawableWidth,
      int drawableHeight, bool overlay, int *x, int *y, int *width,
      int *height) const
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
         const double pixelScaleX = m_clientWidth > 0
            ? static_cast<double>(drawableWidth) / m_clientWidth : 1.0;
         const double pixelScaleY = m_clientHeight > 0
            ? static_cast<double>(drawableHeight) / m_clientHeight : 1.0;
         *width = std::min(drawableWidth,
            std::max(static_cast<int>(logicalWidth * pixelScaleX), 0));
         *height = std::min(drawableHeight,
            std::max(static_cast<int>(logicalHeight * pixelScaleY), 0));
         return;
      }
      if (m_sceneActive && m_fitToViewport && !overlay)
      {
         *width = drawableWidth;
         *height = drawableHeight;
         return;
      }
      const double scale = std::min(
         static_cast<double>(drawableWidth) / logicalWidth,
         static_cast<double>(drawableHeight) / logicalHeight);
      *width = std::max(static_cast<int>(logicalWidth * scale), 1);
      *height = std::max(static_cast<int>(logicalHeight * scale), 1);
      *x = (drawableWidth - *width) / 2;
      *y = (drawableHeight - *height) / 2;
   }

   int UseLinearCanvasSampling() const { return m_scaleToWindow ? 1 : 0; }
   int Available() const { return m_renderer != nullptr; }
   void SetGamma(float gamma)
   {
      if (!std::isfinite(gamma) || gamma <= 0.0f)
         return;
      m_gamma = gamma;
      if (m_renderer)
         m_renderer->SetGamma(gamma);
   }
   void SetScaleToWindow(bool enabled) { m_scaleToWindow = enabled; }
   void SetFitToViewport(bool enabled) { m_fitToViewport = enabled; }
   void SetPreserveCanvas(bool enabled) { m_preserve = enabled; }
   int PreserveCanvas() const { return m_preserve; }

   int GetSettledClientSize(int *width, int *height) const
   {
      if (!width || !height || !m_renderer ||
          m_clientWidth <= 0 || m_clientHeight <= 0)
         return 0;
      *width = m_clientWidth;
      *height = m_clientHeight;
      return 1;
   }

   int GetLogicalSize(int *width, int *height) const
   {
      if (!width || !height || m_width <= 0 || m_height <= 0)
         return 0;
      *width = m_width;
      *height = m_height;
      return 1;
   }

   int ClientToLogicalPoint(int *x, int *y) const
   {
      if (!x || !y || !m_renderer || m_width <= 0 || m_height <= 0 ||
          m_clientWidth <= 0 || m_clientHeight <= 0 ||
          m_drawableWidth <= 0 || m_drawableHeight <= 0)
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
      *x = localX == viewportWidth / 2
         ? m_width / 2 : localX * m_width / viewportWidth;
      *y = localY == viewportHeight / 2
         ? m_height / 2 : localY * m_height / viewportHeight;
      return 1;
   }

   int LogicalToClientPoint(int *x, int *y) const
   {
      if (!x || !y || !m_renderer || m_width <= 0 || m_height <= 0 ||
          m_clientWidth <= 0 || m_clientHeight <= 0 ||
          m_drawableWidth <= 0 || m_drawableHeight <= 0)
         return 0;
      int viewportX, viewportY, viewportWidth, viewportHeight;
      GetViewport(m_width, m_height, m_drawableWidth, m_drawableHeight, true,
         &viewportX, &viewportY, &viewportWidth, &viewportHeight);
      if (viewportWidth <= 0 || viewportHeight <= 0)
         return 0;
      const int drawableX = viewportX + (*x == m_width / 2
         ? viewportWidth / 2 : *x * viewportWidth / m_width);
      const int drawableY = viewportY + (*y == m_height / 2
         ? viewportHeight / 2 : *y * viewportHeight / m_height);
      *x = drawableX * m_clientWidth / m_drawableWidth;
      *y = drawableY * m_clientHeight / m_drawableHeight;
      return 1;
   }

   int BeginFrame()
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

   void EndFrame() {}
   void PresentProgress() { DarkRenderPresentProgress(); }
   void DeactivateScene() { m_sceneActive = false; }

   void ClearDepth()
   {
      if (m_renderer && m_commandBuffer)
         m_renderer->ClearDepth(m_commandBuffer);
   }

   void ClearDepthRect(int x0, int y0, int x1, int y1)
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
      GetViewport(m_width, m_height, m_drawableWidth, m_drawableHeight,
         false, &viewportX, &viewportY, &viewportWidth, &viewportHeight);
      m_renderer->SetViewport(viewportX, viewportY, viewportWidth,
         viewportHeight);
      m_renderer->ClearDepthRect(m_commandBuffer, left, top,
         right - left, bottom - top, m_width, m_height);
   }

   void ClearCanvasRegion(int x, int y, int width, int height)
   {
      if (!m_renderer || !m_commandBuffer)
         return;
      int viewportX, viewportY, viewportWidth, viewportHeight;
      GetViewport(m_width, m_height, m_drawableWidth, m_drawableHeight,
         false, &viewportX, &viewportY, &viewportWidth, &viewportHeight);
      m_renderer->SetViewport(viewportX, viewportY, viewportWidth,
         viewportHeight);
      m_renderer->ClearCanvasRegion(m_commandBuffer, x, y, width, height,
         m_width, m_height);
   }

   void SetDepth(bool compare, bool write)
   { if (m_renderer) m_renderer->SetDepth(compare, write); }
   void SetBlend(int mode) { if (m_renderer) m_renderer->SetBlend(mode); }
   void SetAlphaTest(bool enabled)
   { if (m_renderer) m_renderer->SetAlphaTest(enabled); }
   void SetSampler(int level, bool wrap, bool smooth)
   { if (m_renderer) m_renderer->SetSampler(level, wrap, smooth); }
   void SetFog(bool enabled, uint32_t rgb)
   { if (m_renderer) m_renderer->SetFog(enabled, rgb); }

   void *CreateTexture(int width, int height, const void *rgba, int rowBytes)
   {
      return m_renderer
         ? m_renderer->CreateTexture(width, height, rgba, rowBytes) : nullptr;
   }

   int UpdateTexture(void *texture, int width, int height,
      const void *rgba, int rowBytes)
   {
      return m_renderer && m_renderer->UpdateTexture(texture, width, height,
         rgba, rowBytes);
   }

   void DestroyTexture(void *texture)
   { if (m_renderer) m_renderer->DestroyTexture(texture); }
   void BindTexture(int level, void *texture)
   { if (m_renderer) m_renderer->BindTexture(level, texture); }

   int Draw(int primitive, const sRenderBackendVertex *vertices,
      int vertexCount, bool useSecondTexture)
   {
      if (!m_renderer || !m_commandBuffer)
         return 0;
      int x, y, width, height;
      GetViewport(m_width, m_height, m_drawableWidth, m_drawableHeight,
         false, &x, &y, &width, &height);
      m_renderer->SetViewport(x, y, width, height);
      try
      {
         return m_renderer->Draw(m_commandBuffer, vertices, vertexCount,
            primitive, useSecondTexture, m_width, m_height);
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

   void ResetTrace()
   {
      if (m_trace)
      {
         std::fclose(m_trace);
         m_trace = nullptr;
      }
   }

   void StartTrace()
   {
      ResetTrace();
      const char *path = std::getenv("DARK_RENDER_TRACE");
      if (path && *path)
         m_trace = std::fopen(path, "w");
   }

   void Trace(const char *format, va_list args)
   {
      if (!m_trace || !format)
         return;
      std::vfprintf(m_trace, format, args);
      std::fputc('\n', m_trace);
      std::fflush(m_trace);
   }

private:
   cVulkanSceneRenderer *m_renderer{};
   vk::CommandBuffer m_commandBuffer{};
   int m_width{};
   int m_height{};
   int m_drawableWidth{};
   int m_drawableHeight{};
   int m_clientWidth{};
   int m_clientHeight{};
   bool m_preserve{};
   bool m_sceneActive{};
   bool m_scaleToWindow{true};
   bool m_fitToViewport{};
   float m_gamma{1.0f};
   FILE *m_trace{};
};

cVulkanRenderBackend &Backend()
{
   static cVulkanRenderBackend backend;
   return backend;
}

template <typename TOperation>
void IgnoreOperationFailure(TOperation &&operation) noexcept
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
int ReturnZeroOnFailure(TOperation &&operation) noexcept
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
}

void RenderBackendVulkanSetRenderer(cVulkanSceneRenderer *renderer)
{ Backend().SetRenderer(renderer); }

void RenderBackendVulkanAttach(vk::CommandBuffer commandBuffer,
   int logicalWidth, int logicalHeight, int clientWidth, int clientHeight,
   int drawableWidth, int drawableHeight)
{
   Backend().Attach(commandBuffer, logicalWidth, logicalHeight, clientWidth,
      clientHeight, drawableWidth, drawableHeight);
}

void RenderBackendVulkanDetach(void) { Backend().Detach(); }
int RenderBackendVulkanSceneActive(void) { return Backend().SceneActive(); }

void RenderBackendVulkanSetWindowMetrics(int clientWidth, int clientHeight,
   int drawableWidth, int drawableHeight)
{
   Backend().SetWindowMetrics(clientWidth, clientHeight, drawableWidth,
      drawableHeight);
}

void RenderBackendVulkanGetViewport(int logicalWidth, int logicalHeight,
   int drawableWidth, int drawableHeight, int overlay, int *x, int *y,
   int *width, int *height)
{
   Backend().GetViewport(logicalWidth, logicalHeight, drawableWidth,
      drawableHeight, overlay != 0, x, y, width, height);
}

int RenderBackendVulkanUseLinearCanvasSampling(void)
{ return Backend().UseLinearCanvasSampling(); }

extern "C" {
int RenderBackendAvailable(void) { return Backend().Available(); }
void RenderBackendSetGamma(float gamma) { Backend().SetGamma(gamma); }
void RenderBackendSetScaleToWindow(int enabled)
{ Backend().SetScaleToWindow(enabled != 0); }
void RenderBackendSetFitToViewport(int enabled)
{ Backend().SetFitToViewport(enabled != 0); }
void RenderBackendSetPreserveCanvas(int enabled)
{ Backend().SetPreserveCanvas(enabled != 0); }
int RenderBackendPreserveCanvas(void) { return Backend().PreserveCanvas(); }
int RenderBackendGetSettledClientSize(int *width, int *height)
{ return Backend().GetSettledClientSize(width, height); }
int RenderBackendGetLogicalSize(int *width, int *height)
{ return Backend().GetLogicalSize(width, height); }
int RenderBackendClientToLogicalPoint(int *x, int *y)
{ return Backend().ClientToLogicalPoint(x, y); }
int RenderBackendLogicalToClientPoint(int *x, int *y)
{ return Backend().LogicalToClientPoint(x, y); }
int RenderBackendBeginFrame(void)
{ return ReturnZeroOnFailure([] { return Backend().BeginFrame(); }); }
void RenderBackendEndFrame(void) { Backend().EndFrame(); }
void RenderBackendPresentProgress(void)
{ IgnoreOperationFailure([] { Backend().PresentProgress(); }); }
void RenderBackendDeactivateScene(void) { Backend().DeactivateScene(); }
void RenderBackendClearDepth(void)
{ IgnoreOperationFailure([] { Backend().ClearDepth(); }); }
void RenderBackendClearDepthRect(int x0, int y0, int x1, int y1)
{
   IgnoreOperationFailure(
      [=] { Backend().ClearDepthRect(x0, y0, x1, y1); });
}
void RenderBackendClearCanvasRegion(int x, int y, int width, int height)
{
   IgnoreOperationFailure(
      [=] { Backend().ClearCanvasRegion(x, y, width, height); });
}
void RenderBackendSetDepth(int compare, int write)
{ Backend().SetDepth(compare != 0, write != 0); }
void RenderBackendSetBlend(int mode) { Backend().SetBlend(mode); }
void RenderBackendSetAlphaTest(int enabled)
{ Backend().SetAlphaTest(enabled != 0); }
void RenderBackendSetSampler(int level, int wrap, int smooth)
{ Backend().SetSampler(level, wrap != 0, smooth != 0); }
void RenderBackendSetFog(int enabled, uint32_t rgb)
{ Backend().SetFog(enabled != 0, rgb); }
void *RenderBackendCreateTexture(int width, int height, const void *rgba,
   int rowBytes)
{
   try { return Backend().CreateTexture(width, height, rgba, rowBytes); }
   catch (...) { return nullptr; }
}
int RenderBackendUpdateTexture(void *texture, int width, int height,
   const void *rgba, int rowBytes)
{
   try
   {
      return Backend().UpdateTexture(texture, width, height, rgba, rowBytes);
   }
   catch (...) { return 0; }
}
void RenderBackendDestroyTexture(void *texture)
{
   try { Backend().DestroyTexture(texture); }
   catch (...) {}
}
void RenderBackendBindTexture(int level, void *texture)
{ Backend().BindTexture(level, texture); }
int RenderBackendDraw(int primitive, const sRenderBackendVertex *vertices,
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
void RenderBackendTraceReset(void) { Backend().StartTrace(); }
void RenderBackendTrace(const char *format, ...)
{
   va_list args;
   va_start(args, format);
   Backend().Trace(format, args);
   va_end(args);
}
}
