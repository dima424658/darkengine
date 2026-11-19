// Linux capability fault injection: expose a real device with one RTX feature
// masked out, while leaving the original renderer's feature set available.
#include <vulkan/vulkan.h>
#include <dlfcn.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
namespace {
unsigned masked;
__attribute__((destructor)) void report() {
   std::fprintf(stderr,"Fixture missing feature masked=%u\n",masked);
}
}
extern "C" VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceFeatures2(
   VkPhysicalDevice physical,VkPhysicalDeviceFeatures2 *features) {
   static const auto query=reinterpret_cast<PFN_vkGetPhysicalDeviceFeatures2>(dlsym(RTLD_NEXT,"vkGetPhysicalDeviceFeatures2"));
   if(!query)std::abort();
   query(physical,features);
   const char *kind=std::getenv("DARK_TEST_MISSING_FEATURE");
   if(!kind)return;
   if(!std::strcmp(kind,"nrd-storage") && features->features.shaderStorageImageWriteWithoutFormat) {
      features->features.shaderStorageImageWriteWithoutFormat=VK_FALSE;++masked;
   }
   for(auto *next=static_cast<VkBaseOutStructure*>(features->pNext);next;next=next->pNext) {
      if(!std::strcmp(kind,"ray-query") && next->sType==VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR) {
         auto &value=reinterpret_cast<VkPhysicalDeviceRayQueryFeaturesKHR*>(next)->rayQuery;
         if(value){value=VK_FALSE;++masked;}
      }
      if(!std::strcmp(kind,"sharc-atomics") && next->sType==VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_INT64_FEATURES) {
         auto &value=reinterpret_cast<VkPhysicalDeviceShaderAtomicInt64Features*>(next)->shaderBufferInt64Atomics;
         if(value){value=VK_FALSE;++masked;}
      }
   }
}
