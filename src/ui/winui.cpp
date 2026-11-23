#include <cfloat>
#include <cstdio>

#include <lg.h>
#include <darkdlgs/modal.h>
#include <platform_services.h>
#include <winui.h>

namespace
{
class MessageDialog final : public DarkDlgsModal
{
public:
   MessageDialog(const char *message, bool confirm)
      : DarkDlgsModal("DromEd###Message", ImVec2(380, 0)),
        m_message(message ? message : ""), m_confirm(confirm)
   {
   }

   bool Accepted() const { return m_accepted; }

private:
   void SetWindowSizing(ImVec2 size, const ImGuiViewport &) const override
   {
      ImGui::SetNextWindowSize(ImVec2(size.x, 0));
      ImGui::SetNextWindowSizeConstraints(ImVec2(260, 0), ImVec2(FLT_MAX, FLT_MAX));
   }

   void DrawContents() override
   {
      ImGui::PushTextWrapPos(0);
      ImGui::TextUnformatted(m_message);
      ImGui::PopTextWrapPos();
      ImGui::Spacing();
      if (ImGui::Button(m_confirm ? "Yes" : "OK", ImVec2(72, 0)) ||
          ImGui::IsKeyPressed(ImGuiKey_Enter, false) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, false))
      {
         m_accepted = true;
         Close();
      }
      if (m_confirm)
      {
         ImGui::SameLine();
         if (ImGui::Button("No", ImVec2(72, 0)))
            Close();
      }
      if (ImGui::IsKeyPressed(ImGuiKey_Escape, false))
         Close();
   }

   const char *m_message;
   bool m_confirm;
   bool m_accepted = false;
};
}

extern "C" BOOL winui_GetYorN(const char *message)
{
   if (PlatformBatchMode())
      return PlatformAskYesNo("DromEd confirmation", message);

   MessageDialog dialog(message, true);
   return dialog.Run() && dialog.Accepted();
}

extern "C" void winui_Text(const char *message)
{
   if (PlatformBatchMode())
   {
      std::fprintf(stderr, "DromEd: %s\n", message ? message : "");
      return;
   }

   MessageDialog dialog(message, false);
   if (!dialog.Run())
      std::fprintf(stderr, "DromEd: %s\n", message ? message : "");
}
