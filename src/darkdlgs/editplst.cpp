#include <algorithm>
#include <cmath>
#include <cstdio>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <tuple>
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
#include <status.h>

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
   std::string category, name;
};

struct TraitRow
{
   IEditTrait *trait;
   std::string label;
};

struct PropertyBranch
{
   std::map<std::string, std::vector<TraitRow>> categories;
   std::vector<ObjID> donors;
   ObjID archetype = OBJ_NULL;
};

struct SelectedNode
{
   ObjID object;
   ObjID parent = OBJ_NULL;
   IEditTrait *trait = nullptr;
   std::string category;
   bool canRemoveDonor = false;
};

std::set<std::string> g_hiddenCategories;

enum class ObjectAction { None, Add, AddMeta, Remove, Edit, Refresh };

enum class PropertyIcon { Object, Category, Field };

bool PropertyTreeNode(const char *id, ImGuiTreeNodeFlags flags, PropertyIcon icon, const char *label)
{
   const float size = std::floor(ImGui::GetFontSize() * 0.9f);
   const float spaceWidth = ImGui::CalcTextSize(" ").x;
   const int labelPadding = static_cast<int>(std::ceil((size + ImGui::GetStyle().ItemInnerSpacing.x) / spaceWidth));
   const bool open = ImGui::TreeNodeEx(id, flags, "%*s%s", labelPadding, "", label);
   if (!ImGui::IsItemVisible())
      return open;

   const ImVec2 min = ImGui::GetItemRectMin();
   const ImVec2 max = ImGui::GetItemRectMax();
   const ImVec2 origin(min.x + ImGui::GetTreeNodeToLabelSpacing(),
                       std::floor((min.y + max.y - size) * 0.5f));
   const auto point = [&](float x, float y) { return ImVec2(origin.x + x * size, origin.y + y * size); };
   ImDrawList *draw = ImGui::GetWindowDrawList();
   switch (icon)
   {
   case PropertyIcon::Object:
   {
      const ImU32 color = ImGui::GetColorU32(ImVec4(0.53f, 0.82f, 0.95f, 1.0f));
      const ImVec2 outline[] = {point(0.5f, 0.06f), point(0.92f, 0.29f), point(0.92f, 0.73f),
         point(0.5f, 0.96f), point(0.08f, 0.73f), point(0.08f, 0.29f)};
      draw->AddPolyline(outline, IM_ARRAYSIZE(outline), color, 1.0f, ImDrawFlags_Closed);
      const ImVec2 center = point(0.5f, 0.51f);
      draw->AddLine(outline[1], center, color);
      draw->AddLine(outline[3], center, color);
      draw->AddLine(outline[5], center, color);
      break;
   }
   case PropertyIcon::Category:
   {
      const ImU32 color = ImGui::GetColorU32(ImVec4(0.98f, 0.76f, 0.42f, 1.0f));
      const ImVec2 outline[] = {point(0.08f, 0.86f), point(0.08f, 0.19f), point(0.42f, 0.19f),
         point(0.56f, 0.36f), point(0.92f, 0.36f), point(0.92f, 0.86f)};
      draw->AddPolyline(outline, IM_ARRAYSIZE(outline), color, 1.0f, ImDrawFlags_Closed);
      break;
   }
   case PropertyIcon::Field:
   {
      const ImU32 color = ImGui::GetColorU32(ImGuiCol_Text);
      draw->AddQuadFilled(point(0.5f, 0.22f), point(0.78f, 0.5f),
                          point(0.5f, 0.78f), point(0.22f, 0.5f), color);
      break;
   }
   }
   return open;
}

std::string ObjectTitle(const sObjEditorDesc &desc, ObjID object)
{
   return (desc.title[0]
      ? std::string(desc.title, std::find(desc.title, desc.title + sizeof(desc.title), '\0'))
      : std::string(ObjEditName(object))) + "###DarkDlgsObjectProperties";
}

class ObjectProperties : public cCTUnaggregated<IObjEditor, &IID_IObjEditor, kCTU_Default>,
                         private DarkDlgsModal
{
public:
   ObjectProperties(const sObjEditorDesc &desc, ObjID object)
      : DarkDlgsModal(ObjectTitle(desc, object), ImVec2(500, 455)), m_desc(desc), m_object(object), m_selection{object}
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
   const PropertyBranch &Branch(ObjID object);
   void DrawObject(const ImGuiTextFilter &filter, ObjID object, ObjID parent = OBJ_NULL,
                   bool canRemoveDonor = false);
   void DrawCategory(const ImGuiTextFilter &filter, ObjID object, const std::string &category,
                     const std::vector<TraitRow> &rows);
   void DrawActions();
   void DrawContents() override;
   void AfterFrame() override;
   bool KeepVisibleBehindModal() const override { return true; }
   void OnModelessClosed() override;
   void Notify();
   bool OpenModeless();
   void AddTrait(ObjID object);
   void AddMetaProperty(ObjID object);
   void RenameObject(ObjID object);

   sObjEditorDesc m_desc;
   ObjID m_object;
   ObjEditCB m_callback = nullptr;
   ObjEditCBData m_callbackData = nullptr;
   std::vector<TraitEntry> m_traits;
   std::map<ObjID, PropertyBranch> m_branches;
   std::set<ObjID> m_path;
   SelectedNode m_selection;
   char m_filter[128]{};
   bool m_expand = false;
   bool m_done = false;
   ObjectAction m_action = ObjectAction::None;
};

void ObjectProperties::CollectTraits()
{
   AutoAppIPtr_(ObjEditors, editors);
   m_selection = {m_object};
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
      m_traits.push_back({std::move(trait), strings.category[0] ? strings.category : "General",
                          strings.friendly_name[0] ? strings.friendly_name : strings.name});
   }
   std::stable_sort(m_traits.begin(), m_traits.end(), [](const auto &a, const auto &b) {
      return std::tie(a.category, a.name) < std::tie(b.category, b.name);
   });
}

void ObjectProperties::Refresh()
{
   m_branches.clear();
}

const PropertyBranch &ObjectProperties::Branch(ObjID object)
{
   const auto [entry, inserted] = m_branches.try_emplace(object);
   auto &branch = entry->second;
   if (!inserted)
      return branch;

   AutoAppIPtr_(TraitManager, traits);
   if (traits)
   {
      branch.archetype = traits->GetArchetype(object);
      cAutoIPtr<IObjectQuery> query(traits->Query(object, kTraitQueryDonors));
      for (; query && !query->Done(); query->Next())
      {
         const ObjID donor = query->Object();
         if (donor != object)
            branch.donors.push_back(donor);
      }
   }

   for (const auto &item : m_traits)
   {
      auto *trait = item.trait.get();
      if (!trait->IntrinsicTo(object))
         continue;
      char value[256]{};
      if (trait->Capabilities()->flags & kTraitCanUnparse)
         trait->Unparse(object, value, sizeof(value));
      value[sizeof(value) - 1] = '\0';
      std::string label = item.name;
      if (value[0])
         label += std::string(": ") + value;
      branch.categories[item.category].push_back({trait, std::move(label)});
   }
   return branch;
}

void ObjectProperties::DrawCategory(const ImGuiTextFilter &filter, ObjID object, const std::string &category,
                                    const std::vector<TraitRow> &rows)
{
   if (g_hiddenCategories.count(category))
      return;
   const bool categoryMatches = filter.PassFilter(category.c_str());
   if (!categoryMatches && std::none_of(rows.begin(), rows.end(), [&](const auto &row) {
          return filter.PassFilter(row.label.c_str());
       }))
      return;

   ImGui::PushID(category.c_str());
   ImGui::SetNextItemOpen(true, m_expand || filter.IsActive() ? ImGuiCond_Always : ImGuiCond_Once);
   ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_OpenOnArrow |
      ImGuiTreeNodeFlags_DrawLinesFull;
   if (m_selection.object == object && m_selection.category == category)
      flags |= ImGuiTreeNodeFlags_Selected;
   const bool open = PropertyTreeNode("category", flags, PropertyIcon::Category, category.c_str());
   if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
      m_selection = {object, OBJ_NULL, nullptr, category};
   if (open)
   {
      for (const auto &row : rows)
      {
         if (!categoryMatches && !filter.PassFilter(row.label.c_str()))
            continue;
         ImGui::PushID(row.trait);
         ImGuiTreeNodeFlags leaf = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen |
            ImGuiTreeNodeFlags_SpanAvailWidth;
         if (m_selection.object == object && m_selection.trait == row.trait)
            leaf |= ImGuiTreeNodeFlags_Selected;
         PropertyTreeNode("property", leaf, PropertyIcon::Field, row.label.c_str());
         if (ImGui::IsItemClicked())
         {
            m_selection = {object, OBJ_NULL, row.trait, {}};
            if ((row.trait->Capabilities()->flags & kTraitCanEdit) &&
                ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
               m_action = ObjectAction::Edit;
         }
         ImGui::PopID();
      }
      ImGui::TreePop();
   }
   ImGui::PopID();
}

void ObjectProperties::DrawObject(const ImGuiTextFilter &filter, ObjID object, ObjID parent, bool canRemoveDonor)
{
   if (!m_path.insert(object).second)
      return;
   ImGui::PushID(object);
   if (m_expand || !parent)
      ImGui::SetNextItemOpen(true, m_expand ? ImGuiCond_Always : ImGuiCond_Once);
   ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_OpenOnArrow |
      ImGuiTreeNodeFlags_DrawLinesFull;
   if (m_selection.object == object && m_selection.parent == parent &&
       !m_selection.trait && m_selection.category.empty())
      flags |= ImGuiTreeNodeFlags_Selected;
   const bool open = PropertyTreeNode("object", flags, PropertyIcon::Object, ObjEditName(object));
   if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
   {
      m_selection = {object, parent, nullptr, {}, canRemoveDonor};
      if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
         m_action = ObjectAction::Edit;
   }
   if (open)
   {
      const auto &branch = Branch(object);
      for (const auto &[category, rows] : branch.categories)
         DrawCategory(filter, object, category, rows);
      for (ObjID donor : branch.donors)
         DrawObject(filter, donor, object, donor != branch.archetype);
      ImGui::TreePop();
   }
   ImGui::PopID();
   m_path.erase(object);
}

void ObjectProperties::DrawActions()
{
   const ImVec2 button(-1, 0);
   if (ImGui::Button("Add", button))
      ImGui::OpenPopup("Add Trait");
   if (ImGui::BeginPopup("Add Trait"))
   {
      if (ImGui::MenuItem("Property..."))
         m_action = ObjectAction::Add;
      if (ImGui::MenuItem("MetaProperty..."))
         m_action = ObjectAction::AddMeta;
      ImGui::EndPopup();
   }

   const auto caps = m_selection.trait ? m_selection.trait->Capabilities()->flags : 0;
   ImGui::BeginDisabled(!(caps & kTraitCanRemove) && !m_selection.canRemoveDonor);
   if (ImGui::Button("Delete", button))
      m_action = ObjectAction::Remove;
   ImGui::EndDisabled();
   ImGui::BeginDisabled(m_selection.trait ? !(caps & kTraitCanEdit) : !m_selection.category.empty());
   if (ImGui::Button("Edit", button))
      m_action = ObjectAction::Edit;
   ImGui::EndDisabled();
   if (ImGui::Button("Expand", button))
      m_expand = true;
   if (ImGui::Button("Refresh", button))
      m_action = ObjectAction::Refresh;
   ImGui::BeginDisabled(m_selection.category.empty());
   if (ImGui::Button("Hide", button))
   {
      g_hiddenCategories.insert(m_selection.category);
      m_selection = {m_selection.object};
   }
   ImGui::EndDisabled();

   ImGui::TextUnformatted("Hidden");
   const float footer = ImGui::GetFrameHeightWithSpacing();
   if (ImGui::BeginChild("Hidden categories", ImVec2(0, -footer), ImGuiChildFlags_Borders))
   {
      for (auto it = g_hiddenCategories.begin(); it != g_hiddenCategories.end(); ++it)
      {
         if (ImGui::Selectable(it->c_str(), false, ImGuiSelectableFlags_AllowDoubleClick) &&
             ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
         {
            g_hiddenCategories.erase(it);
            break;
         }
         ImGui::SetItemTooltip("Double-click to show");
      }
   }
   ImGui::EndChild();
   if (ImGui::Button("Done", button))
   {
      m_done = true;
      Close();
   }
}

void ObjectProperties::DrawContents()
{
   const float sidebar = ImGui::GetFontSize() * 9.0f;
   if (ImGui::BeginChild("Property browser", ImVec2(-sidebar - ImGui::GetStyle().ItemSpacing.x, 0)))
   {
      ImGui::SetNextItemWidth(-1);
      ImGui::InputTextWithHint("##filter", "Filter properties", m_filter, sizeof(m_filter));
      if (ImGui::BeginChild("Properties", ImVec2(0, 0), ImGuiChildFlags_Borders))
         DrawObject(ImGuiTextFilter(m_filter), m_object);
      ImGui::EndChild();
      m_expand = false;
   }
   ImGui::EndChild();
   ImGui::SameLine();
   if (ImGui::BeginChild("Actions", ImVec2(sidebar, 0)))
      DrawActions();
   ImGui::EndChild();

   if (HasKeyboardFocus() && ImGui::IsKeyPressed(ImGuiKey_Escape, false))
      Close();
}

void ObjectProperties::AddTrait(ObjID object)
{
   std::vector<std::string> names;
   std::vector<IEditTrait *> traits;
   for (const auto &item : m_traits)
      if ((item.trait->Capabilities()->flags & kTraitCanAdd) && !item.trait->IntrinsicTo(object))
      {
         names.push_back(item.category + " / " + item.name);
         traits.push_back(item.trait.get());
      }

   std::vector<const char *> labels;
   for (const auto &name : names)
      labels.push_back(name.c_str());
   const int selected = DoSimpleMenu("Add Property", labels.data(), static_cast<int>(labels.size()));
   if (selected >= 0)
      traits[selected]->Add(object);
}

void ObjectProperties::AddMetaProperty(ObjID object)
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
      if (obj != root && obj != object && manager->IsMetaProperty(obj) &&
          !manager->ObjHasDonorIntrinsically(object, obj) && !manager->ObjHasDonor(obj, object))
         choices.emplace_back(ObjEditName(obj), obj);
   }
   query = nullptr;
   std::sort(choices.begin(), choices.end());
   std::vector<const char *> labels;
   for (const auto &choice : choices)
      labels.push_back(choice.first.c_str());
   const int selected = DoSimpleMenu("Add MetaProperty", labels.data(), static_cast<int>(labels.size()));
   if (selected >= 0)
      manager->AddObjMetaProperty(object, choices[selected].second);
}

class ObjectNameDialog final : public DarkDlgsModal
{
public:
   explicit ObjectNameDialog(const char *current)
      : DarkDlgsModal("Rename Object###DarkDlgsRenameObject", ImVec2(360, 150))
   {
      if (current)
         std::snprintf(name, sizeof(name), "%s", current);
   }

   char name[128]{};
   bool accepted = false;

private:
   void DrawContents() override
   {
      if (FirstFrame())
         ImGui::SetKeyboardFocusHere();
      const bool enter = ImGui::InputText("Name", name, sizeof(name),
         ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue);
      if ((ImGui::Button("OK") || enter) && name[0])
      {
         accepted = true;
         Close();
      }
      ImGui::SameLine();
      if (ImGui::Button("Cancel") || ImGui::IsKeyPressed(ImGuiKey_Escape, false))
         Close();
   }
};

void ObjectProperties::RenameObject(ObjID object)
{
   AutoAppIPtr_(ObjectSystem, objects);
   if (!objects || !objects->Exists(object))
      return;
   ObjectNameDialog dialog(objects->GetName(object));
   if (dialog.Run() && dialog.accepted)
   {
      const ObjID existing = objects->GetObjectNamed(dialog.name);
      if ((existing && existing != object) || FAILED(objects->NameObject(object, dialog.name)))
         Status("Could not rename object. Check that the name is available.");
      else if (object == m_object)
         SetTitle(ObjectTitle(m_desc, m_object));
   }
}

void ObjectProperties::AfterFrame()
{
   const auto action = m_action;
   m_action = ObjectAction::None;
   if (action == ObjectAction::None)
      return;
   AddRef();
   cAutoIPtr<IObjEditor> keepAlive(this);
   const SelectedNode selection = m_selection;
   switch (action)
   {
   case ObjectAction::None:
   case ObjectAction::Refresh:
      break;
   case ObjectAction::Add:
      AddTrait(selection.object);
      break;
   case ObjectAction::AddMeta:
      AddMetaProperty(selection.object);
      break;
   case ObjectAction::Edit:
      if (selection.trait)
         selection.trait->Edit(selection.object);
      else
         RenameObject(selection.object);
      break;
   case ObjectAction::Remove:
      if (selection.trait)
      {
         selection.trait->Remove(selection.object);
         m_selection = {selection.object};
      }
      else if (selection.canRemoveDonor)
      {
         AutoAppIPtr_(TraitManager, manager);
         if (manager)
            manager->RemoveObjMetaProperty(selection.parent, selection.object);
         m_selection = {selection.parent};
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
