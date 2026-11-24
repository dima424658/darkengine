#ifndef DARK_VULKAN_DLSS_HPP
#define DARK_VULKAN_DLSS_HPP
#include <vulkan/vulkan.hpp>
#include <memory>
#include <string>
#include <vector>
namespace dark {
class VulkanDlss {
public:
   enum class Quality { Off, Quality, Balanced, Performance, Dlaa };
   struct Texture { vk::Image image; vk::ImageView view; vk::Format format; uint32_t width, height; };
   VulkanDlss();
   ~VulkanDlss();
   static void InstanceExtensions(std::vector<const char *> &extensions);
   static bool DeviceExtensions(vk::Instance instance, vk::PhysicalDevice physical, std::vector<const char *> &extensions);
   bool Initialize(vk::Instance instance, vk::PhysicalDevice physical, vk::Device device, Quality quality,
                   std::string &error);
   bool Resolution(uint32_t outputWidth, uint32_t outputHeight, uint32_t &width, uint32_t &height, std::string &error);
   bool Record(vk::CommandBuffer command, const Texture &color, const Texture &depth, const Texture &motion,
               const Texture &output, float jitterX, float jitterY, bool reset, std::string &error);
   void CompleteSubmission(bool accepted);
   bool Enabled() const;
private:
   struct Impl;
   std::unique_ptr<Impl> m;
};
}
#endif
