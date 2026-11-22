#include "modal.h"

#include <cfloat>
#include <cstdint>
#include <utility>

#include <SDL3/SDL.h>

#include <appagg.h>
#include <editorplatform.h>
#include <gshelapi.h>
#include <menubar.h>
#include <platform_services.h>
#include <vkrend.h>

#include "darkdlgs.h"

namespace
{
DarkDlgsModal *g_modal;
}

DarkDlgsModal::DarkDlgsModal(std::string title, ImVec2 size)
   : m_title(std::move(title)), m_size(size)
{
}

bool DarkDlgsModalActive()
{
   return g_modal != nullptr;
}

void DrawDarkDlgs()
{
   if (g_modal)
      g_modal->Draw();
}

void DarkDlgsModal::BindPlatformWindow(const ImGuiViewport &parent)
{
   const ImGuiViewport *viewport = ImGui::GetWindowViewport();
   if (viewport == &parent || !viewport->PlatformHandle)
      return;

   const auto id = static_cast<SDL_WindowID>(reinterpret_cast<uintptr_t>(viewport->PlatformHandle));
   if (id == m_windowId)
      return;

   const auto parentId = static_cast<SDL_WindowID>(reinterpret_cast<uintptr_t>(parent.PlatformHandle));
   SDL_Window *window = SDL_GetWindowFromID(id);
   SDL_Window *parentWindow = SDL_GetWindowFromID(parentId);
   if (window && parentWindow && SDL_SetWindowParent(window, parentWindow))
      SDL_SetWindowModal(window, true);
   m_windowId = id;
}

void DarkDlgsModal::Draw()
{
   if (!m_open)
      return;

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
   ImGui::SetNextWindowSize(m_size, ImGuiCond_Appearing);
   ImGui::SetNextWindowSizeConstraints(ImVec2(240, 150), ImVec2(FLT_MAX, FLT_MAX));
   if (m_firstFrame)
      ImGui::SetNextWindowFocus();
   if (ImGui::Begin(m_title.c_str(), &m_open, flags) && m_open)
   {
      if (detached)
         BindPlatformWindow(*mainViewport);
      DrawContents();
      m_firstFrame = false;
   }
   ImGui::End();
}

bool DarkDlgsModal::Run()
{
   if (PlatformBatchMode() || !MenuBarActive() || g_modal)
      return false;

   AutoAppIPtr_(GameShell, shell);
   AutoAppIPtr_(VulkanRenderer, renderer);
   if (!shell || !renderer)
      return false;

   m_open = true;
   m_firstFrame = true;
   m_windowId = 0;
   g_modal = this;
   EditorUpdateCursor();

   // Modal callers need a result before the editor resumes. Pump input and
   // present the dialog without advancing the editor or the simulation.
   while (m_open)
   {
      shell->PumpEvents(kPumpAll);
      renderer->PresentProgress();
      AfterFrame();
      if (m_open)
         SDL_Delay(8);
   }

   g_modal = nullptr;
   EditorUpdateCursor();
   renderer->PresentProgress();
   return true;
}
