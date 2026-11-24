// Linux-only test shim: emulate a device without sampled descriptor indexing.
// Use only through LD_PRELOAD for the documented fallback regression.
#include <vulkan/vulkan.h>
#include <dlfcn.h>
#include <vector>
#include <cstring>
#include <algorithm>
extern "C" VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateDeviceExtensionProperties(VkPhysicalDevice physical,const char *layer,uint32_t *count,VkExtensionProperties *out) {
 static auto real=reinterpret_cast<PFN_vkEnumerateDeviceExtensionProperties>(dlsym(RTLD_NEXT,"vkEnumerateDeviceExtensionProperties"));
 uint32_t size=0;VkResult result=real(physical,layer,&size,nullptr);if(result!=VK_SUCCESS)return result;
 std::vector<VkExtensionProperties> extensions(size);result=real(physical,layer,&size,extensions.data());if(result!=VK_SUCCESS)return result;
 extensions.resize(size);extensions.erase(std::remove_if(extensions.begin(),extensions.end(),[](const auto &e){return std::strcmp(e.extensionName,VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME)==0;}),extensions.end());
 if(!out){*count=uint32_t(extensions.size());return VK_SUCCESS;}
 const uint32_t written=std::min(*count,uint32_t(extensions.size()));std::copy_n(extensions.begin(),written,out);*count=written;
 return written<extensions.size()?VK_INCOMPLETE:VK_SUCCESS;
}
extern "C" VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance instance,const char *name) {
 static auto real=reinterpret_cast<PFN_vkGetInstanceProcAddr>(dlsym(RTLD_NEXT,"vkGetInstanceProcAddr"));
 if(std::strcmp(name,"vkEnumerateDeviceExtensionProperties")==0)return reinterpret_cast<PFN_vkVoidFunction>(&vkEnumerateDeviceExtensionProperties);
 return real(instance,name);
}
