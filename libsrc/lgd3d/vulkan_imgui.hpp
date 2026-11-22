#ifndef DARK_VULKAN_IMGUI_HPP
#define DARK_VULKAN_IMGUI_HPP

#include <memory>

#include <vulkan/vulkan.hpp>

class cVulkanContext;
struct ImGuiContext;
union SDL_Event;

class cVulkanImGui
{
public:
   cVulkanImGui() = default;
   cVulkanImGui(const cVulkanImGui &) = delete;
   cVulkanImGui &operator=(const cVulkanImGui &) = delete;

   bool BeginFrame(const cVulkanContext &context);
   // Render after a successful BeginFrame.
   void Render(vk::CommandBuffer commands);
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
};

#endif
