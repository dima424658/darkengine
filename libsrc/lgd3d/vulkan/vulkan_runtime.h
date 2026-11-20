#pragma once

#include "vulkan_context.h"
#include "vulkan_scene_renderer.h"
#include <canvas_snapshot.h>
#include <array>
#include <atomic>
#include <chrono>

namespace lgd3d
{
class cVulkanRuntime
{
public:
   bool Open(int width, int height, const char * title);
   void Close();
   bool TakeResize(int * width, int * height);
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
   bool PresentCanvasRect(const void * pixels, int width, int height,
                          int rowBytes, int depth, const uint8_t * palette,
                          int x0, int y0, int x1, int y1);
   bool ClearOverlay(void * pixels, int width, int height, int rowBytes,
                     int depth);
   bool HasDirectCanvas() const;
   void InvalidateCanvas();
   void PresentRect(const void * pixels, int width, int height, int rowBytes,
                    int depth, const uint8_t * palette, int x0, int y0, int x1,
                    int y1);
   void Present(const void * pixels, int width, int height, int rowBytes,
                int bitDepth, const uint8_t * palette);
   bool PresentMovieFrame(const void * rgba, int width, int height,
                          int rowBytes);

private:
   void UpdateWindowDimensions();
   void HandleSwapchainRecreation();
   void ProcessPendingLifecycleResize();
   bool ApplyPendingResize();
   bool CaptureCanvas(const void * pixels, int width, int height, int rowBytes,
                      int depth, const uint8_t * palette, int x0, int y0,
                      int x1, int y1);
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
   std::array<uint64_t, kVulkanFramesInFlight> m_canvasTextureGenerations{};
   std::array<void *, kVulkanFramesInFlight> m_canvasPaletteTextures{};
   std::array<uint64_t, kVulkanFramesInFlight> m_canvasPaletteGenerations{};
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
   void * m_movieTexture{};
   cCanvasSnapshot m_canvas;
   const void * m_canvasSource{};
   bool m_directCanvas{};
   bool m_overlayClearEligible{};
};
} // namespace lgd3d
