# CURRENT RUNTIME STATE

## CURRENT WORK BLOCK
- Objective: TASK 4 - add Vita mod-directory overlays and parse NewDark SUB1 subtitles with Windows-1250 support.
- Progress: Fixed the parser compile error and built the focused host test. The NewDark syntax and mod overlay checks pass, but the Windows-1250 Polish assertion fails.
- Blockers: Investigate the CP1250 test byte sequence/decoded output, correct the encoding path, and rerun the focused test.

## ENCOUNTERED DEAD ENDS & DEFERRED TASKS
- 2026-11-24: First real build failed on the nonexistent `SceKernel_stub`; resolved by linking `SceLibKernel_stub`.
- 2026-11-24: Second link exposed unresolved C++ runtime symbols from vitaGL; resolved by using the C++ linker driver for the C bootstrap target. All three VPKs now build successfully.
- 2026-11-24: First TASK 4 test build failed because the parser used `Cue::hasText`, which was not represented in the cue model. Add the field and rerun the focused test.
- 2026-11-24: The parser compiled, but the Windows-1250 Polish test failed. Inspect the literal and code-page conversion before continuing.
