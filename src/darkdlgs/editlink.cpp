#include <algorithm>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include <appagg.h>
#include <editlink.h>
#include <edittool.h>
#include <iobjsys.h>
#include <linkbase.h>
#include <linkedst.h>
#include <linkman.h>
#include <lnkquery.h>
#include <objedit.h>
#include <relation.h>
#include <sdesbase.h>
#include <sdestool.h>

#include "darkdlgs.h"
#include "edlinkst.h"
#include "modal.h"

namespace
{
bool g_showInternalLinks;

bool VisibleRelation(IRelation &relation, bool showAll)
{
   return relation.GetID() != RELID_NULL &&
      (showAll || !(relation.Describe()->flags & kRelationNoEdit));
}

struct RelationChoice
{
   RelationID id;
   std::string name;
};

class CreateLink final : public DarkDlgsModal
{
public:
   CreateLink(const sLink &link, bool showAll)
      : DarkDlgsModal("Create Link###DarkDlgsCreateLink", ImVec2(440, 190)),
        m_relation(link.flavor)
   {
      if (link.source)
         std::snprintf(m_source, sizeof(m_source), "%d", link.source);
      if (link.dest)
         std::snprintf(m_dest, sizeof(m_dest), "%d", link.dest);

      AutoAppIPtr_(LinkManager, links);
      sRelationIter iter{};
      if (links && links->IterStart(&iter) == S_OK)
      {
         RelationID id;
         while (links->IterNext(&iter, &id))
         {
            cAutoIPtr<IRelation> relation(links->GetRelation(id));
            if (relation && VisibleRelation(*relation, showAll))
               m_choices.push_back({id, relation->Describe()->name});
         }
         links->IterStop(&iter);
      }
      std::sort(m_choices.begin(), m_choices.end(), [](const auto &a, const auto &b) { return a.name < b.name; });
   }

   bool Accept(sLink &link)
   {
      if (!Run() || !m_accepted)
         return false;
      link = m_result;
      return true;
   }

private:
   void DrawContents() override
   {
      ImGui::InputText("From", m_source, sizeof(m_source));
      ImGui::InputText("To", m_dest, sizeof(m_dest));
      const auto choice = std::find_if(m_choices.begin(), m_choices.end(),
         [this](const auto &item) { return item.id == m_relation; });
      if (ImGui::BeginCombo("Flavor", choice != m_choices.end() ? choice->name.c_str() : "Select a relation"))
      {
         for (const auto &item : m_choices)
            if (ImGui::Selectable(item.name.c_str(), item.id == m_relation))
               m_relation = item.id;
         ImGui::EndCombo();
      }
      if (ImGui::Button("OK"))
      {
         AutoAppIPtr_(ObjectSystem, objects);
         const ObjID src = EditGetObjNamed(m_source);
         const ObjID dst = EditGetObjNamed(m_dest);
         const auto selection = std::find_if(m_choices.begin(), m_choices.end(),
            [this](const auto &item) { return item.id == m_relation; });
         if (objects && src && dst && objects->Exists(src) && objects->Exists(dst) &&
             selection != m_choices.end())
         {
            m_result = {src, dst, m_relation};
            m_accepted = true;
            Close();
         }
         else
            m_error = true;
      }
      ImGui::SameLine();
      if (ImGui::Button("Cancel") || ImGui::IsKeyPressed(ImGuiKey_Escape, false))
         Close();
      if (m_error)
         ImGui::TextUnformatted("Enter two existing objects and select a relation.");
   }

   std::vector<RelationChoice> m_choices;
   char m_source[128]{};
   char m_dest[128]{};
   RelationID m_relation;
   sLink m_result{};
   bool m_accepted = false;
   bool m_error = false;
};

struct LinkRow
{
   LinkID id;
   std::string flavor, source, dest, data;
   bool editableData;
};

enum class LinkAction { None, Add, Remove, Data };

class LinksDialog : public DarkDlgsModal
{
public:
   LinksDialog(const sLinkEditorDesc &desc, sLink pattern, bool autoDelete = false)
      : DarkDlgsModal((desc.title[0]
                         ? std::string(desc.title, std::find(desc.title, desc.title + sizeof(desc.title), '\0'))
                         : "Link Editor")
                      + "###DarkDlgsLinks", ImVec2(720, 410)),
        m_flags(desc.flags), m_pattern(pattern), m_autoDelete(autoDelete)
   {
      Refresh();
   }

private:
   void Refresh();
   void DrawContents() override;
   void AfterFrame() override;
   void OnModelessClosed() override { if (m_autoDelete) delete this; }

   eLinkEditorFlags m_flags;
   sLink m_pattern;
   bool m_autoDelete;
   std::vector<LinkRow> m_rows;
   LinkID m_selected = LINKID_NULL;
   LinkAction m_action = LinkAction::None;
};

void LinksDialog::Refresh()
{
   m_rows.clear();
   AutoAppIPtr_(LinkManager, links);
   AutoAppIPtr_(StructDescTools, types);
   if (!links)
      return;
   cAutoIPtr<ILinkQuery> query(links->Query(m_pattern.source, m_pattern.dest, m_pattern.flavor));
   for (; query && !query->Done(); query->Next())
   {
      sLink link{};
      if (query->Link(&link) != S_OK)
         continue;
      cAutoIPtr<IRelation> relation(links->GetRelation(link.flavor));
      if (!relation || !VisibleRelation(*relation, m_flags & kLinkEditShowAll))
         continue;
      const auto *data = relation->DescribeData();
      const auto *desc = types && data->size ? types->Lookup(data->type) : nullptr;
      LinkRow row{query->ID(), relation->Describe()->name, ObjEditName(link.source),
                  ObjEditName(link.dest), data->size ? data->type : "", desc && desc->size == data->size};
      m_rows.push_back(std::move(row));
   }
   std::sort(m_rows.begin(), m_rows.end(), [](const auto &a, const auto &b) { return a.id < b.id; });
}

void LinksDialog::DrawContents()
{
   const char *const headings[] = {"ID", "Flavor", "From", "To", "Data"};
   const eLinkEditorFlags hidden[] = {kLinkEditNoIDs, kLinkEditNoFlavors, kLinkEditNoSources,
      kLinkEditNoDests, kLinkEditNoData};
   int count = 0;
   for (auto flag : hidden)
      count += !(m_flags & flag);

   const float footer = ImGui::GetFrameHeightWithSpacing();
   const auto selection = std::find_if(m_rows.begin(), m_rows.end(),
      [this](const auto &row) { return row.id == m_selected; });
   if (ImGui::BeginChild("Links", ImVec2(0, -footer), ImGuiChildFlags_Borders))
   {
      if (m_rows.empty())
         ImGui::TextUnformatted("No matching links.");
      else if (count && ImGui::BeginTable("Link table", count, ImGuiTableFlags_RowBg))
      {
         for (int col = 0; col < 5; ++col)
            if (!(m_flags & hidden[col]))
               ImGui::TableSetupColumn(headings[col]);
         ImGui::TableHeadersRow();
         ImGuiListClipper clipper;
         clipper.Begin(static_cast<int>(m_rows.size()));
         while (clipper.Step())
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i)
            {
               const auto &row = m_rows[i];
               const std::string id = std::to_string(row.id);
               const char *values[] = {id.c_str(), row.flavor.c_str(), row.source.c_str(),
                  row.dest.c_str(), row.data.c_str()};
               ImGui::PushID(i);
               ImGui::TableNextRow();
               bool first = true;
               for (int col = 0; col < 5; ++col)
               {
                  if (m_flags & hidden[col])
                     continue;
                  ImGui::TableNextColumn();
                  if (first)
                  {
                     const ImVec2 pos = ImGui::GetCursorScreenPos();
                     if (ImGui::Selectable("##link", m_selected == row.id,
                         ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick))
                     {
                        m_selected = row.id;
                        if (row.editableData && (m_flags & kLinkEditEditButton) &&
                            ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                           m_action = LinkAction::Data;
                     }
                     ImGui::GetWindowDrawList()->AddText(pos, ImGui::GetColorU32(ImGuiCol_Text), values[col]);
                     first = false;
                  }
                  else
                     ImGui::TextUnformatted(values[col]);
               }
               ImGui::PopID();
            }
         ImGui::EndTable();
      }
   }
   ImGui::EndChild();

   bool button = false;
   const auto editButton = [&](eLinkEditorFlags flag, const char *label, LinkAction action, bool enabled) {
      if (!(m_flags & flag))
         return;
      if (button)
         ImGui::SameLine();
      ImGui::BeginDisabled(!enabled);
      if (ImGui::Button(label))
         m_action = action;
      ImGui::EndDisabled();
      button = true;
   };
   editButton(kLinkEditAddButton, "Add", LinkAction::Add, true);
   editButton(kLinkEditDeleteButton, "Delete", LinkAction::Remove, selection != m_rows.end());
   editButton(kLinkEditEditButton, "Edit Data", LinkAction::Data,
      selection != m_rows.end() && selection->editableData);
   if (m_flags & kLinkEditDoneButton)
   {
      if (button)
         ImGui::SameLine();
      if (ImGui::Button("Done"))
         Close();
   }
   if (HasKeyboardFocus() && ImGui::IsKeyPressed(ImGuiKey_Escape, false))
      Close();
}

void LinksDialog::AfterFrame()
{
   const LinkAction action = m_action;
   m_action = LinkAction::None;
   if (action == LinkAction::None)
      return;

   AutoAppIPtr_(LinkManager, links);
   if (!links)
      return;
   if (action == LinkAction::Add)
   {
      sLink link = m_pattern;
      CreateLink dialog(link, m_flags & kLinkEditShowAll);
      if (dialog.Accept(link))
         m_selected = links->Add(link.source, link.dest, link.flavor);
   }
   else if (action == LinkAction::Remove)
   {
      links->Remove(m_selected);
      m_selected = LINKID_NULL;
   }
   else
   {
      sLink link{};
      if (!links->Get(m_selected, &link))
         return;
      cAutoIPtr<IRelation> relation(links->GetRelation(link.flavor));
      AutoAppIPtr_(EditTools, editor);
      AutoAppIPtr_(StructDescTools, types);
      if (!relation || !editor || !types)
         return;
      const auto &data = *relation->DescribeData();
      const auto *desc = data.size ? types->Lookup(data.type) : nullptr;
      if (!desc || desc->size != data.size)
         return;

      std::vector<char> copy(data.size);
      if (void *current = relation->GetData(m_selected))
         std::memcpy(copy.data(), current, copy.size());
      if (editor->EditTypedData("Link Data", data.type, copy.data()) == S_OK)
         relation->SetData(m_selected, copy.data());
   }
   Refresh();
}
}

struct LinkEditor : LinksDialog
{
   LinkEditor(const sLinkEditorDesc &desc, sLink pattern) : LinksDialog(desc, pattern) {}
};

LinkEditor *LinkEditorCreate(ObjID src, ObjID dest, RelationID flavor, LinkEditorDesc *desc)
{
   sLinkEditorDesc current{"Link Editor", kLinkEditAllButtons};
   if (g_showInternalLinks)
      current.flags |= kLinkEditShowAll;
   if (desc)
   {
      if (desc->title[0])
         std::snprintf(current.title, sizeof(current.title), "%.*s", static_cast<int>(sizeof(desc->title)), desc->title);
      if (!(desc->flags & kLegacyLinkEditDoneButton))
         current.flags &= ~kLinkEditDoneButton;
   }
   auto dialog = std::make_unique<LinkEditor>(current, sLink{src, dest, flavor});
   if (!dialog->ShowModeless())
      return nullptr;
   return dialog.release();
}

void LinkEditorDestroy(LinkEditor *editor)
{
   if (editor)
   {
      editor->DismissModeless();
      delete editor;
   }
}

BOOL LinkEditorShowAllLinks(BOOL newval)
{
   const BOOL old = g_showInternalLinks;
   g_showInternalLinks = newval != FALSE;
   return old;
}

BOOL LGAPI DisplayLinkDialog(sLink &link)
{
   CreateLink dialog(link, g_showInternalLinks);
   return dialog.Accept(link);
}

void LGAPI DoLinkEditor(const sLinkEditorDesc *desc, ObjID src, ObjID dest, RelationID flavor)
{
   const sLinkEditorDesc standard{"", kLinkEditAllButtons};
   const auto &options = desc ? *desc : standard;
   if (options.flags & kLinkEditModeless)
   {
      auto dialog = std::make_unique<LinksDialog>(options, sLink{src, dest, flavor}, true);
      if (dialog->ShowModeless())
         dialog.release();
   }
   else
   {
      LinksDialog dialog(options, sLink{src, dest, flavor});
      dialog.Run();
   }
}
