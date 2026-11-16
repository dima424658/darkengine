#include "vulkan_world_renderer.hpp"
#include "canvas_pixels.hpp"
#include <config.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include <render_backend.h>
#include <tracy/Tracy.hpp>
#include <platform_services.h>

#include "runtime.h"
#include "event_bridge.h"
#include <display_window.h>
#include "vulkan_context.hpp"
#include "vulkan_render_backend.hpp"
#include <sdl_vulkan.hpp>
#include "vulkan_scene_renderer.hpp"
#include "canvas_snapshot.hpp"
#include <config.h>

namespace
{
vk::SurfaceKHR CreateSDLVulkanSurface(vk::Instance instance,
   std::string &surfaceError)
{
   char error[512]{};
   vk::SurfaceKHR surface{};
   if (!SDLVulkanCreateSurface(instance, surface, error, sizeof(error)))
      surfaceError = error;
   return surface;
}

bool ModernWorldRequested() {
   char mode[32]{};
   return config_get_raw("renderer_mode",mode,sizeof(mode)) && (std::strcmp(mode,"modern")==0 || std::strcmp(mode,"modern_rt")==0);
}
class cVulkanRuntime
{
public:
   bool Open(int width, int height, const char *title);
   bool WorldEnabled() const { return m_modernWorldEnabled; }
   bool RenderWorld();
   bool InitializeWorldRenderer();
   void Close();
   bool TakeResize(int *width, int *height);
   bool BeginEventPump();
   void WindowResized();
   void WindowMinimized();
   void WindowRestored();
   void ApplicationSuspended();
   void ApplicationResumed();
   void EndEventPump();
   void SetPhysicalDeviceIndex(int index);
   void BeginApplicationFrame();
   void EndApplicationFrame();
   void PresentProgress();
   bool PresentCanvasRect(const void *pixels, int width, int height, int rowBytes,
      int depth, const uint8_t *palette, int x0, int y0, int x1, int y1);
   bool ClearOverlay(void *pixels, int width, int height, int rowBytes, int depth);
   bool HasDirectCanvas() const { return m_directCanvas; }
   void InvalidateCanvas();
   void PresentRect(const void *pixels, int width, int height, int rowBytes,
      int depth, const uint8_t *palette, int x0, int y0, int x1, int y1);
   void Present(const void *pixels, int width, int height, int rowBytes,
      int bitDepth, const uint8_t *palette);
   bool PresentMovieFrame(const void *rgba, int width, int height,
      int rowBytes);
private:
   void UpdateWindowDimensions();
   void HandleSwapchainRecreation();
   void ProcessPendingLifecycleResize();
   bool ApplyPendingResize();
   bool PreparePresentation();
   bool CaptureCanvas(const void *pixels, int width, int height, int rowBytes,
      int depth, const uint8_t *palette, int x0, int y0, int x1, int y1);
   bool PrepareCanvasTexture();
   void DrawCanvas(vk::CommandBuffer commandBuffer, bool transparent);

   bool m_windowOpen{};
   cVulkanContext m_context;
   dark::VulkanWorldRenderer m_worldRenderer;
   bool m_modernWorldEnabled{}, m_restoreWorld{}, m_worldRendered{};
   vk::CommandBuffer m_worldPreparation{};
   std::string m_worldShaderDirectory;
   bool m_worldFeaturesAvailable{};
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
   std::array<void *, kVulkanFramesInFlight> m_canvasPaletteTextures{};
   std::array<uint64_t, kVulkanFramesInFlight> m_canvasPaletteGenerations{};
   std::atomic<bool> m_windowMinimized{};
   std::atomic<bool> m_appInBackground{};
   std::atomic<bool> m_lifecycleResizePending{};
   std::atomic<bool> m_surfaceRecreationPending{};
   std::chrono::steady_clock::time_point m_nextSurfaceRecreationAttempt{};
   bool m_surfaceRecreationErrorReported{};
   bool m_frameActive{};
   std::chrono::steady_clock::time_point m_frameStart{},m_acquiredAt{},m_lastSubmission{};
   int m_physicalDeviceIndex{-1};
   bool m_resizePending{};
   bool m_windowSizeChanged{};
   bool m_exitHookRegistered{};
   void *m_movieTexture{};
   cCanvasSnapshot m_canvas;
   const void *m_canvasSource{};
   bool m_directCanvas{};
   bool m_overlayClearEligible{};
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

void cVulkanRuntime::HandleSwapchainRecreation()
{
   if (!m_context.NeedsSwapchainRecreation())
      return;
   if (m_context.NeedsSurfaceRecreation())
      m_surfaceRecreationPending.store(true, std::memory_order_release);
   else
      UpdateWindowDimensions();
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
      uint32_t drawableWidth = 0;
      uint32_t drawableHeight = 0;
      std::string surfaceError;
      if (!SDLVulkanGetDrawableSize(drawableWidth, drawableHeight, error,
             sizeof(error)) ||
          !m_context.RecreateSurface(
             [&surfaceError](vk::Instance instance) {
                return CreateSDLVulkanSurface(instance, surfaceError);
             }, drawableWidth, drawableHeight))
      {
         if (!surfaceError.empty())
            std::snprintf(error, sizeof(error), "%s", surfaceError.c_str());
         else if (!error[0])
            std::snprintf(error, sizeof(error), "%s",
               "Vulkan surface recreation failed");
         if (error[0] && !m_surfaceRecreationErrorReported)
         {
            std::fprintf(stderr, "Vulkan surface recreation failed: %s\n",
               error);
            m_surfaceRecreationErrorReported = true;
         }
         return false;
      }
      m_surfaceRecreationPending.store(false, std::memory_order_release);
      m_drawableWidth = static_cast<int>(drawableWidth);
      m_drawableHeight = static_cast<int>(drawableHeight);
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

bool cVulkanRuntime::CaptureCanvas(const void *pixels, int width, int height, int rowBytes,
   int depth, const uint8_t *palette, int x0, int y0, int x1, int y1)
{
   try
   {
      if (m_canvasSource != pixels)
      {
         x0 = y0 = 0;
         x1 = width;
         y1 = height;
      }
      if (m_canvas.UpdateRect(pixels, width, height, rowBytes, depth, palette,
            RenderBackendVulkanSceneActive() != 0, x0, y0, x1, y1))
      {
         m_canvasWidth = width;
         m_canvasHeight = height;
         m_canvasSource = pixels;
         // Partial flushes cannot prove that other rows were not written.
         m_overlayClearEligible = x0 <= 0 && y0 <= 0 && x1 >= width && y1 >= height;
         return true;
      }
   }
   catch (const std::exception &)
   {
      // Retain the last complete canvas if allocating a new snapshot fails.
   }
   return false;
}

bool cVulkanRuntime::PrepareCanvasTexture()
{
   if (m_canvas.Pixels().empty() || !m_canvasWidth || !m_canvasHeight)
      return true;
   if (!m_context.FrameActive())
      return false;
   const uint32_t slot = m_context.FrameIndex();
   if (slot >= m_canvasTextures.size())
      return false;
   const int rowBytes = m_canvas.RowBytes();
   const int textureWidth = rowBytes / 4;
   void *&texture = m_canvasTextures[slot];
   if (!texture || m_canvasTextureWidths[slot] != textureWidth ||
       m_canvasTextureHeights[slot] != m_canvasHeight)
   {
      if (texture)
         m_renderer.DestroyTexture(texture);
      texture = m_renderer.CreateTexture(textureWidth, m_canvasHeight,
         m_canvas.Pixels().data(), rowBytes);
      if (!texture) return false;
      m_canvasTextureWidths[slot] = textureWidth;
      m_canvasTextureHeights[slot] = m_canvasHeight;
      m_canvasTextureGenerations[slot] = m_canvas.Generation();
   }
   else if (m_canvasTextureGenerations[slot] != m_canvas.Generation())
   {
      if (!m_renderer.UpdateFrameTexture(texture, textureWidth,
            m_canvasHeight, m_canvas.Pixels().data(), rowBytes))
         return false;
      m_canvasTextureGenerations[slot] = m_canvas.Generation();
   }
   if (m_canvas.Depth() == 8)
   {
      void *&palette = m_canvasPaletteTextures[slot];
      if (!palette)
         palette = m_renderer.CreateTexture(256, 1, m_canvas.Palette().data(), 1024);
      else if (m_canvasPaletteGenerations[slot] != m_canvas.PaletteGeneration() &&
               !m_renderer.UpdateFrameTexture(palette, 256, 1,
                  m_canvas.Palette().data(), 1024))
         return false;
      if (!palette) return false;
      m_canvasPaletteGenerations[slot] = m_canvas.PaletteGeneration();
   }

   return true;
}

void cVulkanRuntime::DrawCanvas(vk::CommandBuffer commandBuffer, bool transparent)
{
   const uint32_t slot = m_context.FrameIndex();
   if (!RenderBackendVulkanCanvasOverlayEnabled() ||
       slot >= m_canvasTextures.size() || !m_canvasTextures[slot] ||
       !m_canvasWidth || !m_canvasHeight) return;
   const bool smooth = RenderBackendVulkanUseLinearCanvasSampling() != 0;
   const uint32_t canvasMode = static_cast<uint32_t>(m_canvas.Depth()) |
      (m_canvas.Transparent() ? 0x100u : 0u) | (smooth ? 0x200u : 0u);
   int x, y, width, height;
   RenderBackendVulkanGetViewport(m_canvasWidth, m_canvasHeight,
      static_cast<int>(m_context.SwapchainExtent().width),
      static_cast<int>(m_context.SwapchainExtent().height), 1, &x, &y, &width, &height);
   (void)m_renderer.DrawOverlay(commandBuffer, m_canvasTextures[slot],
      m_canvas.Depth() == 8 ? m_canvasPaletteTextures[slot] : nullptr,
      m_canvasWidth, m_canvasHeight,
      vk::Rect2D({x, y}, {static_cast<uint32_t>(width), static_cast<uint32_t>(height)}),
      true, smooth, canvasMode, transparent);
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
   RenderBackendTraceReset();
   m_appInBackground.store(false, std::memory_order_relaxed);
   m_windowMinimized.store(false, std::memory_order_relaxed);
   m_lifecycleResizePending.store(false, std::memory_order_relaxed);
   m_width = width; m_height = height;
   UpdateWindowDimensions();
   char error[512]{};
   std::vector<const char *> surfaceExtensions;
   uint32_t drawableWidth = 0;
   uint32_t drawableHeight = 0;
   if (!SDLVulkanGetInstanceExtensions(surfaceExtensions, drawableWidth,
          drawableHeight, error, sizeof(error)))
   {
      std::fprintf(stderr, "Vulkan initialization failed: %s\n", error);
      Close();
      return false;
   }
   std::string surfaceError;
   int vsync = 1;
   config_get_int("vsync", &vsync);
   m_context.SetVSync(vsync != 0);
   if (!m_context.Initialize(surfaceExtensions,
         [&surfaceError](vk::Instance instance) {
            return CreateSDLVulkanSurface(instance, surfaceError);
         }, drawableWidth, drawableHeight, m_physicalDeviceIndex, error,
         sizeof(error), ModernWorldRequested()))
   {
      if (!surfaceError.empty())
         std::snprintf(error, sizeof(error), "%s", surfaceError.c_str());
      std::fprintf(stderr, "Vulkan initialization failed: %s\n", error);
      Close();
      return false;
   }
   m_drawableWidth = static_cast<int>(drawableWidth);
   m_drawableHeight = static_cast<int>(drawableHeight);
   // Register after Vulkan initialization so this callback runs before any
   // atexit cleanup registered by the Vulkan loader or layers. Exit handlers
   // run in reverse registration order, and Close must use their dispatch
   // state while tearing down the renderer and device.
   if (!m_exitHookRegistered)
   {
      std::atexit(DarkRenderClose);
      m_exitHookRegistered = true;
   }
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
   m_worldFeaturesAvailable = m_context.WorldRenderingEnabled();
   if (!m_renderer.Initialize(m_context,
         (shaderDir + "/scene.vert.spv").c_str(),
         (shaderDir + "/scene.frag.spv").c_str()))
   {
      std::fprintf(stderr, "Vulkan scene renderer initialization failed\n");
      Close();
      return false;
   }
   m_worldShaderDirectory = shaderDir;
   if (ModernWorldRequested()) InitializeWorldRenderer();
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
   for (void *&texture : m_canvasPaletteTextures)
   {
      if (texture) m_renderer.DestroyTexture(texture);
      texture = nullptr;
   }
   if (m_movieTexture)
   {
      m_renderer.DestroyTexture(m_movieTexture);
      m_movieTexture = nullptr;
   }
   m_renderer.Shutdown();
   m_worldRenderer.Shutdown();
   m_modernWorldEnabled = m_restoreWorld = m_worldRendered = false;
   m_worldPreparation = nullptr;

   m_worldFeaturesAvailable = false;
   m_context.Shutdown();
   SDLDisplayCloseWindow();
   m_windowOpen = false;
   m_width = m_height = m_clientWidth = m_clientHeight = 0;
   m_drawableWidth = m_drawableHeight = 0;
   m_canvasWidth = m_canvasHeight = 0;
   m_canvas = cCanvasSnapshot{};
   m_canvasSource = nullptr;
   m_directCanvas = false;
   m_overlayClearEligible = false;
   m_frameActive = false;
   m_canvasTextureWidths.fill(0);
   m_canvasTextureHeights.fill(0);
   m_canvasTextureGenerations.fill(0);
   m_canvasPaletteGenerations.fill(0);
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
   if (!m_windowOpen || !m_windowSizeChanged || !width || !height)
      return false;
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
{
   ProcessPendingLifecycleResize();
}

void cVulkanRuntime::SetPhysicalDeviceIndex(int index)
{ m_physicalDeviceIndex = index; }

bool cVulkanRuntime::PreparePresentation()
{
   if (SDLDisplayStartupWindowPending())
   {
      if (!SDLDisplayShowForPresentation())
         return false;
      UpdateWindowDimensions();
   }
   return m_drawableWidth > 0 && m_drawableHeight > 0 && ApplyPendingResize();
}

void cVulkanRuntime::BeginApplicationFrame()
{
   ZoneScopedN("Vulkan begin frame");
   if (!m_frameActive)
   {
      // No direct capture in the preceding frame (e.g. paused flushing or a
      // menu) means its writes are not covered by the retained overlay rows.
      m_overlayClearEligible = m_overlayClearEligible && m_directCanvas;
      m_directCanvas = false;
   }
   ProcessPendingLifecycleResize();
   if (!m_windowOpen || m_frameActive ||
       m_windowMinimized.load(std::memory_order_acquire) || !PreparePresentation())
      return;
   if (m_restoreWorld)
   {
      m_context.RaiiDevice().waitIdle();
      m_worldRenderer.Shutdown();
      m_worldFeaturesAvailable = m_context.WorldRenderingEnabled();
      m_restoreWorld = false;
      InitializeWorldRenderer();
   }
   m_frameStart=std::chrono::steady_clock::now();
   m_worldRendered = false;m_worldPreparation = nullptr;
   vk::CommandBuffer commandBuffer{};
   if (!m_context.BeginFrame({0.0f, 0.0f, 0.0f, 1.0f}, &commandBuffer))
   {
      if (m_context.NeedsSurfaceRecreation())
         m_surfaceRecreationPending.store(true, std::memory_order_release);
      else if (m_context.NeedsSwapchainRecreation())
         UpdateWindowDimensions();
      return;
   }
   m_acquiredAt=std::chrono::steady_clock::now();
   m_activeCommandBuffer = commandBuffer;
   m_renderer.BeginFrame(m_context.FrameIndex());
   RenderBackendVulkanAttach(commandBuffer, m_width, m_height, m_clientWidth,
      m_clientHeight, m_drawableWidth, m_drawableHeight);
   m_frameActive = true;
}

void cVulkanRuntime::EndApplicationFrame()
{
   ZoneScopedN("Vulkan present");
   if (!m_frameActive) return;
   const auto canvasStart=std::chrono::steady_clock::now();
   const bool canvasReady = PrepareCanvasTexture();
   if (!canvasReady)
      std::fprintf(stderr, "Vulkan canvas texture update failed\n");
   const bool sceneActive = RenderBackendVulkanSceneActive() != 0;
   DrawCanvas(m_activeCommandBuffer, sceneActive);
   // Scene pixels are composited into this swapchain image only. The next
   // frame starts from a freshly acquired image, so don't let later canvas
   // updates treat palette index zero as transparent over a nonexistent
   // retained scene.
   RenderBackendDeactivateScene();
   RenderBackendVulkanDetach();
   const auto submitStart=std::chrono::steady_clock::now();
   const bool submitted = m_context.EndFrame(m_worldPreparation);
   if (submitted && (sceneActive || (canvasReady && m_canvasWidth > 0)))
      SDLDisplayRevealAfterPresent();
   if(submitted) {
      const auto now=std::chrono::steady_clock::now();
      const double interval=m_lastSubmission.time_since_epoch().count()?std::chrono::duration<double,std::milli>(now-m_lastSubmission).count():-1;
      RenderBackendTrace("frame-pacing modern=%u interval_ms=%.3f render_submit_ms=%.3f acquire_ms=%.3f canvas_ms=%.3f submit_present_ms=%.3f",m_worldRendered?1u:0u,
         interval,std::chrono::duration<double,std::milli>(now-m_frameStart).count(),
         std::chrono::duration<double,std::milli>(m_acquiredAt-m_frameStart).count(),
         std::chrono::duration<double,std::milli>(submitStart-canvasStart).count(),
         std::chrono::duration<double,std::milli>(now-submitStart).count());
      m_lastSubmission=now;
   }
   if (!submitted && m_modernWorldEnabled)
   {
      m_modernWorldEnabled = false;m_restoreWorld = true;
   }
   m_worldPreparation = nullptr;
   m_activeCommandBuffer = nullptr;
   m_frameActive = false;
   HandleSwapchainRecreation();
}

bool cVulkanRuntime::InitializeWorldRenderer()
{
   std::string failure;
   m_modernWorldEnabled = m_worldFeaturesAvailable && m_worldRenderer.Initialize(
      m_context.PhysicalDevice(),m_context.Device(),m_context.GraphicsQueueFamily(),
      m_context.RenderPass(),m_context.DepthFormat(),m_context.SwapchainFormat(),m_worldShaderDirectory,failure,m_context.WorldIndirectShadows());
   if (!m_modernWorldEnabled)
      std::fprintf(stderr,"Modern world unavailable: %s; retaining legacy renderer\n",
         failure.empty()?m_context.WorldRenderingUnavailableReason().c_str():failure.c_str());
   else {
      dark::ShadowSettings settings;
      int indirect=1;config_get_int("renderer_shadow_indirect",&indirect);settings.indirect=indirect!=0;
      int staticDepth=0;config_get_int("renderer_shadow_static_cache",&staticDepth);settings.staticDepthCache=staticDepth!=0;
      int lightBudget=int(settings.localLights),faceBudget=int(settings.localFaces);
      config_get_int("renderer_shadow_lights",&lightBudget);
      config_get_int("renderer_shadow_faces",&faceBudget);
      settings.localLights=uint32_t(std::clamp(lightBudget,0,int(dark::ShadowSettings::maxLocalLights)));
      settings.localFaces=uint32_t(std::clamp(faceBudget,0,int(dark::ShadowSettings::maxLocalFaces)));
      // Retain lower-resolution shadows when camera priority moves a light
      // outside the high-resolution pool; otherwise its occlusion disappears.
      int distantShadows=1;config_get_int("renderer_shadow_cache",&distantShadows);
      if(distantShadows) {
         settings.cachedLights=dark::ShadowSettings::maxCachedLights;
         settings.cachedFaces=dark::ShadowSettings::maxCachedFaces;
      }
      char filter[32]{};
      if(config_get_raw("renderer_shadow_filter",filter,sizeof(filter))) {
         if(std::strcmp(filter,"hard")==0)settings.filter=dark::ShadowFilter::Hard;
         else if(std::strcmp(filter,"pcf")==0)settings.filter=dark::ShadowFilter::PCF;
         else if(std::strcmp(filter,"pcss")!=0)
            std::fprintf(stderr,"Unknown renderer_shadow_filter '%s'; using pcss\n",filter);
      }
      m_worldRenderer.SetShadowSettings(settings);
      const char *name=settings.filter==dark::ShadowFilter::Hard?"hard":settings.filter==dark::ShadowFilter::PCF?"pcf":"pcss";
      std::fprintf(stderr,"Modern world lighting enabled (experimental, %s shadows, %u local lights / %u faces)\n",name,settings.localLights,settings.localFaces);
   }
   return m_modernWorldEnabled;
}

bool cVulkanRuntime::RenderWorld()
{
   if (!m_modernWorldEnabled || !m_frameActive || m_worldRendered ||
       !dark::GetWorldScene().hasCamera || dark::GetWorldScene().meshes.empty()) return false;
   int x,y,width,height;
   const auto extent=m_context.SwapchainExtent();
   RenderBackendVulkanGetViewport(m_width,m_height,extent.width,extent.height,0,&x,&y,&width,&height);
   if (width<=0 || height<=0) return false;
   std::string failure;
   const auto recordStart=std::chrono::steady_clock::now();
   m_worldPreparation=m_worldRenderer.Record(m_context.FrameIndex(),dark::GetWorldScene(),
      uint32_t(width),uint32_t(height),failure);
   if (!m_worldPreparation)
   {
      std::fprintf(stderr,"Modern world disabled: %s; retaining legacy renderer\n",failure.c_str());
      m_modernWorldEnabled = false;m_worldFeaturesAvailable = false;
      return false;
   }
   m_worldRenderer.Composite(m_activeCommandBuffer,m_context.FrameIndex(),x,y,uint32_t(width),uint32_t(height));
   m_renderer.InvalidateDrawState();
   const auto shadows=m_worldRenderer.GetShadowWorkload(m_context.FrameIndex());
   RenderBackendTrace("world-shadow-culling bounds_tests=%u",shadows.boundsTests);
   RenderBackendTrace("world-shadow-submission batches=%u",shadows.drawBatches);
   RenderBackendTrace("world-shadow-static rendered=%u reused=%u copies=%u",shadows.staticRendered,shadows.staticReused,shadows.depthCopies);
   RenderBackendTrace("world-shadow-memory slot_bytes=%llu",static_cast<unsigned long long>(shadows.depthMemoryBytes));
   const auto &selection=m_worldRenderer.GetShadowSelection();
   RenderBackendTrace("world-shadow-budget eligible_lights=%u selected_lights=%u selected_faces=%u",
      selection.eligibleLocalLights,uint32_t(selection.localLights.size()),selection.localFaces);
   RenderBackendTrace("world-shadow-cache selected_lights=%u",uint32_t(selection.cachedLights.size()));
   RenderBackendTrace("world-shadows slot=%u extent=%ux%u rendered=%u reused=%u draws=%u previous_slot_gpu_ms=%.3f previous_world_gpu_ms=%.3f record_cpu_ms=%.3f",m_context.FrameIndex(),uint32_t(width),uint32_t(height),
      shadows.renderedViews,shadows.reusedViews,shadows.drawCalls,shadows.gpuMilliseconds,shadows.worldGpuMilliseconds,
      std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-recordStart).count());
   RenderBackendTrace("world-record-cpu plan_ms=%.3f uploads_tiles_ms=%.3f shadows_ms=%.3f lighting_ms=%.3f",
      shadows.recordCpuMilliseconds[0],shadows.recordCpuMilliseconds[1],shadows.recordCpuMilliseconds[2],shadows.recordCpuMilliseconds[3]);
   RenderBackendTrace("world-gpu-phases previous_depth_ms=%.3f previous_lighting_ms=%.3f",
      shadows.depthLightingGpuMilliseconds[0],shadows.depthLightingGpuMilliseconds[1]);
   const auto textures=m_worldRenderer.GetTextureWorkload(m_context.FrameIndex());
   RenderBackendTrace("world-textures created_images=%u created_staging=%u uploads=%u evicted_images=%u cached_bytes=%llu",
      textures.createdImages,textures.createdStagingBuffers,textures.uploads,textures.evictedImages,static_cast<unsigned long long>(textures.cachedBytes));
   m_worldRendered=true;return true;
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
{
   PresentRect(pixels, width, height, rowBytes, bitDepth, palette, 0, 0, width, height);
}

void cVulkanRuntime::PresentRect(const void *pixels, int width, int height,
   int rowBytes, int depth, const uint8_t *palette, int x0, int y0, int x1, int y1)
{
   m_directCanvas = false;
   CaptureCanvas(pixels, width, height, rowBytes, depth, palette, x0, y0, x1, y1);
}

void cVulkanRuntime::InvalidateCanvas()
{
   m_canvas = cCanvasSnapshot{};
   m_canvasWidth = m_canvasHeight = 0;
   m_canvasSource = nullptr;
   m_directCanvas = false;
   m_overlayClearEligible = false;
   m_canvasTextureGenerations.fill(0);
   m_canvasPaletteGenerations.fill(0);
}

bool cVulkanRuntime::PresentCanvasRect(const void *pixels, int width, int height,
   int rowBytes, int depth, const uint8_t *palette, int x0, int y0, int x1, int y1)
{
   // Preserve the legacy visible-canvas path for menus, software rendering,
   // retained editor panes, and out-of-frame progress updates.
   if (!m_frameActive || !RenderBackendVulkanSceneActive() || RenderBackendPreserveCanvas())
      return false;
   if (!CaptureCanvas(pixels, width, height, rowBytes, depth, palette, x0, y0, x1, y1))
      return false;
   m_directCanvas = true;
   return true;
}

bool cVulkanRuntime::ClearOverlay(void *pixels, int width, int height,
   int rowBytes, int depth)
{
   const bool eligible = m_overlayClearEligible;
   m_overlayClearEligible = false;
   return eligible && pixels == m_canvasSource &&
      m_canvas.ClearOverlay(pixels, width, height, rowBytes, depth);
}

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

   // Movie frames acquire their own swapchain image instead of passing
   // through BeginApplicationFrame. Apply a resize deferred while the game
   // frame was active before calculating this movie frame's fit viewport.
   if (!PreparePresentation())
   {
      if (resumeFrame)
         BeginApplicationFrame();
      return true;
   }

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
         const vk::Extent2D extent = m_context.SwapchainExtent();
         const double scale = std::min(
            static_cast<double>(extent.width) / width,
            static_cast<double>(extent.height) / height);
         const int viewportWidth = std::max(
            static_cast<int>(width * scale), 1);
         const int viewportHeight = std::max(
            static_cast<int>(height * scale), 1);
         drew = m_renderer.DrawOverlay(commandBuffer, m_movieTexture, nullptr,
            width, height, vk::Rect2D(
               {(static_cast<int>(extent.width) - viewportWidth) / 2,
                (static_cast<int>(extent.height) - viewportHeight) / 2},
               {static_cast<uint32_t>(viewportWidth), static_cast<uint32_t>(viewportHeight)}),
            false, true);
      }
      catch (...)
      {
         // Keep Vulkan and allocation failures inside this C ABI boundary.
      }
      if (m_context.EndFrame() && drew)
         SDLDisplayRevealAfterPresent();
      m_renderer.BindTexture(0, nullptr);
      HandleSwapchainRecreation();
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

extern "C" void RenderBackendSetPhysicalDeviceIndex(int index)
{ VulkanRuntime().SetPhysicalDeviceIndex(index); }

extern "C" int RenderBackendPresentCanvasRect(const void *pixels, int width, int height,
   int rowBytes, int depth, const uint8_t *palette, int x0, int y0, int x1, int y1)
{
   return VulkanRuntime().PresentCanvasRect(pixels, width, height, rowBytes,
      depth, palette, x0, y0, x1, y1);
}

extern "C" int RenderBackendClearOverlayPixels(void *pixels, int width, int height,
   int rowBytes, int depth)
{
   return VulkanRuntime().ClearOverlay(pixels, width, height, rowBytes, depth);
}

bool DarkRenderHasDirectCanvas()
{ return VulkanRuntime().HasDirectCanvas(); }

void DarkRenderPresentRect(const void *pixels, int width, int height,
   int rowBytes, int depth, const uint8_t *palette, int x0, int y0, int x1, int y1)
{
   VulkanRuntime().PresentRect(pixels, width, height, rowBytes, depth,
      palette, x0, y0, x1, y1);
}
void DarkRenderInvalidateCanvas()
{ VulkanRuntime().InvalidateCanvas(); }
extern "C" int RenderBackendWorldEnabled(void) { return VulkanRuntime().WorldEnabled()?1:0; }
extern "C" int RenderBackendRenderWorld(void)
{
   try { return VulkanRuntime().RenderWorld()?1:0; }
   catch(const std::exception &error) { std::fprintf(stderr,"Modern world failed: %s\n",error.what());return 0; }
}
