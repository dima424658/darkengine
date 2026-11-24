# CURRENT RUNTIME STATE

## CURRENT WORK BLOCK
- Objective: TASK 4 - add Vita mod-directory overlays and parse NewDark SUB1 subtitles with Windows-1250 support.
- Progress: Implemented deterministic `mods/` loose-file lookup and the first SUB1 parser/test target. The first host compile failed before tests could run.
- Blockers: `vita_subtitles.cpp` referenced a nonexistent `Cue::hasText` member; track whether a `Text` field was parsed and retry.

## ENCOUNTERED DEAD ENDS & DEFERRED TASKS
- 2026-11-24: First real build failed on the nonexistent `SceKernel_stub`; resolved by linking `SceLibKernel_stub`.
- 2026-11-24: Second link exposed unresolved C++ runtime symbols from vitaGL; resolved by using the C++ linker driver for the C bootstrap target. All three VPKs now build successfully.
- 2026-11-24: First TASK 4 test build failed because the parser used `Cue::hasText`, which was not represented in the cue model. Add the field and rerun the focused test.
