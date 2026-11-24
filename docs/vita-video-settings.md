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
