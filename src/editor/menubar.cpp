#include <algorithm>
#include <cmath>
#include <cstdint>

#include <SDL3/SDL.h>
#include <imgui.h>

#include <appagg.h>
#include <darkdlgs/darkdlgs.h>
#include <dispwnd.h>
#include <editorplatform.h>
#include <gshelapi.h>
#include <menubar.h>
#include <render_backend.h>
#include <uiedit.h>
#include <vkrend.h>
#include <vulkan_imgui.hpp>
#include <vulkan_overlay.hpp>

namespace
{
constexpr const char *kViewportMenu = "Viewport menu";

struct ViewportMenu
{
   const char *const *items;
   int count;
   int x, y;
   int selection = 0;
   ImVec2 rightRelease{};
   bool releasedRight = false;
   bool drewInputFrame = false;
   bool opening = true;
   bool open = true;
};

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
   int RunViewportMenu(const char *const *items, int count, int x, int y);

private:
   void SetHeight(int height);
   void DrawViewportMenu();

   cVulkanImGui m_ImGui;
   ViewportMenu *m_ViewportMenu{};
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
   DrawViewportMenu();
   m_ImGui.Render(commands);
   m_PlatformWindowsPending = true;
   m_fFrame = false;
}

int cMenuBarOverlay::RunViewportMenu(const char *const *items, int count, int x, int y)
{
   if (!Active() || m_ViewportMenu || count <= 0)
      return 0;

   AutoAppIPtr_(GameShell, shell);
   AutoAppIPtr_(VulkanRenderer, renderer);
   if (!shell || !renderer)
      return 0;

   RenderBackendLogicalToClientPoint(&x, &y);
   ViewportMenu menu{items, count, x, y};
   m_ViewportMenu = &menu;
   EditorUpdateCursor();
   while (menu.open && Active())
   {
      shell->PumpEvents(kPumpAll);
      renderer->PresentProgress();
      if (menu.open)
         SDL_Delay(8);
   }
   m_ViewportMenu = nullptr;
   EditorUpdateCursor();
   return menu.selection;
}

void cMenuBarOverlay::DrawViewportMenu()
{
   if (!m_ViewportMenu)
      return;
   auto &menu = *m_ViewportMenu;
   if (menu.opening)
   {
      // The legacy event can arrive after ImGui started its frame. Let ImGui
      // consume the right press before opening a popup it would dismiss.
      if (!menu.drewInputFrame)
      {
         menu.drewInputFrame = true;
         return;
      }
      if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
         return;

      const ImGuiViewport *viewport = ImGui::GetMainViewport();
      const auto &style = ImGui::GetStyle();
      float labelWidth = 0;
      for (int i = 0; i < menu.count; ++i)
         labelWidth = std::max(labelWidth, ImGui::CalcTextSize(menu.items[i]).x);
      const float width = labelWidth + 2.0f * (style.WindowPadding.x + ImGui::GetFontSize());
      const float height = menu.count * ImGui::GetTextLineHeightWithSpacing() + 2.0f * style.WindowPadding.y;
      const ImVec2 limit(viewport->WorkPos.x + std::max(0.0f, viewport->WorkSize.x - width),
                         viewport->WorkPos.y + std::max(0.0f, viewport->WorkSize.y - height));
      const ImVec2 position(std::clamp(viewport->Pos.x + menu.x - 1, viewport->WorkPos.x, limit.x),
                            std::clamp(viewport->Pos.y + menu.y - 1, viewport->WorkPos.y, limit.y));
      ImGui::OpenPopup(kViewportMenu);
      ImGui::SetNextWindowPos(position);
      menu.opening = false;
   }
   if (!ImGui::BeginPopup(kViewportMenu))
   {
      menu.open = false;
      return;
   }

   for (int i = 0; i < menu.count; ++i)
   {
      ImGui::PushID(i);
      const bool selected = ImGui::MenuItem(menu.items[i]);
      const ImVec2 min = ImGui::GetItemRectMin();
      const ImVec2 max = ImGui::GetItemRectMax();
      const bool released = menu.releasedRight && menu.rightRelease.x >= min.x && menu.rightRelease.x < max.x &&
         menu.rightRelease.y >= min.y && menu.rightRelease.y < max.y;
      ImGui::PopID();
      if (selected || released)
      {
         menu.selection = i + 1;
         ImGui::CloseCurrentPopup();
         menu.open = false;
         break;
      }
   }
   menu.releasedRight = false;
   if (ImGui::IsKeyPressed(ImGuiKey_Escape))
   {
      ImGui::CloseCurrentPopup();
      menu.open = false;
   }
   ImGui::EndPopup();
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
   bool popupOpen = m_ViewportMenu != nullptr;
   if (m_ImGui.ProcessEvent(event))
   {
      if (m_ViewportMenu && event.type == SDL_EVENT_MOUSE_BUTTON_UP && event.button.button == SDL_BUTTON_RIGHT)
      {
         int x = 0, y = 0;
         if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
            SDL_GetWindowPosition(SDL_GetWindowFromID(event.button.windowID), &x, &y);
         m_ViewportMenu->rightRelease = ImVec2(x + event.button.x, y + event.button.y);
         m_ViewportMenu->releasedRight = true;
      }
      if (!modal && event.type == SDL_EVENT_WINDOW_FOCUS_LOST &&
          event.window.windowID == SDL_GetWindowID(window->GetSDLWindow()))
         ImGui::SetWindowFocus(NULL);
      popupOpen |= ImGui::IsPopupOpen(NULL, ImGuiPopupFlags_AnyPopup);
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
   if (DarkDlgsModalActive() || m_ViewportMenu)
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

extern "C" int EditorViewportMenu(const char *const *items, int count, int x, int y)
{
   return g_MenuBarOverlay.RunViewportMenu(items, count, x, y);
}

bool MenuBarActive()
{
   return g_MenuBarOverlay.Active();
}

bool MenuBarOwnsPointer()
{
   return g_MenuBarOverlay.OwnsPointer();
}
