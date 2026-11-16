# Standalone CMake/CTest runner

With a C++17 compiler, CMake, and the Vulkan SDK installed, the renderer core tests
can be built independently of the engine on Linux or Windows:

```sh
cmake -S tools/world_scene_test -B build-world-tests
cmake --build build-world-tests --config Release
ctest --test-dir build-world-tests -C Release --output-on-failure
```

GPU tests return CTest's skipped status when required texture-indexing features
are unavailable. A skipped GPU test does not establish compatibility. The native
model adapter regression still uses the engine compile database via `models.py`.
The standalone harness has been configured and CPU-tested on Linux; Windows
execution remains outstanding.

# World scene regression

No retail data, SDL window, or GPU is needed:

```sh
c++ -std=c++17 -O2 -Ih tools/world_scene_test/main.cpp \
  libsrc/lgd3d/world_scene.cpp -o /tmp/dark-world-scene-test
/tmp/dark-world-scene-test
```

Checks copied texture/mesh ownership, malformed topology, non-finite inputs,
pose/bounds updates, reference lifetime, handle invalidation, and mission reset.
Visibility checks cover all Vulkan frustum planes, boundary crossings,
camera-enclosing bounds, and rotated/mirrored/nonuniformly scaled instances.

For production HDR lighting and alpha-tested shadow pixel readback:

```sh
cmake --build build-linux --target thief2 -j 8
c++ -std=c++17 -O1 -Ih -I3rdparty/tracy/public \
  tools/world_scene_test/lighting.cpp libsrc/lgd3d/vulkan_world_features.cpp \
  libsrc/lgd3d/vulkan_world_renderer.cpp \
  libsrc/lgd3d/world_scene.cpp libsrc/lgd3d/world_shadows.cpp \
  libsrc/lgd3d/vulkan_shadow_maps.cpp libsrc/lgd3d/vulkan_world_geometry.cpp \
  -lvulkan -o /tmp/dark-world-lighting-test
/tmp/dark-world-lighting-test "$PWD/build-linux/generated/vulkan"
```

This also reads back the production tone-map composite over a known background,
checking uncovered sky preservation, opaque replacement, and partial coverage.
The partial-coverage check verifies unpremultiplication before tone mapping.

For the production MD/MM decoders with synthetic packed assets (Linux):

```sh
python3 tools/world_scene_test/models.py build-linux
```

This uses the configured engine's compile database and matrix/fixed-point
libraries. Explicit checks remain active regardless of NDEBUG. Section collection discards the engine
integration functions, so the decoder test needs no retail data or display.
It verifies polygon/UV extraction, joint
pivots, rotation/slide, native nonuniform scaling, cycles, joint parameter bounds,
and malformed vertex indices and polygon lengths. MM cases include rigid and
weighted segments, vertex normals, UVs, material groups, and invalid mappings,
weights and bounds. Native light checks cover authored point/spot/sun conversion
and stale cone/radius values in dynamic-light records.

## Shadow-map replacement foundation

Budget selection and PCSS defaults (no GPU):

```sh
c++ -std=c++17 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -Ih \
  tools/world_scene_test/shadow_budget.cpp libsrc/lgd3d/world_shadows.cpp \
  libsrc/lgd3d/world_scene.cpp -o /tmp/dark-shadow-budget-test
/tmp/dark-shadow-budget-test
```

GPU geometry without RT features or device addresses:

```sh
c++ -std=c++17 -O1 -Ih -I3rdparty/tracy/public \
  tools/world_scene_test/geometry.cpp libsrc/lgd3d/vulkan_world_geometry.cpp \
  libsrc/lgd3d/vulkan_world_features.cpp libsrc/lgd3d/world_scene.cpp \
  -lvulkan -o /tmp/dark-world-geometry-test
/tmp/dark-world-geometry-test
```

The device requests only sampled-texture indexing. Exact GPU vertex/index readback
checks independent frame slots, reusable pose buffers, and stale-resource release.
These foundation tests do not yet render shadow maps or PCSS.

Shadow cameras and actual raster depth maps:

```sh
c++ -std=c++17 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -Ih \
  tools/world_scene_test/shadow_cameras.cpp libsrc/lgd3d/world_shadows.cpp \
  libsrc/lgd3d/world_scene.cpp -o /tmp/dark-shadow-camera-test
/tmp/dark-shadow-camera-test
cmake --build build-linux --target thief2 -j 8
c++ -std=c++17 -O1 -Ih -I3rdparty/tracy/public \
  tools/world_scene_test/shadow_maps.cpp libsrc/lgd3d/vulkan_shadow_maps.cpp \
  libsrc/lgd3d/vulkan_world_geometry.cpp libsrc/lgd3d/vulkan_world_features.cpp \
  libsrc/lgd3d/world_scene.cpp libsrc/lgd3d/world_shadows.cpp \
  -lvulkan -o /tmp/dark-shadow-map-test
/tmp/dark-shadow-map-test "$PWD/build-linux/generated/vulkan"
```

The GPU test reads actual point/spot/sun depth maps and verifies cutouts, blended
and invisible casters, instance movement, and vertex deformation in both slots.
It does not yet test PCSS sampling or the in-game shadow-map path.

## Offscreen RenderDoc capture (Linux)

With RenderDoc headers and its shared library installed, capture the production
shadow and HDR passes without opening an SDL window:

```sh
cc -shared -fPIC tools/world_scene_test/renderdoc_capture.c -ldl -o /tmp/dark-test-capture.so
DARK_TEST_CAPTURE=/tmp/dark-shadow-offscreen LD_PRELOAD=/tmp/dark-test-capture.so:/usr/lib/librenderdoc.so \
  ./build-world-tests/test_lighting ./build-world-tests/shaders
```

Adjust the shared library path for your installation. The helper starts capture
at the first command buffer and ends after its submitted fence completes. It is
only intended for these single-threaded standalone tests. Open the resulting
`/tmp/dark-shadow-offscreen_capture.rdc` with RenderDoc or its MCP server.
The first lighting submission contains six point-light depth passes and the HDR
receiver pass. This capture does not cover native mission integration or the
later finite-emitter tests.

## Vulkan synchronization validation

With `VK_LAYER_KHRONOS_validation` installed or exposed through `VK_LAYER_PATH`,
run the GPU suite with synchronization checking enabled:

```sh
VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation VK_LAYER_VALIDATE_SYNC=1 \
  ctest --test-dir build-world-tests --output-on-failure
```

Inspect `build-world-tests/Testing/Temporary/LastTest.log` for validation messages
as well as the test exit status: validation output alone does not fail these
executables. The same environment settings apply to retail mission runs.

## Automatic legacy fallback (Linux)

The CPU `ranges` test checks geometry arena splits and coalescing against an
independent occupancy oracle over 20,000 fragmented mixed-alignment operations.
It also checks recovery of the complete arena and alignment-overflow rejection.
GPU geometry/lighting suites cover the allocator's integration and frame slots.

To exercise scalar shadow submission with the optional indirect device features
disabled, run the GPU suites with `DARK_TEST_SCALAR_SHADOWS=1`. The default GPU
run enables batching when supported. Tests verify actual multi-command batches,
per-instance transforms/materials, and an exact full depth-image comparison
against scalar submission. Runtime `renderer_shadow_indirect=0` keeps the device
features enabled but forces scalar draws for profiling.

For an injected allocation failure during retail world recording (Linux x86-64):

```sh
python3 tools/world_scene_test/native_allocation_failure.py /path/to/test-game --portal-drawing 0
python3 tools/world_scene_test/native_allocation_failure.py /path/to/test-game --portal-drawing 1 --skip-allocations 16
```

The fixture compiles a temporary Vulkan allocation wrapper and arms it at the
first production world-recording call using GDB. Exactly one allocation returns
`VK_ERROR_OUT_OF_DEVICE_MEMORY`; later allocations run normally. It requires
the production error message, continued native terrain drawing, no suppression
of that drawing after failure, and normal process exit. `--skip-allocations N`
permits N allocations before failing one, to cover partial renderer setup.
Enable Khronos synchronization validation with the usual environment variables;
any validation error still fails the fixture. For an isolated X11 run set
`SDL_VIDEODRIVER=x11` and `DISPLAY` to the test server. This verifies a recoverable
single allocation failure, not sustained exhaustion, device loss or queue-submit
failure. The wrapper is a test artifact and is never linked into the game.

For a rejected modern queue submission and subsequent renderer recovery:

```sh
python3 tools/world_scene_test/native_submit_failure.py /path/to/test-game --portal-drawing 0
python3 tools/world_scene_test/native_submit_failure.py /path/to/test-game --portal-drawing 1 --after 12 --static-depth
```

This Linux x86-64 fixture compiles a temporary `vkQueueSubmit` wrapper. It rejects
exactly one submission containing the modern preparation and frame command
buffers with `VK_ERROR_OUT_OF_DEVICE_MEMORY`, leaving that submission unexecuted.
Upload submissions proceed normally. The fixture requires renderer reinitialization,
successful modern submissions after the failure and normal shutdown; Vulkan
validation errors fail it. `--after N` permits N successful modern submissions
before injecting the error, and checks their count. `--static-depth` also covers
the optional stable depth bases. `--equip-sword` invokes the native
`inv_select sword` command before the first recorded world frame; use a longer
warmup (for example `--after 90`) to let the raise animation finish before the
failure. The fixture checks that the command was requested; actual overlay
appearance is checked separately in a captured frame. This tests recoverable submit rejection, not
actual hardware device loss or the visual contents of the recovered frames.
Use `--log /path/to/log` to retain the complete native/GDB output. Optional
`DARK_TEST_INFERIOR_PRELOAD` libraries are loaded only into the game for capture
instrumentation; if `DARK_NATIVE_CAPTURE` is set, the capture helper's readiness
log is set to the fixture's native output file.

For a native render-camera yaw sweep during shadow profiling (Linux x86-64):

```sh
python3 tools/world_scene_test/native_shadow_camera.py /path/to/test-game --static-depth --trace /tmp/shadow-camera.log
python3 tools/world_scene_test/native_shadow_camera.py /path/to/test-game --static-depth --yaw-step 0
```

This fixture (Mission 2 by default) changes only the yaw in `cam_render_scene`'s incoming
stack copy of the render-camera position; it does not write player position or
camera ownership. The default sweep advances about one degree per frame for
180 frames at 1920x1080. `--frames`, `--yaw-step`, `--log` and `--build` allow
controlled runtime checks. `--mission 1` selects Mission 1; `--visibility 0/1`
and `--portal-drawing 0/1` explicitly override the respective renderer settings.
Omitting those overrides exercises production defaults. It requires modern rendering, the requested scene
count and normal shutdown, and rejects Vulkan validation errors. GDB's stops
affect scheduling and wall-clock intervals: use its traces as diagnostics, not
as gameplay FPS evidence.

For stationary performance measurements without GDB (Linux):

```sh
python3 tools/world_scene_test/native_performance.py /path/to/test-game --static-depth 0 --visibility 0 --portal-drawing 1 --output /tmp/native-default.json
python3 tools/world_scene_test/native_performance.py /path/to/test-game --static-depth 1 --visibility 0 --portal-drawing 1 --output /tmp/native-static.json
```

This runs the normal executable for 300 Mission 2 frames at 1920x1080, discards
60 warmup samples, and reports frame submission intervals plus GPU/CPU timings
as JSON. It requires all requested modern records and timing groups, a successful
exit and no Vulkan validation/fatal errors. `--mission`, `--frames`, `--warmup`
and `--build` adjust the fixture; omitting a renderer override uses the game's
configuration. `--trace` preserves a raw trace (replacing the selected output),
and `--log` preserves native output. Run timing comparisons without validation
and keep display/compositor settings consistent. The view is stationary: these
results do not prove sustained traversal FPS. `fps_from_mean_interval` is a
submission-rate equivalent, not a count of frames displayed by the compositor.

For native water and late effects with modern opaque drawing (Linux x86-64):

```sh
python3 tools/world_scene_test/native_water.py /path/to/test-game --portal-drawing 0
python3 tools/world_scene_test/native_water.py /path/to/test-game --portal-drawing 1
```

This fixture detaches the render camera into the tested Mission 2 water view
without moving the player. It checks that water and observed late effects reach
the backend unsuppressed, excluding intentionally suppressed opaque model calls,
and requires modern initialization and normal shutdown. Enable Khronos validation
through the usual environment variables; validation errors fail the fixture.
The GPU lighting suite separately reads back native linear/reciprocal depth,
including pitched/translated views, masked holes, blended-over-opaque surfaces,
both slots and switching the bridge off. The optional bridge checks report a
skip if R32 float sampling/color blending is unavailable. The native fixture
uses the current Linux x86-64 camera layout and retail Mission 2 data.

For a rendered retail Mission 1 emitter transition:

```sh
python3 tools/world_scene_test/native_torch_camera.py /path/to/test-game
```

This Linux x86-64/GDB fixture finds a clear initial line of sight using copied
terrain/model triangles (including alpha masks), checks native cells along a
camera-only route and renders 122 frames. It reads the production renderer's
actual selection after recording: the emitter must remain covered throughout
the route and exercise both distant and active pools. `--cache 0` instead
reproduces the missing distant coverage. It never moves the native player or
physics, so it does not validate player traversal or representative performance.
With the tested vanilla data, the route approaches fireplace flame 805 from
60 to 2 feet. The fixture name retains "torch" because the same flame model and
light path motivated the original diagnostic; this is a fireplace test.

The opt-in native terrain visibility path can be checked with:

```sh
python3 tools/world_scene_test/native_visibility.py /path/to/test-game --visibility 1 --portal-drawing 0
```

Run with `--visibility 0` to check the default conservative path, or
`--portal-drawing 1` to retain native world drawing. The enabled fixture also
invalidates the copied snapshot, checks that all terrain becomes visible again,
then verifies the next frame restores filtering. Hidden chunks retain caster
flags. The fixture independently reads native object reference lists and checks
model visibility against their union, including objects spanning visible and
hidden cells. Use `--mission 2` for the larger retail scene. Attachments retain
conservative visibility. Missing snapshots restore all raster drawing.

The enabled terrain path separates per-cell raster receivers from merged shadow
casters. The fixture compares complete triangle multisets (attributes, material,
transform and fog), checks one caster copy, and verifies snapshot fallback draws
the merged copy while hiding the raster-only duplicates.

To diagnose Mission 1 torch selection on a copied native scene:

```sh
python3 tools/world_scene_test/native_torch_budget.py /path/to/test-game
```

This samples six synthetic camera distances for torches 448, 805 and 876 at the
default and expanded shadow budgets. It compiles the current CPU selection and
scene sources into a read-only helper and leaves the native player unchanged.
Results identify emitter ownership and selection changes; successful execution
does not assert visual correctness or that those camera positions are traversable.

Add `--cache` to require a valid plan entry for every sampled torch with the
experimental distant pool enabled. The GPU lighting regression separately
checks point/spot occlusion outside a zero-sized active budget, caster removal
and restoration, both frame slots, and returning to the default active pool.

The lighting test also renders nine camera positions across an active/distant
priority crossover and back. Each position checks both frame slots against its
unoccluded HDR reference and requires the receiver to remain below 20% radiance.
The log must exercise both active and cached selection states.

The standalone lighting regression includes an explicit expanded budget (18
emitters, 23 faces at 32-square resolution) through the production settings path,
and restores the default targets in both retired frame slots afterward.

Camera-plan tests also cover retained local layer ranges and fragmentation.
The shadow-map GPU test removes a point light before an unchanged spotlight,
verifies the spotlight retains its depth and array layer despite the changed
view-buffer index, and checks zero redraws in both frame slots.

It also warms an excluded cube face, moves/rotates a caster into it, and checks
the resulting radial depth against the analytic center-texel ray intersection.
Unchanged faces reuse geometric membership; moving bounds force new tests.

The retail Mission 2 lamppost adapter regression can be run separately:

```sh
python3 tools/world_scene_test/native_lamppost.py /path/to/test-game
```

It checks object 234's five shaded housing parts, two authored glowing lens
parts, caster flags, and copied light ownership at the native light-source vhot.
It requires GDB and a Linux game build. GPU emitter self-shadow behavior is
covered separately by the standalone lighting test.

The test-only shim hides `VK_EXT_descriptor_indexing` from device extension
queries. It does not change production renderer settings or system drivers:

```sh
c++ -std=c++17 -shared -fPIC tools/world_scene_test/hide_indexing.cpp -ldl -o /tmp/dark-hide-indexing.so
LD_PRELOAD=/tmp/dark-hide-indexing.so ./build-linux/thief-runner \
  game_path=/path/to/test-game content=vanilla mission_loop_test=1 auto_stop=12 renderer_mode=modern
```

Expect the missing indexing reason and `retaining legacy renderer`, successful
mission rendering and normal exit. This tests a simulated missing capability,
not actual older hardware.

## Windows cross-build verification

The same standalone project can be cross-built with MinGW-w64 by setting
`CMAKE_SYSTEM_NAME=Windows`, `CMAKE_CXX_COMPILER` to the Windows compiler, and
`Vulkan_INCLUDE_DIR`/`Vulkan_LIBRARY` to Windows-compatible Vulkan SDK inputs.
Use the host `glslc` through `Vulkan_GLSLC_EXECUTABLE`. A cross-build can run CTest
under Wine with `CMAKE_CROSSCOMPILING_EMULATOR=/usr/bin/wine` and a dedicated
`WINEPREFIX`. The Vulkan library must be a Windows import library; the host Linux
`libvulkan.so` cannot be used. Passing Wine tests does not replace native Windows
game/editor and driver validation.

## Linux native material-property invalidation

With a built Linux game target, GDB, and a test game directory containing retail
Thief 2 vanilla Mission 1 data:

```sh
python3 tools/world_scene_test/material_properties.py /path/to/test-game
```

This runs 24 modern-renderer frames and uses symbolic debugger calls to mutate
object 11's alpha/self-illumination and shadow-casting state, its archetype
values, and a temporary metaproperty. It verifies the renderer's observed values after concrete edits,
removal, archetype edits, and hierarchy add/remove. Edits are confined to the
new game process; no mission is saved. The fixture is specific to retail Mission
1's object layout. It can inherit Vulkan validation environment variables and
fails on validation errors or synchronization hazards. Its timeout terminates
the debugger's process group, including the game. Windows/DromEd are deferred.

## Linux native model reload

On Linux x86-64, with the built game, GDB and retail Mission 1 test data:

```sh
python3 tools/world_scene_test/model_reload.py /path/to/test-game
```

The 12-frame fixture reloads object 11's native model slot while holding the
resource resident, requiring the pointer and byte extent to remain identical.
It checks an advancing load revision, zero revision for an invalid handle, one
modern rebuild, a complete modern import afterward, and no additional revision
change on the next frame. The fixture preserves the slot's original reference
count and releases its temporary resource lock/reference. No mission is saved.
The GDB script uses the Linux x86-64 ModelHandle layout and IUnknown/IDataSource
vtable slots; it is not a cross-platform test. Vulkan validation variables can be
inherited; validation errors, VUIDs, sync hazards and fatal signals fail the run.

## Linux unloaded texture palette snapshots

```sh
python3 tools/world_scene_test/palette_snapshot.py /path/to/test-game
```

This Linux x86-64/GDB fixture builds a small read-only helper against the scene
store's C++ types. In a disposable retail Mission 1 process it creates an unloaded
1x1 indexed bitmap, obtains the production model adapter's copied texture, and
changes its palette entry from red to blue. It requires updated copied RGBA,
the same scene texture handle, and a stable revision for an unchanged palette
write. The original palette entry is restored before the game continues. No
mission is saved. The script uses the native bitmap layout and little-endian
RGBA values; it is not a cross-platform test. It accepts Vulkan validation
variables and fails on validation errors, VUIDs, sync hazards or fatal signals.

The scene test also checks owned receiver-light lists and invalid-input/no-op
behavior. The lighting GPU test checks empty/default lists, emitter exclusion,
switching GPU light indices, and removal of a linked light using HDR readback.
These generic tests do not establish native per-cell lighting integration.

## Linux native cell-light reach

```sh
python3 tools/world_scene_test/native_cell_lights.py /path/to/test-game
```

This Linux x86-64 retail Mission 1 fixture independently reads the native cell's
static light indices, translates them through copied light handles, and checks
all imported parts of object 11. It adds a native dynamic light for one frame,
requires eligibility in every linked receiver, then checks removal from both
the light store and all receiver lists. Finally it moves the object through the
native position API into a cell with a different static-light count and checks
its updated copied list. Changes affect only the disposable process; no mission
is saved. A small read-only helper uses the native Linux PortalCell/Location
layout and the copied scene's C++ types. Validation variables are inherited;
validation errors, VUIDs, sync hazards and fatal signals fail the fixture.

Receiver GPU regressions also cover enabled-light remapping, a resized target
with a different tile-index prefix, and queued frame-slot isolation after a
receiver list changes. They exercise the renderer's cached list packing.
