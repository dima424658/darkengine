#include <cmath>

#include <SDL3/SDL.h>
#include <imgui.h>

#include <appagg.h>
#include <dispwnd.h>
#include <menubar.h>
#include <vkrend.h>
#include <vulkan_imgui.hpp>
#include <vulkan_overlay.hpp>

namespace
{
class cMenuBarOverlay final : public IVulkanOverlay
{
public:
   void Prepare(const cVulkanContext &context) override;
   void Draw(vk::CommandBuffer commands) override;
   void Reset() override;
   bool ProcessEvent(const SDL_Event &event) override;

   void SetDraw(void (*pfnDraw)());
   bool OwnsPointer() const;

private:
   void SetHeight(int height);

   cVulkanImGui m_ImGui;
   void (*m_pfnDraw)(){};
   int m_Height{};
   bool m_fFrame{};
};

cMenuBarOverlay g_MenuBarOverlay;

void cMenuBarOverlay::SetHeight(int height)
{
   if (m_Height == height)
      return;
   m_Height = height;
   AutoAppIPtr_(VulkanRenderer,renderer);
   if (renderer)
      renderer->SetTopInset(height);
}

void cMenuBarOverlay::Prepare(const cVulkanContext &context)
{
   if (!m_pfnDraw)
      return;

   m_fFrame = m_ImGui.BeginFrame(context);
   SetHeight(m_fFrame ? static_cast<int>(std::ceil(ImGui::GetFrameHeight())) : 0);
}

void cMenuBarOverlay::Draw(vk::CommandBuffer commands)
{
   if (!m_fFrame)
      return;

   m_ImGui.SetCurrentContext();
   if (m_pfnDraw && ImGui::BeginMainMenuBar())
   {
      m_pfnDraw();
      ImGui::EndMainMenuBar();
   }
   m_ImGui.Render(commands);
   m_fFrame = false;
}

void cMenuBarOverlay::Reset()
{
   m_ImGui.Reset();
   m_fFrame = false;
}

bool cMenuBarOverlay::ProcessEvent(const SDL_Event &event)
{
   if (!m_pfnDraw)
      return false;
   AutoAppIPtr_(DisplayWindow,window);
   if (!window || window->IsRelativeMouseActive())
      return false;

   bool popupOpen = false;
   if (m_ImGui.ProcessEvent(event))
   {
      if (event.type == SDL_EVENT_WINDOW_FOCUS_LOST)
         ImGui::SetWindowFocus(NULL);
      popupOpen = ImGui::IsPopupOpen(NULL, ImGuiPopupFlags_AnyPopup);
   }
   // Use the event coordinates so moving into the bar and clicking within
   // a single frame cannot send an editor click.
   switch (event.type)
   {
   case SDL_EVENT_MOUSE_MOTION:
      return popupOpen || event.motion.y < m_Height;
   case SDL_EVENT_MOUSE_BUTTON_DOWN:
   case SDL_EVENT_MOUSE_BUTTON_UP:
      return popupOpen || event.button.y < m_Height;
   case SDL_EVENT_MOUSE_WHEEL:
      return popupOpen || event.wheel.mouse_y < m_Height;
   case SDL_EVENT_KEY_DOWN:
   case SDL_EVENT_KEY_UP:
      return popupOpen;
   default:
      return false;
   }
}

void cMenuBarOverlay::SetDraw(void (*pfnDraw)())
{
   if (m_ImGui.SetCurrentContext())
   {
      ImGui::SetWindowFocus(NULL);
      ImGuiIO &io = ImGui::GetIO();
      io.ClearEventsQueue();
      io.ClearInputKeys();
      io.ClearInputMouse();
   }

   m_pfnDraw = pfnDraw;
   if (!pfnDraw)
      SetHeight(0);
}

bool cMenuBarOverlay::OwnsPointer() const
{
   if (!m_pfnDraw || !m_ImGui.SetCurrentContext())
      return false;
   AutoAppIPtr_(DisplayWindow,window);
   if (!window || SDL_GetMouseFocus() != window->GetSDLWindow())
      return false;
   float y;
   SDL_GetMouseState(NULL, &y);
   return y < m_Height || ImGui::IsPopupOpen(NULL, ImGuiPopupFlags_AnyPopup);
}

}

void SetMenuBar(void (*pfnDraw)())
{
   g_MenuBarOverlay.SetDraw(pfnDraw);
   if (pfnDraw)
   {
      AutoAppIPtr_(VulkanRenderer,renderer);
      if (renderer)
         renderer->SetOverlay(&g_MenuBarOverlay);
   }
}

bool MenuBarOwnsPointer()
{
   return g_MenuBarOverlay.OwnsPointer();
}
