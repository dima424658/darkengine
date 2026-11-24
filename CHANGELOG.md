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
