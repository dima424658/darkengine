#include <win32_platform.h>
#include <xinput.h>
#include <string.h>

#include <gamepad.h>
#include <gamepad_backend.h>

typedef DWORD (WINAPI *tXInputGetState)(DWORD, XINPUT_STATE *);
typedef DWORD (WINAPI *tXInputSetState)(DWORD, XINPUT_VIBRATION *);

static HMODULE g_xinput_module;
static tXInputGetState g_xinput_get_state;
static tXInputSetState g_xinput_set_state;
static int g_connected[GAMEPAD_MAX_CONTROLLERS];
static DWORD g_next_scan[GAMEPAD_MAX_CONTROLLERS];

static int XInputBackendInit(void)
{
   static const char *dll_names[] = {
      "xinput1_4.dll",
      "xinput1_3.dll",
      "xinput9_1_0.dll"
   };

   for (unsigned i = 0; i < sizeof(dll_names) / sizeof(dll_names[0]); ++i)
   {
      g_xinput_module = LoadLibraryA(dll_names[i]);
      if (g_xinput_module)
         break;
   }
   if (!g_xinput_module)
      return 0;

   g_xinput_get_state = (tXInputGetState)GetProcAddress(g_xinput_module, "XInputGetState");
   g_xinput_set_state = (tXInputSetState)GetProcAddress(g_xinput_module, "XInputSetState");
   if (!g_xinput_get_state || !g_xinput_set_state)
   {
      FreeLibrary(g_xinput_module);
      g_xinput_module = 0;
      g_xinput_get_state = 0;
      g_xinput_set_state = 0;
      return 0;
   }

   memset(g_connected, 0, sizeof(g_connected));
   memset(g_next_scan, 0, sizeof(g_next_scan));
   return 1;
}

static void XInputBackendRumble(unsigned index, float low, float high)
{
   if (!g_xinput_set_state || index >= GAMEPAD_MAX_CONTROLLERS)
      return;
   if (low < 0.0f) low = 0.0f;
   if (low > 1.0f) low = 1.0f;
   if (high < 0.0f) high = 0.0f;
   if (high > 1.0f) high = 1.0f;

   XINPUT_VIBRATION vibration = {};
   vibration.wLeftMotorSpeed = (WORD)(low * 65535.0f + 0.5f);
   vibration.wRightMotorSpeed = (WORD)(high * 65535.0f + 0.5f);
   g_xinput_set_state(index, &vibration);
}

static void XInputBackendTerm(void)
{
   if (g_xinput_set_state)
   {
      for (unsigned i = 0; i < GAMEPAD_MAX_CONTROLLERS; ++i)
         XInputBackendRumble(i, 0.0f, 0.0f);
   }
   if (g_xinput_module)
      FreeLibrary(g_xinput_module);
   g_xinput_module = 0;
   g_xinput_get_state = 0;
   g_xinput_set_state = 0;
   memset(g_connected, 0, sizeof(g_connected));
}

static int XInputBackendPoll(unsigned index, sGamepadRawState *raw)
{
   if (!raw || !g_xinput_get_state || index >= GAMEPAD_MAX_CONTROLLERS)
      return 0;
   memset(raw, 0, sizeof(*raw));

   const DWORD now = GetTickCount();
   if (!g_connected[index] && (LONG)(now - g_next_scan[index]) < 0)
      return 1;

   XINPUT_STATE state = {};
   const DWORD result = g_xinput_get_state(index, &state);
   if (result != ERROR_SUCCESS)
   {
      if (g_connected[index])
         XInputBackendRumble(index, 0.0f, 0.0f);
      g_connected[index] = 0;
      g_next_scan[index] = now + 1000;
      return 1;
   }

   g_connected[index] = 1;
   raw->connected = 1;
   raw->packet_number = state.dwPacketNumber;
   raw->left_x = state.Gamepad.sThumbLX;
   raw->left_y = state.Gamepad.sThumbLY;
   raw->right_x = state.Gamepad.sThumbRX;
   raw->right_y = state.Gamepad.sThumbRY;
   raw->left_trigger = state.Gamepad.bLeftTrigger;
   raw->right_trigger = state.Gamepad.bRightTrigger;

   const WORD buttons = state.Gamepad.wButtons;
   if (buttons & XINPUT_GAMEPAD_A) raw->buttons |= 1u << kGamepadButtonA;
   if (buttons & XINPUT_GAMEPAD_B) raw->buttons |= 1u << kGamepadButtonB;
   if (buttons & XINPUT_GAMEPAD_X) raw->buttons |= 1u << kGamepadButtonX;
   if (buttons & XINPUT_GAMEPAD_Y) raw->buttons |= 1u << kGamepadButtonY;
   if (buttons & XINPUT_GAMEPAD_LEFT_SHOULDER) raw->buttons |= 1u << kGamepadButtonLeftShoulder;
   if (buttons & XINPUT_GAMEPAD_RIGHT_SHOULDER) raw->buttons |= 1u << kGamepadButtonRightShoulder;
   if (buttons & XINPUT_GAMEPAD_BACK) raw->buttons |= 1u << kGamepadButtonView;
   if (buttons & XINPUT_GAMEPAD_START) raw->buttons |= 1u << kGamepadButtonMenu;
   if (buttons & XINPUT_GAMEPAD_LEFT_THUMB) raw->buttons |= 1u << kGamepadButtonLeftStick;
   if (buttons & XINPUT_GAMEPAD_RIGHT_THUMB) raw->buttons |= 1u << kGamepadButtonRightStick;
   if (buttons & XINPUT_GAMEPAD_DPAD_UP) raw->buttons |= 1u << kGamepadButtonDpadUp;
   if (buttons & XINPUT_GAMEPAD_DPAD_DOWN) raw->buttons |= 1u << kGamepadButtonDpadDown;
   if (buttons & XINPUT_GAMEPAD_DPAD_LEFT) raw->buttons |= 1u << kGamepadButtonDpadLeft;
   if (buttons & XINPUT_GAMEPAD_DPAD_RIGHT) raw->buttons |= 1u << kGamepadButtonDpadRight;
   return 1;
}

static const sGamepadBackendOps g_xinput_backend = {
   "xinput",
   XInputBackendInit,
   XInputBackendTerm,
   XInputBackendPoll,
   XInputBackendRumble
};

extern "C" const sGamepadBackendOps *GamepadXInputBackend(void)
{
   return &g_xinput_backend;
}
