# CURRENT RUNTIME STATE

## CURRENT WORK BLOCK
- Objective: TASK 6 - add three physical control layouts and temporary touch trigger icons.
- Progress: TASK 4 and TASK 5 are implemented and verified. TASK 5 adds independent brightness/gamma sliders to the Vita bootstrap, GLES2 correction shaders, `vita_options.cfg` persistence, D-pad/touch adjustment, and host regression tests. All three Vita ARMv7 VPKs build.
- Blockers: Gamma/brightness ranges are conservative and await visual calibration on a physical Vita 2000. The VPKs remain bootstrap packages rather than full game-engine builds. Proceed with TASK 6.

## ENCOUNTERED DEAD ENDS & DEFERRED TASKS
- 2026-11-24: First real build failed on the nonexistent `SceKernel_stub`; resolved by linking `SceLibKernel_stub`.
- 2026-11-24: Second link exposed unresolved C++ runtime symbols from vitaGL; resolved by using the C++ linker driver for the C bootstrap target. All three VPKs now build successfully.
- 2026-11-24: First TASK 4 test build failed because the parser used `Cue::hasText`, which was not represented in the cue model. Add the field and rerun the focused test.
- 2026-11-24: The initial Windows-1250 assertion used byte 0x9F (`ź`) while expecting `ż`; corrected the fixture to byte 0xBF. Parser tests then passed.
