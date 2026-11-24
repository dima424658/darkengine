# Vita controls

The Vita controls module exposes the physical-button and stick mappings used
by the three selectable layouts. The current bootstrap is a platform test
shell; it demonstrates layout selection and touch triggers, while gameplay
integration can consume `MapButton` and `MapAxis` from
`libsrc/platform/vita_controls.hpp`.

| Input | Layout A: Dual Analog FPS | Layout B: Retro D-Pad Move |
| --- | --- | --- |
| Cross / Circle / Square / Triangle | Attack / Jump / Use / Inventory | Attack / Jump / Use / Inventory |
| Left / right shoulder | Lean left / lean right | Block / cycle weapon |
| Left / right stick click | Crouch / cycle weapon | Unassigned |
| D-pad | Item selection and leaning | Move forward/back and strafe |
| Left stick | Move horizontally/vertically | Unassigned |
| Right stick | Look horizontally/vertically | Look horizontally/vertically |
| Select / Start | Quick save / pause | Quick save / pause |

Layout C loads custom mappings from `vita_controls.cfg`. For example:

```ini
layout=C
button.cross=use
button.left_shoulder=block
axis.left_x=look_horizontal
axis.left_y=look_vertical
```

Unspecified mappings are `none`. Supported button action values are
`block`, `lean_forward`, `use_weapon`, `drop_item`, `cycle_weapon`,
`quick_save`, `quick_load`, `move_forward`, `move_backward`, `strafe_left`,
`strafe_right`, `look_up`, `look_down`, `attack`, `jump`, `crouch`, `use`,
`inventory`, `previous_item`, `next_item`, `lean_left`, `lean_right`, and
`pause`. Stick axes accept `move_horizontal`, `move_vertical`,
`look_horizontal`, or `look_vertical`.

The lower-left and lower-right front-touch corners act as virtual LB and RB.
Icons appear while a trigger is held and for about 2.2 seconds after touching
either corner. A short press activates its layout-specific tap action; holding
for 400 ms activates the long-press action once.
