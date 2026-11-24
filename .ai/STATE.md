# CURRENT RUNTIME STATE

## CURRENT WORK BLOCK
- Objective: TASK 10 - implement the Ghost Check counter and Vita-safe mission/readback feedback.
- Progress: TASK 6-9 are implemented and verified. Three Vita VPKs build locally and contain `eboot.bin`; THFG00001/2/3 title metadata and names are correct. TASK 7 VRAM, TASK 8 EFX, and TASK 9 30/60 FPS modules pass host regressions.
- Blockers: Ghost Check UI semantics require a host-safe mission summary contract before engine integration.

## ENCOUNTERED DEAD ENDS & DEFERRED TASKS
- 2026-11-24: First real build failed on the nonexistent `SceKernel_stub`; resolved by linking `SceLibKernel_stub`.
- 2026-11-24: Second link exposed unresolved C++ runtime symbols from vitaGL; resolved by using the C++ linker driver for the C bootstrap target. All three VPKs now build successfully.
- 2026-11-24: First TASK 4 test build failed because the parser used `Cue::hasText`, which was not represented in the cue model. Add the field and rerun the focused test.
- 2026-11-24: The initial Windows-1250 assertion used byte 0x9F (`ź`) while expecting `ż`; corrected the fixture to byte 0xBF. Parser tests then passed.
