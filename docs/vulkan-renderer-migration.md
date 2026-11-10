# Vulkan renderer migration

## Current state

The SDL3/Vulkan runtime is wired to the legacy `RenderBackend` API on macOS,
Linux, Win32, and Android. The scene renderer lives under `libsrc/lgd3d` and
the SDL window and Vulkan surface providers live under `libsrc/dispdev`.
SDL appcore, display, game-shell, input, movie, script, timer, and disk-free
providers are compiled as separate CMake libraries; the executable keeps a
single interface target to link the provider set.
`CMakePresets.json` provides Ninja Release presets for macOS and Linux; the
macOS and Linux presets build `thief2` and DromEd. Windows presets build both
targets as well. Both `thief2` and DromEd now build on Debian 13 for x86_64
and ARM64 with GCC 14 and Vulkan-Hpp 1.4.
Windows uses the same SDL3/Vulkan display path as the other desktop platforms;
its DromEd dialogs and resize hooks also use the shared SDL providers. The
Win32 DirectDraw/GDI provider and renderer stub have been removed. Linux
builds are verified for x86_64 and ARM64. On x86_64, the game launches and
renders a mission under Xvfb with lavapipe. DromEd also loads `MISS1.MIS` under
Xvfb with lavapipe: its perspective viewport shows textured world surfaces,
and its top, right, and front viewports show editor geometry. DromEd also
loads this mission on ARM64, captures all four viewports, and exits cleanly
with `VK_LAYER_KHRONOS_validation` and synchronization validation enabled.
The ARM64 run exposed and fixed a precedence bug in the editor's face-aligned
primal lookup, which had indexed beyond the primal table and corrupted wedge
metadata. The x86_64 DromEd render check uses
`render_test=GOODIES/DROMED/miss1/MISS1.MIS`; the compiled game mission at
`redist/miss1.mis` does not contain editable source brushes and is not a valid
fixture for checking all four editor viewports. The editor trace reports a
valid level with 1,104 world cells, and the captured perspective pane shows
textured surfaces alongside geometry in the three orthographic panes. The
Linux x86_64 DromEd view was also resized under Xvfb from 1024x768 to
1280x900; the post-resize in-game capture retained all four panes and the
process exited with status 0. Additional x86_64 runs of `miss1.mis`,
`miss2.mis`, `miss3.mis`, `miss4.mis`, `miss5.mis`, `miss12.mis`, and
`miss18.mis`, plus `miss8.mis` with `obj_min=-16384`, exit without Vulkan
validation errors. Those missions exposed a point-list pipeline validation
error; the shared vertex shader now writes a point size of 1 pixel. The
x86_64 and ARM64 DromEd mission captures also exit without validation errors
after this shader change. `miss2.mis` captures at gamma 0.5, 1.0, and 1.5
show successively lower mean RGB values (approximately 60, 18, and 7), matching
the game's gamma direction. A clean Windows MSVC Release build using vcpkg
manifest mode installed SDL3 with Vulkan support, FFmpeg, libpng, and the
available vcpkg Squirrel package, then built both `thief-runner.exe` and
`DromEd.exe`. That build used the available vcpkg Squirrel 3.1 package; the
project's required Squirrel 3.2 source dependency still needs separate
validation on Windows. The manifest-built game opened
an 800x600 Vulkan window and loaded `miss1.mis` cleanly. Its DromEd
`render_test=GOODIES/DROMED/miss1/MISS1.MIS` run loaded 690 source brushes,
reconstructed 195 object brushes, and captured the textured perspective pane
plus top, right, and front geometry panes. A vcpkg SDL3 install without its
`vulkan` feature can build successfully but cannot create a Vulkan surface at
runtime. After fixing the face-aligned primal lookup and adding the point-size
shader output, the Windows MSVC game and DromEd builds pass, and the mission
capture exits with status 0. The separate Windows game
capture showed coherent world textures and the HUD without obvious stretching.
Windows DromEd was also resized through SDL from its initial client size to
1280x900 after loading `MISS1.MIS`; the captured image is 1280x900 with all
four panes present, and the process exited with status 0. Interactive manual
window dragging and broader editor interaction remain to be checked. Windows
captures of `miss1.mis` and `miss2.mis`, plus the resized DromEd mission, also
exit cleanly on an NVIDIA GeForce RTX 4070 Ti with Khronos synchronization
validation enabled and no validation messages in their saved logs. The full
manifest-built game and DromEd were rechecked with the same validation layer;
both exited successfully, including DromEd's 1280x900 resize capture. The
`miss2` capture shows coherent world textures, lighting, and HUD. Resize events
immediately update the swapchain and dependent resources. The vcpkg manifest
includes FFmpeg; a reduced
Squirrel-disabled Windows startup run decoded `INTRO.AVI` through FFmpeg and
presented video frames through Vulkan in an 800x600 window. Its capture showed
the 320x240 movie scaled to the full 4:3 viewport, and the validation log had
no Vulkan errors. macOS DromEd also
completed the automated resize run with
status 0; the HiDPI capture was 2560x1694 and retained all four panes. The run
completed rendering and resize checks. The Homebrew validation-layer manifest
must be added through `VK_LAYER_PATH` on this host for the Vulkan loader to
discover it. MinGW syntax checks pass for the SDL window/event
providers, gamepad provider, Vulkan
implementation, and DromEd's editor platform, dialogs, and menu sources.
COM-dependent DromEd sources have also passed the MSVC build. The ARM64 Android
Debug APK now builds successfully on macOS with Android SDK platform 36, build
tools 36.0.0, NDK 30.0.16248370, CMake 3.31.6, and the Gradle 9.6.0 wrapper.
The APK includes the native engine and compiled Vulkan SPIR-V. No Android
device is attached, so runtime rendering, input, and the resume path remain
unverified. Following the SDL provider target split, the macOS and Android
builds succeed, and the Windows MSVC build links both executables. Windows
Thief loaded `miss1.mis` and exited cleanly; DromEd loaded the editor mission,
resized to 1280x900, captured all four panes, and exited cleanly. The Android
resume path recreates the SDL Vulkan surface and
swapchain while preserving the logical device and engine textures. On macOS,
`thief-runner quickstart=miss1.mis auto_stop=180 game_hardware=1` loads the
mission from `redist`, renders the textured environment and HUD, and exits with
status 0.
Its 800x600-point SDL window produces a 1600x1200-pixel drawable capture on
the HiDPI display. macOS `MISS1.MIS` captures at gamma 0.5
and 1.5 have mean RGB values of approximately (73.5, 65.2, 59.3) and
(10.0, 6.8, 6.1), respectively, confirming the Vulkan gamma path changes
output in the expected direction. Legacy texture-manager scale factors are
now applied during RGBA expansion by repeating source texels and rows, with
dimension and row-stride validation before upload. RGBA uploads copy rows
directly to mapped Vulkan staging memory, avoiding a second full-size CPU
buffer. This does not replace checks across more missions and GPUs. The CPU
canvas uses one Vulkan texture per
in-flight frame slot and updates it after that slot's fence completes, avoiding
image allocation on each changed frame. A macOS `MISS1.MIS` capture after this
change showed the environment and HUD rendering. Movie playback reuses a Vulkan
texture for decoded frames instead of waiting for the whole device after
destroying a per-frame texture. Linux movie playback decoded `B01.AVI` and
presented it through Vulkan. After resizing the window to 1280x720, an in-game
capture showed the 4:3 movie centered with black side bars. The OpenGL, D3D11,
SDL2, and legacy Win32
display renderer implementations have been removed.

Additional macOS Vulkan captures of `miss3.mis` and `miss4.mis` both rendered
textured environments at the 1600x1200 HiDPI drawable size and exited with
status 0. Texture preflight found no missing terrain surfaces, object models,
object textures, checkerboards, or replacements in either mission. The `miss4`
run used the Khronos validation layer with synchronization validation enabled;
its log contained no Vulkan validation messages.

After the SDL providers were split into subsystem-specific CMake libraries,
the Linux x86_64 build linked both executables, and the Windows MSVC Release
build linked `thief-runner.exe` and `DromEd.exe`. The Windows game loaded
`miss1.mis` and `miss2.mis` and exited with status 0. Fresh in-game captures
show textured world surfaces, lighting, and HUD in both missions. DromEd loaded
`GOODIES/DROMED/miss1/MISS1.MIS` and exited with status 0 after capturing the
textured perspective viewport and all three orthographic viewports. The macOS
and Android builds also passed after the provider split. DromEd's post-load
resize path was rerun on this Windows build: a 1280x900 capture retained all
four viewports and the process exited with status 0. A second DromEd capture
used `render_test_angles=0,90,0`; the diagnostic log confirms the mission load
completed and the alternate heading was applied, and its capture retained the
textured perspective view plus all orthographic geometry. The current Windows
`miss2.mis` run loaded `VK_LAYER_KHRONOS_validation` from the SDK, exited with
status 0, and logged no validation errors, warnings, or VUIDs. Android runtime
rendering remains unverified because no device is attached.

The latest macOS provider-split build also passed runtime checks: Thief loaded
`miss2.mis` and exited with status 0, with a 1600x1200-pixel HiDPI in-game
capture showing the textured environment and HUD. macOS DromEd loaded
`MISS1.MIS`, ran the post-load resize check, captured all four viewports at
2560x1694 drawable pixels, and exited with status 0.
With `VK_LAYER_PATH` set to the Homebrew manifest directory and
`VK_LAYER_VALIDATE_SYNC=1`, the same DromEd resize run loaded
`VK_LAYER_KHRONOS_validation` and exited with no validation errors, warnings,
VUIDs, or synchronization hazards.
The macOS `miss18.mis` run also passed synchronization validation. Its texture
audit found 19,998 terrain surfaces with no missing terrain textures and one
missing object texture: `pldguard.gif` for model `pdguard` (`Plaid1`), which is
absent from `redist`. This accounts for the checkerboard fallback in that
mission capture; the terrain texture path reports no missing resources.
The same macOS audit on `miss1.mis` and `miss2.mis` found 6,067 and 26,982
terrain surfaces, respectively, with zero missing terrain textures, models,
object textures, or replacement textures. Both runs used synchronization
validation and reported no Vulkan validation messages.
On macOS, a game script invoked `movie intro.avi`; FFmpeg decoded the full
128.67-second clip and the game exited with status 0 afterward. An in-game
capture showed the 320x240 movie frame presented across the 1600x1200 HiDPI
drawable. Synchronization validation reported no Vulkan messages.

## Goal

Replace the OpenGL/OpenGL ES and D3D11 renderers with one Vulkan renderer that
implements `h/render_backend.h`. Keep the legacy engine-facing `RenderBackend*`
calls stable while moving graphics API work behind that interface. Vulkan-Hpp
is the C++ API for Vulkan objects and commands; shader source is compiled to
SPIR-V with `glslc`.

## Backend boundaries

The Vulkan renderer owns the instance, selected physical device, logical
device, queues, swapchain, frame synchronization, scene attachments, graphics
pipelines, texture images, descriptor sets, and command recording. Window
systems only provide the native surface and drawable size:

- SDL3 creates surfaces on macOS, Linux, Win32, and Android.
- The rendering and resource implementation remains shared across these
  surface providers.

`RenderBackendBeginFrame` and `RenderBackendEndFrame` are hardware-scene
operations, not necessarily swapchain presentation. DromEd can issue several
hardware scene passes while rendering its viewports. `DarkRenderBegin/EndApplicationFrame`
is the outer frame boundary where one swapchain image is acquired, completed,
and presented. `DarkRenderPresent` contributes the legacy software canvas to
that final image.

## Required behavior

- `RenderBackendDraw` snapshots primitive, vertex data, texture bindings,
  depth, blend, alpha-test, fog, and sampler state for each draw.
- The two texture slots and legacy RGBA texture update/create/destroy calls
  map to Vulkan images, image views, samplers, and descriptors.
- Scene color and depth attachments persist across viewport passes when the
  legacy preserve-canvas mode is active. The non-preserving mode clears them.
- `RenderBackendClearCanvasRegion` clears a logical rectangle after converting
  it to drawable pixels, with top-left logical coordinates.
- The CPU canvas is uploaded as a texture and composited over the Vulkan scene;
  legacy black transparency-key behavior and gamma behavior are preserved.
- Device loss, swapchain resize, and shutdown release resources in dependency
  order and can recreate swapchain-dependent objects.

## Migration sequence

1. Extend visual parity checks across more missions and GPUs on macOS, Linux,
   and Windows. Check interactive DromEd redraw and manual resize behavior on
   Windows.
   Include texture orientation and repeat behavior, HiDPI, gamma, and frame
   lifecycle.
2. Validate Android Vulkan surface startup, touch/gamepad input, and movie
   overlay on a Vulkan-capable device; the ARM64 APK now builds with packaged
   SPIR-V.
3. Windows game and initial DromEd mission rendering are verified on physical
   hardware; verify broader gameplay parity and editor interaction there.

Parity checks cover game rendering, editor viewport redraw and preserve
semantics, textures, movies/software UI composition, gamma, resizing/HiDPI,
input-coordinate conversion and shutdown/restart. The final build matrix
must include macOS/MoltenVK, Windows, Linux, and Android Vulkan-capable targets.
