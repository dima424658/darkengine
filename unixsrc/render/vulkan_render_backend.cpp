#include "vulkan_render_backend.hpp"
#include <render_backend.h>

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>

namespace
{
cVulkanSceneRenderer *g_renderer;
vk::CommandBuffer g_commandBuffer;
int g_width;
int g_height;
int g_drawableWidth;
int g_drawableHeight;
bool g_preserve;
bool g_sceneActive;
int g_clientWidth;
int g_clientHeight;
bool g_scaleToWindow = true;
bool g_fitToViewport;
bool g_interactiveResize;
FILE *g_trace;
}

void RenderBackendVulkanSetRenderer(cVulkanSceneRenderer *renderer)
{
   g_renderer = renderer;
   g_commandBuffer = nullptr;
   g_width = g_height = 0;
   g_drawableWidth = g_drawableHeight = 0;
   g_sceneActive = false;
}

void RenderBackendVulkanAttach(vk::CommandBuffer commandBuffer,
   int logicalWidth, int logicalHeight, int clientWidth, int clientHeight,
   int drawableWidth, int drawableHeight)
{
   g_commandBuffer = commandBuffer;
   g_width = logicalWidth;
   g_height = logicalHeight;
   g_clientWidth = clientWidth;
   g_clientHeight = clientHeight;
   g_drawableWidth = drawableWidth;
   g_drawableHeight = drawableHeight;
}

void RenderBackendVulkanDetach(void)
{
   g_commandBuffer = nullptr;
}

int RenderBackendVulkanSceneActive(void) { return g_sceneActive; }

void RenderBackendVulkanSetWindowMetrics(int clientWidth, int clientHeight,
   int drawableWidth, int drawableHeight)
{
   g_clientWidth = clientWidth;
   g_clientHeight = clientHeight;
   g_drawableWidth = drawableWidth;
   g_drawableHeight = drawableHeight;
}

void RenderBackendVulkanGetViewport(int logicalWidth, int logicalHeight,
   int drawableWidth, int drawableHeight, int overlay, int *x, int *y,
   int *width, int *height)
{
   if (!x || !y || !width || !height) return;
   *x = *y = 0;
   *width = std::max(std::min(logicalWidth, drawableWidth), 0);
   *height = std::max(std::min(logicalHeight, drawableHeight), 0);
   if (!logicalWidth || !logicalHeight || !drawableWidth || !drawableHeight)
      return;
   if (!g_scaleToWindow)
   {
      const double pixelScaleX = g_clientWidth > 0
         ? static_cast<double>(drawableWidth) / g_clientWidth : 1.0;
      const double pixelScaleY = g_clientHeight > 0
         ? static_cast<double>(drawableHeight) / g_clientHeight : 1.0;
      *width = std::min(drawableWidth,
         std::max(static_cast<int>(logicalWidth * pixelScaleX), 0));
      *height = std::min(drawableHeight,
         std::max(static_cast<int>(logicalHeight * pixelScaleY), 0));
      return;
   }
   if (g_sceneActive && g_fitToViewport && !overlay)
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

int RenderBackendVulkanUseLinearCanvasSampling(void)
{ return g_scaleToWindow ? 1 : 0; }

extern "C" {
int RenderBackendAvailable(void) { return g_renderer != nullptr; }
void RenderBackendSetGamma(float gamma)
{ if (g_renderer) g_renderer->SetGamma(gamma); }
void RenderBackendSetScaleToWindow(int enabled)
{ g_scaleToWindow = enabled != 0; }
void RenderBackendSetFitToViewport(int enabled)
{ g_fitToViewport = enabled != 0; }
void RenderBackendSetInteractiveResize(int resizing)
{ g_interactiveResize = resizing != 0; }
void RenderBackendSetPreserveCanvas(int enabled) { g_preserve = enabled != 0; }
int RenderBackendPreserveCanvas(void) { return g_preserve; }
int RenderBackendGetSettledClientSize(int *width, int *height)
{
   if (!width || !height || !g_renderer || g_interactiveResize ||
       g_clientWidth <= 0 || g_clientHeight <= 0)
      return 0;
   *width = g_clientWidth;
   *height = g_clientHeight;
   return 1;
}
int RenderBackendClientToLogicalPoint(int *x, int *y)
{
   if (!x || !y || !g_renderer || g_width <= 0 || g_height <= 0 ||
       g_clientWidth <= 0 || g_clientHeight <= 0 ||
       g_drawableWidth <= 0 || g_drawableHeight <= 0)
      return 0;
   const int drawableX = *x * g_drawableWidth / g_clientWidth;
   const int drawableY = *y * g_drawableHeight / g_clientHeight;
   int vx, vy, vw, vh;
   RenderBackendVulkanGetViewport(g_width, g_height, g_drawableWidth,
      g_drawableHeight, 1, &vx, &vy, &vw, &vh);
   if (vw <= 0 || vh <= 0) return 0;
   const int localX = drawableX - vx;
   const int localY = drawableY - vy;
   *x = localX == vw / 2 ? g_width / 2 : localX * g_width / vw;
   *y = localY == vh / 2 ? g_height / 2 : localY * g_height / vh;
   return 1;
}
int RenderBackendLogicalToClientPoint(int *x, int *y)
{
   if (!x || !y || !g_renderer || g_width <= 0 || g_height <= 0 ||
       g_clientWidth <= 0 || g_clientHeight <= 0 ||
       g_drawableWidth <= 0 || g_drawableHeight <= 0)
      return 0;
   int vx, vy, vw, vh;
   RenderBackendVulkanGetViewport(g_width, g_height, g_drawableWidth,
      g_drawableHeight, 1, &vx, &vy, &vw, &vh);
   if (vw <= 0 || vh <= 0) return 0;
   const int drawableX = vx + (*x == g_width / 2
      ? vw / 2 : *x * vw / g_width);
   const int drawableY = vy + (*y == g_height / 2
      ? vh / 2 : *y * vh / g_height);
   *x = drawableX * g_clientWidth / g_drawableWidth;
   *y = drawableY * g_clientHeight / g_drawableHeight;
   return 1;
}
int RenderBackendBeginFrame(void)
{
   if (!g_renderer || !g_commandBuffer)
      return 0;
   if (!g_preserve)
   {
      RenderBackendClearCanvasRegion(0, 0, g_width, g_height);
      g_renderer->ClearDepth(g_commandBuffer);
   }
   g_sceneActive = true;
   return 1;
}
void RenderBackendEndFrame(void) {}
void RenderBackendDeactivateScene(void) { g_sceneActive = false; }
void RenderBackendClearDepth(void)
{ if (g_renderer && g_commandBuffer) g_renderer->ClearDepth(g_commandBuffer); }
void RenderBackendClearCanvasRegion(int x, int y, int width, int height)
{
   if (g_renderer && g_commandBuffer)
   {
      int viewportX, viewportY, viewportWidth, viewportHeight;
      RenderBackendVulkanGetViewport(g_width, g_height, g_drawableWidth,
         g_drawableHeight, 0, &viewportX, &viewportY, &viewportWidth,
         &viewportHeight);
      g_renderer->SetViewport(viewportX, viewportY, viewportWidth,
         viewportHeight);
      g_renderer->ClearCanvasRegion(g_commandBuffer, x, y, width, height,
         g_width, g_height);
   }
}
void RenderBackendSetDepth(int compare, int write)
{ if (g_renderer) g_renderer->SetDepth(compare != 0, write != 0); }
void RenderBackendSetBlend(int mode)
{ if (g_renderer) g_renderer->SetBlend(mode); }
void RenderBackendSetAlphaTest(int enabled)
{ if (g_renderer) g_renderer->SetAlphaTest(enabled != 0); }
void RenderBackendSetSampler(int level, int wrap, int smooth)
{ if (g_renderer) g_renderer->SetSampler(level, wrap != 0, smooth != 0); }
void RenderBackendSetFog(int enabled, uint32_t rgb)
{ if (g_renderer) g_renderer->SetFog(enabled != 0, rgb); }
void *RenderBackendCreateTexture(int width, int height, const void *rgba,
   int rowBytes)
{ return g_renderer ? g_renderer->CreateTexture(width, height, rgba, rowBytes) : nullptr; }
int RenderBackendUpdateTexture(void *texture, int width, int height,
   const void *rgba, int rowBytes)
{ return g_renderer && g_renderer->UpdateTexture(texture, width, height, rgba, rowBytes); }
void RenderBackendDestroyTexture(void *texture)
{ if (g_renderer) g_renderer->DestroyTexture(texture); }
void RenderBackendBindTexture(int level, void *texture)
{ if (g_renderer) g_renderer->BindTexture(level, texture); }
int RenderBackendDraw(int primitive, const sRenderBackendVertex *vertices,
   int vertexCount, int useSecondTexture)
{
   if (!g_renderer || !g_commandBuffer)
      return 0;
   if (g_renderer && g_commandBuffer)
   {
      int x, y, width, height;
      RenderBackendVulkanGetViewport(g_width, g_height, g_drawableWidth,
         g_drawableHeight, 0, &x, &y, &width, &height);
      g_renderer->SetViewport(x, y, width, height);
   }
   try
   {
      return g_renderer->Draw(g_commandBuffer, vertices, vertexCount,
         primitive, useSecondTexture != 0, g_width, g_height);
   }
   catch (const std::exception &)
   {
      return 0;
   }
}
void RenderBackendTraceReset(void)
{
   if (g_trace) { std::fclose(g_trace); g_trace = nullptr; }
   const char *path = std::getenv("DARK_RENDER_TRACE");
   if (path && *path) g_trace = std::fopen(path, "w");
}
void RenderBackendTrace(const char *format, ...)
{
   if (!g_trace || !format) return;
   va_list args;
   va_start(args, format);
   std::vfprintf(g_trace, format, args);
   va_end(args);
   std::fputc('\n', g_trace);
   std::fflush(g_trace);
}
}
