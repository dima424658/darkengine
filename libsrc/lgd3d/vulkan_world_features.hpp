#ifndef DARK_VULKAN_WORLD_FEATURES_HPP
#define DARK_VULKAN_WORLD_FEATURES_HPP
#include <vulkan/vulkan.hpp>
#include <string>
#include <vector>
namespace dark {
// Raster world rendering only needs sampled-texture descriptor indexing.
// Keep this owner alive through device creation because it owns the pNext chain.
struct VulkanWorldFeatures {
   vk::PhysicalDeviceFeatures2 features;
   vk::PhysicalDeviceDescriptorIndexingFeatures descriptors;
   std::vector<const char *> extensions;
   std::string unavailable;
   VulkanWorldFeatures(){features.pNext=&descriptors;}
   VulkanWorldFeatures(const VulkanWorldFeatures &)=delete;
   VulkanWorldFeatures &operator=(const VulkanWorldFeatures &)=delete;
   bool Probe(vk::PhysicalDevice physical);
   bool IndirectShadows() const {return features.features.multiDrawIndirect && features.features.drawIndirectFirstInstance;}
};
}
#endif
