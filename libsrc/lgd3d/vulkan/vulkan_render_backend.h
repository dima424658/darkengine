#pragma once

#include "vulkan_scene_renderer.h"
#include <cstdio>
#include <cstdarg>

namespace lgd3d
{
class cVulkanRenderBackend
{
public:
   cVulkanRenderBackend();

   ~cVulkanRenderBackend();

   void SetRenderer(cVulkanSceneRenderer * renderer);

   void Attach(vk::CommandBuffer commandBuffer, int logicalWidth,
               int logicalHeight, int clientWidth, int clientHeight,
               int drawableWidth, int drawableHeight);

   void Detach();
   bool SceneActive() const;

   void SetWindowMetrics(int clientWidth, int clientHeight, int drawableWidth,
                         int drawableHeight);

   void GetViewport(int logicalWidth, int logicalHeight, int drawableWidth,
                    int drawableHeight, bool overlay, int * x, int * y,
                    int * width, int * height) const;

   int UseLinearCanvasSampling() const;
   int Available() const;
   void SetGamma(float gamma);
   void SetScaleToWindow(bool enabled);
   void SetFitToViewport(bool enabled);
   void SetPreserveCanvas(bool enabled);
   int PreserveCanvas() const;
   void SetCanvasOverlayEnabled(bool enabled);
   int CanvasOverlayEnabled() const;

   int GetClientSize(int * width, int * height) const;

   int GetLogicalSize(int * width, int * height) const;

   int ClientToLogicalPoint(int * x, int * y) const;

   int LogicalToClientPoint(int * x, int * y) const;

   int BeginScene();

   void PresentProgress();
   void DeactivateScene();

   void ClearDepth();

   void ClearDepthRect(int x0, int y0, int x1, int y1);

   void ClearCanvasRegion(int x, int y, int width, int height);

   void PushDrawState();

   void PopDrawState();

   void SetDepth(bool compare, bool write);
   void SetBlend(int mode);
   void SetAlphaTest(bool enabled);
   void SetSampler(int level, bool wrap, bool smooth);
   void SetFog(bool enabled, uint32_t rgb);

   void * CreateTexture(int width, int height, const void * rgba, int rowBytes);

   int UpdateTexture(void * texture, int width, int height, const void * rgba,
                     int rowBytes);

   void DestroyTexture(void * texture);
   void BindTexture(int level, void * texture);

   int Draw(int primitive, const sRenderBackendVertex * vertices,
            int vertexCount, bool useSecondTexture);

   void ResetTrace();

   void StartTrace();

   void Trace(const char * format, va_list args);

private:
   cVulkanSceneRenderer * m_renderer{};
   vk::CommandBuffer m_commandBuffer{};
   int m_width{};
   int m_height{};
   int m_drawableWidth{};
   int m_drawableHeight{};
   int m_clientWidth{};
   int m_clientHeight{};
   bool m_preserve{};
   bool m_sceneActive{};
   bool m_canvasOverlayEnabled{true};
   bool m_scaleToWindow{true};
   bool m_fitToViewport{};
   float m_gamma{1.0f};
   FILE * m_trace{};
};
} // namespace lgd3d
