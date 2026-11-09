#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include <render_backend.h>
#include <platform_services.h>

#include "runtime.h"
#include "event_bridge.h"
#include "../dispdev/display_window.h"
#include "vulkan_context.hpp"
#include "vulkan_render_backend.hpp"
#include "../dispdev/sdl_vulkan.hpp"
#include "vulkan_scene_renderer.hpp"

namespace
{
class cVulkanRuntime
{
public:
   bool Open(int width, int height, const char *title);
   void Close();
   bool TakeResize(int *width, int *height);
   bool BeginEventPump();
   void WindowResized();
   void WindowMinimized();
   void WindowRestored();
   void ApplicationSuspended();
   void ApplicationResumed();
   void EndEventPump();
   int CaptureComplete() const;
   void SetPhysicalDeviceIndex(int index);
   void BeginApplicationFrame();
   void EndApplicationFrame();
   void PresentProgress();
   void Present(const void *pixels, int width, int height, int rowBytes,
      int bitDepth, const uint8_t *palette);
   bool PresentMovieFrame(const void *rgba, int width, int height,
      int rowBytes);
private:
   void UpdateWindowDimensions();
   void ProcessPendingLifecycleResize();
   bool ApplyPendingResize();
   void ConvertCanvas(const void *pixels, int width, int height, int rowBytes,
      int depth, const uint8_t *palette);
   bool PrepareCanvasTexture();
   void DrawCanvas(vk::CommandBuffer commandBuffer, bool transparent);

   bool m_windowOpen{};
   cVulkanContext m_context;
   cVulkanSceneRenderer m_renderer;
   vk::CommandBuffer m_activeCommandBuffer{};
   int m_width{}, m_height{};
   int m_clientWidth{}, m_clientHeight{};
   int m_drawableWidth{}, m_drawableHeight{};
   int m_canvasWidth{}, m_canvasHeight{};
   std::array<void *, kVulkanFramesInFlight> m_canvasTextures{};
   std::array<int, kVulkanFramesInFlight> m_canvasTextureWidths{};
   std::array<int, kVulkanFramesInFlight> m_canvasTextureHeights{};
   std::array<uint64_t, kVulkanFramesInFlight>
      m_canvasTextureGenerations{};
   uint64_t m_canvasGeneration{};
   std::atomic<bool> m_windowMinimized{};
   std::atomic<bool> m_appInBackground{};
   std::atomic<bool> m_lifecycleResizePending{};
   std::atomic<bool> m_surfaceRecreationPending{};
   std::chrono::steady_clock::time_point m_nextSurfaceRecreationAttempt{};
   bool m_surfaceRecreationErrorReported{};
   bool m_frameActive{};
   int m_physicalDeviceIndex{-1};
   bool m_resizePending{};
   bool m_windowSizeChanged{};
   bool m_exitHookRegistered{};
   void *m_movieTexture{};
   std::vector<uint8_t> m_canvasPixels;
};

cVulkanRuntime &VulkanRuntime()
{
   static cVulkanRuntime runtime;
   return runtime;
}

void cVulkanRuntime::UpdateWindowDimensions()
{
   if (!m_windowOpen) return;
   const int oldWidth = m_clientWidth, oldHeight = m_clientHeight;
   const int oldDrawableWidth = m_drawableWidth;
   const int oldDrawableHeight = m_drawableHeight;
   SDLDisplayGetWindowMetrics(&m_clientWidth, &m_clientHeight,
      &m_drawableWidth, &m_drawableHeight);
   if (m_clientWidth > 0 && m_clientHeight > 0 &&
       (m_clientWidth != oldWidth || m_clientHeight != oldHeight))
      m_windowSizeChanged = true;
   RenderBackendVulkanSetWindowMetrics(m_clientWidth, m_clientHeight,
      m_drawableWidth, m_drawableHeight);
   const bool drawableSizeChanged =
      m_drawableWidth != oldDrawableWidth ||
      m_drawableHeight != oldDrawableHeight;
   const bool resizeNeeded = drawableSizeChanged || m_resizePending ||
      m_context.NeedsSwapchainRecreation();
   if (m_context.NeedsSurfaceRecreation())
      m_surfaceRecreationPending.store(true, std::memory_order_release);
   if (m_surfaceRecreationPending.load(std::memory_order_acquire))
   {
      m_resizePending = true;
      return;
   }
   if (m_context.Device() && resizeNeeded && m_drawableWidth > 0 &&
       m_drawableHeight > 0)
   {
      if (m_frameActive)
         m_resizePending = true;
      else
      {
         const bool resized = m_context.Resize(
            static_cast<uint32_t>(m_drawableWidth),
            static_cast<uint32_t>(m_drawableHeight));
         m_renderer.ResetPipelines();
         m_resizePending = !resized;
      }
   }
}

void cVulkanRuntime::ProcessPendingLifecycleResize()
{
   if (!m_lifecycleResizePending.exchange(false, std::memory_order_acq_rel))
      return;
   UpdateWindowDimensions();
   if (!m_appInBackground.load(std::memory_order_relaxed))
      m_windowMinimized.store(false, std::memory_order_release);
}

bool cVulkanRuntime::ApplyPendingResize()
{
   if (m_frameActive)
      return true;
   if (m_context.NeedsSurfaceRecreation())
      m_surfaceRecreationPending.store(true, std::memory_order_release);
   if (m_surfaceRecreationPending.load(std::memory_order_acquire))
   {
      const auto now = std::chrono::steady_clock::now();
      if (now < m_nextSurfaceRecreationAttempt)
         return false;
      m_nextSurfaceRecreationAttempt = now + std::chrono::milliseconds(250);
      char error[512]{};
      if (!VulkanContextRecreateSDLSurface(m_context, error, sizeof(error)))
      {
         if (error[0] && !m_surfaceRecreationErrorReported)
         {
            std::fprintf(stderr, "Vulkan surface recreation failed: %s\n",
               error);
            m_surfaceRecreationErrorReported = true;
         }
         return false;
      }
      m_surfaceRecreationPending.store(false, std::memory_order_release);
      m_resizePending = false;
      m_nextSurfaceRecreationAttempt = {};
      m_surfaceRecreationErrorReported = false;
      m_renderer.ResetPipelines();
      return true;
   }
   if (!m_resizePending)
      return true;
   const bool resized = m_context.Resize(static_cast<uint32_t>(m_drawableWidth),
      static_cast<uint32_t>(m_drawableHeight));
   m_renderer.ResetPipelines();
   if (!resized)
      return false;
   m_resizePending = false;
   return true;
}

void cVulkanRuntime::ConvertCanvas(const void *pixels, int width, int height, int rowBytes,
   int depth, const uint8_t *palette)
{
   if (!pixels || width <= 0 || height <= 0 || rowBytes <= 0) return;
   m_canvasWidth = width;
   m_canvasHeight = height;
   m_canvasPixels.resize(static_cast<size_t>(width) * height * 4);
   for (int y = 0; y < height; ++y)
   {
      const uint8_t *row = static_cast<const uint8_t *>(pixels) +
         static_cast<size_t>(y) * rowBytes;
      for (int x = 0; x < width; ++x)
      {
         uint32_t raw;
         uint8_t r, g, b;
         if (depth == 8)
         {
            raw = row[x];
            if (!palette) continue;
            r = palette[raw * 3]; g = palette[raw * 3 + 1];
            b = palette[raw * 3 + 2];
         }
         else
         {
            const uint16_t value = reinterpret_cast<const uint16_t *>(row)[x];
            raw = value;
            if (depth == 16)
            {
               r = ((value >> 11) & 31) * 255 / 31;
               g = ((value >> 5) & 63) * 255 / 63;
               b = (value & 31) * 255 / 31;
            }
            else
            {
               r = ((value >> 10) & 31) * 255 / 31;
               g = ((value >> 5) & 31) * 255 / 31;
               b = (value & 31) * 255 / 31;
            }
         }
         const size_t offset = (static_cast<size_t>(y) * width + x) * 4;
         m_canvasPixels[offset] = r;
         m_canvasPixels[offset + 1] = g;
         m_canvasPixels[offset + 2] = b;
         m_canvasPixels[offset + 3] =
            RenderBackendVulkanSceneActive() && raw == 0 ? 0 : 255;
      }
   }
   ++m_canvasGeneration;
}

bool cVulkanRuntime::PrepareCanvasTexture()
{
   if (m_canvasPixels.empty() || !m_canvasWidth || !m_canvasHeight)
      return true;
   if (!m_context.FrameActive())
      return false;
   const uint32_t slot = m_context.FrameIndex();
   if (slot >= m_canvasTextures.size())
      return false;
   const int rowBytes = m_canvasWidth * 4;
   void *&texture = m_canvasTextures[slot];
   if (!texture || m_canvasTextureWidths[slot] != m_canvasWidth ||
       m_canvasTextureHeights[slot] != m_canvasHeight)
   {
      if (texture)
         m_renderer.DestroyTexture(texture);
      texture = m_renderer.CreateTexture(m_canvasWidth, m_canvasHeight,
         m_canvasPixels.data(), rowBytes);
      if (!texture) return false;
      m_canvasTextureWidths[slot] = m_canvasWidth;
      m_canvasTextureHeights[slot] = m_canvasHeight;
      m_canvasTextureGenerations[slot] = m_canvasGeneration;
   }
   else if (m_canvasTextureGenerations[slot] != m_canvasGeneration)
   {
      if (!m_renderer.UpdateFrameTexture(texture, m_canvasWidth,
            m_canvasHeight, m_canvasPixels.data(), rowBytes))
         return false;
      m_canvasTextureGenerations[slot] = m_canvasGeneration;
   }
   return true;
}

void cVulkanRuntime::DrawCanvas(vk::CommandBuffer commandBuffer, bool transparent)
{
   const uint32_t slot = m_context.FrameIndex();
   if (slot >= m_canvasTextures.size() || !m_canvasTextures[slot] ||
       !m_canvasWidth || !m_canvasHeight) return;
   m_renderer.SetDepth(false, false);
   m_renderer.SetBlend(kRenderBackendBlendAlpha);
   m_renderer.SetAlphaTest(false);
   m_renderer.SetFog(false, 0);
   m_renderer.SetSampler(0, false,
      RenderBackendVulkanUseLinearCanvasSampling() != 0);
   m_renderer.BindTexture(0, m_canvasTextures[slot]);
   int viewportX, viewportY, viewportWidth, viewportHeight;
   RenderBackendVulkanGetViewport(m_canvasWidth, m_canvasHeight,
      static_cast<int>(m_context.SwapchainExtent().width),
      static_cast<int>(m_context.SwapchainExtent().height), 1, &viewportX,
      &viewportY, &viewportWidth, &viewportHeight);
   m_renderer.SetViewport(viewportX, viewportY, viewportWidth, viewportHeight);
   const float w = static_cast<float>(m_canvasWidth);
   const float h = static_cast<float>(m_canvasHeight);
   const sRenderBackendVertex quad[] = {
      {0,0,0,1, 1,1,1,1, 1, 0,0, 0,0},
      {w,0,0,1, 1,1,1,1, 1, 1,0, 1,0},
      {w,h,0,1, 1,1,1,1, 1, 1,1, 1,1},
      {0,0,0,1, 1,1,1,1, 1, 0,0, 0,0},
      {w,h,0,1, 1,1,1,1, 1, 1,1, 1,1},
      {0,h,0,1, 1,1,1,1, 1, 0,1, 0,1}};
   (void)m_renderer.Draw(commandBuffer, quad, 6, kRenderBackendTriangles,
      false, m_canvasWidth, m_canvasHeight, transparent);
}

bool cVulkanRuntime::Open(int width, int height, const char *title)
{
   if (m_windowOpen)
   {
      m_width = width; m_height = height;
      SDLDisplaySetTitle(title);
      UpdateWindowDimensions();
      return true;
   }
   if (!SDLDisplayOpenWindow(width, height, title))
   {
      return false;
   }
   m_windowOpen = true;
   m_appInBackground.store(false, std::memory_order_relaxed);
   m_windowMinimized.store(false, std::memory_order_relaxed);
   m_lifecycleResizePending.store(false, std::memory_order_relaxed);
   m_width = width; m_height = height;
   UpdateWindowDimensions();
   char error[512]{};
   if (!VulkanContextInitializeSDL(m_context, m_physicalDeviceIndex, error,
         sizeof(error)))
   {
      std::fprintf(stderr, "Vulkan initialization failed: %s\n", error);
      Close();
      return false;
   }
   // Register after Vulkan initialization so this callback runs before any
   // atexit cleanup registered by the Vulkan loader or layers. Exit handlers
   // run in reverse registration order, and Close must use their dispatch
   // state while tearing down the renderer and device.
   if (!m_exitHookRegistered)
   {
      std::atexit(DarkRenderClose);
      m_exitHookRegistered = true;
   }
   const char *capturePath = std::getenv("DARK_RENDER_CAPTURE");
   const char *captureFrameEnv = std::getenv("DARK_RENDER_CAPTURE_FRAME");
   const unsigned long parsedCaptureFrame = captureFrameEnv
      ? std::strtoul(captureFrameEnv, nullptr, 10) : 1ul;
   const uint32_t captureFrame = parsedCaptureFrame > 0 &&
      parsedCaptureFrame <= UINT32_MAX
      ? static_cast<uint32_t>(parsedCaptureFrame) : 1u;
   m_context.ConfigureFrameCapture(capturePath, captureFrame);
#ifndef DARK_VULKAN_SHADER_DIR
#define DARK_VULKAN_SHADER_DIR "generated/vulkan"
#endif
   std::string shaderDir = DARK_VULKAN_SHADER_DIR;
#if !defined(__ANDROID__)
   char executablePath[1024]{};
   if (PlatformGetExecutableDirectory(executablePath,
         static_cast<int>(sizeof(executablePath))))
   {
      std::string executableDirectory(executablePath);
      while (!executableDirectory.empty() &&
             (executableDirectory.back() == '/' ||
              executableDirectory.back() == '\\'))
         executableDirectory.pop_back();
      std::string executableShaderDir = executableDirectory + "/" +
         DARK_VULKAN_SHADER_DIR;
      std::ifstream vertexShader(executableShaderDir + "/scene.vert.spv",
         std::ios::binary);
      if (vertexShader.good())
         shaderDir = std::move(executableShaderDir);
      else
      {
         const size_t separator = executableDirectory.find_last_of("/\\");
         if (separator != std::string::npos)
         {
            std::string parentShaderDir =
               executableDirectory.substr(0, separator) + "/" +
               DARK_VULKAN_SHADER_DIR;
            std::ifstream parentVertexShader(
               parentShaderDir + "/scene.vert.spv", std::ios::binary);
            if (parentVertexShader.good())
               shaderDir = std::move(parentShaderDir);
         }
      }
   }
#endif
   if (!m_renderer.Initialize(m_context,
         (shaderDir + "/scene.vert.spv").c_str(),
         (shaderDir + "/scene.frag.spv").c_str()))
   {
      std::fprintf(stderr, "Vulkan scene renderer initialization failed\n");
      Close();
      return false;
   }
   RenderBackendVulkanSetRenderer(&m_renderer);
   m_windowMinimized = SDLDisplayWindowIsMinimized();
   std::fprintf(stderr, "SDL3 window opened (%dx%d points, %dx%d pixels)\n",
      m_clientWidth, m_clientHeight,
      m_drawableWidth, m_drawableHeight);
   return true;
}

void cVulkanRuntime::Close()
{
   if (m_frameActive)
   {
      RenderBackendVulkanDetach();
      // Shutdown can run from the atexit hook while a frame is only partly
      // recorded. Do not submit or present that frame; the context teardown
      // waits for the device and releases the recording command buffer.
      m_frameActive = false;
   }
   RenderBackendVulkanSetRenderer(nullptr);
   for (void *&texture : m_canvasTextures)
   {
      if (texture)
      {
         m_renderer.DestroyTexture(texture);
         texture = nullptr;
      }
   }
   if (m_movieTexture)
   {
      m_renderer.DestroyTexture(m_movieTexture);
      m_movieTexture = nullptr;
   }
   m_renderer.Shutdown();
   m_context.Shutdown();
   SDLDisplayCloseWindow();
   m_windowOpen = false;
   m_width = m_height = m_clientWidth = m_clientHeight = 0;
   m_drawableWidth = m_drawableHeight = 0;
   m_canvasWidth = m_canvasHeight = 0;
   m_canvasPixels.clear();
   m_frameActive = false;
   m_canvasTextureWidths.fill(0);
   m_canvasTextureHeights.fill(0);
   m_canvasTextureGenerations.fill(0);
   m_canvasGeneration = 0;
   m_activeCommandBuffer = nullptr;
   m_resizePending = false;
   m_windowSizeChanged = false;
   m_lifecycleResizePending.store(false, std::memory_order_release);
   m_windowMinimized.store(false, std::memory_order_release);
   m_appInBackground.store(false, std::memory_order_release);
   m_surfaceRecreationPending.store(false, std::memory_order_release);
   m_nextSurfaceRecreationAttempt = {};
   m_surfaceRecreationErrorReported = false;
}

bool cVulkanRuntime::TakeResize(int *width, int *height)
{
   if (!m_windowOpen || !m_windowSizeChanged || !width || !height) return false;
   *width = m_clientWidth;
   *height = m_clientHeight;
   m_windowSizeChanged = false;
   return true;
}

bool cVulkanRuntime::BeginEventPump()
{
   if (!m_windowOpen) return false;
   ProcessPendingLifecycleResize();
   return true;
}

void cVulkanRuntime::WindowResized()
{
   UpdateWindowDimensions();
}

void cVulkanRuntime::WindowMinimized()
{
   m_windowMinimized = true;
   UpdateWindowDimensions();
}

void cVulkanRuntime::WindowRestored()
{
   m_windowMinimized = false;
   UpdateWindowDimensions();
}

void cVulkanRuntime::ApplicationSuspended()
{
   m_appInBackground.store(true, std::memory_order_relaxed);
   m_windowMinimized.store(true, std::memory_order_relaxed);
}

void cVulkanRuntime::ApplicationResumed()
{
   m_appInBackground.store(false, std::memory_order_relaxed);
   m_surfaceRecreationPending.store(true, std::memory_order_release);
   m_lifecycleResizePending.store(true, std::memory_order_release);
}

void cVulkanRuntime::EndEventPump()
{ ProcessPendingLifecycleResize(); }

int cVulkanRuntime::CaptureComplete() const
{ return m_context.FrameCaptureComplete() ? 1 : 0; }

void cVulkanRuntime::SetPhysicalDeviceIndex(int index)
{ m_physicalDeviceIndex = index; }

void cVulkanRuntime::BeginApplicationFrame()
{
   ProcessPendingLifecycleResize();
   if (!m_windowOpen || m_frameActive ||
       m_windowMinimized.load(std::memory_order_acquire) ||
       m_drawableWidth <= 0 || m_drawableHeight <= 0)
      return;
   if (!ApplyPendingResize()) return;
   vk::CommandBuffer commandBuffer{};
   if (!m_context.BeginFrame({0.0f, 0.0f, 0.0f, 1.0f}, &commandBuffer))
   {
      if (m_context.NeedsSurfaceRecreation())
         m_surfaceRecreationPending.store(true, std::memory_order_release);
      else if (m_context.NeedsSwapchainRecreation())
         UpdateWindowDimensions();
      return;
   }
   m_activeCommandBuffer = commandBuffer;
   m_renderer.BeginFrame(m_context.FrameIndex());
   RenderBackendVulkanAttach(commandBuffer, m_width, m_height, m_clientWidth,
      m_clientHeight, m_drawableWidth, m_drawableHeight);
   m_frameActive = true;
}

void cVulkanRuntime::EndApplicationFrame()
{
   if (!m_frameActive) return;
   if (!PrepareCanvasTexture())
      std::fprintf(stderr, "Vulkan canvas texture update failed\n");
   DrawCanvas(m_activeCommandBuffer, RenderBackendVulkanSceneActive() != 0);
   // Scene pixels are composited into this swapchain image only. The next
   // frame starts from a freshly acquired image, so don't let later canvas
   // updates treat palette index zero as transparent over a nonexistent
   // retained scene.
   RenderBackendDeactivateScene();
   RenderBackendVulkanDetach();
   (void)m_context.EndFrame();
   m_activeCommandBuffer = nullptr;
   m_frameActive = false;
   if (m_context.NeedsSwapchainRecreation())
   {
      if (m_context.NeedsSurfaceRecreation())
         m_surfaceRecreationPending.store(true, std::memory_order_release);
      else
         UpdateWindowDimensions();
   }
}

void cVulkanRuntime::PresentProgress()
{
   if (!m_windowOpen)
      return;

   const bool resumeFrame = m_frameActive;
   if (!resumeFrame)
   {
      BeginApplicationFrame();
      if (!m_frameActive)
         return;
   }

   EndApplicationFrame();
   if (resumeFrame)
      BeginApplicationFrame();
}

void cVulkanRuntime::Present(const void *pixels, int width, int height, int rowBytes,
   int bitDepth, const uint8_t *palette)
{ ConvertCanvas(pixels, width, height, rowBytes, bitDepth, palette); }

bool cVulkanRuntime::PresentMovieFrame(const void *rgba, int width, int height,
   int rowBytes)
{
   if (!m_windowOpen || !rgba || width <= 0 || height <= 0 || rowBytes <= 0 ||
       width > std::numeric_limits<int>::max() / 4)
      return false;
   const size_t pixelStride = static_cast<size_t>(width) * 4;
   const size_t sourceStride = static_cast<size_t>(rowBytes);
   const size_t rowCount = static_cast<size_t>(height);
   const size_t maxSize = std::numeric_limits<size_t>::max();
   if (sourceStride < pixelStride || rowCount > maxSize / pixelStride ||
       (rowCount > 1 && (rowCount - 1) >
          (maxSize - pixelStride) / sourceStride))
      return false;

   std::vector<uint8_t> pixels;
   try
   {
      pixels.resize(pixelStride * rowCount);
   }
   catch (...)
   {
      return false;
   }
   for (int y = 0; y < height; ++y)
      std::memcpy(pixels.data() + static_cast<size_t>(y) * pixelStride,
         static_cast<const uint8_t *>(rgba) +
            static_cast<size_t>(y) * sourceStride, pixelStride);
   // Movie frames are presented synchronously from the game loop. Suspend its
   // current swapchain frame so the movie can acquire and present its own
   // frame, then leave the game loop ready to render the next frame.
   const bool resumeFrame = m_frameActive;
   if (resumeFrame)
      EndApplicationFrame();

   try
   {
      if (m_movieTexture)
      {
         if (!m_renderer.UpdateTexture(m_movieTexture, width, height,
               pixels.data(), static_cast<int>(pixelStride)))
         {
            if (resumeFrame)
               BeginApplicationFrame();
            return false;
         }
      }
      else
      {
         m_movieTexture = m_renderer.CreateTexture(width, height,
            pixels.data(), static_cast<int>(pixelStride));
         if (!m_movieTexture)
         {
            if (resumeFrame)
               BeginApplicationFrame();
            return false;
         }
      }
   }
   catch (...)
   {
      if (resumeFrame)
         BeginApplicationFrame();
      return false;
   }

   vk::CommandBuffer commandBuffer{};
   const bool began = m_context.BeginFrame({0.0f,0.0f,0.0f,1.0f}, &commandBuffer);
   bool drew = false;
   if (began)
   {
      try
      {
         m_renderer.BeginFrame(m_context.FrameIndex());
         m_renderer.SetDepth(false, false);
         m_renderer.SetBlend(kRenderBackendBlendOpaque);
         m_renderer.SetAlphaTest(false);
         m_renderer.SetFog(false, 0);
         m_renderer.SetSampler(0, false, true);
         m_renderer.BindTexture(0, m_movieTexture);
         const float w = static_cast<float>(width);
         const float h = static_cast<float>(height);
         const sRenderBackendVertex quad[] = {
            {0,0,0,1,1,1,1,1,1,0,0,0,0},
            {w,0,0,1,1,1,1,1,1,1,0,0,0},
            {w,h,0,1,1,1,1,1,1,1,1,0,0},
            {0,0,0,1,1,1,1,1,1,0,0,0,0},
            {w,h,0,1,1,1,1,1,1,1,1,0,0},
            {0,h,0,1,1,1,1,1,1,0,1,0,0}};
         const vk::Extent2D extent = m_context.SwapchainExtent();
         const double scale = std::min(
            static_cast<double>(extent.width) / width,
            static_cast<double>(extent.height) / height);
         const int viewportWidth = std::max(
            static_cast<int>(width * scale), 1);
         const int viewportHeight = std::max(
            static_cast<int>(height * scale), 1);
         m_renderer.SetViewport(
            (static_cast<int>(extent.width) - viewportWidth) / 2,
            (static_cast<int>(extent.height) - viewportHeight) / 2,
            viewportWidth, viewportHeight);
         drew = m_renderer.Draw(commandBuffer, quad, 6,
            kRenderBackendTriangles, false, width, height);
      }
      catch (...)
      {
         // Keep Vulkan and allocation failures inside this C ABI boundary.
      }
      (void)m_context.EndFrame();
      m_renderer.BindTexture(0, nullptr);
   }
   if (resumeFrame)
      BeginApplicationFrame();
   return began && drew;
}

}

bool DarkRenderOpen(int width, int height, const char *title)
{ return VulkanRuntime().Open(width, height, title); }
void DarkRenderClose(void) { VulkanRuntime().Close(); }
bool DarkRenderTakeResize(int *width, int *height)
{ return VulkanRuntime().TakeResize(width, height); }
bool DarkRenderBeginEventPump(void)
{ return VulkanRuntime().BeginEventPump(); }
void DarkRenderWindowResized(void) { VulkanRuntime().WindowResized(); }
void DarkRenderWindowMinimized(void) { VulkanRuntime().WindowMinimized(); }
void DarkRenderWindowRestored(void) { VulkanRuntime().WindowRestored(); }
void DarkRenderApplicationSuspended(void)
{ VulkanRuntime().ApplicationSuspended(); }
void DarkRenderApplicationResumed(void)
{ VulkanRuntime().ApplicationResumed(); }
void DarkRenderEndEventPump(void) { VulkanRuntime().EndEventPump(); }
void DarkRenderBeginApplicationFrame(void)
{ VulkanRuntime().BeginApplicationFrame(); }
void DarkRenderEndApplicationFrame(void)
{ VulkanRuntime().EndApplicationFrame(); }
void DarkRenderPresentProgress(void)
{ VulkanRuntime().PresentProgress(); }
extern "C" void RenderBackendSetWindowedClientSizeHint(int width, int height)
{ SDLDisplaySetWindowedClientSizeHint(width, height); }
void DarkRenderPresent(const void *pixels, int width, int height, int rowBytes,
   int bitDepth, const uint8_t *palette)
{ VulkanRuntime().Present(pixels, width, height, rowBytes, bitDepth, palette); }
bool DarkRenderPresentMovieFrame(const void *rgba, int width, int height,
   int rowBytes)
{ return VulkanRuntime().PresentMovieFrame(rgba, width, height, rowBytes); }

extern "C" int RenderBackendCaptureComplete(void)
{ return VulkanRuntime().CaptureComplete(); }
extern "C" void RenderBackendSetPhysicalDeviceIndex(int index)
{ VulkanRuntime().SetPhysicalDeviceIndex(index); }
