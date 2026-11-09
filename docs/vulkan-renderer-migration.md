# Vulkan renderer migration

## Current state

The SDL3/Vulkan runtime is wired to the legacy `RenderBackend` API on macOS,
Linux, Win32, and Android. The scene renderer lives under `libsrc/lgd3d` and
the SDL window and Vulkan surface providers live under `libsrc/dispdev`.
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
and its top, right, and front viewports show editor geometry. On both Linux
architectures, a mission run exits cleanly with `VK_LAYER_KHRONOS_validation`
enabled. The x86_64 DromEd render check uses
`render_test=GOODIES/DROMED/miss1/MISS1.MIS`; the compiled game mission at
`redist/miss1.mis` does not contain editable source brushes and is not a valid
fixture for checking all four editor viewports. The editor trace reports a
valid level with 1,104 world cells, and the captured perspective pane shows
textured surfaces alongside geometry in the three orthographic panes. Windows builds
and runtime parity remain unverified. MinGW syntax checks pass for the SDL
window/event providers, gamepad provider, Vulkan implementation, and DromEd's
editor platform, dialogs, and menu sources. They do not validate the full
Windows target; COM-dependent DromEd sources still need an MSVC build. Android
`assembleDebug` now
succeeds for ARM64 and packages SDL3,
the native engine, and Vulkan SPIR-V;
runtime testing still needs a device. The Android resume path recreates the
SDL Vulkan surface and swapchain while preserving the logical device and engine
textures; this lifecycle path still needs device verification. A macOS visual
capture loaded Thief Gold `MISS1.MIS`; its world surfaces appeared coherently
textured with no obvious stretching. Legacy texture-manager scale factors are
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

1. Validate Vulkan output and legacy scene parity in missions and DromEd on
   macOS, Linux, Win32, and Android, including texture orientation and repeat
   behavior, HiDPI, resize, gamma, and frame lifecycle.
2. Validate Android Vulkan surface startup, touch/gamepad input, and movie
   overlay on a Vulkan-capable device; the ARM64 APK now builds with packaged
   SPIR-V.
3. Validate Windows SDL3/Vulkan startup and rendering on physical hardware;
   the legacy DirectDraw/GDI display path is retired.

Parity checks cover game rendering, editor viewport redraw and preserve
semantics, textures, movies/software UI composition, gamma, resizing/HiDPI,
input-coordinate conversion and shutdown/restart. The final build matrix
must include macOS/MoltenVK, Windows, Linux, and Android Vulkan-capable targets.
