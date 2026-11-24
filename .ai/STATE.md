# CURRENT RUNTIME STATE

## CURRENT WORK BLOCK
- Objective: Project handheld remaster pipeline complete, including Spatial Upscaling profiles; physical Vita 2000 calibration remains the only external hardware step.
- Progress: TASK 6-12 are complete. Spatial Upscaling offers 960x544 native, 816x462, 720x408, and 640x368; every sub-native profile uses the Mobile GLES 2.0 FSR 1.0 vitaGL pass.
- Blockers: Physical Vita runtime, shader visual calibration, and OpenAL device acoustics await hardware.

## ENCOUNTERED DEAD ENDS & DEFERRED TASKS
- 2026-11-24: First real build failed on the nonexistent `SceKernel_stub`; resolved by linking `SceLibKernel_stub`.
- 2026-11-24: Second link exposed unresolved C++ runtime symbols from vitaGL; resolved by using the C++ linker driver for the C bootstrap target. All three VPKs now build successfully.
- 2026-11-24: First TASK 4 test build failed because the parser used `Cue::hasText`, which was not represented in the cue model. Add the field and rerun the focused test.
- 2026-11-24: The initial Windows-1250 assertion used byte 0x9F (`ź`) while expecting `ż`; corrected the fixture to byte 0xBF. Parser tests then passed.
