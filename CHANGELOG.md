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
