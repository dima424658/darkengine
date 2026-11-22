#pragma once

#include <string>

#include <imgui.h>

class DarkDlgsModal
{
public:
   DarkDlgsModal(std::string title, ImVec2 size);
   bool Run();
   void Draw();

protected:
   ~DarkDlgsModal() = default;
   bool FirstFrame() const { return m_firstFrame; }
   void Close() { m_open = false; }

private:
   virtual void DrawContents() = 0;
   virtual void AfterFrame() {}
   void BindPlatformWindow(const ImGuiViewport &parent);

   std::string m_title;
   ImVec2 m_size;
   unsigned int m_windowId = 0;
   bool m_open = false;
   bool m_firstFrame = true;
};
