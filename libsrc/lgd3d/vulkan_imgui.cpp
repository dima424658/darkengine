#include <algorithm>
#include <cstdint>

#include <SDL3/SDL.h>
#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_vulkan.h>

#include <appagg.h>
#include <dispwnd.h>

#include "vulkan_context.hpp"
#include "vulkan_imgui.hpp"

void cVulkanImGui::ContextDeleter::operator()(ImGuiContext *context) const noexcept
{
   ImGui::DestroyContext(context);
}

bool cVulkanImGui::Init(const cVulkanContext &context)
{
   AutoAppIPtr_(DisplayWindow,window);
   if (!window || !window->GetSDLWindow())
      return false;

   IMGUI_CHECKVERSION();
   ImGuiContext *previous = ImGui::GetCurrentContext();
   m_pContext.reset(ImGui::CreateContext());
   ImGui::SetCurrentContext(m_pContext.get());
   ImGuiIO &io = ImGui::GetIO();
   io.IniFilename = nullptr;
   io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard |
      ImGuiConfigFlags_NoMouseCursorChange;
   io.ConfigNavCaptureKeyboard = false;

   if (!ImGui_ImplSDL3_InitForVulkan(window->GetSDLWindow()))
   {
      m_pContext.reset();
      ImGui::SetCurrentContext(previous);
      return false;
   }
   ImGui_ImplSDL3_SetMouseCaptureMode(ImGui_ImplSDL3_MouseCaptureMode_Disabled);
   ImGui_ImplSDL3_SetGamepadMode(ImGui_ImplSDL3_GamepadMode_Manual, nullptr, 0);
   // ImGui's secondary swapchains submit and present on the graphics queue.
   if ((io.BackendFlags & ImGuiBackendFlags_PlatformHasViewports) &&
       context.GraphicsQueueFamily() == context.PresentQueueFamily())
      io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

   ImGui_ImplVulkan_InitInfo info{};
   info.ApiVersion = VK_API_VERSION_1_1;
   info.Instance = context.Instance();
   info.PhysicalDevice = context.PhysicalDevice();
   info.Device = context.Device();
   info.QueueFamily = context.GraphicsQueueFamily();
   info.Queue = context.GraphicsQueue();
   info.MinImageCount = kVulkanFramesInFlight;
   info.ImageCount = std::max(context.SwapchainImageCount(), kVulkanFramesInFlight);
   info.DescriptorPoolSize = IMGUI_IMPL_VULKAN_MINIMUM_SAMPLED_IMAGE_POOL_SIZE;
   info.PipelineInfoMain.RenderPass = context.RenderPass();
   ImGui_ImplVulkan_Init(&info);
   return true;
}

bool cVulkanImGui::BeginFrame(const cVulkanContext &context)
{
   if (!SetCurrentContext() && !Init(context))
      return false;

   ImGui_ImplVulkan_NewFrame();
   ImGui_ImplSDL3_NewFrame();
   ImGuiIO &io = ImGui::GetIO();
   const vk::Extent2D extent = context.SwapchainExtent();
   if (io.DisplaySize.x <= 0.0f || io.DisplaySize.y <= 0.0f)
      return false;
   io.DisplayFramebufferScale = ImVec2(extent.width / io.DisplaySize.x,
      extent.height / io.DisplaySize.y);
   for (const auto &event : m_windowEvents)
      ImGui_ImplSDL3_ProcessEvent(&event);
   m_windowEvents.clear();
   ImGui::NewFrame();
   return true;
}

void cVulkanImGui::Render(vk::CommandBuffer commands)
{
   ImGui::SetCurrentContext(m_pContext.get());
   ImGui::Render();
   ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commands);
}

void cVulkanImGui::PresentPlatformWindows()
{
   if (SetCurrentContext() && (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable))
   {
      ImGui::UpdatePlatformWindows();
#if defined(__APPLE__)
      // The SDL ImGui backend leaves viewports unparented on macOS. Popups
      // from detached dialogs need a native parent to appear above them.
      const auto *mainViewport = ImGui::GetMainViewport();
      for (const auto *viewport : ImGui::GetPlatformIO().Viewports)
      {
         const auto *parent = viewport->ParentViewport;
         if (!parent || parent == mainViewport)
            continue;
         const auto id = static_cast<SDL_WindowID>(reinterpret_cast<uintptr_t>(viewport->PlatformHandle));
         const auto parentId = static_cast<SDL_WindowID>(reinterpret_cast<uintptr_t>(parent->PlatformHandle));
         SDL_Window *window = SDL_GetWindowFromID(id);
         SDL_Window *parentWindow = SDL_GetWindowFromID(parentId);
         if (window && parentWindow && SDL_GetWindowParent(window) != parentWindow)
            SDL_SetWindowParent(window, parentWindow);
      }
#endif
      ImGui::RenderPlatformWindowsDefault();
   }
}

void cVulkanImGui::Reset()
{
   m_windowEvents.clear();
   ImGuiContext *previous = ImGui::GetCurrentContext();
   if (!SetCurrentContext())
      return;

   if (previous == m_pContext.get())
      previous = nullptr;

   ImGui_ImplVulkan_Shutdown();
   ImGui_ImplSDL3_Shutdown();
   m_pContext.reset();
   ImGui::SetCurrentContext(previous);
}

bool cVulkanImGui::ProcessEvent(const SDL_Event &event)
{
   if (!SetCurrentContext())
      return false;

   // The backend sets viewport requests immediately. Dispatch these at the
   // start of a frame so UpdatePlatformWindows cannot clear them before NewFrame.
   if (event.type == SDL_EVENT_WINDOW_MOVED || event.type == SDL_EVENT_WINDOW_RESIZED ||
       event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
   {
      const auto handle = reinterpret_cast<void *>(static_cast<uintptr_t>(event.window.windowID));
      if (!ImGui::FindViewportByPlatformHandle(handle))
         return false;
      m_windowEvents.push_back(event);
      return true;
   }

   // The SDL backend forwards button state without the pointer position.
   // After a swapchain rebuild, the pointer may not have moved yet.
   if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN ||
       event.type == SDL_EVENT_MOUSE_BUTTON_UP)
   {
      const auto handle = reinterpret_cast<void *>(static_cast<uintptr_t>(event.button.windowID));
      if (ImGui::FindViewportByPlatformHandle(handle))
      {
         int x = 0;
         int y = 0;
         if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
            SDL_GetWindowPosition(SDL_GetWindowFromID(event.button.windowID), &x, &y);
         ImGui::GetIO().AddMousePosEvent(event.button.x + x, event.button.y + y);
      }
   }
   return ImGui_ImplSDL3_ProcessEvent(&event);
}

bool cVulkanImGui::SetCurrentContext() const
{
   if (!m_pContext)
      return false;
   ImGui::SetCurrentContext(m_pContext.get());
   return true;
}
