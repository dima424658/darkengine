#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <string>
#include <type_traits>
#include <vector>

#include <sdesbase.h>
#include <sdesguid.h>

#include "darkdlgs.h"
#include "h2ocolor.h"
#include "modal.h"
#include <isdescst.h>

namespace
{
std::string StructTitle(const sStructEditorDesc &editor, const sStructDesc &structure)
{
   const char *name = editor.title[0] ? editor.title : structure.name;
   const size_t limit = editor.title[0] ? sizeof(editor.title) : sizeof(structure.name);
   return std::string(name, std::find(name, name + limit, '\0')) + "###DarkDlgsStructEditor";
}

bool VisibleField(const sFieldDesc &field)
{
   return !(field.flags & (kFieldFlagInvisible | kFieldFlagIrrelevant));
}

float FieldRowHeight()
{
   return ImGui::GetFrameHeight() + 2.0f * ImGui::GetStyle().CellPadding.y;
}

template<class T, int Count = 1>
void EditNumbers(char *data, const sFieldDesc &field, ImGuiDataType type, const char *format = nullptr,
                 ImGuiInputTextFlags flags = 0)
{
   T values[Count];
   std::memcpy(values, data, sizeof(values));
   const bool changed = Count == 1
      ? ImGui::InputScalar("##value", type, values, nullptr, nullptr, format, flags)
      : ImGui::InputScalarN("##value", type, values, Count, nullptr, nullptr, format, flags);
   if (!changed)
      return;

   for (T &value : values)
   {
      if constexpr (std::is_floating_point_v<T>)
         if (!std::isfinite(value))
            return;
      if ((field.flags & kFieldFlagUIRanged) && field.min <= field.max)
      {
         const long double low = std::max<long double>(field.min,
            static_cast<long double>(std::numeric_limits<T>::lowest()));
         const long double high = std::min<long double>(field.max,
            static_cast<long double>(std::numeric_limits<T>::max()));
         if (low <= high)
            value = static_cast<T>(std::clamp<long double>(value, low, high));
      }
   }
   std::memcpy(data, values, sizeof(values));
}

template<class T>
void EditIntegral(char *data, const sFieldDesc &field, ImGuiDataType type)
{
   if (field.type == kFieldTypeBool)
   {
      T value;
      std::memcpy(&value, data, sizeof(value));
      bool checked = value != 0;
      if (ImGui::Checkbox("##value", &checked))
      {
         value = checked ? 1 : 0;
         std::memcpy(data, &value, sizeof(value));
      }
      return;
   }

   const auto *names = static_cast<const char *const *>(field.data);
   if (field.type == kFieldTypeEnum && names && field.datasize)
   {
      T value;
      std::memcpy(&value, data, sizeof(value));
      const long double index = static_cast<long double>(value) - field.min;
      const char *selected = index >= 0 && index < field.datasize
         ? names[static_cast<size_t>(index)] : nullptr;
      const std::string number = selected ? std::string() : std::to_string(value);
      if (ImGui::BeginCombo("##value", selected ? selected : number.c_str()))
      {
         for (size_t i = 0; i < field.datasize; ++i)
         {
            const long double option = static_cast<long double>(field.min) + i;
            if (!names[i] || option < static_cast<long double>(std::numeric_limits<T>::lowest()) ||
                option > static_cast<long double>(std::numeric_limits<T>::max()))
               continue;
            ImGui::PushID(static_cast<int>(i));
            if (ImGui::Selectable(names[i], index == i))
            {
               value = static_cast<T>(option);
               std::memcpy(data, &value, sizeof(value));
            }
            ImGui::PopID();
         }
         ImGui::EndCombo();
      }
      return;
   }

   if (field.type == kFieldTypeBits && names && field.datasize)
   {
      using U = std::make_unsigned_t<T>;
      U value;
      std::memcpy(&value, data, sizeof(value));
      const auto firstBit = static_cast<unsigned>(field.min) & 0xff;
      const std::string preview = "Flags (" + std::to_string(value) + ")";
      if (ImGui::BeginCombo("##value", preview.c_str()))
      {
         for (size_t i = 0; i < field.datasize && firstBit + i < std::numeric_limits<U>::digits; ++i)
         {
            if (!names[i])
               continue;
            const U bit = static_cast<U>(U{1} << (firstBit + i));
            ImGui::PushID(static_cast<int>(i));
            if (ImGui::Selectable(names[i], (value & bit) != 0, ImGuiSelectableFlags_NoAutoClosePopups))
            {
               value ^= bit;
               std::memcpy(data, &value, sizeof(value));
            }
            ImGui::PopID();
         }
         ImGui::EndCombo();
      }
      return;
   }

   const bool hex = (field.flags & kFieldFlagHex) || field.type == kFieldTypeBits;
   EditNumbers<T>(data, field, type, hex ? (sizeof(T) == 8 ? "%llX" : "%X") : nullptr,
                  hex ? ImGuiInputTextFlags_CharsHexadecimal : 0);
}

template<int Count, bool Angle>
bool EditScaledNumbers(char *data, const sFieldDesc &field)
{
   using Stored = std::conditional_t<Angle, ImU16, ImS32>;
   Stored stored[Count];
   if (field.size != sizeof(stored))
      return false;
   std::memcpy(stored, data, sizeof(stored));
   double values[Count];
   constexpr double scale = Angle ? 65536.0 / 360.0 : 65536.0;
   for (int i = 0; i < Count; ++i)
      values[i] = stored[Angle ? Count - 1 - i : i] / scale;

   EditNumbers<double, Count>(reinterpret_cast<char *>(values), field, ImGuiDataType_Double, "%.10g");
   for (int i = 0; i < Count; ++i)
   {
      Stored &value = stored[Angle ? Count - 1 - i : i];
      if (values[i] == value / scale)
         continue;
      if constexpr (Angle)
      {
         const double units = std::floor(std::fmod(values[i], 360.0) * scale + 0.5);
         value = static_cast<Stored>(static_cast<int>(units));
      }
      else
      {
         const double units = std::round(values[i] * scale);
         value = static_cast<Stored>(std::clamp<double>(
            units, std::numeric_limits<Stored>::min(), std::numeric_limits<Stored>::max()));
      }
   }
   std::memcpy(data, stored, sizeof(stored));
   return true;
}

bool EditField(char *data, const sFieldDesc &field)
{
   switch (field.type)
   {
   case kFieldTypeInt:
   case kFieldTypeShort:
   case kFieldTypeBool:
   case kFieldTypeEnum:
   case kFieldTypeBits:
   {
      const bool unsignedValue = (field.flags & (kFieldFlagUnsigned | kFieldFlagHex)) ||
         field.type == kFieldTypeBits;
      switch (field.size)
      {
      case 1:
         if (unsignedValue) EditIntegral<ImU8>(data, field, ImGuiDataType_U8);
         else EditIntegral<ImS8>(data, field, ImGuiDataType_S8);
         return true;
      case 2:
         if (unsignedValue) EditIntegral<ImU16>(data, field, ImGuiDataType_U16);
         else EditIntegral<ImS16>(data, field, ImGuiDataType_S16);
         return true;
      case 4:
         if (unsignedValue) EditIntegral<ImU32>(data, field, ImGuiDataType_U32);
         else EditIntegral<ImS32>(data, field, ImGuiDataType_S32);
         return true;
      case 8:
         if (unsignedValue) EditIntegral<ImU64>(data, field, ImGuiDataType_U64);
         else EditIntegral<ImS64>(data, field, ImGuiDataType_S64);
         return true;
      }
      break;
   }
   case kFieldTypeFloat:
      if (field.size != sizeof(float)) break;
      EditNumbers<float>(data, field, ImGuiDataType_Float, "%.9g");
      return true;
   case kFieldTypeDouble:
      if (field.size != sizeof(double)) break;
      EditNumbers<double>(data, field, ImGuiDataType_Double, "%.17g");
      return true;
   case kFieldTypeVector:
      if (field.size != 3 * sizeof(float)) break;
      EditNumbers<float, 3>(data, field, ImGuiDataType_Float, "%.9g");
      return true;
   case kFieldTypeDoubleVec:
      if (field.size != 3 * sizeof(double)) break;
      EditNumbers<double, 3>(data, field, ImGuiDataType_Double, "%.17g");
      return true;
   case kFieldTypeFix:
      return EditScaledNumbers<1, false>(data, field);
   case kFieldTypeFixVec:
      return EditScaledNumbers<3, false>(data, field);
   case kFieldTypeAng:
      return EditScaledNumbers<1, true>(data, field);
   case kFieldTypeAngVec:
      return EditScaledNumbers<3, true>(data, field);
   case kFieldTypeRGBA:
   {
      sRGBA color;
      if (field.size != sizeof(color))
         break;
      std::memcpy(&color, data, sizeof(color));
      float rgba[] = {color.rgb[0] / 255.0f, color.rgb[1] / 255.0f, color.rgb[2] / 255.0f, color.alpha};
      if (ImGui::ColorEdit4("##value", rgba) &&
          std::all_of(std::begin(rgba), std::end(rgba), [](float value) { return std::isfinite(value); }))
      {
         for (int i = 0; i < 3; ++i)
            color.rgb[i] = static_cast<uchar>(std::lround(std::clamp(rgba[i], 0.0f, 1.0f) * 255.0f));
         color.alpha = std::clamp(rgba[3], 0.0f, 1.0f);
         std::memcpy(data, &color, sizeof(color));
      }
      return true;
   }
   case kFieldTypeStringPtr:
   case kFieldTypeVoidPtr:
   {
      const void *pointer;
      if (field.size != sizeof(pointer))
         break;
      std::memcpy(&pointer, data, sizeof(pointer));
      ImGui::TextDisabled("%s", pointer ? "Assigned (read-only)" : "Null (read-only)");
      return true;
   }
   case kFieldTypePoint:
      if (field.size != 2 * sizeof(ImS16)) break;
      EditNumbers<ImS16, 2>(data, field, ImGuiDataType_S16);
      return true;
   case kFieldTypeString:
   {
      std::vector<char> text(data, data + field.size);
      text.back() = '\0';
      if (ImGui::InputText("##value", text.data(), text.size()))
         std::memcpy(data, text.data(), text.size());
      return true;
   }
   }
   return false;
}

class StructEditor final : public cCTUnaggregated<IStructEditor, &IID_IStructEditor, kCTU_Default>,
                           private DarkDlgsModal
{
public:
   StructEditor(const sStructEditorDesc &editor, const sStructDesc &structure, void *data)
      : DarkDlgsModal(StructTitle(editor, structure), ImVec2(500, 0)),
        m_editor(editor), m_structure(structure), m_data(data)
   {
   }

   STDMETHOD_(BOOL, Go)(eSdescModality modality) override;
   STDMETHOD(SetCallback)(StructEditCB cb, StructEditCBData data) override
   {
      m_callback = cb;
      m_callbackData = data;
      return S_OK;
   }
   STDMETHOD_(const sStructEditorDesc *, Describe)() override { return &m_editor; }
   STDMETHOD_(const sStructDesc *, DescribeStruct)() override { return &m_structure; }
   STDMETHOD_(void *, Struct)() override { return m_data; }

private:
   void SetWindowSizing(ImVec2 size, const ImGuiViewport &viewport) const override;
   void DrawContents() override;
   bool HasLabel(const sFieldDesc &field) const;
   float FooterHeight() const;
   void AfterFrame() override;
   void Notify(ulong event);
   void Finish();
   void OnModelessClosed() override;
   void Commit();

   sStructEditorDesc m_editor;
   const sStructDesc &m_structure;
   void *m_data;
   std::vector<char> m_draft;
   StructEditCB m_callback = nullptr;
   StructEditCBData m_callbackData = nullptr;
   bool m_accepted = false;
   bool m_apply = false;
};

bool StructEditor::HasLabel(const sFieldDesc &field) const
{
   return field.name[0] && !(m_editor.flags & kStructEditNoFieldNames);
}

float StructEditor::FooterHeight() const
{
   const auto flags = m_editor.flags;
   const bool hasButtons = !(flags & kStructEditNoOKButton) || !(flags & kStructEditNoCancelButton) ||
      (m_callback && !(flags & kStructEditNoApplyButton));
   return hasButtons ? ImGui::GetFrameHeightWithSpacing() : 0.0f;
}

void StructEditor::SetWindowSizing(ImVec2 size, const ImGuiViewport &viewport) const
{
   const auto &style = ImGui::GetStyle();
   const bool detached = (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) != 0;
   const float titleHeight = detached ? 0.0f : ImGui::GetFrameHeight();
   const float padding = 4.0f * style.WindowPadding.y + 2.0f * style.ChildBorderSize;
   const float minimum = titleHeight + padding + FieldRowHeight() + FooterHeight();
   if (FirstFrame())
   {
      float height = titleHeight + padding + FooterHeight();
      int columns = 0;
      for (int i = 0; i < m_structure.nfields; ++i)
      {
         const auto &field = m_structure.fields[i];
         if (!VisibleField(field))
            continue;
         const int rowColumns = HasLabel(field) ? 2 : 1;
         if (columns && columns != rowColumns)
            height += style.ItemSpacing.y;
         columns = rowColumns;
         height += FieldRowHeight();
      }
      const float maximum = std::max(minimum, viewport.WorkSize.y - 2.0f * ImGui::GetFrameHeightWithSpacing());
      size.y = std::clamp(height, minimum, maximum);
      ImGui::SetNextWindowSize(size);
   }
   constexpr float unconstrained = std::numeric_limits<float>::max();
   ImGui::SetNextWindowSizeConstraints(ImVec2(240, minimum), ImVec2(unconstrained, unconstrained));
}

void StructEditor::DrawContents()
{
   const float footer = FooterHeight();
   if (FirstFrame())
      ImGui::SetNextWindowScroll(ImVec2(0, 0));
   if (ImGui::BeginChild("Fields", ImVec2(0, -footer), ImGuiChildFlags_Borders))
   {
      int columns = 0;
      bool tableOpen = false;
      for (int i = 0; i < m_structure.nfields; ++i)
      {
         const sFieldDesc &field = m_structure.fields[i];
         if (!VisibleField(field))
            continue;

         const bool hasLabel = HasLabel(field);
         const int rowColumns = hasLabel ? 2 : 1;
         if (columns != rowColumns)
         {
            if (tableOpen)
               ImGui::EndTable();
            columns = rowColumns;
            const std::string tableId = "Values##" + std::to_string(i);
            tableOpen = ImGui::BeginTable(tableId.c_str(), columns, ImGuiTableFlags_RowBg);
            if (tableOpen)
            {
               if (hasLabel)
                  ImGui::TableSetupColumn("Field", ImGuiTableColumnFlags_WidthStretch, 0.55f);
               ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 0.45f);
            }
         }
         if (!tableOpen)
            continue;

         ImGui::TableNextRow(ImGuiTableRowFlags_None, FieldRowHeight());
         ImGui::TableNextColumn();
         if (hasLabel)
         {
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(field.name, std::find(field.name, field.name + sizeof(field.name), '\0'));
            ImGui::TableNextColumn();
         }
         ImGui::PushID(i);
         ImGui::SetNextItemWidth(-1.0f);
         ImGui::BeginDisabled((m_structure.flags & kStructFlagNotEdit) || (field.flags & kFieldFlagNotEdit));
         if (!field.size || field.offset > m_draft.size() || field.size > m_draft.size() - field.offset ||
             !EditField(m_draft.data() + field.offset, field))
            ImGui::TextDisabled("Unsupported field");
         ImGui::EndDisabled();
         ImGui::PopID();
      }
      if (tableOpen)
         ImGui::EndTable();
   }
   ImGui::EndChild();

   const bool showOK = !(m_editor.flags & kStructEditNoOKButton);
   const bool showApply = m_callback && !(m_editor.flags & kStructEditNoApplyButton);
   const bool showCancel = !(m_editor.flags & kStructEditNoCancelButton);
   bool accept = showOK && ImGui::Button("OK");
   if (showApply)
   {
      if (showOK)
         ImGui::SameLine();
      m_apply = ImGui::Button("Apply");
   }
   if (showCancel && (showOK || showApply))
      ImGui::SameLine();
   const bool keyboard = HasKeyboardFocus();
   const bool cancel = (showCancel && ImGui::Button("Cancel")) ||
      (keyboard && ImGui::IsKeyPressed(ImGuiKey_Escape, false));
   if (showOK && keyboard && !m_apply && !ImGui::GetIO().WantTextInput)
      accept |= ImGui::IsKeyPressed(ImGuiKey_Enter, false) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, false);
   if (cancel || accept)
   {
      m_accepted = accept && !cancel;
      Close();
   }
}

void StructEditor::Notify(ulong event)
{
   if (m_callback)
   {
      sStructEditEvent info{event, this};
      m_callback(&info, m_callbackData);
   }
}

void StructEditor::Commit()
{
   std::memcpy(m_data, m_draft.data(), m_draft.size());
}

void StructEditor::AfterFrame()
{
   if (m_apply)
   {
      AddRef();
      cAutoIPtr<IStructEditor> keepAlive(this);
      m_apply = false;
      Commit();
      Notify(kStructEdApply);
      if (IsOpen())
         std::memcpy(m_draft.data(), m_data, m_draft.size());
   }
}

void StructEditor::Finish()
{
   if (m_accepted)
      Commit();
   Notify(m_accepted ? kStructEdOK : kStructEdCancel);
}

void StructEditor::OnModelessClosed()
{
   Finish();
   Release();
}

STDMETHODIMP_(BOOL) StructEditor::Go(eSdescModality modality)
{
   if (IsOpen())
      return FALSE;

   // Callbacks can release the caller's reference.
   AddRef();
   cAutoIPtr<IStructEditor> keepAlive(this);
   const char *data = static_cast<const char *>(m_data);
   m_draft.assign(data, data + m_structure.size);
   m_accepted = m_apply = false;
   if (modality == kStructEdModeless && ShowModeless())
   {
      AddRef(); // Held by the modeless window until it closes.
      return FALSE;
   }
   if (modality == kStructEdModal)
      Run();
   Finish();
   return m_accepted;
}
}

IStructEditor *LGAPI ConstructStructEditor(const sStructEditorDesc *editor,
                                          const sStructDesc *structure, void *data)
{
   if (!editor || !structure || !data || !structure->size || !structure->fields ||
       structure->nfields <= 0 || (structure->flags & kStructFlagInvisible))
      return nullptr;
   return new StructEditor(*editor, *structure, data);
}
