#include "vulkan_world_features.hpp"
#include <algorithm>
#include <cstring>
namespace dark {
bool VulkanWorldFeatures::Probe(vk::PhysicalDevice physical) {
   unavailable.clear();extensions.clear();
   const auto available=physical.enumerateDeviceExtensionProperties();
   if(std::none_of(available.begin(),available.end(),[](const auto &extension){return std::strcmp(extension.extensionName,VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME)==0;})) {
      unavailable="sampled texture descriptor indexing is unavailable";return false;
   }
   physical.getFeatures2(&features);
   const bool supported=descriptors.runtimeDescriptorArray && descriptors.shaderSampledImageArrayNonUniformIndexing && descriptors.descriptorBindingPartiallyBound;
   const bool indirect=features.features.multiDrawIndirect && features.features.drawIndirectFirstInstance;
   // Enable only the optional core pair used by shadow draw batching.
   features.features=vk::PhysicalDeviceFeatures{};descriptors=vk::PhysicalDeviceDescriptorIndexingFeatures{};
   if(!supported){unavailable="required sampled texture indexing features are unavailable";return false;}
   descriptors.runtimeDescriptorArray=VK_TRUE;descriptors.shaderSampledImageArrayNonUniformIndexing=VK_TRUE;descriptors.descriptorBindingPartiallyBound=VK_TRUE;
   features.features.multiDrawIndirect=indirect;features.features.drawIndirectFirstInstance=indirect;
   extensions.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);return true;
}
}
