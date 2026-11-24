# CURRENT RUNTIME STATE

## CURRENT WORK BLOCK
- Objective: TASK 4 - Universal mod loading and Polish .sub subtitle parsing.
- Progress: Removed placeholder VPK packaging and added VitaSDK ARMv7, SDL2, vitaGL and SELF/VPK CMake targets for all three title IDs.
- Blockers: `VITASDK` is unset and `/usr/local/vitasdk` is absent in this session; native configure/build verification is blocked until the SDK path is exposed.

## ENCOUNTERED DEAD ENDS & DEFERRED TASKS
- 2026-11-24: Earlier placeholder CMake configure failed on missing VitaSDK and FFmpeg submodule sources; placeholder package generation has since been removed.
- 2026-11-24: Installed Ninja in the environment. `cmake --preset psvita` still fails before compiler detection because `VITASDK` is unset; no VitaSDK files were found in standard paths or by filesystem search.
- Deferred: Validate ARMv7 ELF, SELF, and all three VPK artifacts as soon as the VitaSDK installation is exposed to this build session.
