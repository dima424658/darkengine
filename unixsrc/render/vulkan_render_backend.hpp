#ifndef DARK_VULKAN_RENDER_BACKEND_HPP
#define DARK_VULKAN_RENDER_BACKEND_HPP

#include "vulkan_scene_renderer.hpp"

void RenderBackendVulkanSetRenderer(cVulkanSceneRenderer *renderer);
void RenderBackendVulkanAttach(vk::CommandBuffer commandBuffer,
   int logicalWidth, int logicalHeight, int clientWidth, int clientHeight,
   int drawableWidth, int drawableHeight);
void RenderBackendVulkanDetach(void);
int RenderBackendVulkanSceneActive(void);
void RenderBackendVulkanSetWindowMetrics(int clientWidth, int clientHeight,
   int drawableWidth, int drawableHeight);
void RenderBackendVulkanGetViewport(int logicalWidth, int logicalHeight,
   int drawableWidth, int drawableHeight, int overlay, int *x, int *y,
   int *width, int *height);
int RenderBackendVulkanUseLinearCanvasSampling(void);

#endif
