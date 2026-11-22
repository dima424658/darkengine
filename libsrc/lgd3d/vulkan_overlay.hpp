#ifndef DARK_VULKAN_OVERLAY_HPP
#define DARK_VULKAN_OVERLAY_HPP

#include <vulkan/vulkan.hpp>

class cVulkanContext;
union SDL_Event;

struct IVulkanOverlay
{
   virtual void Prepare(const cVulkanContext &context) = 0;
   virtual void Draw(vk::CommandBuffer commands) = 0;
   // Called after the device goes idle and before its swapchain is destroyed.
   virtual void Reset() = 0;
   virtual bool ProcessEvent(const SDL_Event &event) = 0;

protected:
   ~IVulkanOverlay() = default;
};

#endif
