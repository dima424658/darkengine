# Changelog

## 2026-11-24 - Vita Port TASK 1

- Added guarded PS Vita CMake target and VitaSDK toolchain wrapper.
- Added a `psvita` CMake preset and `vita-test-vpk` packaging target.
- Added three staged Vita package layouts for THFG00001, THFG00002, and THFG00003.
- Deferred real Vita packaging because VitaSDK and Ninja are unavailable in the current environment.
