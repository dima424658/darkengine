#include "vulkan_world_features.hpp"
#include <algorithm>
#include <cstring>
namespace dark {
bool VulkanWorldFeatures::Probe(vk::PhysicalDevice physical,bool requestRayTracing) {
   unavailable.clear();extensions.clear();rayTracing=false;
   descriptors.pNext=nullptr;
   const auto available=physical.enumerateDeviceExtensionProperties();
   if(std::none_of(available.begin(),available.end(),[](const auto &extension){return std::strcmp(extension.extensionName,VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME)==0;})) {
      unavailable="sampled texture descriptor indexing is unavailable";return false;
   }
   physical.getFeatures2(&features);
   const bool supported=descriptors.runtimeDescriptorArray && descriptors.shaderSampledImageArrayNonUniformIndexing && descriptors.descriptorBindingPartiallyBound;
   [[maybe_unused]] const bool unformattedStorageWrites=features.features.shaderStorageImageWriteWithoutFormat;
   [[maybe_unused]] const bool shaderInt64=features.features.shaderInt64;
   // Enable only features consumed by the selected renderer.
   features.features=vk::PhysicalDeviceFeatures{};descriptors=vk::PhysicalDeviceDescriptorIndexingFeatures{};
   if(!supported){unavailable="required sampled texture indexing features are unavailable";return false;}
   descriptors.runtimeDescriptorArray=VK_TRUE;descriptors.shaderSampledImageArrayNonUniformIndexing=VK_TRUE;descriptors.descriptorBindingPartiallyBound=VK_TRUE;
   extensions.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
   if(requestRayTracing) {
#if !DARK_RTX_SDK_AVAILABLE
      unavailable="RTX SDK lighting was disabled at build time";return false;
#else
      const char *required[]={VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,VK_KHR_RAY_QUERY_EXTENSION_NAME,
         VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
         VK_KHR_SPIRV_1_4_EXTENSION_NAME,VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME
#if DARK_RTX_SDK_AVAILABLE
         ,VK_KHR_SHADER_FLOAT16_INT8_EXTENSION_NAME,VK_KHR_16BIT_STORAGE_EXTENSION_NAME,VK_KHR_SHADER_ATOMIC_INT64_EXTENSION_NAME
#endif
      };
      for(const char *name:required)if(std::none_of(available.begin(),available.end(),[&](const auto &ext){return std::strcmp(ext.extensionName,name)==0;})) {
         unavailable=std::string("missing RTX lighting device extension: ")+name;return false;
      }
      {
         vk::PhysicalDeviceFeatures2 query;query.pNext=&addresses;
         addresses.pNext=&acceleration;acceleration.pNext=&queries;queries.pNext=nullptr;
#if DARK_RTX_SDK_AVAILABLE
         queries.pNext=&float16;float16.pNext=&storage16;storage16.pNext=&atomics64;atomics64.pNext=nullptr;
#endif
         physical.getFeatures2(&query);
         rayTracing=addresses.bufferDeviceAddress && acceleration.accelerationStructure && queries.rayQuery;
#if DARK_RTX_SDK_AVAILABLE
         // DXC emits formatless storage-image writes for NRD compute kernels.
         vk::PhysicalDeviceSubgroupProperties subgroup;
         vk::PhysicalDeviceProperties2 properties;properties.pNext=&subgroup;physical.getProperties2(&properties);
         rayTracing=rayTracing && unformattedStorageWrites && shaderInt64 && float16.shaderFloat16 &&
            storage16.storageBuffer16BitAccess && atomics64.shaderBufferInt64Atomics &&
            bool(subgroup.supportedStages & vk::ShaderStageFlagBits::eCompute) &&
            bool(subgroup.supportedOperations & vk::SubgroupFeatureFlagBits::eBallot);
#endif
         addresses=vk::PhysicalDeviceBufferDeviceAddressFeatures{};
         acceleration=vk::PhysicalDeviceAccelerationStructureFeaturesKHR{};
         queries=vk::PhysicalDeviceRayQueryFeaturesKHR{};
         float16=vk::PhysicalDeviceShaderFloat16Int8Features{};storage16=vk::PhysicalDevice16BitStorageFeatures{};
         atomics64=vk::PhysicalDeviceShaderAtomicInt64Features{};
         if(rayTracing) {
#if DARK_RTX_SDK_AVAILABLE
            features.features.shaderStorageImageWriteWithoutFormat=VK_TRUE;
            features.features.shaderInt64=VK_TRUE;
            float16.shaderFloat16=VK_TRUE;storage16.storageBuffer16BitAccess=VK_TRUE;atomics64.shaderBufferInt64Atomics=VK_TRUE;
#endif
            addresses.bufferDeviceAddress=acceleration.accelerationStructure=queries.rayQuery=VK_TRUE;
            descriptors.pNext=&addresses;addresses.pNext=&acceleration;acceleration.pNext=&queries;
#if DARK_RTX_SDK_AVAILABLE
            queries.pNext=&float16;float16.pNext=&storage16;storage16.pNext=&atomics64;
#endif
            extensions.insert(extensions.end(),std::begin(required),std::end(required));
         }
      }
      if(!rayTracing){unavailable="required RTX lighting shader, storage or ray-query features are unavailable";return false;}
#endif
   }
   return true;
}
}
