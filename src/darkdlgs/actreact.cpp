#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include <appagg.h>
#include <aredit.h>
#include <iobjsys.h>
#include <linkbase.h>
#include <linkman.h>
#include <lnkquery.h>
#include <objedit.h>
#include <objquery.h>
#include <pg8rbase.h>
#include <propag8n.h>
#include <propag8r.h>
#include <reaction.h>
#include <receptro.h>
#include <relation.h>
#include <sdesbase.h>
#include <sdestool.h>
#include <ssrcbase.h>
#include <stimguid.h>
#include <stimuli.h>
#include <stimsens.h>
#include <stimsrc.h>

#include "darkdlgs.h"
#include "modal.h"
#include <isdescst.h>

namespace
{
template<class ID>
struct Choice
{
   ID id;
   std::string name;
};

template<class ID>
bool Pick(const char *label, const std::vector<Choice<ID>> &options, ID &value)
{
   const char *preview = "Select...";
   for (const auto &option : options)
      if (option.id == value)
         preview = option.name.c_str();
   bool changed = false;
   if (ImGui::BeginCombo(label, preview))
   {
      for (const auto &option : options)
      {
         ImGui::PushID(static_cast<int>(option.id));
         if (ImGui::Selectable(option.name.c_str(), option.id == value))
         {
            changed = value != option.id;
            value = option.id;
         }
         ImGui::PopID();
      }
      ImGui::EndCombo();
   }
   return changed;
}

std::vector<Choice<StimID>> StimulusChoices()
{
   AutoAppIPtr_(Stimuli, stimuli);
   std::vector<Choice<StimID>> result;
   if (stimuli)
   {
      const StimID root = stimuli->GetRootStimulus();
      cAutoIPtr<IObjectQuery> query(stimuli->QueryHeirs(root));
      for (; query && !query->Done(); query->Next())
      {
         const StimID id = query->Object();
         if (id != root && stimuli->IsStimulus(id))
            result.push_back({id, ObjEditName(id)});
      }
   }
   return result;
}

bool OpenParameters(const char *title, const sStructDesc *structure, void *data, size_t capacity)
{
   if (!structure || structure->size > capacity)
      return false;
   sStructEditorDesc desc{"", kStructEditNoApplyButton};
   std::snprintf(desc.title, sizeof(desc.title), "%s", title);
   cAutoIPtr<IStructEditor> editor(ConstructStructEditor(&desc, structure, data));
   return editor && editor->Go(kStructEdModal);
}

class StimulusDialog : public DarkDlgsModal
{
protected:
   StimulusDialog(const char *title, sObjStimPair pair, bool readOnly, bool lockPair)
      : DarkDlgsModal(title, ImVec2(500, 315)), m_pair(pair), m_readOnly(readOnly),
        m_lockPair(lockPair), m_stimuli(StimulusChoices())
   {
      if (pair.obj)
         std::snprintf(m_object, sizeof(m_object), "%d", pair.obj);
   }

   bool DrawPair()
   {
      ImGui::BeginDisabled(m_readOnly || m_lockPair);
      ImGui::InputText("Object", m_object, sizeof(m_object));
      const bool changed = Pick("Stimulus", m_stimuli, m_pair.stim);
      ImGui::EndDisabled();
      return changed;
   }

   bool ValidatePair()
   {
      AutoAppIPtr_(ObjectSystem, objects);
      AutoAppIPtr_(Stimuli, stimuli);
      const ObjID object = EditGetObjNamed(m_object);
      if (!objects || !stimuli || !object || !objects->Exists(object) ||
          !m_pair.stim || !stimuli->IsStimulus(m_pair.stim))
         return false;
      m_pair.obj = object;
      return true;
   }

   void DrawButtons(bool valid)
   {
      if (ImGui::Button(m_readOnly ? "Done" : "OK"))
      {
         if (m_readOnly || (valid && ValidatePair()))
         {
            m_accepted = true;
            Close();
         }
         else
            m_error = true;
      }
      if (!m_readOnly)
      {
         ImGui::SameLine();
         if (ImGui::Button("Cancel"))
            Close();
      }
      if (ImGui::IsKeyPressed(ImGuiKey_Escape, false))
         Close();
      if (m_error)
         ImGui::TextUnformatted("Check the object, stimulus, and parameters.");
   }

   sObjStimPair m_pair;
   bool m_readOnly;
   bool m_accepted = false;

private:
   bool m_lockPair;
   bool m_error = false;
   char m_object[128]{};
   std::vector<Choice<StimID>> m_stimuli;
};

class SourceDialog final : public StimulusDialog
{
public:
   SourceDialog(sObjStimPair pair, const sStimSourceDesc &desc, bool readOnly)
      : StimulusDialog("Act/React Source###DarkDlgsSource", pair, readOnly, false), m_desc(desc)
   {
      RefreshPropagators();
   }

   HRESULT Edit(sObjStimPair &pair, sStimSourceDesc &desc)
   {
      if (!Run() || !m_accepted)
         return S_FALSE;
      if (!m_readOnly)
      {
         pair = m_pair;
         desc = m_desc;
      }
      return S_OK;
   }

private:
   enum class Parameters { None, Shape, Life };
   void RefreshPropagators()
   {
      AutoAppIPtr_(Propagation, propagation);
      m_propagators.clear();
      if (!propagation)
         return;
      cAutoIPtr<IPropagatorQuery> query(propagation->GetPropagators(m_pair.stim));
      if (query)
         query->Start();
      bool currentAvailable = false;
      for (; query && !query->Done(); query->Next())
      {
         const auto id = query->ID();
         if (id == PGATOR_NULL)
            continue;
         cAutoIPtr<IPropagator> propagator(query->Propagator());
         if (propagator)
         {
            m_propagators.push_back({id, propagator->Describe()->name});
            currentAvailable |= id == m_desc.propagator;
         }
      }
      if (!currentAvailable)
         m_desc.propagator = PGATOR_NULL;
   }

   void DrawContents() override
   {
      if (DrawPair())
         RefreshPropagators();
      ImGui::BeginDisabled(m_readOnly);
      if (Pick("Propagator", m_propagators, m_desc.propagator))
      {
         AutoAppIPtr_(Propagation, propagation);
         cAutoIPtr<IPropagator> propagator(propagation ? propagation->GetPropagator(m_desc.propagator) : nullptr);
         const float previousLevel = m_desc.level;
         const auto id = m_desc.propagator;
         m_desc = {};
         m_desc.propagator = id;
         if (propagator)
            propagator->InitSource(&m_desc);
         m_desc.level = previousLevel;
      }
      ImGui::InputFloat("Intensity", &m_desc.level, 0, 0, "%.7g");
      if ((m_desc.valid_fields & kStimSrcShapeValid) && ImGui::Button("Shape"))
         m_parameters = Parameters::Shape;
      if (m_desc.valid_fields & kStimSrcLifeValid)
      {
         if (m_desc.valid_fields & kStimSrcShapeValid)
            ImGui::SameLine();
         if (ImGui::Button("Life Cycle"))
            m_parameters = Parameters::Life;
      }
      ImGui::EndDisabled();
      DrawButtons(m_desc.propagator != PGATOR_NULL && std::isfinite(m_desc.level));
   }

   void AfterFrame() override
   {
      const auto parameters = m_parameters;
      m_parameters = Parameters::None;
      if (parameters == Parameters::None)
         return;
      AutoAppIPtr_(Propagation, propagation);
      cAutoIPtr<IPropagator> propagator(propagation ? propagation->GetPropagator(m_desc.propagator) : nullptr);
      if (!propagator)
         return;
      if (parameters == Parameters::Shape)
         OpenParameters("Shape Data", propagator->DescribeShapes(), &m_desc.shape, sizeof(m_desc.shape));
      else
         OpenParameters("Life Cycle", propagator->DescribeLifeCycles(), &m_desc.life, sizeof(m_desc.life));
   }

   sStimSourceDesc m_desc;
   std::vector<Choice<PropagatorID>> m_propagators;
   Parameters m_parameters = Parameters::None;
};

class ReceptronDialog final : public StimulusDialog
{
public:
   ReceptronDialog(sObjStimPair pair, const sReceptron &tron, bool readOnly, bool lockPair = false)
      : StimulusDialog("Act/React Receptron###DarkDlgsReceptron", pair, readOnly, lockPair), m_value(tron)
   {
      AutoAppIPtr_(Reactions, reactions);
      if (reactions)
      {
         cAutoIPtr<IReactionQuery> query(reactions->QueryAll());
         for (; query && !query->Done(); query->Next())
         {
            if (const auto *item = query->Reaction())
               m_reactions.push_back({query->ID(), item->friendly_name[0] ? item->friendly_name : item->name});
         }
      }
      for (int i = 0; i < kReactNumObjs; ++i)
         if (m_value.effect.param.obj[i])
            std::snprintf(m_objects[i], sizeof(m_objects[i]), "%d", m_value.effect.param.obj[i]);
   }

   HRESULT Edit(sObjStimPair &pair, sReceptron &tron)
   {
      if (!Run() || !m_accepted)
         return S_FALSE;
      if (!m_readOnly)
      {
         pair = m_pair;
         tron = m_value;
      }
      return S_OK;
   }

private:
   void DrawContents() override
   {
      DrawPair();
      ImGui::BeginDisabled(m_readOnly);
      bool min = !(m_value.trigger.flags & kStimTrigNoMin);
      bool max = !(m_value.trigger.flags & kStimTrigNoMax);
      bool active = !(m_value.trigger.flags & kStimTrigInactive);
      if (ImGui::Checkbox("Minimum", &min))
         m_value.trigger.flags ^= kStimTrigNoMin;
      ImGui::SameLine();
      ImGui::BeginDisabled(!min);
      ImGui::InputFloat("##min", &m_value.trigger.min);
      ImGui::EndDisabled();
      if (ImGui::Checkbox("Maximum", &max))
         m_value.trigger.flags ^= kStimTrigNoMax;
      ImGui::SameLine();
      ImGui::BeginDisabled(!max);
      ImGui::InputFloat("##max", &m_value.trigger.max);
      ImGui::EndDisabled();
      if (ImGui::Checkbox("Active", &active))
         m_value.trigger.flags ^= kStimTrigInactive;
      if (Pick("Reaction", m_reactions, m_value.effect.kind))
      {
         m_value.effect.param = {};
         for (auto &object : m_objects)
            object[0] = '\0';
      }
      AutoAppIPtr_(Reactions, reactions);
      const auto *desc = reactions ? reactions->DescribeReaction(m_value.effect.kind) : nullptr;
      bool valid = desc && (!min || std::isfinite(m_value.trigger.min)) &&
         (!max || std::isfinite(m_value.trigger.max)) &&
         (!min || !max || m_value.trigger.min <= m_value.trigger.max);
      if (desc)
      {
         valid &= ObjectParameter("Direct Object", kReactDirectObj, desc->flags & kReactionHasDirectObj);
         valid &= ObjectParameter("Indirect Object", kReactIndirectObj, desc->flags & kReactionHasIndirectObj);
         AutoAppIPtr_(StructDescTools, types);
         if (types && types->Lookup(desc->param_type.name) && ImGui::Button("Reaction Parameters"))
            m_parameters = true;
      }
      ImGui::EndDisabled();
      DrawButtons(valid);
   }

   bool ObjectParameter(const char *label, int index, bool present)
   {
      if (!present)
         return true;
      ImGui::InputText(label, m_objects[index], sizeof(m_objects[index]));
      const ObjID object = EditGetObjNamed(m_objects[index]);
      AutoAppIPtr_(ObjectSystem, objects);
      if (m_objects[index][0] && (!object || !objects || !objects->Exists(object)))
         return false;
      m_value.effect.param.obj[index] = object;
      return true;
   }

   void AfterFrame() override
   {
      if (!m_parameters)
         return;
      m_parameters = false;
      AutoAppIPtr_(Reactions, reactions);
      AutoAppIPtr_(StructDescTools, types);
      const auto *desc = reactions ? reactions->DescribeReaction(m_value.effect.kind) : nullptr;
      if (desc && types)
         OpenParameters("Reaction Parameters", types->Lookup(desc->param_type.name), &m_value.effect.param.data,
                        sizeof(m_value.effect.param.data));
   }

   sReceptron m_value;
   char m_objects[kReactNumObjs][128]{};
   std::vector<Choice<ReactionID>> m_reactions;
   bool m_parameters = false;
};

enum class ListAction { None, Add, Edit, Delete };

class StimulusList final : public DarkDlgsModal
{
public:
   StimulusList(sObjStimPair pattern, bool sources, bool readOnly)
      : DarkDlgsModal(sources ? "Act/React Sources###DarkDlgsSources" :
         "Act/React Receptrons###DarkDlgsReceptrons", ImVec2(680, 430)),
        m_pattern(pattern), m_sources(sources), m_readOnly(readOnly)
   {
      Refresh();
   }

private:
   struct Row
   {
      ulong id;
      std::string label;
   };

   void Refresh()
   {
      m_rows.clear();
      AutoAppIPtr_(Reactions, reactions);
      if (m_sources)
      {
         AutoAppIPtr_(LinkManager, links);
         AutoAppIPtr_(Propagation, propagation);
         cAutoIPtr<IRelation> relation(links ? links->GetRelationNamed("arSrcDesc") : nullptr);
         if (!relation || !relation->GetID())
            return;
         cAutoIPtr<ILinkQuery> query(relation->Query(m_pattern.obj, m_pattern.stim));
         for (; query && !query->Done(); query->Next())
         {
            sLink link{};
            const auto *data = static_cast<const sStimSourceDesc *>(query->Data());
            if (query->Link(&link) != S_OK || !data)
               continue;
            cAutoIPtr<IPropagator> propagator(propagation ? propagation->GetPropagator(data->propagator) : nullptr);
            AddRow(query->ID(), {link.source, link.dest}, propagator ? propagator->Describe()->name : "");
         }
      }
      else
      {
         AutoAppIPtr_(StimSensors, sensors);
         cAutoIPtr<IReceptronQuery> query(sensors ? sensors->QueryReceptrons(m_pattern.obj, m_pattern.stim) : nullptr);
         for (; query && !query->Done(); query->Next())
         {
            const auto *data = query->Receptron();
            const auto *desc = data && reactions ? reactions->DescribeReaction(data->effect.kind) : nullptr;
            AddRow(query->ID(), query->Elems(), desc ? desc->friendly_name : "");
         }
      }
   }

   void AddRow(ulong id, sObjStimPair pair, const char *type)
   {
      std::string name = ObjEditName(pair.obj);
      name += " / ";
      name += ObjEditName(pair.stim);
      if (type && *type)
      {
         name += " / ";
         name += type;
      }
      m_rows.push_back({id, std::move(name)});
   }

   void DrawContents() override
   {
      if (ImGui::BeginChild("Items", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), ImGuiChildFlags_Borders))
      {
         if (m_rows.empty())
            ImGui::TextUnformatted(m_sources ? "No matching sources." : "No matching receptrons.");
         ImGuiListClipper clipper;
         clipper.Begin(static_cast<int>(m_rows.size()));
         while (clipper.Step())
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i)
            {
               const auto &row = m_rows[i];
               ImGui::PushID(i);
               if (ImGui::Selectable(row.label.c_str(), m_selected == row.id, ImGuiSelectableFlags_AllowDoubleClick))
               {
                  m_selected = row.id;
                  if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                     m_action = ListAction::Edit;
               }
               ImGui::PopID();
            }
      }
      ImGui::EndChild();
      if (!m_readOnly)
      {
         if (ImGui::Button("Add"))
            m_action = ListAction::Add;
         ImGui::SameLine();
      }
      ImGui::BeginDisabled(!m_selected);
      if (ImGui::Button(m_readOnly ? "View" : "Edit"))
         m_action = ListAction::Edit;
      if (!m_readOnly)
      {
         ImGui::SameLine();
         if (ImGui::Button("Delete"))
            m_action = ListAction::Delete;
      }
      ImGui::EndDisabled();
      ImGui::SameLine();
      if (ImGui::Button("Done") || ImGui::IsKeyPressed(ImGuiKey_Escape, false))
         Close();
   }

   void AfterFrame() override
   {
      const auto action = m_action;
      m_action = ListAction::None;
      if (action == ListAction::None)
         return;
      if (m_sources)
         EditSource(action);
      else
         EditReceptron(action);
      Refresh();
   }

   void EditSource(ListAction action)
   {
      AutoAppIPtr_(StimSources, sources);
      if (!sources)
         return;
      if (action == ListAction::Delete)
      {
         sources->RemoveSource(m_selected);
         m_selected = SRCID_NULL;
         return;
      }

      auto pair = m_pattern;
      sStimSourceDesc data{};
      const bool existing = action == ListAction::Edit;
      if (existing)
      {
         if (sources->DescribeSource(m_selected, &data) != S_OK)
            return;
         pair = sources->GetSourceElems(m_selected);
      }
      SourceDialog dialog(pair, data, m_readOnly);
      if (dialog.Edit(pair, data) != S_OK || m_readOnly)
         return;
      const auto added = sources->AddSource(pair.obj, pair.stim, &data);
      if (added != SRCID_NULL)
      {
         if (existing)
            sources->RemoveSource(m_selected);
         m_selected = added;
      }
   }

   void EditReceptron(ListAction action)
   {
      AutoAppIPtr_(StimSensors, sensors);
      if (!sensors)
         return;
      if (action == ListAction::Delete)
      {
         sensors->RemoveReceptron(m_selected);
         m_selected = RECEPTRON_NULL;
         return;
      }

      auto pair = m_pattern;
      sReceptron data{};
      data.effect.kind = REACTION_NULL;
      data.trigger.flags = kStimTrigNoMin | kStimTrigNoMax;
      const bool existing = action == ListAction::Edit;
      if (existing)
      {
         if (sensors->GetReceptron(m_selected, &data) != S_OK)
            return;
         pair = sensors->GetReceptronElems(m_selected);
      }
      ReceptronDialog dialog(pair, data, m_readOnly, existing);
      if (dialog.Edit(pair, data) != S_OK || m_readOnly)
         return;
      if (existing)
         sensors->SetReceptron(m_selected, &data);
      else
         m_selected = sensors->AddReceptron(pair.obj, pair.stim, &data);
   }

   sObjStimPair m_pattern;
   bool m_sources;
   bool m_readOnly;
   std::vector<Row> m_rows;
   ulong m_selected = 0;
   ListAction m_action = ListAction::None;
};

class ActReactEditors final : public cCTUnaggregated<IActReactEditors, &IID_IActReactEditors, kCTU_Default>
{
public:
   STDMETHOD(SourceDialog)(eARDlgFlags flags, sObjStimPair *pair, sStimSourceDesc *data) override
   {
      if (!pair || !data)
         return E_INVALIDARG;
      ::SourceDialog dialog(*pair, *data, flags & kARDlgReadOnly);
      return dialog.Edit(*pair, *data);
   }

   STDMETHOD(ReceptronDialog)(eARDlgFlags flags, sObjStimPair *pair, sReceptron *data) override
   {
      if (!pair || !data)
         return E_INVALIDARG;
      sReceptron draft = *data;
      if (!pair->stim && !draft.order && !draft.effect.kind)
      {
         draft.effect.kind = REACTION_NULL;
         draft.trigger.flags |= kStimTrigNoMin | kStimTrigNoMax;
      }
      ::ReceptronDialog dialog(*pair, draft, flags & kARDlgReadOnly);
      if (dialog.Edit(*pair, draft) != S_OK)
         return S_FALSE;
      if (!(flags & kARDlgReadOnly))
         *data = draft;
      return S_OK;
   }

   STDMETHOD(SourceList)(eARDlgFlags flags, const sObjStimPair *pair) override
   {
      StimulusList dialog(pair ? *pair : sObjStimPair{}, true, flags & kARDlgReadOnly);
      return dialog.Run() ? S_OK : S_FALSE;
   }

   STDMETHOD(ReceptronList)(eARDlgFlags flags, const sObjStimPair *pair) override
   {
      StimulusList dialog(pair ? *pair : sObjStimPair{}, false, flags & kARDlgReadOnly);
      return dialog.Run() ? S_OK : S_FALSE;
   }
};
}

IActReactEditors *LGAPI ExportActReactEditors()
{
   return new ActReactEditors;
}
