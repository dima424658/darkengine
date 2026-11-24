#include "darkdlgs.h"
#include "modal.h"

namespace
{
class SimpleMenu final : public DarkDlgsModal
{
public:
   SimpleMenu(const char *title, const menustring *items, int count)
      : DarkDlgsModal(std::string(title && *title ? title : "List selection")
                      + "###DarkDlgsSimpleMenu", ImVec2(460, 330)),
        m_items(items), m_count(count)
   {
   }

   int Result() const { return m_result; }

private:
   void DrawContents() override;

   const menustring *m_items;
   int m_count;
   int m_selected = 0;
   int m_result = -1;
};

void SimpleMenu::DrawContents()
{
   bool accept = false;
   const float footer = ImGui::GetFrameHeightWithSpacing();
   if (FirstFrame())
      ImGui::SetNextWindowScroll(ImVec2(0, 0));
   if (ImGui::BeginChild("Choices", ImVec2(0, -footer), ImGuiChildFlags_Borders))
   {
      ImGuiListClipper clipper;
      clipper.Begin(m_count);
      while (clipper.Step())
      {
         for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i)
         {
            ImGui::PushID(i);
            const ImVec2 textPos = ImGui::GetCursorScreenPos();
            const bool activated = ImGui::Selectable("##choice", m_selected == i,
               ImGuiSelectableFlags_AllowDoubleClick);
            const char *label = m_items[i] ? m_items[i] : "";
            ImGui::GetWindowDrawList()->AddText(textPos, ImGui::GetColorU32(ImGuiCol_Text), label);
            if (activated || ImGui::IsItemFocused())
               m_selected = i;
            if (activated && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
               accept = true;
            if (m_selected == i)
               ImGui::SetItemDefaultFocus();
            ImGui::PopID();
         }
      }
   }
   ImGui::EndChild();

   accept |= ImGui::Button("OK");
   ImGui::SameLine();
   if (ImGui::Button("Cancel") || ImGui::IsKeyPressed(ImGuiKey_Escape, false))
      Close();
   else if (accept || ImGui::IsKeyPressed(ImGuiKey_Enter, false) ||
            ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, false))
   {
      m_result = m_selected;
      Close();
   }
}
}

int LGAPI DoSimpleMenu(const char *title, const menustring *items, int count)
{
   if (!items || count <= 0)
      return -1;

   SimpleMenu menu(title, items, count);
   return menu.Run() ? menu.Result() : -1;
}
