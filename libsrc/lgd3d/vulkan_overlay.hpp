#ifndef DARK_VULKAN_OVERLAY_HPP
#define DARK_VULKAN_OVERLAY_HPP

#include <vulkan/vulkan.hpp>

class cVulkanContext;
union SDL_Event;

struct sVulkanOverlay
{
   void (*prepare)(const cVulkanContext &context);
   void (*draw)(vk::CommandBuffer commands);
   // Called after the device goes idle and before its swapchain is destroyed.
   void (*reset)();
   bool (*processEvent)(const SDL_Event &event);
};

void DarkRenderSetOverlay(const sVulkanOverlay &overlay);

#endif
