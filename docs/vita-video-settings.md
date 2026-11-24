# Vita video correction controls

The Vita bootstrap exposes two independent video sliders. D-pad up/down
selects brightness or gamma, left/right changes the selected value, and the
front touchscreen can set either slider directly. Values are saved to
`ux0:data/thiefX/vita_options.cfg`; malformed settings are reported rather than
silently accepted.

Brightness uses a neutral midpoint and a conservative additive range of
`-0.12..+0.12`. Gamma uses a neutral midpoint and a correction range of
`0.80..1.20`; the shader applies the inverse gamma exponent after brightness.
The narrow range is intended to avoid harsh clipping on the Vita 2000 LCD.
It is a starting point, not a hardware-calibrated profile.

`DarkVitaVideo::VertexShaderSource` and `FragmentShaderSource` provide a
vitaGL-compatible GLES 2 shader pair. `ApplyCorrection` is the CPU reference
used by tests. Runtime shader compilation and visual calibration still need a
physical Vita; the current VPKs are the bootstrap/settings UI, not the full
game renderer.

## Spatial Upscaling

The Vita menu exposes four 3D render resolutions:

| Profile | Render resolution | vitaGL output |
| --- | ---: | --- |
| 100% Native | 960x544 | Native no FSR pass |
| 85% Scale | 816x462 | Mobile AMD FSR 1.0 |
| Uncharted Scale | 720x408 | Mobile AMD FSR 1.0 |
| Extreme Performance | 640x368 | Mobile AMD FSR 1.0 |

Sub-native frames are rendered into a Vita FBO and sharpened/upscaled to the
960x544 display through the Mobile GLES 2.0 FSR 1.0 shader. The selected
profile is stored as `spatial_upscaling` in `vita_options.cfg`.
