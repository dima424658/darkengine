#include <algorithm>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

#include <appagg.h>
#include <edtraist.h>
#include <edtrait.h>
#include <iobjed.h>
#include <iobjsys.h>
#include <objediid.h>
#include <objedit.h>
#include <objquery.h>
#include <traitbas.h>
#include <traitman.h>

#include "darkdlgs.h"
#include "editplst.h"
#include "modal.h"

namespace
{
struct ReleaseTrait
{
   void operator()(IEditTrait *trait) const { trait->Release(); }
};
using TraitPtr = std::unique_ptr<IEditTrait, ReleaseTrait>;

struct TraitEntry
{
   TraitPtr trait;
   std::string label;
};

struct TraitRow
{
   IEditTrait *trait;
   std::string label, origin, value;
   ObjID donor;
   bool metaproperty;
};

enum class ObjectAction { None, Add, AddMeta, Remove, Edit };

std::string ObjectTitle(const sObjEditorDesc &desc, ObjID object)
{
   return (desc.title[0]
      ? std::string(desc.title, std::find(desc.title, desc.title + sizeof(desc.title), '\0'))
      : std::string("Properties of ") + ObjEditName(object)) + "###DarkDlgsObjectProperties";
}

class ObjectProperties : public cCTUnaggregated<IObjEditor, &IID_IObjEditor, kCTU_Default>,
                         private DarkDlgsModal
{
public:
   ObjectProperties(const sObjEditorDesc &desc, ObjID object)
      : DarkDlgsModal(ObjectTitle(desc, object), ImVec2(670, 450)), m_desc(desc), m_object(object)
   {
   }

   STDMETHOD_(BOOL, Go)(eObjEdModality modality) override;
   STDMETHOD(SetCallback)(ObjEditCB callback, ObjEditCBData data) override
   {
      m_callback = callback;
      m_callbackData = data;
      return S_OK;
   }
   STDMETHOD_(const sObjEditorDesc *, Describe)() override { return &m_desc; }
   STDMETHOD_(ObjID, Object)() override { return m_object; }
   bool OpenPropertyList() { return OpenModeless(); }

private:
   void CollectTraits();
   void Refresh();
   void DrawContents() override;
   void AfterFrame() override;
   void OnModelessClosed() override;
   void Notify();
   bool OpenModeless();
   void AddTrait();
   void AddMetaProperty();

   sObjEditorDesc m_desc;
   ObjID m_object;
   ObjEditCB m_callback = nullptr;
   ObjEditCBData m_callbackData = nullptr;
   std::vector<TraitEntry> m_traits;
   std::vector<TraitRow> m_rows;
   char m_filter[128]{};
   bool m_showInherited = true;
   bool m_done = false;
   int m_selected = -1;
   ObjectAction m_action = ObjectAction::None;
};

void ObjectProperties::CollectTraits()
{
   AutoAppIPtr_(ObjEditors, editors);
   m_traits.clear();
   if (!editors)
      return;

   sEditTraitIter iter{};
   for (IEditTrait *item = editors->FirstTrait(&iter); item; item = editors->NextTrait(&iter))
   {
      TraitPtr trait(item);
      const auto &desc = *trait->Describe();
      if ((desc.flags & kTraitInvisible) && !(m_desc.flags & kObjEdShowAll))
         continue;
      const auto &strings = desc.strings;
      std::string label;
      if (strings.category[0])
         label = std::string(strings.category) + " / ";
      label += strings.friendly_name[0] ? strings.friendly_name : strings.name;
      m_traits.push_back({std::move(trait), std::move(label)});
   }
   std::stable_sort(m_traits.begin(), m_traits.end(), [](const auto &a, const auto &b) { return a.label < b.label; });
}

void ObjectProperties::Refresh()
{
   m_rows.clear();
   m_selected = -1;
   AutoAppIPtr_(TraitManager, manager);
   if (!manager)
      return;

   std::vector<ObjID> donors;
   if (m_showInherited)
   {
      cAutoIPtr<IObjectQuery> query(manager->Query(m_object, kTraitQueryAllDonors));
      for (; query && !query->Done(); query->Next())
         donors.push_back(query->Object());
   }

   cAutoIPtr<IObjectQuery> direct(manager->Query(m_object, kTraitQueryDonors));
   for (; direct && !direct->Done(); direct->Next())
   {
      const ObjID donor = direct->Object();
      if (manager->IsMetaProperty(donor) && donor != manager->GetArchetype(m_object))
         m_rows.push_back({nullptr, ObjEditName(donor), "MetaProperty", {}, donor, true});
   }

   for (const auto &item : m_traits)
   {
      auto *trait = item.trait.get();
      ObjID origin = m_object;
      if (!trait->IntrinsicTo(m_object))
      {
         const auto donor = std::find_if(donors.begin(), donors.end(),
            [trait](ObjID obj) { return trait->IntrinsicTo(obj); });
         if (donor == donors.end())
            continue;
         origin = *donor;
      }
      char value[256]{};
      if (trait->Capabilities()->flags & kTraitCanUnparse)
         trait->Unparse(origin, value, sizeof(value));
      value[sizeof(value) - 1] = '\0';
      m_rows.push_back({trait, item.label, origin == m_object ? "Local" : ObjEditName(origin),
                        value, origin, false});
   }
}

void ObjectProperties::DrawContents()
{
   ImGui::InputTextWithHint("##filter", "Filter properties", m_filter, sizeof(m_filter));
   ImGui::SameLine();
   if (ImGui::Checkbox("Inherited", &m_showInherited))
      Refresh();

   const float footer = ImGui::GetFrameHeightWithSpacing();
   if (ImGui::BeginChild("Properties", ImVec2(0, -footer), ImGuiChildFlags_Borders))
   {
      if (ImGui::BeginTable("Traits", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable))
      {
         ImGui::TableSetupColumn("Property");
         ImGui::TableSetupColumn("Value");
         ImGui::TableSetupColumn("Origin");
         ImGui::TableHeadersRow();
         const ImGuiTextFilter filter(m_filter);
         for (size_t i = 0; i < m_rows.size(); ++i)
         {
            const auto &row = m_rows[i];
            if (!filter.PassFilter(row.label.c_str()))
               continue;
            ImGui::PushID(static_cast<int>(i));
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            const ImVec2 pos = ImGui::GetCursorScreenPos();
            if (ImGui::Selectable("##property", m_selected == static_cast<int>(i),
                 ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick))
            {
               m_selected = static_cast<int>(i);
               if (row.trait && row.donor == m_object && (row.trait->Capabilities()->flags & kTraitCanEdit) &&
                   ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                  m_action = ObjectAction::Edit;
            }
            ImGui::GetWindowDrawList()->AddText(pos, ImGui::GetColorU32(ImGuiCol_Text), row.label.c_str());
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(row.value.c_str());
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(row.origin.c_str());
            ImGui::PopID();
         }
         ImGui::EndTable();
      }
   }
   ImGui::EndChild();

   const TraitRow *row = m_selected >= 0 ? &m_rows[m_selected] : nullptr;
   if (ImGui::Button("Add Property"))
      m_action = ObjectAction::Add;
   ImGui::SameLine();
   if (ImGui::Button("Add MetaProperty"))
      m_action = ObjectAction::AddMeta;
   ImGui::SameLine();
   const auto caps = row && row->trait && row->donor == m_object ? row->trait->Capabilities()->flags : 0;
   ImGui::BeginDisabled(!(caps & kTraitCanEdit));
   if (ImGui::Button("Edit"))
      m_action = ObjectAction::Edit;
   ImGui::EndDisabled();
   ImGui::SameLine();
   ImGui::BeginDisabled(!(caps & kTraitCanRemove) && !(row && row->metaproperty));
   if (ImGui::Button("Remove"))
      m_action = ObjectAction::Remove;
   ImGui::EndDisabled();
   ImGui::SameLine();
   if (ImGui::Button("Done"))
   {
      m_done = true;
      Close();
   }
   if (HasKeyboardFocus() && ImGui::IsKeyPressed(ImGuiKey_Escape, false))
      Close();
}

void ObjectProperties::AddTrait()
{
   std::vector<const char *> labels;
   std::vector<IEditTrait *> traits;
   for (const auto &item : m_traits)
      if ((item.trait->Capabilities()->flags & kTraitCanAdd) && !item.trait->IntrinsicTo(m_object))
      {
         labels.push_back(item.label.c_str());
         traits.push_back(item.trait.get());
      }

   const int selected = DoSimpleMenu("Add Property", labels.data(), static_cast<int>(labels.size()));
   if (selected >= 0)
      traits[selected]->Add(m_object);
}

void ObjectProperties::AddMetaProperty()
{
   AutoAppIPtr_(TraitManager, manager);
   if (!manager)
      return;
   const ObjID root = manager->RootMetaProperty();
   cAutoIPtr<IObjectQuery> query(manager->Query(root, kTraitQueryAllDescendents));
   std::vector<std::pair<std::string, ObjID>> choices;
   for (; query && !query->Done(); query->Next())
   {
      const auto obj = query->Object();
      if (obj != root && obj != m_object && manager->IsMetaProperty(obj) &&
          !manager->ObjHasDonorIntrinsically(m_object, obj) && !manager->ObjHasDonor(obj, m_object))
         choices.emplace_back(ObjEditName(obj), obj);
   }
   query = nullptr;
   std::sort(choices.begin(), choices.end());
   std::vector<const char *> labels;
   for (const auto &choice : choices)
      labels.push_back(choice.first.c_str());
   const int selected = DoSimpleMenu("Add MetaProperty", labels.data(), static_cast<int>(labels.size()));
   if (selected >= 0)
      manager->AddObjMetaProperty(m_object, choices[selected].second);
}

void ObjectProperties::AfterFrame()
{
   const auto action = m_action;
   m_action = ObjectAction::None;
   if (action == ObjectAction::None)
      return;
   AddRef();
   cAutoIPtr<IObjEditor> keepAlive(this);
   switch (action)
   {
   case ObjectAction::None:
      return;
   case ObjectAction::Add:
      AddTrait();
      break;
   case ObjectAction::AddMeta:
      AddMetaProperty();
      break;
   case ObjectAction::Edit:
      if (m_selected >= 0 && m_rows[m_selected].trait)
         m_rows[m_selected].trait->Edit(m_object);
      break;
   case ObjectAction::Remove:
      if (m_selected >= 0)
      {
         const auto &row = m_rows[m_selected];
         if (row.metaproperty)
         {
            AutoAppIPtr_(TraitManager, manager);
            if (manager)
               manager->RemoveObjMetaProperty(m_object, row.donor);
         }
         else if (row.trait)
            row.trait->Remove(m_object);
      }
      break;
   }
   if (IsOpen())
      Refresh();
}

void ObjectProperties::Notify()
{
   if (m_callback)
   {
      sObjEditorEvent event{m_done ? kObjEdDone : kObjEdCancel, this};
      m_callback(&event, m_callbackData);
   }
}

void ObjectProperties::OnModelessClosed()
{
   Notify();
   Release();
}

bool ObjectProperties::OpenModeless()
{
   CollectTraits();
   Refresh();
   m_done = false;
   if (!ShowModeless())
      return false;
   AddRef();
   return true;
}

STDMETHODIMP_(BOOL) ObjectProperties::Go(eObjEdModality modality)
{
   if (IsOpen())
      return FALSE;
   AddRef();
   cAutoIPtr<IObjEditor> keepAlive(this);
   if (modality == kObjEdModeless)
   {
      if (!OpenModeless())
         Notify();
      return FALSE;
   }

   m_done = false;
   CollectTraits();
   Refresh();
   Run();
   Notify();
   return m_done;
}
}

struct PropertyListEditor : ObjectProperties
{
   PropertyListEditor(const sObjEditorDesc &desc, ObjID object) : ObjectProperties(desc, object) {}
};

PropertyListEditor *PropertyListEditorCreate(ObjID object, PropertyListEditorDesc *desc)
{
   AutoAppIPtr_(ObjectSystem, objects);
   if (!objects || !object || !objects->Exists(object))
      return nullptr;
   sObjEditorDesc options{};
   if (desc)
   {
      std::snprintf(options.title, sizeof(options.title), "%.*s", static_cast<int>(sizeof(desc->title)), desc->title);
      options.flags = desc->flags;
   }
   auto *editor = new PropertyListEditor(options, object);
   const bool opened = editor->OpenPropertyList();
   editor->Release();
   return opened ? editor : nullptr;
}

IObjEditor *LGAPI ConstructObjEditor(const sObjEditorDesc *desc, ObjID object)
{
   AutoAppIPtr_(ObjectSystem, objects);
   if (!objects || !object || !objects->Exists(object))
      return nullptr;
   return new ObjectProperties(desc ? *desc : sObjEditorDesc{}, object);
}
