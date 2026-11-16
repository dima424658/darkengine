#include <vulkan/vulkan.h>
#include <dlfcn.h>
extern "C" {
int dark_fail_submit=0;
unsigned dark_skip_modern_submits=0;
unsigned dark_failed_submits=0;
unsigned dark_successful_modern_submits=0;
unsigned dark_recovered_modern_submits=0;
VKAPI_ATTR VkResult VKAPI_CALL vkQueueSubmit(VkQueue queue,uint32_t count,
   const VkSubmitInfo *submits,VkFence fence) {
   static const auto submit=reinterpret_cast<PFN_vkQueueSubmit>(dlsym(RTLD_NEXT,"vkQueueSubmit"));
   bool modern=false;
   for(uint32_t i=0;i<count;++i)modern|=submits[i].commandBufferCount==2;
   if(dark_fail_submit && modern) {
      if(dark_skip_modern_submits)--dark_skip_modern_submits;
      else {
         dark_fail_submit=0;++dark_failed_submits;
         return VK_ERROR_OUT_OF_DEVICE_MEMORY;
      }
   }
   const auto result=submit?submit(queue,count,submits,fence):VK_ERROR_INITIALIZATION_FAILED;
   if(result==VK_SUCCESS && modern) {
      ++dark_successful_modern_submits;
      if(dark_failed_submits)++dark_recovered_modern_submits;
   }
   return result;
}
}
