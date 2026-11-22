#include <cfloat>
#include <cstdint>
#include <string>

#include <SDL3/SDL.h>
#include <imgui.h>

#include <appagg.h>
#include <editorplatform.h>
#include <gshelapi.h>
#include <menubar.h>
#include <platform_services.h>
#include <vkrend.h>

#include "darkdlgs.h"

namespace
{
struct SimpleMenu
{
   std::string title;
   const menustring *items;
   int count;
   int selected = 0;
   int result = -1;
   bool open = true;
   bool firstFrame = true;
   SDL_WindowID windowId = 0;
};

SimpleMenu *g_menu;

void BindModalWindow(SimpleMenu &menu, const ImGuiViewport &parent)
{
   const ImGuiViewport *viewport = ImGui::GetWindowViewport();
   if (viewport == &parent || !viewport->PlatformHandle)
      return;

   const auto id = static_cast<SDL_WindowID>(reinterpret_cast<uintptr_t>(viewport->PlatformHandle));
   if (id == menu.windowId)
      return;

   const auto parentId = static_cast<SDL_WindowID>(reinterpret_cast<uintptr_t>(parent.PlatformHandle));
   SDL_Window *window = SDL_GetWindowFromID(id);
   SDL_Window *parentWindow = SDL_GetWindowFromID(parentId);
   if (window && parentWindow && SDL_SetWindowParent(window, parentWindow))
      SDL_SetWindowModal(window, true);
   menu.windowId = id;
}
}

bool DarkDlgsModalActive()
{
   return g_menu != nullptr;
}

void DrawDarkDlgs()
{
   if (!g_menu || !g_menu->open)
      return;

   SimpleMenu &menu = *g_menu;
   const ImGuiViewport *mainViewport = ImGui::GetMainViewport();
   const bool detached = (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) != 0;
   ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;
   if (detached)
   {
      ImGuiWindowClass windowClass;
      windowClass.ParentViewportId = mainViewport->ID;
      windowClass.ViewportFlagsOverrideSet = ImGuiViewportFlags_NoAutoMerge;
      windowClass.ViewportFlagsOverrideClear = ImGuiViewportFlags_NoDecoration;
      ImGui::SetNextWindowClass(&windowClass);
      flags |= ImGuiWindowFlags_NoTitleBar;
   }

   ImGui::SetNextWindowPos(mainViewport->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
   ImGui::SetNextWindowSize(ImVec2(460, 330), ImGuiCond_Appearing);
   ImGui::SetNextWindowSizeConstraints(ImVec2(240, 150), ImVec2(FLT_MAX, FLT_MAX));
   if (menu.firstFrame)
      ImGui::SetNextWindowFocus();
   bool accept = false;
   if (ImGui::Begin(menu.title.c_str(), &menu.open, flags))
   {
      if (detached && menu.open)
         BindModalWindow(menu, *mainViewport);
      const float footer = ImGui::GetFrameHeightWithSpacing();
      if (menu.firstFrame)
         ImGui::SetNextWindowScroll(ImVec2(0, 0));
      if (ImGui::BeginChild("Choices", ImVec2(0, -footer), ImGuiChildFlags_Borders))
      {
         ImGuiListClipper clipper;
         clipper.Begin(menu.count);
         while (clipper.Step())
         {
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i)
            {
               ImGui::PushID(i);
               const ImVec2 textPos = ImGui::GetCursorScreenPos();
               const bool activated = ImGui::Selectable("##choice", menu.selected == i,
                  ImGuiSelectableFlags_AllowDoubleClick);
               const char *label = menu.items[i] ? menu.items[i] : "";
               ImGui::GetWindowDrawList()->AddText(textPos,
                  ImGui::GetColorU32(ImGuiCol_Text), label);
               if (activated || ImGui::IsItemFocused())
                  menu.selected = i;
               if (activated && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                  accept = true;
               if (menu.selected == i)
                  ImGui::SetItemDefaultFocus();
               ImGui::PopID();
            }
         }
      }
      ImGui::EndChild();

      accept |= ImGui::Button("OK");
      ImGui::SameLine();
      if (ImGui::Button("Cancel") || ImGui::IsKeyPressed(ImGuiKey_Escape, false))
         menu.open = false;
      if (menu.open && (accept || ImGui::IsKeyPressed(ImGuiKey_Enter, false) ||
                        ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, false)))
      {
         menu.result = menu.selected;
         menu.open = false;
      }
      menu.firstFrame = false;
   }
   ImGui::End();
}

int LGAPI DoSimpleMenu(const char *title, const menustring *items, int count)
{
   if (PlatformBatchMode() || !MenuBarActive() || !items || count <= 0 || DarkDlgsModalActive())
      return -1;

   AutoAppIPtr_(GameShell, shell);
   AutoAppIPtr_(VulkanRenderer, renderer);
   if (!shell || !renderer)
      return -1;

   SimpleMenu menu{title && *title ? title : "List selection", items, count};
   menu.title += "###DarkDlgsSimpleMenu";
   g_menu = &menu;
   EditorUpdateCursor();

   // Callers consume the selection synchronously. Advance only UI events
   // and presentation while the dialog is open.
   while (menu.open)
   {
      shell->PumpEvents(kPumpAll);
      renderer->PresentProgress();
      if (menu.open)
         SDL_Delay(8);
   }

   g_menu = nullptr;
   EditorUpdateCursor();
   renderer->PresentProgress();
   return menu.result;
}
