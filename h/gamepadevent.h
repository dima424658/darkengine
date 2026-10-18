#ifndef __GAMEPADEVENT_H
#define __GAMEPADEVENT_H

/*
 * UI_EVENT_JOY has no source field.  The high bit of joynum is unused by the
 * legacy joystick path and identifies events emitted by the gamepad bridge.
 * Keeping the marker in the recorded event preserves input playback.
 */
#define GAMEPAD_UI_EVENT_MARKER 0x80
#define GAMEPAD_UI_EVENT_VALUE(value) ((value) & 0x7f)
#define GAMEPAD_UI_EVENT_IS_GAMEPAD(value) (((value) & GAMEPAD_UI_EVENT_MARKER) != 0)

#endif
