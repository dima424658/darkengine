#ifndef DARK_VULKAN_RTXDI_HPP
#define DARK_VULKAN_RTXDI_HPP
#include "world_scene.hpp"
#include <vulkan/vulkan.hpp>
#include <array>
#include <memory>
#include <string>
namespace dark {
class VulkanRtxdi {
public:
   struct Texture {vk::Image image;vk::ImageView view;vk::Format format;};
   struct Input {
      std::array<Texture,6> surface;
      vk::DescriptorBufferInfo lights,draws,links;
      const std::vector<RenderSceneHandle> *lightIds=nullptr;
      float eye[3]{};
      RenderSceneCamera camera{};
      vk::ImageView linearDepth;
      vk::DescriptorSet previousSceneSet;
      // Optional caller-owned timestamp range: start, DI, GI, NRD, history.
      vk::QueryPool timing;uint32_t timestampBase=0;
      // Optional two timestamps after initial/temporal DI, in the same pool.
      uint32_t directTimestampBase=UINT32_MAX;
      bool historyValid=false;
      bool gi=false,cacheEnabled=true,collectStatistics=false;
      uint64_t materialRevision=0;
      uint64_t lightsRevision=0;
      uint64_t texturesRevision=0;
      bool geometryChanged=false;
      uint32_t sharcDownscale=5,sharcAccumulation=8,sharcStaleFrames=64;
      float sharcSceneScale=50,giStrength=1;
      float denoiserFrameTimeMilliseconds=0;
   };
   VulkanRtxdi();~VulkanRtxdi();
   bool Initialize(vk::PhysicalDevice physical,vk::Device device,vk::DescriptorSetLayout sceneLayout,
      uint32_t width,uint32_t height,const std::string &directory,std::string &error,uint32_t cacheEntries=1u<<20);
   // A slot must be retired. All submissions run on the same ordered queue.
   bool Record(vk::CommandBuffer command,uint32_t slot,vk::DescriptorSet sceneSet,const Input &input,std::string &error);
   Texture Diffuse(uint32_t slot) const;
   Texture Specular(uint32_t slot) const;
   Texture DenoisedDiffuse(uint32_t slot) const;
   Texture DenoisedSpecular(uint32_t slot) const;
   std::array<uint32_t,4> Statistics(uint32_t slot) const; // Read only after retiring this slot.
   void CompleteSubmission(bool accepted);
   void Shutdown(); // Retire all GPU work first.
private:
   struct Impl;std::unique_ptr<Impl> m;
};
}
#endif
