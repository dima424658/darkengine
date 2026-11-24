#include "vita_controls.hpp"

#include <algorithm>
#include <fstream>

namespace DarkVitaControls
{
namespace
{
constexpr std::uint32_t kLongPressMilliseconds = 400;
constexpr float kCornerWidth = 0.18f;
constexpr float kCornerHeight = 0.24f;

const char *ActionName(Action action)
{
   switch (action)
   {
   case Action::None: return "none";
   case Action::Block: return "block";
   case Action::LeanForward: return "lean_forward";
   case Action::UseWeapon: return "use_weapon";
   case Action::DropItem: return "drop_item";
   case Action::CycleWeapon: return "cycle_weapon";
   case Action::QuickSave: return "quick_save";
   case Action::QuickLoad: return "quick_load";
   case Action::MoveForward: return "move_forward";
   case Action::MoveBackward: return "move_backward";
   case Action::StrafeLeft: return "strafe_left";
   case Action::StrafeRight: return "strafe_right";
   case Action::LookUp: return "look_up";
   case Action::LookDown: return "look_down";
   case Action::Attack: return "attack";
   case Action::Jump: return "jump";
   case Action::Crouch: return "crouch";
   case Action::Use: return "use";
   case Action::Inventory: return "inventory";
   case Action::PreviousItem: return "previous_item";
   case Action::NextItem: return "next_item";
   case Action::LeanLeft: return "lean_left";
   case Action::LeanRight: return "lean_right";
   case Action::Pause: return "pause";
   }
   return nullptr;
}

bool ParseAction(const std::string &name, Action &action)
{
   for (int value = 0; value <= static_cast<int>(Action::Pause); ++value)
   {
      const Action candidate = static_cast<Action>(value);
      if (name == ActionName(candidate))
      {
         action = candidate;
         return true;
      }
   }
   return false;
}

const char *ButtonName(PhysicalButton button)
{
   switch (button)
   {
   case PhysicalButton::Cross: return "cross";
   case PhysicalButton::Circle: return "circle";
   case PhysicalButton::Square: return "square";
   case PhysicalButton::Triangle: return "triangle";
   case PhysicalButton::LeftShoulder: return "left_shoulder";
   case PhysicalButton::RightShoulder: return "right_shoulder";
   case PhysicalButton::Select: return "select";
   case PhysicalButton::Start: return "start";
   case PhysicalButton::LeftStick: return "left_stick";
   case PhysicalButton::RightStick: return "right_stick";
   case PhysicalButton::DpadUp: return "dpad_up";
   case PhysicalButton::DpadDown: return "dpad_down";
   case PhysicalButton::DpadLeft: return "dpad_left";
   case PhysicalButton::DpadRight: return "dpad_right";
   case PhysicalButton::Count: break;
   }
   return nullptr;
}

bool ParseButton(const std::string &name, PhysicalButton &button)
{
   for (int value = 0; value < static_cast<int>(PhysicalButton::Count); ++value)
   {
      const auto candidate = static_cast<PhysicalButton>(value);
      if (name == ButtonName(candidate))
      {
         button = candidate;
         return true;
      }
   }
   return false;
}

const char *AxisName(AnalogAxis axis)
{
   switch (axis)
   {
   case AnalogAxis::LeftX: return "left_x";
   case AnalogAxis::LeftY: return "left_y";
   case AnalogAxis::RightX: return "right_x";
   case AnalogAxis::RightY: return "right_y";
   case AnalogAxis::Count: break;
   }
   return nullptr;
}

bool ParseAxis(const std::string &name, AnalogAxis &axis)
{
   for (int value = 0; value < static_cast<int>(AnalogAxis::Count); ++value)
   {
      const auto candidate = static_cast<AnalogAxis>(value);
      if (name == AxisName(candidate))
      {
         axis = candidate;
         return true;
      }
   }
   return false;
}

const char *AxisActionName(AxisAction action)
{
   switch (action)
   {
   case AxisAction::None: return "none";
   case AxisAction::MoveHorizontal: return "move_horizontal";
   case AxisAction::MoveVertical: return "move_vertical";
   case AxisAction::LookHorizontal: return "look_horizontal";
   case AxisAction::LookVertical: return "look_vertical";
   }
   return nullptr;
}

bool ParseAxisAction(const std::string &name, AxisAction &action)
{
   for (int value = 0; value <= static_cast<int>(AxisAction::LookVertical);
        ++value)
   {
      const auto candidate = static_cast<AxisAction>(value);
      if (name == AxisActionName(candidate))
      {
         action = candidate;
         return true;
      }
   }
   return false;
}

bool ValidLayout(Layout layout)
{
   return layout == Layout::DualAnalogFps ||
          layout == Layout::RetroDpadMove || layout == Layout::Custom;
}

bool ValidButton(PhysicalButton button)
{
   return static_cast<std::size_t>(button) <
          static_cast<std::size_t>(PhysicalButton::Count);
}

bool ValidAxis(AnalogAxis axis)
{
   return static_cast<std::size_t>(axis) <
          static_cast<std::size_t>(AnalogAxis::Count);
}
}

CustomBindings::CustomBindings()
{
   m_buttons.fill(Action::None);
   m_axes.fill(AxisAction::None);
}

bool CustomBindings::SetButton(PhysicalButton button, Action action)
{
   if (!ValidButton(button) || !ActionName(action))
      return false;
   m_buttons[static_cast<std::size_t>(button)] = action;
   return true;
}

bool CustomBindings::SetAxis(AnalogAxis axis, AxisAction action)
{
   if (!ValidAxis(axis) || !AxisActionName(action))
      return false;
   m_axes[static_cast<std::size_t>(axis)] = action;
   return true;
}

Action CustomBindings::GetButton(PhysicalButton button) const
{
   return ValidButton(button) ? m_buttons[static_cast<std::size_t>(button)]
                              : Action::None;
}

AxisAction CustomBindings::GetAxis(AnalogAxis axis) const
{
   return ValidAxis(axis) ? m_axes[static_cast<std::size_t>(axis)]
                          : AxisAction::None;
}

Layout ClampLayout(int value)
{
   return static_cast<Layout>(std::clamp(value, 0, 2));
}

const char *LayoutName(Layout layout)
{
   switch (layout)
   {
   case Layout::DualAnalogFps: return "A - Dual Analog FPS";
   case Layout::RetroDpadMove: return "B - Retro D-Pad";
   case Layout::Custom: return "C - Custom";
   }
   return "C - Custom";
}

bool LayoutFromName(const std::string &name, Layout &layout)
{
   if (name == "A" || name == "dual")
      layout = Layout::DualAnalogFps;
   else if (name == "B" || name == "retro")
      layout = Layout::RetroDpadMove;
   else if (name == "C" || name == "custom")
      layout = Layout::Custom;
   else
      return false;
   return true;
}

Settings Load(const std::string &path)
{
   Settings settings;
   std::ifstream input(path);
   if (!input)
      return settings;

   std::string line;
   while (std::getline(input, line))
   {
      const std::size_t separator = line.find('=');
      if (separator == std::string::npos)
         continue;
      const std::string key = line.substr(0, separator);
      const std::string value = line.substr(separator + 1);
      if (key == "layout")
         LayoutFromName(value, settings.layout);
      else if (key.compare(0, 7, "button.") == 0)
      {
         PhysicalButton button;
         Action action;
         if (ParseButton(key.substr(7), button) &&
             ParseAction(value, action))
            settings.custom.SetButton(button, action);
      }
      else if (key.compare(0, 5, "axis.") == 0)
      {
         AnalogAxis axis;
         AxisAction action;
         if (ParseAxis(key.substr(5), axis) &&
             ParseAxisAction(value, action))
            settings.custom.SetAxis(axis, action);
      }
   }
   return settings;
}

bool Save(const std::string &path, const Settings &settings,
          std::string *error)
{
   if (path.empty())
   {
      if (error)
         *error = "invalid Vita controls settings path";
      return false;
   }
   if (!ValidLayout(settings.layout))
   {
      if (error)
         *error = "invalid Vita control layout";
      return false;
   }
   std::ofstream output(path, std::ios::trunc);
   if (!output)
   {
      if (error)
         *error = "cannot create Vita controls settings file";
      return false;
   }
   switch (settings.layout)
   {
   case Layout::DualAnalogFps: output << "layout=A\n"; break;
   case Layout::RetroDpadMove: output << "layout=B\n"; break;
   case Layout::Custom: output << "layout=C\n"; break;
   }
   for (int value = 0; value < static_cast<int>(PhysicalButton::Count); ++value)
   {
      const auto button = static_cast<PhysicalButton>(value);
      output << "button." << ButtonName(button) << "="
             << ActionName(settings.custom.GetButton(button)) << '\n';
   }
   for (int value = 0; value < static_cast<int>(AnalogAxis::Count); ++value)
   {
      const auto axis = static_cast<AnalogAxis>(value);
      output << "axis." << AxisName(axis) << "="
             << AxisActionName(settings.custom.GetAxis(axis)) << '\n';
   }
   if (!output)
   {
      if (error)
         *error = "failed while writing Vita controls settings file";
      return false;
   }
   if (error)
      error->clear();
   return true;
}

Action ShortTriggerAction(Layout layout, Trigger trigger)
{
   if (trigger == Trigger::Left)
      return layout == Layout::RetroDpadMove ? Action::LeanForward
                                             : Action::Block;
   return Action::UseWeapon;
}

Action LongTriggerAction(Layout layout, Trigger trigger)
{
   if (trigger == Trigger::Left)
   {
      if (layout == Layout::RetroDpadMove)
         return Action::Block;
      if (layout == Layout::Custom)
         return Action::QuickSave;
      return Action::LeanForward;
   }
   if (layout == Layout::RetroDpadMove)
      return Action::DropItem;
   if (layout == Layout::Custom)
      return Action::QuickLoad;
   return Action::CycleWeapon;
}

Action MapButton(Layout layout, PhysicalButton button,
                 const CustomBindings &custom)
{
   if (!ValidButton(button) || !ValidLayout(layout))
      return Action::None;
   if (layout == Layout::Custom)
      return custom.GetButton(button);

   switch (button)
   {
   case PhysicalButton::Cross: return Action::Attack;
   case PhysicalButton::Circle: return Action::Jump;
   case PhysicalButton::Square: return Action::Use;
   case PhysicalButton::Triangle: return Action::Inventory;
   case PhysicalButton::Select: return Action::QuickSave;
   case PhysicalButton::Start: return Action::Pause;
   default: break;
   }

   if (layout == Layout::DualAnalogFps)
   {
      switch (button)
      {
      case PhysicalButton::LeftShoulder: return Action::LeanLeft;
      case PhysicalButton::RightShoulder: return Action::LeanRight;
      case PhysicalButton::LeftStick: return Action::Crouch;
      case PhysicalButton::RightStick: return Action::CycleWeapon;
      case PhysicalButton::DpadUp: return Action::NextItem;
      case PhysicalButton::DpadDown: return Action::PreviousItem;
      case PhysicalButton::DpadLeft: return Action::LeanLeft;
      case PhysicalButton::DpadRight: return Action::LeanRight;
      default: return Action::None;
      }
   }
   switch (button)
   {
   case PhysicalButton::LeftShoulder: return Action::Block;
   case PhysicalButton::RightShoulder: return Action::CycleWeapon;
   case PhysicalButton::DpadUp: return Action::MoveForward;
   case PhysicalButton::DpadDown: return Action::MoveBackward;
   case PhysicalButton::DpadLeft: return Action::StrafeLeft;
   case PhysicalButton::DpadRight: return Action::StrafeRight;
   default: return Action::None;
   }
}

AxisAction MapAxis(Layout layout, AnalogAxis axis,
                   const CustomBindings &custom)
{
   if (!ValidAxis(axis) || !ValidLayout(layout))
      return AxisAction::None;
   if (layout == Layout::Custom)
      return custom.GetAxis(axis);
   if (layout == Layout::DualAnalogFps)
   {
      switch (axis)
      {
      case AnalogAxis::LeftX: return AxisAction::MoveHorizontal;
      case AnalogAxis::LeftY: return AxisAction::MoveVertical;
      case AnalogAxis::RightX: return AxisAction::LookHorizontal;
      case AnalogAxis::RightY: return AxisAction::LookVertical;
      case AnalogAxis::Count: break;
      }
   }
   if (axis == AnalogAxis::RightX)
      return AxisAction::LookHorizontal;
   if (axis == AnalogAxis::RightY)
      return AxisAction::LookVertical;
   return AxisAction::None;
}

bool PointInTriggerCorner(float normalizedX, float normalizedY,
                          Trigger trigger)
{
   if (normalizedX < 0.0f || normalizedX > 1.0f ||
       normalizedY < 0.0f || normalizedY > 1.0f)
      return false;
   if (normalizedY < 1.0f - kCornerHeight)
      return false;
   if (trigger == Trigger::Left)
      return normalizedX <= kCornerWidth;
   return normalizedX >= 1.0f - kCornerWidth;
}

Action UpdateTouchTrigger(Layout layout, Trigger trigger,
                          TouchTrigger &state, std::int64_t fingerId,
                          bool down, std::uint32_t nowMilliseconds)
{
   if (down)
   {
      if (!state.down)
      {
         state.down = true;
         state.fingerId = fingerId;
         state.startedMilliseconds = nowMilliseconds;
         state.longPressSent = false;
      }
      else if (state.fingerId == fingerId && !state.longPressSent &&
               static_cast<std::uint32_t>(
                  nowMilliseconds - state.startedMilliseconds) >=
                  kLongPressMilliseconds)
      {
         state.longPressSent = true;
         return LongTriggerAction(layout, trigger);
      }
      return Action::None;
   }

   if (!state.down || state.fingerId != fingerId)
      return Action::None;
   const Action released =
      !state.longPressSent &&
      static_cast<std::uint32_t>(
         nowMilliseconds - state.startedMilliseconds) <
         kLongPressMilliseconds
         ? ShortTriggerAction(layout, trigger)
         : Action::None;
   state.down = false;
   state.fingerId = -1;
   state.startedMilliseconds = 0;
   state.longPressSent = false;
   return released;
}
}
