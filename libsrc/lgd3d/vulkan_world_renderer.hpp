#ifndef DARK_VULKAN_WORLD_RENDERER_HPP
#define DARK_VULKAN_WORLD_RENDERER_HPP
#include "world_scene.hpp"
#include "world_shadows.hpp"
#include <vulkan/vulkan.hpp>
#include <memory>
#include <array>
#include <string>
namespace dark {
/* One HDR forward pass followed by a tone-map draw into the application's
   existing render pass. Record() runs after native scene capture. Its command
   buffer must be submitted before the application command buffer, using the
   same frame-slot fence. */
class VulkanWorldRenderer {
public:
   VulkanWorldRenderer();
   ~VulkanWorldRenderer();
   // indirectShadows may be true only if multiDrawIndirect and
   // drawIndirectFirstInstance were enabled when creating device.
   bool Initialize(vk::PhysicalDevice physical,vk::Device device,uint32_t graphicsFamily,
      vk::RenderPass compositePass,vk::Format depthFormat,vk::Format outputFormat,
      const std::string &shaderDirectory,std::string &error,bool indirectShadows=false);
   VkCommandBuffer Record(uint32_t slot,const WorldScene &scene,
      uint32_t width,uint32_t height,std::string &error);
   void Composite(vk::CommandBuffer command,uint32_t slot,int x,int y,uint32_t width,uint32_t height);
   /* HDR image for GPU diagnostics/readback; caller owns synchronization. */
   VkImage HdrImage(uint32_t slot) const;
   // Settings affect subsequent recording; slot resources are resized only
   // after the caller retires that slot's frame fence.
   void SetShadowSettings(const ShadowSettings &settings);
   struct ShadowWorkload {uint32_t renderedViews,reusedViews,drawCalls;double gpuMilliseconds,worldGpuMilliseconds;std::array<double,4> recordCpuMilliseconds;std::array<double,2> depthLightingGpuMilliseconds;uint32_t boundsTests,drawBatches,staticRendered,staticReused,depthCopies;uint64_t depthMemoryBytes;};
   ShadowWorkload GetShadowWorkload(uint32_t slot) const;
   struct TextureWorkload {uint32_t createdImages=0,createdStagingBuffers=0,uploads=0,evictedImages=0;uint64_t cachedBytes=0;};
   TextureWorkload GetTextureWorkload(uint32_t slot) const;
   const ShadowSelection &GetShadowSelection() const;
   void SetTextureCacheBudget(uint64_t bytes); // Evicts only images unreferenced by frame slots.
   void Shutdown(); // All GPU work must be retired first.
private:
   struct Impl;
   std::unique_ptr<Impl> m;
};
}
#endif
