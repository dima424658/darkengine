#ifndef DARK_VULKAN_WORLD_FEATURES_HPP
#define DARK_VULKAN_WORLD_FEATURES_HPP
#include <vulkan/vulkan.hpp>
#include <string>
#include <vector>
namespace dark {
// Probe sampled-texture indexing for geometry tests, or the complete RTX
// lighting feature set for the game. Unsupported RTX uses the native renderer.
// Keep this owner alive through device creation because it owns the pNext chain.
struct VulkanWorldFeatures {
   vk::PhysicalDeviceFeatures2 features;
   vk::PhysicalDeviceDescriptorIndexingFeatures descriptors;
   vk::PhysicalDeviceBufferDeviceAddressFeatures addresses;
   vk::PhysicalDeviceAccelerationStructureFeaturesKHR acceleration;
   vk::PhysicalDeviceRayQueryFeaturesKHR queries;
   vk::PhysicalDeviceShaderFloat16Int8Features float16;
   vk::PhysicalDevice16BitStorageFeatures storage16;
   vk::PhysicalDeviceShaderAtomicInt64Features atomics64;
   bool rayTracing=false;
   std::vector<const char *> extensions;
   std::string unavailable;
   VulkanWorldFeatures(){features.pNext=&descriptors;}
   VulkanWorldFeatures(const VulkanWorldFeatures &)=delete;
   VulkanWorldFeatures &operator=(const VulkanWorldFeatures &)=delete;
   bool Probe(vk::PhysicalDevice physical,bool requestRayTracing=false);
};
}
#endif
