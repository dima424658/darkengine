#pragma pack(push, 8)
#include <SDL2/SDL.h>

#include <algorithm>
#include <cstring>
#pragma pack(pop)

#include <gamepad.h>
#include <gamepad_backend.h>

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

   for (unsigned i = 0; i < GAMEPAD_MAX_CONTROLLERS; ++i)
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

      unsigned slot = GAMEPAD_MAX_CONTROLLERS;
      for (unsigned i = 0; i < GAMEPAD_MAX_CONTROLLERS; ++i)
         if (!g_slots[i].controller)
         {
            slot = i;
            break;
         }
      if (slot == GAMEPAD_MAX_CONTROLLERS)
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
