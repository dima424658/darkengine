#ifndef DARK_VULKAN_IMGUI_HPP
#define DARK_VULKAN_IMGUI_HPP

#include <memory>
#include <vector>

#include <SDL3/SDL_events.h>
#include <vulkan/vulkan.hpp>

class cVulkanContext;
struct ImGuiContext;

class cVulkanImGui
{
public:
   cVulkanImGui() = default;
   cVulkanImGui(const cVulkanImGui &) = delete;
   cVulkanImGui &operator=(const cVulkanImGui &) = delete;

   bool BeginFrame(const cVulkanContext &context);
   // Render after a successful BeginFrame.
   void Render(vk::CommandBuffer commands);
   void PresentPlatformWindows();
   // Reset while the Vulkan device is still alive and idle.
   void Reset();
   bool ProcessEvent(const SDL_Event &event);
   bool SetCurrentContext() const;

private:
   bool Init(const cVulkanContext &context);

   struct ContextDeleter
   {
      void operator()(ImGuiContext *context) const noexcept;
   };

   std::unique_ptr<ImGuiContext, ContextDeleter> m_pContext;
   std::vector<SDL_Event> m_windowEvents;
};

#endif
