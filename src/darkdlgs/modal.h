#pragma once

#include <string>

#include <imgui.h>

class DarkDlgsModal
{
public:
   DarkDlgsModal(std::string title, ImVec2 size);
   bool Run();
   bool ShowModeless();
   void DismissModeless();
   void Draw();

protected:
   ~DarkDlgsModal() = default;
   bool FirstFrame() const { return m_firstFrame; }
   bool IsOpen() const { return m_open; }
   bool HasKeyboardFocus() const;
   void Close() { m_open = false; }
   void SetTitle(std::string title);
   virtual bool KeepVisibleBehindModal() const { return false; }

private:
   virtual void SetWindowSizing(ImVec2 size, const ImGuiViewport &viewport) const;
   virtual void DrawContents() = 0;
   virtual void AfterFrame() {}
   virtual void OnModelessClosed() {}
   friend void UpdateDarkDlgs();
   friend void CloseDarkDlgs();
   void Open();
   void Suspend();
   void BindPlatformWindow(const ImGuiViewport &parent);

   std::string m_title;
   ImVec2 m_size;
   ImVec2 m_windowCenter{};
   DarkDlgsModal *m_parent = nullptr;
   unsigned int m_windowId = 0;
   bool m_open = false;
   bool m_firstFrame = true;
   bool m_focus = false;
};
