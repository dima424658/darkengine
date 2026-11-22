#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <vector>

#include <appagg.h>
#include <iobjed.h>
#include <iobjsys.h>
#include <objedit.h>
#include <objquery.h>
#include <osysbase.h>
#include <otreebas.h>
#include <traitbas.h>
#include <traitman.h>
#include <winui.h>

#include "darkdlgs.h"
#include "modal.h"

namespace
{
struct Tree
{
   sObjTreeDesc desc;
   ObjID root;
};

struct TreeNode
{
   ObjID id;
   std::string label;
};

enum class TreeAction { None, Add, Delete, Properties, Create };

class Hierarchy final : public DarkDlgsModal
{
public:
   Hierarchy(const sObjTreeEditorDesc &desc, ObjID focus)
      : DarkDlgsModal((desc.title[0] ?
         std::string(desc.title, std::find(desc.title, desc.title + sizeof(desc.title), '\0')) :
         "Object Hierarchy") + "###DarkDlgsHierarchy", ImVec2(580, 480)),
        m_flags(desc.flags)
   {
      AutoAppIPtr_(ObjectSystem, objects);
      AutoAppIPtr_(TraitManager, traits);
      if (!objects || !traits || !desc.trees)
         return;
      for (const sObjTreeDesc *tree = desc.trees; tree->treename[0]; ++tree)
      {
         const ObjID root = objects->GetObjectNamed(tree->rootname.text);
         if (root)
            m_trees.push_back({*tree, root});
      }
      if (m_trees.empty())
         return;

      m_selected = m_trees.front().root;
      if (!focus || !objects->Exists(focus))
         return;
      std::set<ObjID> ancestors;
      for (ObjID obj = focus; obj && ancestors.insert(obj).second; obj = traits->GetArchetype(obj))
      {
         const auto tree = std::find_if(m_trees.begin(), m_trees.end(),
            [obj](const Tree &item) { return item.root == obj; });
         if (tree != m_trees.end() &&
             (!OBJ_IS_CONCRETE(focus) || ((tree->desc.flags | m_flags) & kObjTreePermitConcretes)))
         {
            m_tree = static_cast<size_t>(tree - m_trees.begin());
            m_selected = focus;
            m_expand = std::move(ancestors);
            m_scrollToSelection = true;
            break;
         }
      }
   }

private:
   const std::vector<TreeNode> &Children(ObjID obj);
   void DrawNode(ObjID obj, const std::string &label);
   void DrawContents() override;
   void AfterFrame() override;
   void AddNode();
   void DeleteNode();
   void FinishAction(eTreeCBResult result);

   std::vector<Tree> m_trees;
   ulong m_flags;
   size_t m_tree = 0;
   ObjID m_selected = OBJ_NULL;
   std::map<ObjID, std::vector<TreeNode>> m_children;
   std::set<ObjID> m_expand;
   std::set<ObjID> m_path;
   bool m_scrollToSelection = false;
   TreeAction m_action = TreeAction::None;
   const char *m_message = nullptr;
};

const std::vector<TreeNode> &Hierarchy::Children(ObjID obj)
{
   const auto [it, inserted] = m_children.try_emplace(obj);
   auto &children = it->second;
   if (!inserted)
      return children;
   AutoAppIPtr_(TraitManager, traits);
   if (!traits)
      return children;

   const bool concrete = (m_trees[m_tree].desc.flags | m_flags) & kObjTreePermitConcretes;
   cAutoIPtr<IObjectQuery> query(traits->Query(obj, kTraitQueryDescendents));
   for (; query && !query->Done(); query->Next())
   {
      const ObjID child = query->Object();
      if ((concrete || OBJ_IS_ABSTRACT(child)) && traits->GetArchetype(child) == obj && child != obj)
         children.push_back({child, ObjEditName(child)});
   }
   std::sort(children.begin(), children.end(), [](const auto &a, const auto &b) { return a.label < b.label; });
   return children;
}

void Hierarchy::DrawNode(ObjID obj, const std::string &label)
{
   if (!m_path.insert(obj).second)
      return;
   const auto &children = Children(obj);
   ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_OpenOnArrow;
   if (obj == m_selected)
      flags |= ImGuiTreeNodeFlags_Selected;
   if (children.empty())
      flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
   if (m_expand.count(obj))
      ImGui::SetNextItemOpen(true, ImGuiCond_Always);
   else if (obj == m_trees[m_tree].root)
      ImGui::SetNextItemOpen(true, ImGuiCond_Once);

   ImGui::PushID(obj);
   const bool open = ImGui::TreeNodeEx("node", flags, "%s", label.c_str());
   if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
   {
      m_selected = obj;
      if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
         m_action = TreeAction::Properties;
   }
   if (m_scrollToSelection && obj == m_selected)
   {
      m_scrollToSelection = false;
      ImGui::SetScrollHereY();
   }
   if (open && !children.empty())
   {
      for (const auto &child : children)
         DrawNode(child.id, child.label);
      ImGui::TreePop();
   }
   ImGui::PopID();
   m_path.erase(obj);
}

void Hierarchy::DrawContents()
{
   if (m_trees.empty())
   {
      ImGui::TextUnformatted("No hierarchy roots in this database.");
      if (ImGui::Button("Done") || ImGui::IsKeyPressed(ImGuiKey_Escape, false))
         Close();
      return;
   }

   if (ImGui::BeginCombo("Tree", m_trees[m_tree].desc.treename))
   {
      for (size_t i = 0; i < m_trees.size(); ++i)
         if (ImGui::Selectable(m_trees[i].desc.treename, m_tree == i))
         {
            m_tree = i;
            m_selected = m_trees[i].root;
            m_children.clear();
            m_expand.clear();
         }
      ImGui::EndCombo();
   }

   const float footer = ImGui::GetFrameHeightWithSpacing() * (m_message ? 2 : 1);
   const Tree &tree = m_trees[m_tree];
   if (ImGui::BeginChild("Hierarchy", ImVec2(0, -footer), ImGuiChildFlags_Borders))
      DrawNode(tree.root, ObjEditName(tree.root));
   ImGui::EndChild();
   m_expand.clear();

   if (ImGui::Button("Properties"))
      m_action = TreeAction::Properties;
   const ulong flags = tree.desc.flags | m_flags;
   if (!(flags & kObjTreeNoAddButton))
   {
      ImGui::SameLine();
      if (ImGui::Button("Add"))
         m_action = TreeAction::Add;
   }
   if (!(flags & kObjTreeNoDelButton))
   {
      ImGui::SameLine();
      ImGui::BeginDisabled(m_selected == tree.root);
      if (ImGui::Button("Delete"))
         m_action = TreeAction::Delete;
      ImGui::EndDisabled();
   }
   if (!(flags & kObjTreeNoCreateButton) && tree.desc.func.create)
   {
      ImGui::SameLine();
      if (ImGui::Button("Create"))
         m_action = TreeAction::Create;
   }
   ImGui::SameLine();
   if (ImGui::Button("Done") || ImGui::IsKeyPressed(ImGuiKey_Escape, false))
      Close();
   if (m_message)
      ImGui::TextUnformatted(m_message);
}

class NewName final : public DarkDlgsModal
{
public:
   NewName() : DarkDlgsModal("New Object Name###DarkDlgsNewObject", ImVec2(420, 150)) {}
   char name[128]{};
   bool accepted = false;

private:
   void DrawContents() override
   {
      if (FirstFrame())
         ImGui::SetKeyboardFocusHere();
      const bool enter = ImGui::InputText("Name", name, sizeof(name), ImGuiInputTextFlags_EnterReturnsTrue);
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

void Hierarchy::FinishAction(eTreeCBResult result)
{
   if (result & (kObjTreeChanged | kObjTreeNormal))
      m_children.clear();
   if (result & kObjTreeQuit)
      Close();
}

void Hierarchy::AddNode()
{
   NewName dialog;
   if (!dialog.Run() || !dialog.accepted)
      return;
   AutoAppIPtr_(ObjectSystem, objects);
   AutoAppIPtr_(TraitManager, traits);
   if (!objects || !traits || !objects->Exists(m_selected))
      return;
   if (objects->GetObjectNamed(dialog.name))
   {
      m_message = "An object already has that name.";
      return;
   }

   const auto &tree = m_trees[m_tree].desc;
   const ObjID parent = m_selected;
   eTreeCBResult result = kObjTreeNormal;
   ObjID obj = OBJ_NULL;
   if (tree.func.add)
   {
      obj = objects->BeginCreate(parent, OBJ_IS_CONCRETE(parent) ? kObjectConcrete : kObjectAbstract);
      if (obj)
      {
         result = tree.func.add(obj, dialog.name);
         if ((result & kObjTreeNormal) && !(result & kObjTreeCancel))
            objects->NameObject(obj, dialog.name);
         objects->EndCreate(obj);
         if (result & kObjTreeCancel)
         {
            objects->Destroy(obj);
            obj = OBJ_NULL;
         }
      }
   }
   else if (traits->IsMetaProperty(parent))
      obj = traits->CreateMetaProperty(dialog.name, parent);
   else if (OBJ_IS_ABSTRACT(parent))
      obj = traits->CreateArchetype(dialog.name, parent);
   else
   {
      obj = objects->BeginCreate(parent, kObjectConcrete);
      if (obj)
      {
         objects->NameObject(obj, dialog.name);
         objects->EndCreate(obj);
      }
   }
   if (obj)
   {
      m_selected = obj;
      m_expand.insert(parent);
      m_scrollToSelection = true;
   }
   FinishAction(result);
}

void Hierarchy::DeleteNode()
{
   const auto &tree = m_trees[m_tree];
   if (m_selected == tree.root || !winui_GetYorN("Delete the selected object?"))
      return;
   AutoAppIPtr_(ObjectSystem, objects);
   AutoAppIPtr_(TraitManager, traits);
   if (!objects || !traits || !objects->Exists(m_selected))
      return;

   const ObjID obj = m_selected;
   const auto result = tree.desc.func.del ? tree.desc.func.del(obj) : kObjTreeNormal;
   if ((result & kObjTreeNormal) && !(result & kObjTreeCancel))
   {
      const ObjID parent = traits->GetArchetype(obj);
      std::vector<ObjID> children;
      {
         cAutoIPtr<IObjectQuery> query(traits->Query(obj, kTraitQueryDescendents));
         for (; query && !query->Done(); query->Next())
         {
            const ObjID child = query->Object();
            if (traits->GetArchetype(child) == obj)
               children.push_back(child);
         }
      }
      for (ObjID child : children)
         traits->SetArchetype(child, parent);
      objects->Destroy(obj);
      m_selected = parent ? parent : tree.root;
   }
   FinishAction(result);
}

void Hierarchy::AfterFrame()
{
   const auto action = m_action;
   m_action = TreeAction::None;
   if (action == TreeAction::None || m_trees.empty())
      return;
   m_message = nullptr;
   switch (action)
   {
   case TreeAction::Add:
      AddNode();
      break;
   case TreeAction::Delete:
      DeleteNode();
      break;
   case TreeAction::Properties:
   {
      cAutoIPtr<IObjEditor> editor(ConstructObjEditor(nullptr, m_selected));
      if (editor)
         editor->Go(kObjEdModal);
      m_children.clear();
      break;
   }
   case TreeAction::Create:
      FinishAction(m_trees[m_tree].desc.func.create(m_selected));
      break;
   case TreeAction::None:
      break;
   }
}
}

void LGAPI DoHierarchy(const sObjTreeEditorDesc *desc, ObjID focus)
{
   if (desc)
   {
      Hierarchy dialog(*desc, focus);
      dialog.Run();
   }
}
