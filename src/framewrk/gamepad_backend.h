#ifndef __GAMEPAD_BACKEND_H
#define __GAMEPAD_BACKEND_H

#include <stdint.h>

#define GAMEPAD_RAW_AXIS_MIN (-32768)
#define GAMEPAD_RAW_AXIS_MAX 32767
#define GAMEPAD_RAW_TRIGGER_MAX 255

typedef struct sGamepadRawState
{
   int connected;
   unsigned packet_number;
   uint32_t buttons;
   int left_x;
   int left_y;
   int right_x;
   int right_y;
   unsigned left_trigger;
   unsigned right_trigger;
} sGamepadRawState;

typedef struct sGamepadBackendOps
{
   const char *name;
   int (*init)(void);
   void (*term)(void);
   int (*poll)(unsigned index, sGamepadRawState *state);
   void (*rumble)(unsigned index, float low, float high);
} sGamepadBackendOps;

#ifdef __cplusplus
extern "C" {
#endif

const sGamepadBackendOps *GamepadXInputBackend(void);
const sGamepadBackendOps *GamepadSDLBackend(void);

#ifdef __cplusplus
}
#endif

#endif
