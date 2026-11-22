/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#include <algorithm>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <imgui.h>

#include <lg.h>
#include <cmdterm.h>
#include <config.h>
#include <darklimit.h>
#include <appagg.h>
#include <dispwnd.h>
#include <menus.h>
#include <platform_services.h>
#include <vulkan_imgui.hpp>

namespace
{

constexpr unsigned kMenuCommandBase = 1000;

struct sMenuItem
{
   std::string text;
   int submenu = -1;
   unsigned command = 0;
   bool separator = false;
   bool checked = false;
};

class cMenuSet
{
public:
   void Load(const char *pszName);
   bool Empty() const { return m_Menus.empty() || m_Menus[0].empty(); }
   void Draw() { DrawMenu(0); }
   void SetRadioCheckmark(int menu, int firstPos, int lastPos, int checkPos);
   void QueueCommand(unsigned id);
   void ExecuteCommands();

private:
   using tMenu = std::vector<sMenuItem>;

   int ParseMenu(const std::string &tag, std::vector<std::string> &menusInProgress);
   void DrawMenu(int index);

   std::vector<tMenu> m_Menus;
   std::vector<std::string> m_CommandTexts;
   std::vector<std::string> m_PendingCommands;
};

std::string_view Trim(std::string_view text)
{
   const size_t first = text.find_first_not_of(" \t\r\n");
   if (first == std::string_view::npos)
      return {};
   return text.substr(first, text.find_last_not_of(" \t\r\n") - first + 1);
}

// Windows menu labels use & for a mnemonic and && for a literal ampersand.
std::string MenuText(std::string_view text)
{
   std::string result;
   for (size_t i = 0; i < text.size(); ++i)
   {
      if (text[i] == '&')
      {
         if (i + 1 == text.size() || text[i + 1] != '&')
            continue;
         ++i;
      }
      result += text[i];
   }
   return result;
}

bool ReadMenuPart(const std::string &tag, std::string &definition)
{
   char value[DARK_LIMIT_RUNTIME_CONFIG_LINE_CHARS];
   if (tag.size() >= VARNAME_LEN ||
       !config_get_raw(tag.c_str(), value, sizeof(value)))
      return false;
   definition = Trim(value);
   return true;
}

int cMenuSet::ParseMenu(const std::string &tag, std::vector<std::string> &menusInProgress)
{
   for (const std::string &parent : menusInProgress)
   {
      if (_stricmp(parent.c_str(), tag.c_str()) == 0)
         return -1;
   }

   std::string definition;
   if (!ReadMenuPart(tag, definition) || definition.empty())
      return -1;

   // Reserve the index before parsing children. Radio groups refer to menus
   // by their position in the same traversal used by the original menu set.
   const int index = static_cast<int>(m_Menus.size());
   m_Menus.emplace_back();
   menusInProgress.push_back(tag);
   tMenu menu;
   for (int part = 1;; ++part)
   {
      std::string_view entries = definition;
      while (!entries.empty())
      {
         const size_t end = entries.find('|');
         const std::string_view entry = Trim(entries.substr(0, end));
         entries = end == std::string_view::npos
            ? std::string_view{} : entries.substr(end + 1);
         if (entry.empty())
            continue;

         const size_t colon = entry.find(':');
         const std::string_view text = Trim(entry.substr(0, colon));
         const std::string_view value = colon == std::string_view::npos
            ? text : Trim(entry.substr(colon + 1));

         sMenuItem item;
         item.text = MenuText(text);
         if (value.size() >= 5 && _strnicmp(value.data(), "menu_", 5) == 0)
            item.submenu = ParseMenu(std::string(value), menusInProgress);
         else if (text.substr(0, 3) == "sep")
            item.separator = true;
         else if (!value.empty())
         {
            item.command = kMenuCommandBase + static_cast<unsigned>(m_CommandTexts.size());
            m_CommandTexts.emplace_back(value);
         }
         menu.push_back(std::move(item));
      }

      if (!ReadMenuPart(tag + "_" + std::to_string(part), definition))
         break;
   }
   menusInProgress.pop_back();
   m_Menus[index] = std::move(menu);
   return index;
}

void cMenuSet::Load(const char *pszName)
{
   m_Menus.clear();
   m_CommandTexts.clear();
   m_PendingCommands.clear();
   if (pszName)
   {
      std::vector<std::string> menusInProgress;
      ParseMenu("menu_" + std::string(Trim(pszName)), menusInProgress);
   }
}

void cMenuSet::DrawMenu(int index)
{
   const tMenu &menu = m_Menus[index];
   for (size_t i = 0; i < menu.size(); ++i)
   {
      const sMenuItem &item = menu[i];
      ImGui::PushID(static_cast<int>(i));
      if (item.submenu >= 0)
      {
         if (ImGui::BeginMenu(item.text.c_str()))
         {
            DrawMenu(item.submenu);
            ImGui::EndMenu();
         }
      }
      else if (item.separator)
         ImGui::Separator();
      else if (ImGui::MenuItem(item.text.c_str(), NULL, item.checked,
                  item.command != 0))
         QueueCommand(item.command);
      ImGui::PopID();
   }
}

void cMenuSet::SetRadioCheckmark(int menu, int firstPos, int lastPos, int checkPos)
{
   if (menu < 0 || static_cast<size_t>(menu) >= m_Menus.size())
      return;
   tMenu &items = m_Menus[menu];
   for (int i = std::max(firstPos, 0);
        i <= lastPos && static_cast<size_t>(i) < items.size(); ++i)
      items[i].checked = i == checkPos;
}

void cMenuSet::QueueCommand(unsigned id)
{
   const unsigned index = id - kMenuCommandBase;
   if (index < m_CommandTexts.size())
      m_PendingCommands.push_back(m_CommandTexts[index]);
}

void cMenuSet::ExecuteCommands()
{
   // Commands can open dialogs, change modes, or present another frame.
   std::vector<std::string> commands;
   commands.swap(m_PendingCommands);
   for (const std::string &command : commands)
      cmdterm_execute_command(command.c_str());
}

cMenuSet g_MenuSet;

void DrawMenuBar()
{
   g_MenuSet.Draw();
}

}

///////////////////////////////////////////////////////////////////////////////

void SetMainMenu(const char *pszName)
{
   AutoAppIPtr_(DisplayWindow,window);
   if (window)
      window->SetTitle(pszName ? "DromEd - editor" : "DromEd");
   g_MenuSet.Load(PlatformBatchMode() ? NULL : pszName);
   VulkanImGuiSetMenuBar(g_MenuSet.Empty() ? NULL : DrawMenuBar);
}

void MenuCommand(unsigned id)
{
   g_MenuSet.QueueCommand(id);
}

void SetRadioCheckmark(int menu, int firstPos, int lastPos, int checkPos)
{
   g_MenuSet.SetRadioCheckmark(menu, firstPos, lastPos, checkPos);
}

void ExecutePendingMenuCommands(void)
{
   g_MenuSet.ExecuteCommands();
}
