#include <vulkan/vulkan.h>
#include <dlfcn.h>
extern "C" {
int dark_fail_allocations=0;
unsigned dark_skip_allocations=0;
unsigned dark_failed_allocations=0;
VKAPI_ATTR VkResult VKAPI_CALL vkAllocateMemory(VkDevice device,
   const VkMemoryAllocateInfo *info,const VkAllocationCallbacks *callbacks,VkDeviceMemory *memory) {
   static const auto allocate=reinterpret_cast<PFN_vkAllocateMemory>(dlsym(RTLD_NEXT,"vkAllocateMemory"));
   if(dark_fail_allocations) {
      if(dark_skip_allocations)--dark_skip_allocations;
      else {
         dark_fail_allocations=0;++dark_failed_allocations;
         return VK_ERROR_OUT_OF_DEVICE_MEMORY;
      }
   }
   return allocate?allocate(device,info,callbacks,memory):VK_ERROR_INITIALIZATION_FAILED;
}
}
