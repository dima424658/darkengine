#ifndef DARK_VULKAN_SHADOW_MAPS_HPP
#define DARK_VULKAN_SHADOW_MAPS_HPP
#include "world_shadows.hpp"
#include "vulkan_world_geometry.hpp"
#include <vulkan/vulkan.hpp>
namespace dark {
// Material layout bindings: 1=ShadowView SSBO (vertex/fragment), 2=material SSBO,
// 7=sampled texture array, 10=vertex instance SSBO (owned/updated here).
// Caller uploads 1/2/7 before recording and retires the slot fence before reuse.
class VulkanShadowMaps {
public:
   struct Stats {uint32_t renderedViews=0,reusedViews=0,drawCalls=0;double gpuMilliseconds=-1;uint32_t boundsTests=0,drawBatches=0,staticRendered=0,staticReused=0,depthCopies=0;uint64_t depthMemoryBytes=0;};
   VulkanShadowMaps();
   ~VulkanShadowMaps();
   bool Initialize(vk::PhysicalDevice physical,vk::Device device,vk::DescriptorSetLayout materialLayout,
      const std::string &shaderDirectory,std::string &error,uint32_t graphicsFamily=UINT32_MAX,bool indirectEnabled=false);
   bool Prepare(vk::CommandBuffer command,uint32_t slot,const ShadowSettings &settings,std::string &error);
   bool Record(vk::CommandBuffer command,uint32_t slot,const ShadowPlan &plan,const WorldScene &scene,
      const VulkanWorldGeometry &geometry,vk::DescriptorSet materials,
      const std::unordered_map<RenderSceneHandle,uint32_t> &materialIndices,std::string &error);
   vk::ImageView LocalView(uint32_t slot) const;
   vk::ImageView SunView(uint32_t slot) const;
   vk::ImageView CachedView(uint32_t slot) const;
   VkImage LocalImage(uint32_t slot) const; // Diagnostics; caller owns synchronization.
   VkImage SunImage(uint32_t slot) const;
   Stats FrameStats(uint32_t slot) const;
   void Shutdown(); // Retire GPU work before shutdown or slot reuse.
private:
   struct Impl;
   std::unique_ptr<Impl> m;
};
}
#endif
