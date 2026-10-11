#include <gamepad_backend.h>

extern "C" const sGamepadBackendOps *GamepadXInputBackend(void)
{
   return nullptr;
}
