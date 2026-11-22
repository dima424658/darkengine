#include <algorithm>
#include <cmath>

#include <SDL3/SDL.h>
#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_vulkan.h>

#include <display_window.h>

#include "vulkan_context.hpp"
#include "vulkan_imgui.hpp"
#include "vulkan_overlay.hpp"
#include "vulkan_render_backend.hpp"

namespace
{
void (*g_pDrawMenuBar)();
ImGuiContext *g_pImGui;
bool g_fMenuFrame;
int g_MenuBarHeight;

void SetMenuBarHeight(int height)
{
   g_MenuBarHeight = height;
   RenderBackendVulkanSetTopInset(height);
}

bool InitMenuOverlay(const cVulkanContext &context)
{
   IMGUI_CHECKVERSION();
   g_pImGui = ImGui::CreateContext();
   ImGuiIO &io = ImGui::GetIO();
   io.IniFilename = nullptr;
   io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard |
      ImGuiConfigFlags_NoMouseCursorChange;
   io.ConfigNavCaptureKeyboard = false;

   if (!ImGui_ImplSDL3_InitForVulkan(SDLDisplayGetWindow()))
   {
      ImGui::DestroyContext(g_pImGui);
      g_pImGui = nullptr;
      return false;
   }
   ImGui_ImplSDL3_SetMouseCaptureMode(ImGui_ImplSDL3_MouseCaptureMode_Disabled);
   ImGui_ImplSDL3_SetGamepadMode(ImGui_ImplSDL3_GamepadMode_Manual, nullptr, 0);

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

void PrepareMenuOverlay(const cVulkanContext &context)
{
   if (!g_pDrawMenuBar)
      return;
   if (!g_pImGui && !InitMenuOverlay(context))
   {
      SetMenuBarHeight(0);
      return;
   }

   ImGui_ImplVulkan_NewFrame();
   ImGui_ImplSDL3_NewFrame();
   ImGuiIO &io = ImGui::GetIO();
   const vk::Extent2D extent = context.SwapchainExtent();
   if (io.DisplaySize.x <= 0.0f || io.DisplaySize.y <= 0.0f)
      return;
   io.DisplayFramebufferScale = ImVec2(extent.width / io.DisplaySize.x,
      extent.height / io.DisplaySize.y);
   ImGui::NewFrame();
   g_fMenuFrame = true;
   SetMenuBarHeight(static_cast<int>(std::ceil(ImGui::GetFrameHeight())));
}

void DrawMenuOverlay(vk::CommandBuffer commands)
{
   if (!g_fMenuFrame)
      return;
   if (g_pDrawMenuBar && ImGui::BeginMainMenuBar())
   {
      g_pDrawMenuBar();
      ImGui::EndMainMenuBar();
   }
   ImGui::Render();
   ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commands);
   g_fMenuFrame = false;
}

void ResetMenuOverlay()
{
   if (g_pImGui)
   {
      ImGui_ImplVulkan_Shutdown();
      ImGui_ImplSDL3_Shutdown();
      ImGui::DestroyContext(g_pImGui);
      g_pImGui = nullptr;
      g_fMenuFrame = false;
   }
}

bool ProcessMenuEvent(const SDL_Event &event)
{
   if (!g_pDrawMenuBar || SDLDisplayRelativeMouseActive())
      return false;

   bool popupOpen = false;
   if (g_pImGui)
   {
      // The SDL backend forwards button state without the pointer position.
      // After a swapchain rebuild, the pointer may not have moved yet.
      if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN ||
          event.type == SDL_EVENT_MOUSE_BUTTON_UP)
         ImGui::GetIO().AddMousePosEvent(event.button.x, event.button.y);
      ImGui_ImplSDL3_ProcessEvent(&event);
      if (event.type == SDL_EVENT_WINDOW_FOCUS_LOST)
         ImGui::SetWindowFocus(nullptr);
      popupOpen = ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopup);
   }
   // Use the event coordinates so moving into the bar and clicking within
   // a single frame cannot send an editor click.
   switch (event.type)
   {
   case SDL_EVENT_MOUSE_MOTION:
      return popupOpen || event.motion.y < g_MenuBarHeight;
   case SDL_EVENT_MOUSE_BUTTON_DOWN:
   case SDL_EVENT_MOUSE_BUTTON_UP:
      return popupOpen || event.button.y < g_MenuBarHeight;
   case SDL_EVENT_MOUSE_WHEEL:
      return popupOpen || event.wheel.mouse_y < g_MenuBarHeight;
   case SDL_EVENT_KEY_DOWN:
   case SDL_EVENT_KEY_UP:
      return popupOpen;
   default:
      return false;
   }
}

const sVulkanOverlay kMenuBarOverlay =
{
   PrepareMenuOverlay, DrawMenuOverlay, ResetMenuOverlay, ProcessMenuEvent
};

}

void VulkanImGuiSetMenuBar(void (*draw)())
{
   if (g_pImGui)
   {
      ImGui::SetWindowFocus(nullptr);
      ImGuiIO &io = ImGui::GetIO();
      io.ClearEventsQueue();
      io.ClearInputKeys();
      io.ClearInputMouse();
   }

   g_pDrawMenuBar = draw;
   if (draw)
      DarkRenderSetOverlay(kMenuBarOverlay);
   else
      SetMenuBarHeight(0);
}
