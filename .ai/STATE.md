# CURRENT RUNTIME STATE

## CURRENT WORK BLOCK
- Objective: TASK 5 - add separate Vita Slim brightness and gamma shader controls.
- Progress: TASK 4 is implemented: Vita loose-file lookup discovers `mods/` overlays ahead of base assets, and the SUB1 parser handles NewDark declarations, duplicate IDs, timed cues, comments, escaped text, SRT references, and Windows-1250-to-UTF-8 conversion. Focused host tests pass, and the parser/path sources cross-compile into all three ARMv7 test VPKs.
- Blockers: The current VPKs are still bootstrap packages; full game-engine integration and on-device subtitle presentation remain unverified. Proceed with TASK 5.

## ENCOUNTERED DEAD ENDS & DEFERRED TASKS
- 2026-11-24: First real build failed on the nonexistent `SceKernel_stub`; resolved by linking `SceLibKernel_stub`.
- 2026-11-24: Second link exposed unresolved C++ runtime symbols from vitaGL; resolved by using the C++ linker driver for the C bootstrap target. All three VPKs now build successfully.
- 2026-11-24: First TASK 4 test build failed because the parser used `Cue::hasText`, which was not represented in the cue model. Add the field and rerun the focused test.
- 2026-11-24: The initial Windows-1250 assertion used byte 0x9F (`ź`) while expecting `ż`; corrected the fixture to byte 0xBF. Parser tests then passed.
