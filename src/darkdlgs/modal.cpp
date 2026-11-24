#include "modal.h"

#include <algorithm>
#include <cfloat>
#include <cstdint>
#include <utility>
#include <vector>

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
std::vector<DarkDlgsModal *> g_modeless;
}

DarkDlgsModal::DarkDlgsModal(std::string title, ImVec2 size)
   : m_size(size)
{
   SetTitle(std::move(title));
}

void DarkDlgsModal::SetTitle(std::string title)
{
   m_title = std::move(title) + "/" + std::to_string(reinterpret_cast<uintptr_t>(this));
}

bool DarkDlgsModalActive()
{
   return g_modal != nullptr;
}

void DrawDarkDlgs()
{
   if (g_modal)
      g_modal->Draw();
   else
      for (auto *dialog : g_modeless)
         dialog->Draw();
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
   if (window && parentWindow && SDL_SetWindowParent(window, parentWindow) && this == g_modal)
      SDL_SetWindowModal(window, true);
   m_windowId = id;
}

void DarkDlgsModal::Draw()
{
   if (!m_open)
      return;

   const bool showParent = m_parent && m_parent->KeepVisibleBehindModal();
   if (showParent)
      m_parent->Draw();

   const bool background = g_modal && this != g_modal;
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

   if (m_firstFrame)
   {
      const ImVec2 center = showParent ? m_parent->m_windowCenter : mainViewport->GetCenter();
      ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
   }
   SetWindowSizing(m_size, *mainViewport);
   if (m_focus)
      ImGui::SetNextWindowFocus();
   if (background)
      flags |= ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
   if (ImGui::Begin(m_title.c_str(), background ? nullptr : &m_open, flags) && m_open)
   {
      const ImVec2 position = ImGui::GetWindowPos();
      const ImVec2 size = ImGui::GetWindowSize();
      m_windowCenter = ImVec2(position.x + size.x * 0.5f, position.y + size.y * 0.5f);
      if (detached)
         BindPlatformWindow(*mainViewport);
      ImGui::BeginDisabled(background);
      DrawContents();
      ImGui::EndDisabled();
      m_firstFrame = false;
   }
   ImGui::End();
   m_focus = false;
}

void DarkDlgsModal::SetWindowSizing(ImVec2 size, const ImGuiViewport &) const
{
   if (FirstFrame())
      ImGui::SetNextWindowSize(size);
   ImGui::SetNextWindowSizeConstraints(ImVec2(240, 150), ImVec2(FLT_MAX, FLT_MAX));
}

bool DarkDlgsModal::HasKeyboardFocus() const
{
   return g_modal ? this == g_modal : ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
}

void DarkDlgsModal::Open()
{
   m_open = true;
   m_firstFrame = true;
   m_focus = true;
   m_windowId = 0;
}

void DarkDlgsModal::Suspend()
{
   if (m_windowId)
      if (SDL_Window *window = SDL_GetWindowFromID(m_windowId))
         SDL_SetWindowModal(window, false);
   m_windowId = 0;
}

bool DarkDlgsModal::ShowModeless()
{
   if (PlatformBatchMode() || !MenuBarActive() || m_open)
      return false;
   Open();
   g_modeless.push_back(this);
   return true;
}

void DarkDlgsModal::DismissModeless()
{
   g_modeless.erase(std::remove(g_modeless.begin(), g_modeless.end(), this), g_modeless.end());
   Close();
}

void UpdateDarkDlgs()
{
   const auto dialogs = g_modeless;
   for (auto *dialog : dialogs)
   {
      if (std::find(g_modeless.begin(), g_modeless.end(), dialog) == g_modeless.end())
         continue;
      if (dialog->m_open)
         dialog->AfterFrame();
      if (std::find(g_modeless.begin(), g_modeless.end(), dialog) != g_modeless.end() && !dialog->m_open)
      {
         dialog->DismissModeless();
         dialog->OnModelessClosed();
      }
   }
}

void CloseDarkDlgs()
{
   while (!g_modeless.empty())
   {
      auto *dialog = g_modeless.back();
      dialog->DismissModeless();
      dialog->OnModelessClosed();
   }
}

bool DarkDlgsModal::Run()
{
   if (PlatformBatchMode() || !MenuBarActive() || m_open)
      return false;

   AutoAppIPtr_(GameShell, shell);
   AutoAppIPtr_(VulkanRenderer, renderer);
   if (!shell || !renderer)
      return false;

   auto *previous = g_modal;
   if (previous)
      previous->Suspend();

   Open();
   m_parent = previous;
   g_modal = this;
   EditorUpdateCursor();

   // Modal callers need a result before the editor resumes. Pump input and
   // present the dialog without advancing the editor or the simulation.
   while (m_open)
   {
      shell->PumpEvents(kPumpAll);
      renderer->PresentProgress();
      if (m_open)
         AfterFrame();
      if (m_open)
         SDL_Delay(8);
   }

   Suspend();
   g_modal = previous;
   m_parent = nullptr;
   if (previous)
   {
      previous->m_focus = true;
      previous->m_windowId = 0;
   }
   EditorUpdateCursor();
   renderer->PresentProgress();
   return true;
}
