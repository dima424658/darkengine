# Changelog

## 2026-11-24 - Vita Port TASK 1

- Added guarded PS Vita CMake target and VitaSDK toolchain wrapper.
- Added a `psvita` CMake preset and `vita-test-vpk` packaging target.
- Added three staged Vita package layouts for THFG00001, THFG00002, and THFG00003.
- Deferred real Vita packaging because VitaSDK and Ninja are unavailable in the current environment.
- Verified the Vita placeholder CMake preset and generated package structure for all three title IDs.
- Added Vita asset-root resolution for THFG00001, THFG00002, and THFG00003, including preferred write routing.
- Added and passed the host-side Vita path-routing regression test.
- Added Vita 960x544 viewport fit logic and vitaGL discovery hooks.
- Added and passed host-side 4:3 and 16:9 Vita viewport layout checks.
- Core Port scaffolding: added VitaSDK ARMv7 toolchain configuration and SDL2/vitaGL bootstrap targets for the three Thief title IDs; actual VPK generation remains unverified because VitaSDK is not available in the active build session.
- Removed host-only VPK placeholders. Replaced them with VitaSDK `vita_create_self` and `vita_create_vpk` targets using SDL2 and vitaGL. The CMake preset parses, but actual configuration is blocked by the missing `VITASDK` environment path in this session.
- Explicit toolchain build attempt confirmed `/usr/local/vitasdk/share/vita.toolchain.cmake` is absent in this session. TASK 1–3 remain open for real Vita verification; Part II is not activated yet.

- 2026-11-24: Built the Vita ARMv7 bootstrap for Thief, Thief Gold, and Thief II with VitaSDK 2026.08; verified each VPK contains a Vita SELF payload and correct title metadata. Recorded the C++ linker-driver fix and moved the focus pipeline to Handheld Remaster TASK 4-6.

- 2026-11-24: Completed Handheld Remaster TASK 4. Added ordered loose-file overlays from each Vita title’s `mods/` directory, safe base-root writes, parent-traversal rejection, and a NewDark SUB1 parser with UTF-8/Windows-1250 decoding; focused host tests and all three Vita ARMv7 VPK builds pass. Documented behavior in docs/vita-content.md.

- 2026-11-24: Completed Handheld Remaster TASK 5. Added separate Vita Slim brightness/gamma sliders, a GLES2 correction shader, conservative correction ranges, D-pad/front-touch adjustment, and atomic `vita_options.cfg` persistence. Host tests and all three Vita ARMv7 test VPK builds pass; hardware calibration remains pending.

- 2026-11-24: Completed Handheld Remaster TASK 6. Added Layout A dual-analog FPS, Layout B retro D-pad movement, and Layout C custom bindings, plus lower-corner LB/RB touch icons with layout-specific short/long-press actions. Documented behavior in docs/vita-controls.md.

- 2026-11-24: Completed Handheld Remaster TASK 7. Added 256/512/1024/Unlimited Vita texture profiles with power-of-two dimension bounding, RGBA VRAM estimates, and persistent texture settings. Host regressions pass.

- 2026-11-24: Completed Handheld Remaster TASK 8. Added Vita EAX/EFX room preset mapping, distance/occlusion propagation, underwater attenuation, and headphone wet-mix compensation. Host acoustics regressions pass.

- 2026-11-24: Completed Handheld Remaster TASK 9. Added Vita 30/60 FPS power profiles with ARM/bus/GPU clock switching and rebuilt separate trilogy VPKs via the local VitaSDK. Verified THFG00001, THFG00002, and THFG00003 metadata and embedded eboot.bin payloads.

- 2026-11-24: Completed Handheld Remaster TASK 10. Added Vita-safe Ghost Check aggregation for detections, alarms, discovered bodies, and kills with persistent mission-summary settings.

- 2026-11-24: Completed Handheld Remaster TASK 11. Added bounded 1x-2x readable zoom for documents and UI text with persisted Vita settings.

- 2026-11-24: Completed Handheld Remaster TASK 12. Verified final compact ARMv7 trilogy bootstrap rebuild and package parity; THFG00001, THFG00002, and THFG00003 VPKs contain eboot.bin and matching title metadata.

- 2026-11-24: Added Vita Spatial Upscaling configuration: 960x544 Native, 816x462 85%, 720x408 Uncharted, and 640x368 Extreme, with an AMD FSR 1.0 Mobile GLES 2.0 vitaGL upscaling pass for every sub-native profile.

- TASK 6 controls: added physical Layout A/B mappings, config-backed custom bindings, per-finger LB/RB touch triggers, and temporary icons; clean host build and CTest passed.

- TASK 6 verification: clean host build/CTest passed and the ARMv7 Vita build regenerated all three bootstrap VPKs; physical-device behavior remains unverified.
