#ifndef DARK_VULKAN_NRD_HPP
#define DARK_VULKAN_NRD_HPP
#include <NRD.h>
#include <vulkan/vulkan.hpp>
#include <array>
#include <memory>
#include <string>
namespace dark {
// Native Vulkan integration, guided by nvpro-samples/vk_denoise_nrd's resource
// pool/dispatch model. No NRI or nvpro_core dependency. Submitted command buffers
// must execute in order on one graphics/compute queue; retire a slot before reuse.
class VulkanNrd {
public:
   struct Texture {vk::Image image;vk::ImageView view;};
   using Resources=std::array<Texture,size_t(nrd::ResourceType::MAX_NUM)>;
   VulkanNrd();
   ~VulkanNrd();
   bool Initialize(vk::PhysicalDevice physical,vk::Device device,uint32_t width,uint32_t height,std::string &error);
   // All caller-owned resources enter and leave this call in GENERAL layout.
   bool Record(vk::CommandBuffer command,uint32_t slot,const Resources &resources,
      const nrd::CommonSettings &settings,std::string &error);
   // Commit only after queue submission succeeds; rejection preserves image-layout
   // bookkeeping and forces the next accepted frame to restart accumulation.
   void CompleteSubmission(bool accepted);
   void ResetHistory();
   void Shutdown(); // Retire GPU work before shutdown or resize/reinitialization.
private:
   struct Impl;
   std::unique_ptr<Impl> m;
};
}
#endif
