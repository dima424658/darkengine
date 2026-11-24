// Linux-only preload helper for the standalone offscreen GPU regression.
// Capture the first command buffer through its completed fence.
#define _GNU_SOURCE
#include <renderdoc_app.h>
#include <vulkan/vulkan.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
static RENDERDOC_API_1_6_0 *api;
static int started,finished;
VKAPI_ATTR VkResult VKAPI_CALL vkBeginCommandBuffer(VkCommandBuffer command,const VkCommandBufferBeginInfo *info) {
 static PFN_vkBeginCommandBuffer next;
 if(!next)next=(PFN_vkBeginCommandBuffer)dlsym(RTLD_NEXT,"vkBeginCommandBuffer");
 if(!started) {
  void *module=dlopen("librenderdoc.so",RTLD_NOW|RTLD_NOLOAD);
  pRENDERDOC_GetAPI get=module?(pRENDERDOC_GetAPI)dlsym(module,"RENDERDOC_GetAPI"):NULL;
  if(get && get(eRENDERDOC_API_Version_1_6_0,(void **)&api)) {
   const char *path=getenv("DARK_TEST_CAPTURE");
   api->SetCaptureFilePathTemplate(path?path:"/tmp/dark-shadow-offscreen");
   api->StartFrameCapture(NULL,NULL);started=1;
  }
 }
 return next(command,info);
}
VKAPI_ATTR VkResult VKAPI_CALL vkWaitForFences(VkDevice device,uint32_t count,const VkFence *fences,VkBool32 all,uint64_t timeout) {
 static PFN_vkWaitForFences next;
 if(!next)next=(PFN_vkWaitForFences)dlsym(RTLD_NEXT,"vkWaitForFences");
 VkResult result=next(device,count,fences,all,timeout);
 if(api && started && !finished && result==VK_SUCCESS) {
  fprintf(stderr,"Offscreen capture result: %u\n",api->EndFrameCapture(NULL,NULL));finished=1;
 }
 return result;
}
