#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace DarkVitaControls
{
enum class Layout
{
   DualAnalogFps = 0,
   RetroDpadMove = 1,
   Custom = 2,
};

enum class Trigger
{
   Left = 0,
   Right = 1,
};

enum class Action
{
   None = 0,
   Block,
   LeanForward,
   UseWeapon,
   DropItem,
   CycleWeapon,
   QuickSave,
   QuickLoad,
   MoveForward,
   MoveBackward,
   StrafeLeft,
   StrafeRight,
   LookUp,
   LookDown,
   Attack,
   Jump,
   Crouch,
   Use,
   Inventory,
   PreviousItem,
   NextItem,
   LeanLeft,
   LeanRight,
   Pause,
};

enum class PhysicalButton
{
   Cross,
   Circle,
   Square,
   Triangle,
   LeftShoulder,
   RightShoulder,
   Select,
   Start,
   LeftStick,
   RightStick,
   DpadUp,
   DpadDown,
   DpadLeft,
   DpadRight,
   Count
};

enum class AnalogAxis
{
   LeftX,
   LeftY,
   RightX,
   RightY,
   Count
};

enum class AxisAction
{
   None,
   MoveHorizontal,
   MoveVertical,
   LookHorizontal,
   LookVertical
};

class CustomBindings
{
public:
   CustomBindings();
   bool SetButton(PhysicalButton button, Action action);
   bool SetAxis(AnalogAxis axis, AxisAction action);
   Action GetButton(PhysicalButton button) const;
   AxisAction GetAxis(AnalogAxis axis) const;

private:
   std::array<Action, static_cast<std::size_t>(PhysicalButton::Count)> m_buttons;
   std::array<AxisAction, static_cast<std::size_t>(AnalogAxis::Count)> m_axes;
};

struct Settings
{
   Layout layout = Layout::DualAnalogFps;
   CustomBindings custom;
};

struct TouchTrigger
{
   bool down = false;
   std::int64_t fingerId = -1;
   std::uint32_t startedMilliseconds = 0;
   bool longPressSent = false;
};

Layout ClampLayout(int value);
const char *LayoutName(Layout layout);
bool LayoutFromName(const std::string &name, Layout &layout);
Settings Load(const std::string &path);
bool Save(const std::string &path, const Settings &settings,
          std::string *error);
Action ShortTriggerAction(Layout layout, Trigger trigger);
Action LongTriggerAction(Layout layout, Trigger trigger);
Action MapButton(Layout layout, PhysicalButton button,
                 const CustomBindings &custom);
AxisAction MapAxis(Layout layout, AnalogAxis axis,
                   const CustomBindings &custom);
bool PointInTriggerCorner(float normalizedX, float normalizedY,
                          Trigger trigger);
Action UpdateTouchTrigger(Layout layout, Trigger trigger,
                          TouchTrigger &state, std::int64_t fingerId,
                          bool down, std::uint32_t nowMilliseconds);
}
