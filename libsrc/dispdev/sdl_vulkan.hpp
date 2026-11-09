#ifndef DARK_SDL_VULKAN_H
#define DARK_SDL_VULKAN_H

class cVulkanContext;

bool VulkanContextInitializeSDL(cVulkanContext &context,
   int physicalDeviceIndex, char *error, unsigned errorCapacity);
bool VulkanContextRecreateSDLSurface(cVulkanContext &context, char *error,
   unsigned errorCapacity);

#endif
