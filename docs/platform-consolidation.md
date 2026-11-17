# Platform consolidation after the ai-rework rebase

## Integration

`macos-support` was rebased onto `origin/ai-rework` at
`3d84942adb0fa4c589ce5e8c89ecae8699e553f3`. The original branch is preserved
as `backup/macos-support-before-ai-rebase-20261117`. No changes were imported
from `vulkan-pcss`.

The upstream ranked console completion and noninteractive editor commands are
retained. Batch commands report errors and reject dialogs instead of blocking
for input. The SDL focus-gain handler retains the Vulkan window-restoration
notification; the obsolete Win32 display implementation remains removed.

## Source ownership

- `libsrc/platform` owns the `dark_platform` library, POSIX/Windows services,
  platform I/O selection, editor compatibility definitions, and Win32 headers.
- `src/entry/main.cpp` provides the shared executable entry point.
- `src/editor` owns the shared editor providers and `uiedit.c` UI. The Cocoa
  editor implementation and native menu bars have been removed.
- `unixsrc` and `winsrc` are removed. Platform selection is inside the platform
  library rather than separate executable source trees.

DromEd uses nativefiledialog-extended v1.4.1, pinned at
`65b2f070a0fcfee0d453a64e770a217ab0b6ff4f` in
`3rdparty/nativefiledialog-extended`. Initialize dependencies with
`git submodule update --init --recursive`. Linux defaults to the portal backend:
building requires D-Bus development headers and runtime dialogs require a
working desktop portal file chooser. Game-only configurations do not include
the dialog library. Windows and macOS select NFDe's native backends.

The dialog adapter preserves engine filters and default extensions, uses UTF-8
paths, rejects oversized paths, and checks overwrite confirmation when adding
an extension changes the selected name. Dialogs execute on the main thread.

## Legacy data layout fixes

Resource file directory headers/entries and compound resource reference tables
use explicit disk packing. Their natural host layouts had caused resource
decompression and editor startup crashes. Resource rectangles are copied to
aligned locals before access.

Brush and grid disk layouts also use explicit packing: the BRLIST brush prefix
is 76 bytes and each terrain face is 10 bytes. Compile-time size assertions
guard these layouts. Newly constructed brushes are zero-initialized to prevent
uninitialized timestamps and disabled grid fields from entering saved files.
Runtime structures containing pointers or synchronization objects retain their
normal host alignment.

## Validation on the current Linux PC

- Release builds of `thief2` and `dromed`; fresh game-only CMake configuration
  and a game-only build using the existing dependency cache.
- Game mission startup and DromEd rendering under Khronos validation with
  synchronization validation enabled, without reported validation errors.
  The final editor check rendered 30 frames and resized its SDL window from
  1024x768 to 1280x720 before exiting normally.
- Actual native dialogs: cancellation, filtered mission selection, UTF-8 save
  names, default extension, overwrite rejection, and insufficient buffer space;
  editor rendering resumes after dismissal.
- Batch save/reload of the editable Mission 2 from `EDITOR/MISSIONS.ZIP`, using
  a private copy of the game configuration. Both exports contain 4,423 brushes:
  1,908 terrain, 291 lights, 7 areas, 1,815 objects, 1 flow, and 401 rooms.
  Every brush record is identical between the first and second exports. This
  checks BRLIST stability, not byte identity of the entire mission database.
- Explicit-path batch saves succeed; an unknown batch command returns failure.

Windows and macOS have not been built or tested for this consolidation.
Historical test results in `vulkan-renderer-migration.md` describe earlier
renderer work and do not establish validation of these changes. Broader manual
mission editing and native window dragging remain outside these smoke checks.
