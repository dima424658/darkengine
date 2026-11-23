#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <type_traits>
#include <vector>

#include <imgui.h>
#include <imgui_internal.h>

#include <lg.h>
#include <2d.h>
#include <cmdterm.h>
#include <cmdbutts.h>
#include <command.h>
#include <config.h>
#include <imgui_pnp.h>
#include <imgui_workspace.h>
#include <pnptools.h>
#include <render_backend.h>
#include <uiedit.h>

namespace
{
struct Control
{
   Rect rect;
   std::optional<Rect> heightReference;
   std::function<void(Control &, ImVec2)> draw;
   const char *text = nullptr;
   const char *const *choices = nullptr;
   int choiceCount = 0;
   bool *toggle = nullptr;
   bool hidden = false;
   bool selected = false;
   bool focus = false;
   bool pictureDirty = true;
   int pictureSlot = -1;
   int owner = 0;
};

struct Panel
{
   Point origin;
   std::vector<std::shared_ptr<Control>> controls;
};

std::vector<_PnP_GadgData *> panels;
_PnP_GadgData *current;
Point nextOrigin;

constexpr int kTileSize = 128;
constexpr int kTileCount = 8;
ImTextureData previews;
std::array<const Control *, kTileCount> previewOwners{};
ImGuiContext *previewContext;

Panel &PanelData(_PnP_GadgData *panel)
{
   return *static_cast<Panel *>(panel->userData);
}

int AddControl(const Rect &rect, std::function<void(Control &, ImVec2)> draw)
{
   auto control = std::make_shared<Control>();
   control->rect = rect;
   control->draw = std::move(draw);
   auto &controls = PanelData(current).controls;
   control->owner = static_cast<int>(controls.size());
   controls.push_back(std::move(control));
   return static_cast<int>(controls.size()) - 1;
}

std::shared_ptr<Control> GetControl(int owner)
{
   if (!current || !current->userData || owner < 0)
      return {};
   const auto &controls = PanelData(current).controls;
   return static_cast<size_t>(owner) < controls.size() ? controls[owner] : nullptr;
}

void QueueControlAction(Control &control, std::function<void(Control &)> action)
{
   auto *panel = current;
   std::weak_ptr<Control> weak = GetControl(control.owner);
   QueueEditorWorkspaceAction([panel, weak, action = std::move(action)] {
      if (auto live = weak.lock())
      {
         auto *prior = current;
         current = panel;
         action(*live);
         current = prior && prior->userData ? prior : nullptr;
      }
   });
}

void PositionedText(const char *text, ImVec2 size, bool centered)
{
   const ImVec2 position = ImGui::GetCursorScreenPos();
   const ImVec2 measure = ImGui::CalcTextSize(text);
   const float x = position.x + (centered ? std::max(0.0f, (size.x - measure.x) / 2) : 1.0f);
   const float y = position.y + (size.y - measure.y) / 2;
   ImGui::GetWindowDrawList()->PushClipRect(position, ImVec2(position.x + size.x, position.y + size.y), true);
   ImGui::GetWindowDrawList()->AddText(ImVec2(x, y), ImGui::GetColorU32(ImGuiCol_Text), text);
   ImGui::GetWindowDrawList()->PopClipRect();
}

void DrawPicture(Control &control, ImVec2 size, void (*paint)(int), int data)
{
   if (!paint)
   {
      ImGui::Dummy(size);
      return;
   }

   if (previewContext != ImGui::GetCurrentContext())
   {
      previewContext = ImGui::GetCurrentContext();
      previews.Create(ImTextureFormat_RGBA32, kTileSize, kTileCount * kTileSize);
      ImGui::RegisterUserTexture(&previews);
   }
   if (control.pictureSlot < 0)
   {
      const auto freeSlot = std::find(previewOwners.begin(), previewOwners.end(), nullptr);
      if (freeSlot == previewOwners.end())
         return;
      *freeSlot = &control;
      control.pictureSlot = static_cast<int>(freeSlot - previewOwners.begin());
   }

   const int tileTop = control.pictureSlot * kTileSize;
   if (control.pictureDirty)
   {
      PaintEditorPreview(previews, 0, tileTop, kTileSize, kTileSize, [=] { paint(data); });
      control.pictureDirty = false;
   }

   const float low = static_cast<float>(control.pictureSlot) / kTileCount;
   ImGui::Image(previews.GetTexRef(), size, ImVec2(0, low), ImVec2(1, low + 1.0f / kTileCount));
}

void DrawButton(Control &control, ImVec2 size, const char *label, const std::function<void(Control &)> &action)
{
   if (control.selected)
      ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_Header));
   if (ImGui::Button(label ? label : "", size) || ImGui::IsItemClicked(ImGuiMouseButton_Right))
      QueueControlAction(control, action);
   if (control.selected)
      ImGui::PopStyleColor();
}

struct FieldLayout
{
   explicit FieldLayout(const char *label, ImVec2 size)
      : size(size)
   {
      if (label && *label)
      {
         const float width = std::min(size.x * 0.5f, ImGui::CalcTextSize(label).x + 4.0f);
         const ImVec2 position = ImGui::GetCursorScreenPos();
         PositionedText(label, ImVec2(width, size.y), false);
         ImGui::SetCursorScreenPos(ImVec2(position.x + width, position.y));
         this->size.x -= width;
      }
   }

   ImVec2 size;
};

template<class T>
T StepValue(T value, int direction, float step, T minimum, T maximum, ulong flags)
{
   const double next = static_cast<double>(value) + direction * step;
   if (next > maximum && (flags & PNP_SLIDER_WRAP))
      return minimum;
   if (next < minimum && (flags & PNP_SLIDER_WRAP))
      return maximum;
   return static_cast<T>(std::clamp(next, static_cast<double>(minimum), static_cast<double>(maximum)));
}

template<class T>
int AddSlider(Rect *space, const char *label, T minimum, T maximum, float step, T *value,
              void (*update)(PnP_SliderOp, Rect *, T, intptr_t), intptr_t data, ulong flags)
{
   return AddControl(*space, [=](Control &control, ImVec2 size) {
      FieldLayout field(label, size);
      const ImVec2 position = ImGui::GetCursorScreenPos();
      const float arrow = field.size.x >= 30.0f ? std::min(ImGui::GetFontSize(), field.size.x / 4.0f) : 0.0f;
      const float height = std::min(size.y, ImGui::GetFrameHeight());
      int direction = 0;
      if (arrow > 0)
      {
         ImGui::PushItemFlag(ImGuiItemFlags_ButtonRepeat, true);
         if (ImGui::Button("<", ImVec2(arrow, height)))
            direction = -1;
         ImGui::SetCursorScreenPos(ImVec2(position.x + arrow, position.y));
      }
      ImGui::SetNextItemWidth(std::max(1.0f, field.size.x - 2 * arrow));
      if (std::exchange(control.focus, false))
         ImGui::SetKeyboardFocusHere();

      T next = *value;
      bool changed;
      if constexpr (std::is_same_v<T, ushort>)
      {
         float degrees = static_cast<float>(next) * 360.0f / 65536.0f;
         changed = ImGui::DragFloat("##value", &degrees, step * 360.0f / 65536.0f,
                                    0.0f, 359.999f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
         next = static_cast<ushort>(degrees * 65536.0f / 360.0f);
      }
      else
      {
         constexpr ImGuiDataType type = std::is_same_v<T, float> ? ImGuiDataType_Float :
                                        std::is_same_v<T, short> ? ImGuiDataType_S16 : ImGuiDataType_S32;
         const char *format = std::is_same_v<T, float> ? "%.2f" : "%d";
         ImGuiSliderFlags options = ImGuiSliderFlags_AlwaysClamp;
         if (flags & PNP_SLIDER_WRAP)
            options |= ImGuiSliderFlags_WrapAround;
         changed = ImGui::DragScalar("##value", type, &next, step, &minimum, &maximum, format, options);
      }
      const bool click = (flags & PNP_SLIDER_CLICKS) &&
         (ImGui::IsItemClicked(ImGuiMouseButton_Right) ||
          (ImGui::IsItemDeactivated() && !ImGui::IsItemDeactivatedAfterEdit() && ImGui::IsItemHovered()));
      if (arrow > 0)
      {
         ImGui::SetCursorScreenPos(ImVec2(position.x + field.size.x - arrow, position.y));
         if (ImGui::Button(">", ImVec2(arrow, height)))
            direction = 1;
         ImGui::PopItemFlag();
      }
      if (direction)
      {
         next = StepValue(*value, direction, step, minimum, maximum, flags);
         changed = next != *value;
      }
      if (changed || click)
      {
         QueueControlAction(control, [=](Control &live) {
            *value = next;
            if (update)
               update(changed ? PnP_SliderUpdateOp : PnP_SliderClickOp, &live.rect, next, data);
         });
      }
   });
}
}

void PaintEditorPreview(ImTextureData &texture, int x, int y, int width, int height,
                        const std::function<void()> &paint)
{
   std::vector<uint16_t> pixels(width * height);
   grs_canvas canvas;
   gr_init_canvas(&canvas, reinterpret_cast<uchar *>(pixels.data()), BMT_FLAT16, width, height);
   const int format = grd_bm.flags & 0x7f00;
   canvas.bm.flags = format;
   gr_push_canvas(&canvas);
   paint();
   gr_pop_canvas();
   gr_close_canvas(&canvas);

   const bool rgb565 = format == BMF_RGB_565;
   for (int row = 0; row < height; ++row)
   {
      auto *rgba = static_cast<unsigned char *>(texture.GetPixelsAt(x, y + row));
      for (int column = 0; column < width; ++column)
      {
         const uint16_t pixel = pixels[row * width + column];
         *rgba++ = ((pixel >> (rgb565 ? 11 : 10)) & 31) * 255 / 31;
         *rgba++ = ((pixel >> 5) & (rgb565 ? 63 : 31)) * 255 / (rgb565 ? 63 : 31);
         *rgba++ = (pixel & 31) * 255 / 31;
         *rgba++ = 255;
      }
   }
   ImTextureDataQueueUpload(&texture, x, y, width, height);
}

void ResetEditorPanelRenderer()
{
   if (previewContext)
   {
      previews.DestroyPixels();
      previews.SetStatus(ImTextureStatus_Destroyed);
      previews.RefCount = 0;
      previewContext = nullptr;
   }
   for (auto *panel : panels)
      for (auto &control : PanelData(panel).controls)
         control->pictureDirty = true;
}

namespace
{
Rect ControlClientRect(Rect rect, Point origin)
{
   RECT_MOVE(&rect, origin);
   rect = EditorMapBrushPanelRect(rect);
   int left = rect.ul.x, top = rect.ul.y;
   int right = rect.lr.x, bottom = rect.lr.y;
   RenderBackendLogicalToClientPoint(&left, &top);
   RenderBackendLogicalToClientPoint(&right, &bottom);
   return {MakePoint(left, top), MakePoint(right, bottom)};
}

float PanelFontSize(float baseSize)
{
   // Use the same type size for the brush controls and texture alignment.
   // The smallest visible control determines how much text fits.
   float size = baseSize;
   for (auto *panel : panels)
   {
      const auto &state = PanelData(panel);
      for (const auto &control : state.controls)
      {
         if (control->hidden)
            continue;
         const Rect rect = ControlClientRect(control->heightReference.value_or(control->rect), state.origin);
         if (RectHeight(&rect) > 0)
            size = std::min(size, static_cast<float>(RectHeight(&rect) - 1));
      }
   }
   return std::max(7.0f, size);
}
}

void DrawEditorPanels()
{
   auto *prior = current;
   const ImVec2 viewport = ImGui::GetMainViewport()->Pos;
   const float baseSize = ImGui::GetStyle().FontSizeBase;
   const float fontSize = PanelFontSize(baseSize);
   ImGui::PushFont(nullptr, fontSize);
   for (auto *panel : panels)
   {
      auto &state = PanelData(panel);
      current = panel;
      ImGui::PushID(panel);
      for (size_t index = 0; index < state.controls.size(); ++index)
      {
         auto &control = *state.controls[index];
         if (control.hidden)
            continue;
         Rect rect = ControlClientRect(control.rect, state.origin);
         if (control.heightReference)
         {
            const Rect reference = ControlClientRect(*control.heightReference, state.origin);
            rect.lr.y = rect.ul.y + RectHeight(&reference);
         }
         if (RectWidth(&rect) <= 0 || RectHeight(&rect) <= 0)
            continue;
         ImGui::PushID(static_cast<int>(index));
         ImGui::SetCursorScreenPos(ImVec2(viewport.x + rect.ul.x, viewport.y + rect.ul.y));
         const float padding = std::max(0.0f, (RectHeight(&rect) - ImGui::GetFontSize()) / 2.0f);
         ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, padding));
         control.draw(control, ImVec2(RectWidth(&rect), RectHeight(&rect)));
         ImGui::PopStyleVar();
         ImGui::PopID();
      }
      ImGui::PopID();
   }
   ImGui::PopFont();
   current = prior;
}

void PnP_SetOrigin(Point origin)
{
   nextOrigin = origin;
}

void PnP_SetCurrent(_PnP_GadgData *panel) { current = panel; }

void PnP_GadgStart(_PnP_GadgData *panel)
{
   if (panel->userData)
      PnP_GadgFree(panel);
   panel->userData = new Panel{nextOrigin, {}};
   panels.push_back(panel);
   current = panel;
}

void PnP_GadgFree(_PnP_GadgData *panel)
{
   if (!panel->userData)
      return;
   for (const auto &control : PanelData(panel).controls)
      if (control->pictureSlot >= 0)
         previewOwners[control->pictureSlot] = nullptr;
   delete &PanelData(panel);
   panel->userData = nullptr;
   panels.erase(std::remove(panels.begin(), panels.end(), panel), panels.end());
   if (current == panel)
      current = nullptr;
}

void PnP_Redraw(int owner, void *data)
{
   if (auto control = GetControl(owner))
   {
      control->pictureDirty = true;
      if (data && control->text)
         control->text = static_cast<const char *>(data);
      else if (data && control->toggle)
         control->toggle = static_cast<bool *>(data);
   }
}

void PnP_FullRedraw(_PnP_GadgData *panel)
{
   if (panel->userData)
      for (auto &control : PanelData(panel).controls)
         control->pictureDirty = true;
}

void PnP_Hide(int owner, bool hide)
{
   if (auto control = GetControl(owner))
      control->hidden = hide;
}

void PnP_FocusControl(int owner)
{
   if (auto control = GetControl(owner))
      control->focus = true;
}

void PnP_MatchControlHeight(int owner, const Rect *reference)
{
   if (auto control = GetControl(owner))
      control->heightReference = *reference;
}

int PnP_PictureBox(Rect *space, void (*paint)(int), int data)
{
   return AddControl(*space, [=](Control &control, ImVec2 size) {
      DrawPicture(control, size, paint, data);
   });
}

int PnP_TextBox(Rect *space, char *text)
{
   const int owner = AddControl(*space, [](Control &control, ImVec2 size) {
      PositionedText(control.text ? control.text : "", size, true);
      ImGui::Dummy(size);
   });
   GetControl(owner)->text = text;
   return owner;
}

int PnP_ButtonToggle(Rect *space, char *off, char *on, bool *value,
                     void (*update)(Rect *, bool, int), int data)
{
   const int owner = AddControl(*space, [=](Control &control, ImVec2 size) {
      control.selected = *control.toggle;
      DrawButton(control, size, control.selected && on ? on : off, [=](Control &live) {
         *live.toggle = !*live.toggle;
         if (update)
            update(&live.rect, *live.toggle, data);
      });
   });
   GetControl(owner)->toggle = value;
   return owner;
}

int PnP_ButtonOneShot(Rect *space, const char *label, void (*activate)(int), int data)
{
   return PnP_ChoiceButton(space, label, activate, data);
}

int PnP_ChoiceButton(Rect *space, const char *label, void (*activate)(int), int data)
{
   return AddControl(*space, [=](Control &control, ImVec2 size) {
      DrawButton(control, size, label, [=](Control &) { activate(data); });
   });
}

void PnP_HeadingButton(Rect *space, const char *label, void (*activate)(int), int data)
{
   AddControl(*space, [=](Control &control, ImVec2 size) {
      ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
      ImGui::PushStyleColor(ImGuiCol_Button, 0);
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, 0);
      ImGui::PushStyleColor(ImGuiCol_ButtonActive, 0);
      DrawButton(control, size, label, [=](Control &) { activate(data); });
      ImGui::PopStyleColor(3);
      ImGui::PopStyleVar();
   });
}

void PnP_SetChoice(int owner, BOOL selected)
{
   if (auto control = GetControl(owner))
      control->selected = selected;
}

void PnP_CommandButtons(Rect *bounds, const char *prefix, int count, Point dimensions)
{
   std::vector<Rect> positions(count);
   LayoutRectangles(bounds, positions.data(), count, dimensions, MakePoint(1, 1));
   for (int i = 0; i < count; ++i)
   {
      char key[64], label[32]{}, command[64]{};
      std::snprintf(key, sizeof(key), "%s_name_%d", prefix, i);
      config_get_raw(key, label, sizeof(label));
      std::snprintf(key, sizeof(key), "%s_cmd_%d", prefix, i);
      config_get_raw(key, command, sizeof(command));
      AddControl(positions[i], [name = std::string(label), cmd = std::string(command)](Control &control, ImVec2 size) {
         DrawButton(control, size, name.c_str(), [cmd](Control &) { cmdterm_execute_command(cmd.c_str()); });
      });
   }
}

int PnP_DragLabel(Rect *rect, const char *label, float min, float max, float step, float *value,
   void (*update)(PnP_SliderOp, Rect *, float, intptr_t), intptr_t data, ulong flags)
{
   return AddControl(*rect, [=](Control &control, ImVec2 size) {
      float next = *value;
      ImGui::SetNextItemWidth(size.x);
      ImGuiSliderFlags options = ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_NoInput;
      if (flags & PNP_SLIDER_WRAP)
         options |= ImGuiSliderFlags_WrapAround;
      if (ImGui::DragFloat("##label", &next, step, min, max, label, options))
         QueueControlAction(control, [=](Control &live) {
            *value = next;
            update(PnP_SliderUpdateOp, &live.rect, next, data);
         });
   });
}

#define EDITOR_SLIDER(Type, Value) \
   int PnP_Slider##Type(Rect *rect, const char *name, Value lo, Value hi, Value scale, Value *value, \
                        void (*update)(PnP_SliderOp, Rect *, Value, intptr_t), intptr_t data, ulong flags) \
   { return AddSlider(rect, name, lo, hi, static_cast<float>(scale), value, update, data, flags); }

EDITOR_SLIDER(Float, float)
EDITOR_SLIDER(Int, int)
EDITOR_SLIDER(Short, short)
EDITOR_SLIDER(Fixang, ushort)

int PnP_SliderString(Rect *space, const char *label, int count, const char *const *choices, int *value,
                      void (*update)(PnP_SliderOp, Rect *, int, intptr_t), intptr_t data, ulong)
{
   const int owner = AddControl(*space, [=](Control &control, ImVec2 size) {
      FieldLayout field(label, size);
      ImGui::SetNextItemWidth(std::max(1.0f, field.size.x));
      const char *selected = *value >= 0 && *value < control.choiceCount ? control.choices[*value] : "";
      if (ImGui::BeginCombo("##value", selected))
      {
         for (int i = 0; i < control.choiceCount; ++i)
         {
            ImGui::PushID(i);
            if (ImGui::Selectable(control.choices[i] ? control.choices[i] : "", *value == i))
               QueueControlAction(control, [=](Control &live) {
                  *value = i;
                  if (update)
                     update(PnP_SliderUpdateOp, &live.rect, i, data);
               });
            ImGui::PopID();
         }
         ImGui::EndCombo();
      }
   });
   PnP_StringSliderSetStrings(PnP_GetStringSubGadget(owner), choices, count);
   return owner;
}

void *PnP_GetStringSubGadget(int owner) { return GetControl(owner).get(); }

void PnP_StringSliderSetStrings(void *handle, const char *const *choices, int count)
{
   if (auto *control = static_cast<Control *>(handle))
   {
      control->choices = choices;
      control->choiceCount = count;
   }
}
