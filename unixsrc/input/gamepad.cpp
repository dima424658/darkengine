#pragma pack(push, 8)
#include <SDL2/SDL.h>

#include <algorithm>
#include <cstring>
#include <mutex>
#if defined(__ANDROID__)
#include <jni.h>
#endif
#pragma pack(pop)

#include <gamepad.h>
#include <gamepad_backend.h>

#if defined(__ANDROID__)
extern "C" int DarkInputRelativeMouseActive(void);
#endif

namespace
{
struct sSDLGamepadSlot
{
   SDL_GameController *controller;
   SDL_JoystickID instance;
   sGamepadRawState last;
   unsigned packet;
};

sSDLGamepadSlot g_slots[GAMEPAD_MAX_CONTROLLERS];
Uint32 g_nextRefresh;

#if defined(__ANDROID__)
constexpr unsigned kTouchSlot = GAMEPAD_MAX_CONTROLLERS - 1;
std::mutex g_touchMutex;
sGamepadRawState g_touchState;
uint32_t g_touchDesiredButtons;
uint32_t g_touchPolledButtons;
uint32_t g_touchPendingPresses;
uint32_t g_touchPendingReleases;

void TouchStateChanged()
{
   ++g_touchState.packet_number;
}
#endif

void CloseSlot(unsigned index)
{
   if (index >= GAMEPAD_MAX_CONTROLLERS)
      return;
   if (g_slots[index].controller)
      SDL_GameControllerClose(g_slots[index].controller);
   std::memset(&g_slots[index], 0, sizeof(g_slots[index]));
   g_slots[index].instance = -1;
}

bool HasInstance(SDL_JoystickID instance)
{
   for (unsigned i = 0; i < GAMEPAD_MAX_CONTROLLERS; ++i)
      if (g_slots[i].controller && g_slots[i].instance == instance)
         return true;
   return false;
}

void RefreshControllers(bool force)
{
   const Uint32 now = SDL_GetTicks();
   if (!force && (Sint32)(now - g_nextRefresh) < 0)
      return;
   g_nextRefresh = now + 500;

   const unsigned physicalSlots =
#if defined(__ANDROID__)
      kTouchSlot;
#else
      GAMEPAD_MAX_CONTROLLERS;
#endif
   for (unsigned i = 0; i < physicalSlots; ++i)
      if (g_slots[i].controller &&
          !SDL_GameControllerGetAttached(g_slots[i].controller))
         CloseSlot(i);

   const int devices = SDL_NumJoysticks();
   for (int device = 0; device < devices; ++device)
   {
      if (!SDL_IsGameController(device))
         continue;
      const SDL_JoystickID instance = SDL_JoystickGetDeviceInstanceID(device);
      if (instance < 0 || HasInstance(instance))
         continue;

      unsigned slot = physicalSlots;
      for (unsigned i = 0; i < physicalSlots; ++i)
         if (!g_slots[i].controller)
         {
            slot = i;
            break;
         }
      if (slot == physicalSlots)
         break;

      SDL_GameController *controller = SDL_GameControllerOpen(device);
      if (!controller)
         continue;
      g_slots[slot].controller = controller;
      g_slots[slot].instance = instance;
      g_slots[slot].packet = 0;
      std::memset(&g_slots[slot].last, 0, sizeof(g_slots[slot].last));
      SDL_Log("Dark Engine gamepad %u: %s", slot,
              SDL_GameControllerName(controller));
   }
}

int InvertAxis(Sint16 value)
{
   return value == -32768 ? 32767 : -value;
}

unsigned TriggerValue(Sint16 value)
{
   // SDL's standardized controller triggers rest at zero and rise to 32767.
   return (unsigned)(std::max<int>(value, 0) * GAMEPAD_RAW_TRIGGER_MAX /
                     32767);
}

void AddButton(sGamepadRawState *raw, SDL_GameController *controller,
               SDL_GameControllerButton source, eGamepadButton target)
{
   if (SDL_GameControllerGetButton(controller, source))
      raw->buttons |= 1u << target;
}

int SDLBackendInit()
{
   for (unsigned i = 0; i < GAMEPAD_MAX_CONTROLLERS; ++i)
      g_slots[i].instance = -1;
   if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC) != 0)
      return 0;
   SDL_GameControllerEventState(SDL_ENABLE);
   RefreshControllers(true);
   return 1;
}

void SDLBackendTerm()
{
   for (unsigned i = 0; i < GAMEPAD_MAX_CONTROLLERS; ++i)
      CloseSlot(i);
   SDL_QuitSubSystem(SDL_INIT_HAPTIC | SDL_INIT_GAMECONTROLLER);
}

int SDLBackendPoll(unsigned index, sGamepadRawState *raw)
{
   if (!raw || index >= GAMEPAD_MAX_CONTROLLERS)
      return 0;
   std::memset(raw, 0, sizeof(*raw));
#if defined(__ANDROID__)
   if (index == kTouchSlot)
   {
      std::lock_guard<std::mutex> lock(g_touchMutex);
      // Java may complete a short tap between two engine polls.  Emit a
      // queued press for at least one poll and its release on a later poll so
      // the input binder cannot lose the entire gesture during a slow frame.
      const uint32_t presses = g_touchPendingPresses & ~g_touchPolledButtons;
      if (presses)
      {
         g_touchPolledButtons |= presses;
         g_touchPendingPresses &= ~presses;
      }

      const uint32_t releases = g_touchPendingReleases &
         g_touchPolledButtons & ~presses;
      if (releases)
      {
         g_touchPolledButtons &= ~releases;
         g_touchPendingReleases &= ~releases;
         // A second press may have arrived while this queued release was
         // waiting.  Preserve it for the following poll.
         g_touchPendingPresses |= releases & g_touchDesiredButtons;
      }

      if (presses || releases)
         TouchStateChanged();
      g_touchState.buttons = g_touchPolledButtons;
      *raw = g_touchState;
      return 1;
   }
#endif
   SDL_GameControllerUpdate();
   RefreshControllers(false);

   sSDLGamepadSlot &slot = g_slots[index];
   if (!slot.controller || !SDL_GameControllerGetAttached(slot.controller))
      return 1;

   raw->connected = 1;
   raw->left_x = SDL_GameControllerGetAxis(slot.controller,
                                           SDL_CONTROLLER_AXIS_LEFTX);
   raw->left_y = InvertAxis(SDL_GameControllerGetAxis(
      slot.controller, SDL_CONTROLLER_AXIS_LEFTY));
   raw->right_x = SDL_GameControllerGetAxis(slot.controller,
                                            SDL_CONTROLLER_AXIS_RIGHTX);
   raw->right_y = InvertAxis(SDL_GameControllerGetAxis(
      slot.controller, SDL_CONTROLLER_AXIS_RIGHTY));
   raw->left_trigger = TriggerValue(SDL_GameControllerGetAxis(
      slot.controller, SDL_CONTROLLER_AXIS_TRIGGERLEFT));
   raw->right_trigger = TriggerValue(SDL_GameControllerGetAxis(
      slot.controller, SDL_CONTROLLER_AXIS_TRIGGERRIGHT));

   AddButton(raw, slot.controller, SDL_CONTROLLER_BUTTON_A, kGamepadButtonA);
   AddButton(raw, slot.controller, SDL_CONTROLLER_BUTTON_B, kGamepadButtonB);
   AddButton(raw, slot.controller, SDL_CONTROLLER_BUTTON_X, kGamepadButtonX);
   AddButton(raw, slot.controller, SDL_CONTROLLER_BUTTON_Y, kGamepadButtonY);
   AddButton(raw, slot.controller, SDL_CONTROLLER_BUTTON_LEFTSHOULDER,
             kGamepadButtonLeftShoulder);
   AddButton(raw, slot.controller, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,
             kGamepadButtonRightShoulder);
   AddButton(raw, slot.controller, SDL_CONTROLLER_BUTTON_BACK,
             kGamepadButtonView);
   AddButton(raw, slot.controller, SDL_CONTROLLER_BUTTON_START,
             kGamepadButtonMenu);
   AddButton(raw, slot.controller, SDL_CONTROLLER_BUTTON_LEFTSTICK,
             kGamepadButtonLeftStick);
   AddButton(raw, slot.controller, SDL_CONTROLLER_BUTTON_RIGHTSTICK,
             kGamepadButtonRightStick);
   AddButton(raw, slot.controller, SDL_CONTROLLER_BUTTON_DPAD_UP,
             kGamepadButtonDpadUp);
   AddButton(raw, slot.controller, SDL_CONTROLLER_BUTTON_DPAD_DOWN,
             kGamepadButtonDpadDown);
   AddButton(raw, slot.controller, SDL_CONTROLLER_BUTTON_DPAD_LEFT,
             kGamepadButtonDpadLeft);
   AddButton(raw, slot.controller, SDL_CONTROLLER_BUTTON_DPAD_RIGHT,
             kGamepadButtonDpadRight);

   sGamepadRawState comparison = *raw;
   comparison.packet_number = 0;
   sGamepadRawState previous = slot.last;
   previous.packet_number = 0;
   if (std::memcmp(&comparison, &previous, sizeof(comparison)) != 0)
      ++slot.packet;
   raw->packet_number = slot.packet;
   slot.last = *raw;
   return 1;
}

void SDLBackendRumble(unsigned index, float low, float high)
{
#if defined(__ANDROID__)
   if (index == kTouchSlot)
      return;
#endif
   if (index >= GAMEPAD_MAX_CONTROLLERS || !g_slots[index].controller)
      return;
   const Uint16 lowStrength = (Uint16)(low * 65535.0f + 0.5f);
   const Uint16 highStrength = (Uint16)(high * 65535.0f + 0.5f);
   SDL_GameControllerRumble(g_slots[index].controller, lowStrength,
                            highStrength,
                            (lowStrength || highStrength) ? 250 : 0);
}

const sGamepadBackendOps g_sdlBackend = {
   "sdl", SDLBackendInit, SDLBackendTerm, SDLBackendPoll, SDLBackendRumble
};
}

extern "C" const sGamepadBackendOps *GamepadSDLBackend(void)
{
   return &g_sdlBackend;
}

extern "C" const sGamepadBackendOps *GamepadXInputBackend(void)
{
   return nullptr;
}

#if defined(__ANDROID__)
extern "C" JNIEXPORT void JNICALL
Java_org_opendarkengine_thief2_ThiefActivity_nativeSetTouchControllerConnected(
   JNIEnv *, jclass, jboolean connected)
{
   std::lock_guard<std::mutex> lock(g_touchMutex);
   std::memset(&g_touchState, 0, sizeof(g_touchState));
   g_touchDesiredButtons = 0;
   g_touchPolledButtons = 0;
   g_touchPendingPresses = 0;
   g_touchPendingReleases = 0;
   g_touchState.connected = connected ? 1 : 0;
   TouchStateChanged();
}

extern "C" JNIEXPORT void JNICALL
Java_org_opendarkengine_thief2_ThiefActivity_nativeSetTouchAxis(
   JNIEnv *, jclass, jint axis, jfloat x, jfloat y)
{
   const int rawX = (int)(std::max(-1.0f, std::min(1.0f, x)) *
      (x < 0.0f ? 32768.0f : 32767.0f));
   const int rawY = (int)(std::max(-1.0f, std::min(1.0f, y)) *
      (y < 0.0f ? 32768.0f : 32767.0f));
   std::lock_guard<std::mutex> lock(g_touchMutex);
   if (axis == 0)
   {
      g_touchState.left_x = rawX;
      g_touchState.left_y = rawY;
   }
   else if (axis == 1)
   {
      g_touchState.right_x = rawX;
      g_touchState.right_y = rawY;
   }
   TouchStateChanged();
}

extern "C" JNIEXPORT void JNICALL
Java_org_opendarkengine_thief2_ThiefActivity_nativeSetTouchButton(
   JNIEnv *, jclass, jint button, jboolean down)
{
   if (button < 0 || button >= kGamepadButtonCount)
      return;
   std::lock_guard<std::mutex> lock(g_touchMutex);
   const uint32_t mask = 1u << button;
   if (down)
   {
      if (!(g_touchDesiredButtons & mask))
      {
         g_touchDesiredButtons |= mask;
         g_touchPendingPresses |= mask;
      }
   }
   else
   {
      if (g_touchDesiredButtons & mask)
      {
         g_touchDesiredButtons &= ~mask;
         g_touchPendingReleases |= mask;
      }
   }
}

extern "C" JNIEXPORT jboolean JNICALL
Java_org_opendarkengine_thief2_ThiefActivity_nativeTouchControlsActive(
   JNIEnv *, jclass)
{
   return DarkInputRelativeMouseActive() ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT void JNICALL
Java_org_opendarkengine_thief2_ThiefActivity_nativeNotifyTouchInput(
   JNIEnv *, jclass)
{
   GamepadNotifyInput(kTouchSlot);
}

extern "C" JNIEXPORT jboolean JNICALL
Java_org_opendarkengine_thief2_ThiefActivity_nativeTouchControlsVisible(
   JNIEnv *, jclass)
{
   const unsigned lastInput = GamepadGetLastInputIndex();
   return DarkInputRelativeMouseActive() &&
      (lastInput >= GAMEPAD_MAX_CONTROLLERS || lastInput == kTouchSlot)
      ? JNI_TRUE : JNI_FALSE;
}
#endif
