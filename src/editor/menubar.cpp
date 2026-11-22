#include <cmath>
#include <cstdint>

#include <SDL3/SDL.h>
#include <imgui.h>

#include <appagg.h>
#include <darkdlgs/darkdlgs.h>
#include <dispwnd.h>
#include <menubar.h>
#include <uiedit.h>
#include <vkrend.h>
#include <vulkan_imgui.hpp>
#include <vulkan_overlay.hpp>

namespace
{
ImVec4 StyleRgb(const guiStyle &palette, StyleColorKind kind)
{
   const StyleColor color = palette.colors[kind];
   return ImVec4(guiR(color) / 255.0f, guiG(color) / 255.0f, guiB(color) / 255.0f, 1.0f);
}

ImVec4 Blend(const ImVec4 &background, const ImVec4 &accent, float amount)
{
   return ImVec4(background.x + (accent.x - background.x) * amount,
                 background.y + (accent.y - background.y) * amount,
                 background.z + (accent.z - background.z) * amount, 1.0f);
}

void ApplyEditorTheme()
{
   guiStyle palette = masterEditStyle;
   uiGameLoadStyleColors("edit_", &palette);

   const ImVec4 background = StyleRgb(palette, StyleColorBG);
   const ImVec4 panel = Blend(StyleRgb(palette, StyleColorBG2), StyleRgb(palette, StyleColorDim), 0.12f);
   const ImVec4 text = Blend(StyleRgb(palette, StyleColorText), StyleRgb(palette, StyleColorFG), 0.35f);
   const ImVec4 bright = StyleRgb(palette, StyleColorBright);
   const ImVec4 border = StyleRgb(palette, StyleColorBorder);
   const ImVec4 highlight = StyleRgb(palette, StyleColorHilite);
   const ImVec4 hover = Blend(panel, border, 0.22f);
   const ImVec4 selection = Blend(panel, highlight, 0.26f);

   ImGuiStyle &style = ImGui::GetStyle();
   style.FrameBorderSize = 1.5f;
   style.WindowBorderSize = 1.5f;
   style.ChildBorderSize = 1.5f;
   ImVec4 *colors = style.Colors;
   colors[ImGuiCol_Text] = text;
   colors[ImGuiCol_TextDisabled] = StyleRgb(palette, StyleColorDim);
   colors[ImGuiCol_WindowBg] = colors[ImGuiCol_PopupBg] = background;
   colors[ImGuiCol_MenuBarBg] = colors[ImGuiCol_TitleBg] = colors[ImGuiCol_TitleBgCollapsed] = panel;
   colors[ImGuiCol_TitleBgActive] = hover;
   colors[ImGuiCol_Border] = border;
   colors[ImGuiCol_Separator] = colors[ImGuiCol_TreeLines] = Blend(background, border, 0.55f);
   colors[ImGuiCol_SeparatorHovered] = border;
   colors[ImGuiCol_SeparatorActive] = highlight;

   colors[ImGuiCol_FrameBg] = colors[ImGuiCol_Button] = panel;
   colors[ImGuiCol_FrameBgHovered] = colors[ImGuiCol_ButtonHovered] = hover;
   colors[ImGuiCol_FrameBgActive] = colors[ImGuiCol_ButtonActive] = selection;
   colors[ImGuiCol_Header] = selection;
   colors[ImGuiCol_HeaderHovered] = hover;
   colors[ImGuiCol_HeaderActive] = Blend(panel, highlight, 0.38f);
   colors[ImGuiCol_TextSelectedBg] = colors[ImGuiCol_HeaderActive];
   colors[ImGuiCol_CheckMark] = colors[ImGuiCol_InputTextCursor] = bright;
   colors[ImGuiCol_CheckboxSelectedBg] = selection;
   colors[ImGuiCol_SliderGrab] = border;
   colors[ImGuiCol_SliderGrabActive] = highlight;

   colors[ImGuiCol_ScrollbarBg] = background;
   colors[ImGuiCol_ScrollbarGrab] = Blend(panel, border, 0.38f);
   colors[ImGuiCol_ScrollbarGrabHovered] = border;
   colors[ImGuiCol_ScrollbarGrabActive] = highlight;
   colors[ImGuiCol_ResizeGrip] = hover;
   colors[ImGuiCol_ResizeGripHovered] = border;
   colors[ImGuiCol_ResizeGripActive] = highlight;
   colors[ImGuiCol_TableHeaderBg] = panel;
   colors[ImGuiCol_TableBorderStrong] = colors[ImGuiCol_Separator];
   colors[ImGuiCol_TableBorderLight] = Blend(background, border, 0.30f);
   colors[ImGuiCol_TableRowBgAlt] = Blend(background, panel, 0.55f);
   colors[ImGuiCol_TextLink] = border;
   colors[ImGuiCol_NavCursor] = colors[ImGuiCol_DragDropTarget] = highlight;
}

class cMenuBarOverlay final : public IVulkanOverlay
{
public:
   void Prepare(const cVulkanContext &context) override;
   void Draw(vk::CommandBuffer commands) override;
   void PresentPlatformWindows() override;
   void Reset() override;
   bool ProcessEvent(const SDL_Event &event) override;

   void SetDraw(void (*pfnDraw)());
   bool Active() const { return m_pfnDraw != nullptr; }
   bool OwnsPointer() const;

private:
   void SetHeight(int height);

   cVulkanImGui m_ImGui;
   void (*m_pfnDraw)(){};
   int m_Height{};
   bool m_fFrame{};
   bool m_PlatformWindowsPending{};
   bool m_ThemeApplied{};
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
   if (m_fFrame && !m_ThemeApplied)
   {
      ApplyEditorTheme();
      m_ThemeApplied = true;
   }
   SetHeight(m_fFrame ? static_cast<int>(std::ceil(ImGui::GetFrameHeight())) : 0);
}

void cMenuBarOverlay::Draw(vk::CommandBuffer commands)
{
   if (!m_fFrame)
      return;

   m_ImGui.SetCurrentContext();
   if (m_pfnDraw && ImGui::BeginMainMenuBar())
   {
      ImGui::BeginDisabled(DarkDlgsModalActive());
      m_pfnDraw();
      ImGui::EndDisabled();
      ImGui::EndMainMenuBar();
   }
   DrawDarkDlgs();
   m_ImGui.Render(commands);
   m_PlatformWindowsPending = true;
   m_fFrame = false;
}

void cMenuBarOverlay::PresentPlatformWindows()
{
   if (m_PlatformWindowsPending)
   {
      m_PlatformWindowsPending = false;
      m_ImGui.PresentPlatformWindows();
   }
}

void cMenuBarOverlay::Reset()
{
   m_ImGui.Reset();
   m_fFrame = false;
   m_PlatformWindowsPending = false;
   m_ThemeApplied = false;
}

bool cMenuBarOverlay::ProcessEvent(const SDL_Event &event)
{
   if (!m_pfnDraw)
      return false;
   AutoAppIPtr_(DisplayWindow,window);
   if (!window || window->IsRelativeMouseActive())
      return false;

   const bool modal = DarkDlgsModalActive();
   bool popupOpen = false;
   if (m_ImGui.ProcessEvent(event))
   {
      if (!modal && event.type == SDL_EVENT_WINDOW_FOCUS_LOST &&
          event.window.windowID == SDL_GetWindowID(window->GetSDLWindow()))
         ImGui::SetWindowFocus(NULL);
      popupOpen = ImGui::IsPopupOpen(NULL, ImGuiPopupFlags_AnyPopup);
   }
   // Use the event coordinates so moving into the bar and clicking within
   // a single frame cannot send an editor click.
   switch (event.type)
   {
   case SDL_EVENT_MOUSE_MOTION:
      return modal || popupOpen || event.motion.y < m_Height;
   case SDL_EVENT_MOUSE_BUTTON_DOWN:
   case SDL_EVENT_MOUSE_BUTTON_UP:
      return modal || popupOpen || event.button.y < m_Height;
   case SDL_EVENT_MOUSE_WHEEL:
      return modal || popupOpen || event.wheel.mouse_y < m_Height;
   case SDL_EVENT_KEY_DOWN:
   case SDL_EVENT_KEY_UP:
      return modal || popupOpen;
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
   m_ThemeApplied = false;
   if (!pfnDraw)
      SetHeight(0);
}

bool cMenuBarOverlay::OwnsPointer() const
{
   if (DarkDlgsModalActive())
      return true;
   if (!m_pfnDraw || !m_ImGui.SetCurrentContext())
      return false;
   AutoAppIPtr_(DisplayWindow,window);
   SDL_Window *focus = SDL_GetMouseFocus();
   if (!window || !focus)
      return false;
   if (focus != window->GetSDLWindow())
   {
      const auto handle = reinterpret_cast<void *>(static_cast<uintptr_t>(SDL_GetWindowID(focus)));
      return ImGui::FindViewportByPlatformHandle(handle) != nullptr;
   }
   float y;
   SDL_GetMouseState(NULL, &y);
   return y < m_Height || ImGui::IsPopupOpen(NULL, ImGuiPopupFlags_AnyPopup);
}

}

void SetMenuBar(void (*pfnDraw)())
{
   g_MenuBarOverlay.SetDraw(pfnDraw);
   if (!pfnDraw)
      CloseDarkDlgs();
   if (pfnDraw)
   {
      AutoAppIPtr_(VulkanRenderer,renderer);
      if (renderer)
         renderer->SetOverlay(&g_MenuBarOverlay);
   }
}

bool MenuBarActive()
{
   return g_MenuBarOverlay.Active();
}

bool MenuBarOwnsPointer()
{
   return g_MenuBarOverlay.OwnsPointer();
}
