# Vulkan renderer migration

## Current state

The SDL3/Vulkan runtime is wired to the legacy `RenderBackend` API on macOS,
Linux, Win32, and Android. The macOS game/editor targets and Android ARM64 APK
build successfully. The Visual Studio DirectDraw target uses the software/GDI
fallback and has no hardware renderer; the supported Windows hardware path is
the CMake SDL3/Vulkan build. Windows and Linux builds and device/runtime parity
remain unverified. A macOS visual spot-check loaded Thief Gold `MISS1.MIS`; its
world surfaces appeared coherently textured with no obvious stretching. This
does not replace checks across more missions and GPUs. The OpenGL, D3D11, and
SDL2 renderer implementations have been removed.

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
3. Retire the legacy Visual Studio DirectDraw build path or replace it with the
   shared SDL3/Vulkan runtime; it currently provides software/GDI presentation
   only.

Parity checks cover game rendering, editor viewport redraw and preserve
semantics, textures, movies/software UI composition, gamma, resizing/HiDPI,
input-coordinate conversion and shutdown/restart. The final build matrix
must include macOS/MoltenVK, Windows, Linux, and Android Vulkan-capable targets.
