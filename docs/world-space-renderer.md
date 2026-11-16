# World-space renderer

`renderer_mode=modern` enables experimental HDR direct lighting with Vulkan
shadow maps. `modern_rt` remains a compatibility alias; the renderer no longer
requests RT features or builds acceleration structures. Legacy rendering remains
the default and the fallback when modern initialization fails. Select
`renderer_shadow_filter=hard|pcf|pcss`; PCSS is the default.

Current validation scope is Linux game only; Windows and DromEd work is deferred at the user's request.

Modern receivers now use copied native per-cell static-light reach, with
conservative unions for existing terrain chunks and current-cell lists for
models. Dynamic lights remain eligible separately. `renderer_cell_lights=0`
disables this copied linking for diagnostics; native lighting/stealth data is
never edited by the adapter.

The renderer is not release-complete. Native Windows engine execution, target-hardware/frame
budget validation, attachment visual checks, several material/scene paths, and broader visual
quality checks remain outstanding. See [shadow-map development](shadow-map-renderer.md)
for implementation details and evidence.

## Scene ownership and native adapters

`h/render_scene.h` is a C-compatible, renderer-neutral copied-data interface for
indexed meshes, deformable vertices, instances, RGBA textures, materials, lights,
camera state, and optional owned receiver light lists. Handles are monotonic process-local IDs. Reset invalidates all
handles, and referenced assets cannot be destroyed before their users. Use the
API on the engine/render thread. Transforms are column-major invertible affine
matrices. Base-color textures are sRGB with linear alpha; material/light values
are linear. Unchanged texture updates preserve their revision.

The CPU scene and adapter caches survive legacy AppOnExit static-destruction
ordering; database reset releases their contents. Vulkan resources use two
independent frame slots. Callers retire the slot fence before reuse and all GPU
work before shutdown.

Terrain imports plane normals and the portal renderer's anchored UV basis/WREXT
texture scale, grouped into spatial/material chunks. Sky openings are excluded
from geometry. Material bindings follow native animation-frame bitmap changes,
cache copied frames, and update opacity. Mutation inside the same bitmap and
animation frames with different dimensions still need coverage.

MD models retain offscreen casters, object movement, joint pivot/rotation/slide,
and native nonuniform scaling. MM meshes use current native skeleton poses,
weighted quaternion rotations for stretchy segments, and tracked head orientation.
Both rasterization and shadows use the same posed vertex buffers. Model removal,
replacement, and mission reset release scene references. Materials support native
texture replacements, MM v2 skins/faces, alpha and texture-modulated self illumination, but material
semantics and resource-lifetime coverage still need further work.

The first-person player placeholder, arms, and nocked arrow are excluded from
world geometry. The modern image composites inside the portal camera context,
before native first-person overlays and HUD. HDR coverage preserves the native
sky/background. Premultiplied HDR is unpremultiplied before tone mapping.

Point, spot, and sunlight values update each rendered frame without modifying
native light records, baked lightmaps, AI, or the light gem. Native 1/d point/spot
brightness converts to inverse-square intensity using pi times 16 engine feet;
sunlight uses pi divided by the native 125-foot synthetic distance. This approximates legacy diffuse brightness at the reference
distance, rather than defining physical units. Dynamic lights ignore native cone
and radius fields that their writer does not initialize. Development controls
are `renderer_exposure` (1), `renderer_ambient` (0.02), and
`renderer_light_scale` (1). Cross-mission artistic calibration remains pending.

## Shadows, composition, and diagnostics

Shadow submission uses indexed indirect batches when the device enabled the
optional core indirect feature pair. `renderer_shadow_indirect=0` forces scalar
submission for comparison; devices without that pair use scalar automatically.
This changes submission cost, not shadow coverage or quality.

`renderer_shadow_cache=1` enables an experimental additional pool for up to 128
local lights/768 faces at 128-square resolution. It supplies shadows for eligible
lights outside the active high-resolution budget, with the same point/spot PCSS
sampling and current caster geometry. The pool adds 96 MiB across two frame slots
at its configured capacity. Its default is `0`: dirty distant maps currently all
redraw immediately, and retail CPU/GPU cost remains too high. This is a coverage
prototype, not a completed performance fix or unlimited shadow coverage.

MD self-illumination properties scale the authored per-material glow. Eight-byte
auxiliary material records are supported, so Mission 2 lamppost housings remain
lit and shadowed while their lens materials glow. Copied lights may identify an
emitter object: only that object's authored emitter surfaces are excluded from
its shadow map. Its housing still casts, and its lens still occludes other lights.

`renderer_portal_rendering=0` suppresses native world draw submissions after a
successful modern render. Native portal visibility and object callbacks still
run; sky, first-person overlays and HUD remain. Native water and late translucent
effects now remain enabled, using a modern-to-native depth bridge for occlusion.
The default is `1`; automatic legacy fallback
continues drawing normally even when the option is `0`.

With native opaque drawing disabled, the renderer exports opaque view distance
to an optional R32 float attachment and converts it into the native depth
encoding during composition. Masked holes leave far depth, and blended modern
surfaces preserve opaque depth behind them. This costs approximately 16 MiB at
1080p across two slots, plus driver allocation padding. The extra targets and
shader variants are created only when this mode is used. A device lacking R32
sampling/color blending retains native rendering through the existing fallback.
Portal traversal still runs. Mixed modern/native translucent sorting and every
particle type remain outside the current visual verification scope.

`renderer_portal_visibility=1` separately enables experimental visibility
filtering from a copied, current-camera native cell set. Importing with this
option creates per-cell terrain receivers with exact cell light lists while
retaining merged meshes for shadow casting. Missing or invalid snapshots restore
merged terrain drawing. World models now use all native
cell references, keeping objects that overlap any visible cell; attachments stay
conservative. Referenced world objects without cell entries are omitted from
raster drawing, matching native portal discovery. Shadow-caster flags remain
unchanged. The default is `0`. Native traversal still runs with either setting.

The shadow budget is 16 local lights / 48 local faces plus four stabilized sun
cascades. Point lights use six faces; ordinary spots use one. Wide spots use six.
Default local resolution is 512 square; sun cascades are 2048 square and cover
256 engine feet. Native emitter defaults are 0.25 feet and 0.27 degrees. PCSS
uses 16 blocker-search and 32 filtering samples.

`renderer_shadow_lights` and `renderer_shadow_faces` override the local budget
(defaults 16/48, clamped to 0–64 lights and 0–384 faces). A point light needs six
faces. For diagnosing distance-dependent shadow loss, try
`renderer_shadow_lights=64 renderer_shadow_faces=384`. This changes the resource
budget, not the default policy: competing visible lights can still exceed it.
At the maximum, the two 512-square D32 local arrays use 768 MiB before sun maps
and other renderer resources; device limits/allocation failures retain legacy
fallback. Larger budgets also increase recording and shadow rendering cost.

Cached views invalidate for camera, caster, pose, material, and texture changes.
Raster-frustum culling does not remove offscreen shadow casters. Blended surfaces
do not cast opaque shadows; masked materials test their authored alpha.

`renderer_scene_capture=1` imports scene data while retaining the legacy image.
The old `renderer_scene_acceleration` flag only retains scene-capture compatibility;
it no longer enables RT or builds acceleration structures.

Set `DARK_RENDER_TRACE` to a writable filename to log shadow rendered/reused view
counts, depth draw calls, previous-slot shadow GPU milliseconds, and previous-slot
modern-world GPU milliseconds. Timings are read after slot retirement; unavailable
values are -1. The modern pass still duplicates native world drawing, so its GPU
timing excludes part of the actual frame cost and does not establish a frame-rate
or target-hardware claim.

## Validation and remaining work

See `tools/world_scene_test/README.md` for standalone CMake/CTest and native model
regressions. Coverage includes scene ownership, camera projection, shadow budgets,
GPU geometry, point/spot/sun depth readback, cache reuse/invalidation, alpha holes,
movement/deformation, finite-emitter lighting, and HDR composition. Linux tests
use an RTX 4070 Ti with RT features disabled for modern-device creation.

Retail mission smoke tests cover all three filter settings and graceful shutdown.
Initial courtyard import contains 426 terrain chunks and 748 model objects,
including 15 MM meshes. Previous visual captures verified the native sky, sword,
and HUD; current PCSS retail capture validation is outstanding because X11 window
mapping stalls before Vulkan initialization. Normal Wayland mission runs work.

Remaining work includes attachment visual checks, shared rigid geometry, fog/water and other
material semantics, mutable texture/resource lifetimes, broader lifecycle/error
paths, penumbra/bias/seam/cascade quality, fresh captures, Windows validation, and
performance work toward 1080p/60 FPS on the agreed midrange hardware. Path tracing,
denoising, HDR display output, and gameplay visibility changes are outside this
release.

Native MD/MM self illumination modulates the emissive tint by the linearized base
texture RGB. Constant emission remains available to renderer clients. GPU readback
checks the textured emission separately from direct lighting and tone mapping.

Mesh property attachments and Dark belt/alternate containment equipment are now
captured at the current rigid segment pose, including objects without world
references. Capture reads links and attachment properties without updating native
object positions. Mission 1 smoke testing imports 755 objects / 2019 material
parts (previously 748 / 2002). This proves additional equipment enters the scene;
visual alignment and nested attachments remain unverified.

Attachment owners are ordered by their parent links before applying transforms,
so referenced mesh children resolve after their parents regardless of object ID.
CPU regressions cover a three-level chain in reversed ID order and deterministic
termination of cyclic links. Nested attachment visual alignment remains unverified.

Windows portability evidence: a MinGW-w64 GCC 16.2 toolchain was extracted from
local package caches under `/tmp`. `world_scene.cpp`, `world_shadows.cpp` and the
scene/budget/camera tests compile into static Windows x64 PE executables. All
three executables pass under Wine using a separate temporary prefix. The four
Vulkan source units (features, geometry, shadow maps, world renderer) also compile
for Windows x64 against the same Vulkan headers without errors. This is compile
and Wine CPU-test evidence only: the complete Windows engine, native adapters,
Windows Vulkan linking/runtime and actual Windows hardware remain unverified.

Further Windows cross-compilation covers the native terrain/model adapters,
`vulkan.cpp`, `vulkan_context.cpp`, and `modern.cpp` using the Windows platform
headers. A missing standalone Windows COM-header dependency was fixed by
including `windef.h` before `winerror.h`/`basetyps.h`, making `WINAPI` available.
The units compile successfully with MinGW-w64; existing MAKE_HRESULT redefinition
and ignored novtable warnings remain. This extends compile evidence to native
integration but still does not prove a complete Windows engine build or runtime.

Native texture load/reload callbacks now refresh existing scene snapshots from
the already converted RGBA pixels, including palette-triggered reloads. Repeated
snapshot requests share the same scene handle; unchanged bytes retain revision,
while changed opacity also invalidates affected shadow maps. Snapshot lookup is
cleared on world reset and survives legacy AppOnExit ordering safely. Linux
builds, all six regressions and a 12-frame synchronization-validation mission
run pass without validation messages. Same-pointer changes that bypass native
reload callbacks, dimension changes and resource-pointer reuse within a mission
still require further handling/verification.

`RenderSceneReplaceTexture` now atomically replaces texture dimensions and pixels
while preserving the handle/material references. Same-sized unchanged content
keeps its revision. Invalid input leaves the working texture intact. Native
load/reload snapshot refresh uses this path, so GPU images are recreated safely
through existing per-slot ownership when dimensions change. CPU tests cover
references, revision stability and invalid sizes; the GPU lighting test resizes a
masked texture 1x1 -> 2x2 -> 1x1 and checks the corresponding shadow opening and
closing. All six tests pass under synchronization validation with no messages;
Linux game/editor builds pass. Terrain UV scaling when animation dimensions
change still needs native-adapter handling.

Terrain materials now retain import UV templates per spatial chunk. A frame
change to different bitmap dimensions, including a same-pointer resize, refreshes
the scene image and updates mesh UVs using original-dimension/current-dimension
ratios. Repeated size alternation derives from the original template rather than
accumulating scaling error. Terrain chunks allow topology-preserving updates for
this purpose. Linux game/editor builds and a 12-frame synchronization-validated
retail smoke run pass with no validation messages. A retail fixture that actually
alternates animation frame dimensions remains needed to verify this specific
native appearance path.

Native model type/source/size lookups are now shared by model index within each
capture pass. Object poses and texture/material overrides still refresh per
object; the asset lookup cache is rebuilt every pass, so later resource changes
are observed. Attachment resolution reuses the captured asset size. Linux builds
and native decoder/attachment tests pass. Mission 2 measured about 5.82 ms median
scene capture versus 5.95 ms before, with overall pacing still about 22.7 ms;
this is too small/noisy to establish a meaningful frame-time gain. A separate
legacy Mission 2 baseline measured about 15.87 ms submission interval, so the
remaining modern overhead and game-loop costs need continued profiling.

Windows Vulkan linking/execution evidence is now stronger: the complete standalone
CMake suite cross-compiles into Windows x64 PE executables with MinGW-w64 GCC
16.2. A COFF Vulkan import library was generated from the installed Wine
vulkan-1.dll exports, and all six executables pass under Wine on RTX 4070 Ti.
The GPU tests actually execute (not skip), covering arena offsets/readback,
point/spot/sun depth, two frame slots, movement/deformation, alpha texture resize,
PCSS penumbrae/contact hardening, mixed light lists and HDR composition. Total
CTest time was 27.25 seconds. This validates the Windows ABI and Wine Vulkan
path; the full native Windows game/editor and actual Windows driver/WSI lifecycle
remain unverified. No native Windows performance claim follows from this run.

Full Windows cross-build preparation found and fixed repository build issues:
FFmpeg no longer forces the MSVC toolchain for MinGW, libpng resolves the bundled
zlib target before configuration instead of requiring a host library, and the
Windows include/library generator expressions are quoted correctly. The complete
Windows project now configures/generates, and bundled FFmpeg cross-builds and
installs successfully. Linux game/editor builds and a three-frame modern mission
smoke test pass after these changes. The full Windows game build is now running;
completion/runtime are not yet established.

Linux game-only follow-up: render type is now queried once per supported model
per capture, then passed to material conversion instead of looking it up again
for every material part. Objects without supported models avoid this query.
Values are still refreshed every capture. The Linux thief2 target and native
MD/MM, attachment, and light-conversion tests passed. Mission 2 completed 120
frames at 1920x1080: median frame interval 22.028 ms, scene capture 5.617 ms,
command recording 8.150 ms, and world GPU time 12.866 ms after discarding the
first eight samples. This single run does not establish stable 60 FPS.
Windows and DromEd builds and runtime tests remain deferred by user request.

Shadow-plan distance follow-up (Linux game only): unbounded local-light ranges
now compare squared corner distances and take one square root per light. Sun
caster depth extrema are computed once and shared by all four cascades. The
shadow-camera regression now covers an offset unbounded light, distant invisible
caster corners, and the empty-scene far range. All six renderer tests passed
with synchronization validation enabled, with no VUID or sync-hazard messages.
The Linux thief2 build and 120-frame Mission 2 runtime passed. Compared with the
preceding query-reduction run, median plan CPU time changed from 1.452 to 1.289 ms
and recording from 8.150 to 7.988 ms. Frame interval was essentially unchanged
(22.028 versus 22.092 ms); this is a CPU reduction, not evidence of higher FPS.

Material sidedness is now carried in the existing 64-byte GPU material layout
and honored consistently by the depth prepass, direct-lighting pass, and shadow
map fragment shader. With doubleSided=0, clockwise Vulkan viewport faces are
discarded; doubleSided=1 preserves both faces and the existing back-face shading
normal behavior. Native terrain and MD/MM imports explicitly request two-sided
rendering to preserve their current appearance while native winding conventions
remain unaudited. This does not claim native material sidedness is fully mapped.
GPU regressions reverse receiver/caster winding with a mirrored instance, verify
one-sided radiance and shadow rejection, then verify two-sided restoration. The
standalone shadow test's material allocation and descriptor range were corrected
from 48 to the actual 64-byte layout, with a size assertion. All six renderer
tests passed on NVIDIA with synchronization validation (no validation errors)
and on Lavapipe. Linux thief2 built and Mission 2 completed 120 frames at 1080p,
importing 1464 objects, including 36 animated meshes and 3600 material parts.
Windows and DromEd checks remain deferred.

Fresh Linux RenderDoc verification after material-sidedness support and the
current depth prepass: Mission 1 ran 100 frames and exited normally on a private
Xvfb display with targeted sword selection. Capture
/tmp/dark-sidedness-capture_frame51.rdc contains 21,333 actions, 466 textures and
12 buffers (shared geometry arenas). Final output at event 52156 was exported to
/tmp/dark-current-depth-final.png and inspected: terrain, animated character,
native moon/cloud sky, sword and HUD are present together at 1920x1080. The HDR
target at event 48242 had no NaN, infinity, negative or out-of-range samples in
1,008 uniformly sampled pixels; this is sampling, not exhaustive validation.
RenderDoc MCP's generic pipeline-state accessor reports unavailable methods for
this Vulkan binding, so the local RenderDoc Python API was used to inspect the
actual Vulkan state. It exposed redundant depth writes in the opaque equal-depth
lighting pipeline. Those writes are now disabled. A second capture confirms:
event 36172 has LessEqual depth testing, depth writes, and no color writes;
event 42208 has Equal depth testing, no depth writes, and RGBA writes;
event 48242 has LessEqual depth testing, blending, and no depth writes.
The Linux thief2 build and all six renderer tests with synchronization validation
passed after the change, with no VUID or synchronization-hazard messages.
No performance improvement is claimed from this change without a benchmark.
Windows and DromEd remain deferred.

Visible draw preparation now extracts the camera frustum once and transforms each
visible candidate's bounds once for both culling and depth ordering. Mesh buffer
handles, arena offsets, index counts and draw constants are prepared once and
reused by depth and lighting passes. Vertex/index arenas remain bound until the
actual buffer changes; draws use firstIndex/baseVertex to address each mesh.
The lighting GPU regression adds a second mesh with different vertex/index data,
a compensating instance translation, and nonzero arena offsets. It must produce
the original lit pixel in both frame slots; ignoring either offset misses or
degenerates the triangle. All six tests passed on NVIDIA with sync validation
(no VUID/sync hazards) and on Lavapipe. Linux thief2 built, and Mission 2 completed
120 frames at 1080p. Median lighting-command CPU time was 0.674 ms versus 1.123 ms
in the earlier shadow-distance run; recording was 7.639 versus 7.988 ms and frame
interval 21.460 versus 22.092 ms. These separate runs also bracket the material
sidedness and opaque-depth-write changes, so they are not an isolated benchmark
of draw preparation. The 60 FPS target remains unmet. Windows/DromEd are deferred.

Shadow-cache lifecycle coverage now verifies removal of a previously cached caster
clears its depth in both frame slots, and recreating the placement under a new
handle restores it. GPU depth readback also covers sun arrays resized through
64/256/128 pixels and cascade counts 1/4/4, independently for both slots. Every
recreated array must redraw all active layers once, then reuse all layers on the
next submitted frame with unchanged depth. All six renderer tests passed with
NVIDIA synchronization validation and no VUID/sync-hazard messages; the expanded
shadow-map test also passed on Lavapipe. No production bug was found in these
cases. Source inspection confirms runtime recording failure disables modern
rendering and submission failure schedules a renderer rebuild before reuse.
At the time of these tests, allocation/submission failure injection was unverified;
the subsequent native allocation fixture below covers the allocation cases.
The tests above do not claim to prove those failure paths. No game/editor build was needed
for this test-only change. Windows and DromEd checks remain deferred.

## Native allocation-failure fallback verification

`native_allocation_failure.py` now injects one `VK_ERROR_OUT_OF_DEVICE_MEMORY`
through a temporary preloaded Vulkan allocation wrapper, armed by GDB at the
first production `RenderBackendRenderWorld` call. The first allocation and
allocations after 4/16 successful calls were tested separately with
`renderer_portal_rendering=0` and `1` under Khronos synchronization validation.
All six retail Mission 1 cases report modern rendering disabled, retain native
terrain drawing after the failure, and exit normally after three frames without
validation errors. Modern-only drawing retains 714 native cell draw calls over
all three frames; normal portal drawing retains 476 calls after the failure
(the first frame draws natively before recording modern lighting).

Logs are `build-linux/renderer-evidence/native-allocation-failure-{0,1}-{0,4,16}.log`.
An initial fixture that forced a return directly from a debugger breakpoint hung;
the wrapper-based fixture performs a normal function return and passes. That
debugger-only result is not evidence of a production fallback bug. No production
fallback change was required. These checks cover a single recoverable allocation
failure during initial world recording, including partial setup; sustained
exhaustion and device loss remain unverified; recoverable submit rejection is
covered by the following fixture.

## Native queue-submission recovery verification

`native_submit_failure.py` injects one `VK_ERROR_OUT_OF_DEVICE_MEMORY` from
`vkQueueSubmit` for a modern two-command-buffer frame submission. The wrapper
does not execute that submission. Uploads and later submissions run normally.
Khronos synchronization validation passes for both portal-drawing settings,
rejecting either the first modern submission or a submission after 12 successful
frames with stable-depth composition enabled. Every case reinitializes the
modern renderer and completes six later modern submissions before normal
shutdown. The warmed cases count 18 successful modern submissions in total.

Evidence is in `build-linux/renderer-evidence/native-submit-failure-{0,1}.log`
and `native-submit-failure-warm-{0,1}.log`. The context recreates swapchain
synchronization objects before reusing the failed frame slot, and the runtime
rebuilds world resources after retiring queued work. These native checks verify
continued submissions rather than the visual contents of recovered frames.
They do not establish recovery from real `VK_ERROR_DEVICE_LOST`, sustained memory
exhaustion, or arbitrary failures midway through upload submission.

A subsequent RenderDoc capture checks the rendered result after a warmed-cache
submission failure with native portal drawing enabled and stable-depth composition
enabled. The capture hook requests a frame after 15 post-import presents; the
injected rejection occurs after 12 successful modern submissions. The preserved
native log orders renderer reinitialization before the capture request and ends
with one rejected submission and six recovered submissions. Replay of
`build-linux/renderer-evidence/native-submit-recovered_frame32.rdc` contains
154,932 actions and 196 indirect shadow submissions. The exported final image
(`native-submit-recovered_frame32.png`, final draw event 163672) was inspected:
terrain, the courtyard character, illumination, native sky/moon and HUD are
present after recovery. This is a qualitative 800x600 frame check, not an exact
pre/post image comparison or a first-person weapon check.

RenderDoc MCP replay still reports unsupported `VK_KHR_surface` in its process.
Local RenderDoc Python replay on the private X11 server succeeds. The complete
native/GDB log and replay summary are `native-submit-recovered-full.log` and
`native-submit-recovered_frame32.json`. The test runner's `--log` option preserves
the raw native log; `DARK_TEST_INFERIOR_PRELOAD` loads optional capture helpers
only into the inferior, avoiding instrumentation of Python/GDB itself.

The first-person overlay recovery gap is now covered by a second native capture.
The fixture's `--equip-sword` invokes the same `inv_select sword` command as the
stock `1` key binding before the first world recording. After 90 successful modern
submissions, it rejects one submission, rebuilds, and completes six more.
`native-submit-recovered-sword_frame113.rdc` was captured after the rebuild with
stable-depth composition and native portal drawing enabled. Local replay/export
at final draw event 163779 shows the raised first-person sword, selected-sword
inventory icon, terrain, courtyard character, sky/moon and HUD. The frame has
155,055 actions and 195 indirect shadow submissions; the exported image and
summary share the capture basename. The complete sequence is recorded in
`native-submit-recovered-sword-full.log`. This is qualitative evidence for the
sword overlay, not every weapon, attack animation or translucency interaction.
Both native portal-drawing settings also pass the 90-frame warmed sword-equipped
failure test under Khronos synchronization validation; logs are
`native-submit-sword-validation-{0,1}.log` in the renderer evidence directory.

Terrain fog foundation: the reusable camera now carries linear RGB fog color and
fog distance (zero disables), and instances explicitly opt in with receivesFog.
The world camera GPU block is 144 bytes; draw constants are 72 bytes, both size
asserted. Lighting blends the final linear HDR radiance toward fog color using
camera-forward depth divided by distance, preserving opacity and shadow-map
contents. Modern tone mapping also applies to this fog color. Camera validation
rejects negative/nonfinite distance and negative/nonfinite colors. GPU tests
verify clear instances remain clear, half/full-distance behavior and zero-distance
restoration, using actual HDR readback in both slots.
Native terrain chunks now include CELL_FOG in their grouping key so a shared
material cannot inadvertently fog clear-cell polygons. Camera capture reads the
native fog enable/distance/color, decoding color from sRGB to linear. Native
objects still do not opt into fog: their render queue's captured per-draw fog
state needs separate integration, particularly for objects spanning cells.
Linux thief2 built; all six renderer tests passed on NVIDIA with synchronization
validation (no VUID/sync hazards) and on Lavapipe. Native MD/MM/attachment/light
conversion tests passed. Mission 10 completed 120 frames at 1080p. A subsequent
12-frame diagnostic run reported 738 terrain chunks (730 fog-flagged), versus the
previous 730 total chunks, confirming boundary splitting. Its fog was disabled
and distance zero, so active native mission fog and visual matching remain
unverified; active fog behavior is currently established by the GPU fixture.
Windows and DromEd remain deferred.

Native object fog integration: each camera capture clears a frame-local map, and
the native object queue records its existing per-draw RQ_FLAG_FOG without changing
legacy rendering state. Model instances prefer that captured state, with a
CELL_FOG fallback for unqueued objects. Attachments lacking their own queue state
inherit their captured owner's fog state. If fragments of one object disagree,
the complete modern instance remains clear independent of draw order; exact
per-fragment fog requires geometry splitting and is still incomplete.
Production-adapter CPU tests cover queue overrides, fallback, frame reset, both
mixed-fragment orders and disabled capture. Linux thief2 built and Mission 10 ran
120 frames normally. A controlled active-fog fixture used GDB on a newly launched
game process to set only in-memory native fog globals (enabled, distance 64,
color 80/100/120) before its first camera capture; no mission assets or persistent
configuration were changed. It completed 24 frames normally, including a rerun
with synchronization validation and no VUID/sync-hazard errors. Diagnostics
reported 730 fogged terrain chunks, 29 queued object states, 1109 receiving model
instances and no mixed-fragment objects. Pixel-accurate active retail fog visual
matching is not established by these diagnostics; GPU fog interpolation remains
covered by the existing HDR readback fixture. Windows/DromEd remain deferred.

Active-fog RenderDoc follow-up: /tmp/dark-active-fog-capture_frame47.rdc was
captured from Mission 10 with the controlled in-memory fog fixture, at 1080p.
The game completed 100 frames normally. Final event 6428 was exported to
/tmp/dark-active-fog-final.png and inspected: distance fog is visible across
terrain/objects, and the HUD remains visible. This is a synthetic fog setting,
not evidence of exact retail fog color matching or per-fragment correctness.
Shader reflection confirms the 144-byte camera block and receivesFog input.
The MCP constant-buffer helper returned zero values, so its result was not used
as evidence. Raw RenderDoc buffer readback at event 4390 verified the actual
fogColorDistance values: 0.08021983, 0.12743770, 0.18782076, 64.0 (sRGB-decoded
80/100/120 and distance 64). A 1008-point HDR sample found no nonfinite, negative
or extreme values; sampling is not exhaustive. A new off-axis GPU regression
uses a wide camera and an unobstructed planar receiver, demonstrating forward
camera depth rather than radial eye distance controls fog. It passed on NVIDIA
with synchronization validation (all six tests pass, no VUID/sync hazards) and
on Lavapipe. Windows and DromEd remain deferred.

Rejected CPU shadow spatial-hierarchy experiment: Mission 2 at 1080p, 180 frames
per sequential run, excluding the first eight samples, measured the following
median frame/shadow-record/total-record CPU times (ms): baseline
21.016/3.511/7.543; rebuilding a bounds hierarchy with sorted query results
22.057/4.428/8.423; ordered visibility masks 20.359/3.783/7.650; retained topology
with refit 21.149/3.384/7.422. Draw counts stayed 29,284. A CPU comparison fixture
matched linear culling over rotated, mirrored, sheared and large-coordinate
bounds, and GPU regressions passed, but the small final shadow CPU saving did
not produce a frame-time improvement. The hierarchy, its cache and its test
were removed rather than retaining unhelpful complexity. Restored linear code
measured 20.951/3.523/7.506 ms and completed normally. Linux thief2 rebuilt and all
six retained tests passed with synchronization validation and no VUID/sync-hazard
messages. The ~5.7 ms native scene capture remains a larger remaining CPU cost.
No production optimization is claimed from this experiment. Windows/DromEd
remain deferred.

Property-cache performance follow-up: using the existing command-line override,
Mission 2 at 1080p/PCSS and 180 frames measured median frame/capture times (ms)
of 21.183/5.702 at 8192 entries, 17.843/3.284 at 16384, and 18.713/3.644 at 32768.
A second 16384 run reproduced 17.996/3.307. All runs exited normally; no build,
mission assets, cache invalidation code or persistent configuration was changed.
Peak RSS was 603664/597372/602220 KiB respectively, and 597280 KiB on the repeated
16384 run. This variance cannot isolate the cache's own memory overhead. The
opt-in launch setting and limits are documented in shadow-map-renderer.md.
The current configured capacity is not overridden automatically. Caching alpha
and self-illumination between frames needs correct inherited-property and
hierarchy invalidation; blindly freezing these values would regress live changes.

Material-property revision caching now avoids repeated alpha/self-illumination
Get calls when values are unchanged. The native property initializers register
listeners for set/modify/unset/load; a hierarchy listener invalidates values on
donor add/remove without requesting extra concrete-property rebuilds. Callbacks
advance a shared process-lifetime revision and do not own renderer objects.
Every model refreshes both floats when the revision changes, restoring defaults
before reads so property removal cannot leave stale values. Texture setup,
replacement/skin callbacks, animated texture checks and pose capture still run.
The conservative global revision may invalidate all models for one change.
The new CPU cache fixture passed. The retained native regression
`tools/world_scene_test/material_properties.py` also passed normally and with
synchronization validation: object 11's observed alpha sequence was
1/.25/.75/1/.6/1/.45/1 through concrete edits/removal, archetype edits/removal, and
metaproperty add/remove, with matching self-illumination changes. All edits were
in-memory in a disposable Mission 1 game process, with no mission saved.
Linux thief2 built; native MD/MM, attachment, fog and light-conversion tests and
all six renderer tests passed (no Vulkan VUID/sync hazards).
Sequential Mission 2, 1080p/PCSS, 180-frame measurements after warmup: the existing
8192-entry cache measured 17.583 ms median frame (p95 20.940, p99 22.537) and
2.812 ms capture, compared with 20.951 (p95 23.859, p99 25.354) and 5.628 ms
before this change. A 16384-entry run now measured 18.088/2.830 ms frame/capture;
the cache-size workaround no longer helps this fixture. No cache default or
persistent configuration changed. Stealth formulas and native lighting records
were not changed. Broader performance and stable 60 FPS remain unfinished;
Windows/DromEd remain deferred.

Shadow-casting property follow-up: ObjShad now participates in the native material
revision notifications. The shared MD/MM property refresh reads alpha,
self-illumination and castsShadow together, restoring all defaults before reads.
The per-object, per-frame ObjShad lookup was removed. Texture and pose updates
remain live. Native CPU tests verify removed shadow state restores default true.
The retained native GDB fixture now verifies shadow state through concrete and
archetype edits/removal, plus a metaproperty that disables casting and subsequent
removal. It passed with synchronization validation; the fixture rejects any
validation error or synchronization hazard. Linux thief2 built and Mission 2
completed 180 frames at 1080p/PCSS with the original 8192-entry trait cache.
Median frame/capture times were 17.428/2.618 ms, versus 17.583/2.812 ms in the
preceding alpha/self-illumination run; frame p95 was 20.173 versus 20.940 ms.
This modest CPU reduction does not establish stable 60 FPS. No cache capacity,
native light record or stealth formula changed. Windows/DromEd remain deferred.

Shadow-camera fitting now transforms mesh bounds once, retaining oriented axes
and world AABB extents. Each unbounded light uses the AABB's maximum squared
distance as a conservative rejection bound, with a rounding margin, and evaluates
exact oriented corners only when a mesh can extend the current far distance.
Sun depth extrema use the oriented box's support radius instead of enumerating
eight corners. The set of included visible/offscreen bounds is unchanged.
A CPU regression compares far ranges against independent brute-force local-corner
matrix transforms for 160 rotated/mirrored/sheared meshes and 80 light positions,
including million-unit coordinates; every sun cascade must retain all caster
depths. Existing empty-scene, offscreen and camera stabilization tests also pass.
Linux thief2 built; all six renderer tests passed on NVIDIA with synchronization
validation (no VUID/sync hazards) and on Lavapipe. Mission 2 completed 180 frames
at 1080p/PCSS using the unchanged 8192-entry trait cache. Median planning CPU time
changed from 1.282 to 0.592 ms and total recording from 7.377 to 6.791 ms. Median
frame interval changed from 17.428 to 17.326 ms, while p95 was essentially
unchanged (20.173 versus 20.176 ms). Median/p95 shadow draw counts were identical
(29286/29727). The CPU saving does not establish an FPS improvement or stable
60 FPS. Windows/DromEd remain deferred.

GPU phase timing follow-up: the world renderer now uses four timestamps per slot
and reports `world-gpu-phases previous_depth_ms=... previous_lighting_ms=...`.
Results are read only when the caller has retired that frame slot, without WAIT,
and respect the graphics queue's timestamp valid-bit mask. The depth interval is
bounded by a bottom-of-pipe marker after shadows and a late-fragment-tests marker
after the depth prepass; lighting runs from that marker through world completion.
These are timestamp intervals, not an exhaustive accounting of overlapping GPU
pipeline stages. Unsupported timestamp queues retain unavailable (-1) values.
The GPU regression verifies finite, nonnegative intervals in a retired slot and
that their sum fits inside the total world interval.
Temporary build/runtime fixtures and earlier /tmp captures were absent at the
start of this follow-up. A fresh standalone Release test build and an isolated
retail game directory (copied configs/bindings, symlinked assets) were created;
older capture paths in these notes are historical, not currently retained
artifacts. The Linux thief2 target built and all six fresh tests passed with
newly extracted Khronos synchronization validation, with no validation errors,
VUIDs or sync hazards. Mission 2 completed 180 frames at 1080p/PCSS: median depth
interval 0.109 ms, lighting interval 10.831 ms, total world 12.811 ms, frame
17.180 ms. This identifies direct lighting as the main measured GPU cost.
A BRDF experiment hoisting per-pixel terms and replacing Schlick pow with explicit
multiplication measured 10.806 ms lighting and 17.006 ms frame; that is not a clear
lighting improvement. The shader rewrite was removed, retaining the original
lighting math and the timing instrumentation. All six tests were rebuilt and
passed with sync validation after restoration. Stable 60 FPS and the broader
renderer requirements remain unfinished. Windows/DromEd remain deferred.

Shadow tap setup is now invariant per filtered receiver: filteredShadow reads the
ShadowView once, prepares type/layer/point-base-layer metadata, and projects the
receiver to spot/sun UV once. Every blocker/filter tap reuses those values. Point
face remapping, per-tap boundary checks, receiver-plane compensation, sample
counts, disk offsets and PCSS radii are unchanged. A receiver behind a spot/sun
projection remains fully lit, matching the former all-far-depth tap result.
Linux thief2 built and all six renderer tests passed with Khronos synchronization
validation, with no validation errors/VUID/sync hazards. Mission 2, 1080p/PCSS,
180 frames per sequential run after eight warmup samples, measured median
lighting GPU 8.868 ms initially. A fresh old-shader control measured 10.661 ms;
restoring the optimized shader reproduced 8.930 ms. Shadow draw median/p95 stayed
29286/29727. Frame median was 17.115 ms for the control and 16.917 ms for the
optimized repeat (p95 19.720 ms). This is a repeatable lighting GPU reduction;
it does not establish stable 60 FPS or an equivalent overall frame improvement.
A fresh offscreen RenderDoc capture is retained under
build-linux/renderer-evidence/shadow-uv_capture.rdc, with HDR/depth PNG exports,
control/optimized runtime traces and the validation test log alongside it. Replay
contains the point-shadow depth work and receiver depth/lighting draws; 1024 HDR
samples at event 95 were finite/nonnegative ambient shadow values (~0.004997).
This single captured first submission is not native mission visual coverage;
the full regression also exercises hard/PCF/PCSS and spot/sun filtering. The
artifact directory is inside the ignored Linux build rather than ephemeral /tmp.
Broader renderer requirements and stable 60 FPS remain unfinished.
Windows and DromEd remain deferred.

Canvas conversion follow-up: 15/16-bit rows now process four pixels at once when
the native scene is active. An all-zero source word becomes sixteen zero RGBA
bytes, avoiding four lookup-table conversions. Mixed/nonzero blocks and tails
use the scalar conversion; opaque menu rows keep their existing behavior. RGB555
high-bit black remains opaque unless its complete raw value is zero. The 8-bit
palette path is unchanged. memcpy handles unaligned source/destination rows,
and the RGBA stride is size-asserted. A canvas-conversion trace reports CPU time
separately from the existing upload/present interval.
CPU regression compares block and scalar output for RGB555/RGB565, scene/menu
modes, lengths 0..129, unaligned rows, guard bytes and four zero/mixed/high-bit
patterns; existing scalar tests exhaust all 65536 values. It passed ASan/UBSan.
Linux thief2 built and all six renderer tests passed with synchronization
validation and no validation errors/VUID/sync hazards.
Mission 2 at 1080p/PCSS completed three sequential 180-frame runs: median scene
canvas conversion was 0.738 ms per conversion in the control, 0.494 ms with the
block path, and 0.489 ms in a repeat. There are roughly two conversions per frame
in this fixture. Median frame times were 17.117/16.976/16.922 ms respectively;
p95 was 19.875/20.051/20.244 ms. The conversion improvement is reproducible, but
these runs do not show better tail frame times or stable 60 FPS. Traces and the
final validation log are retained under build-linux/renderer-evidence/canvas-*.
Shadow quality, cache capacity and gameplay calculations are unchanged.
Windows and DromEd remain deferred.

Shadow cache keys now include only depth dependencies: instance/mesh revisions,
opacity and sidedness, plus cutoff, base alpha, texture handle and alpha revision
for masked materials. Opaque base RGB/alpha, emission, BRDF values and textures
cannot change the depth shader result and no longer invalidate its cached maps.
SceneTexture retains a full revision for GPU image uploads and a separate alpha
revision for depth-cache dependencies. RGB-only updates advance the full revision;
alpha changes and dimension replacement advance the alpha revision. Identical
updates and row padding do not change either relevant image revision.
New GPU regressions verify opaque appearance/texture changes reuse cached depth,
masked RGB-only changes also reuse it, and masked material alpha changes redraw
and clear the depth result. Existing texture-alpha, sidedness, pose, deletion,
and two-slot recreation tests remain green. CPU coverage includes RGB versus
alpha revisions, identical updates, resized images, padded rows and alpha changes
in the last row. Linux thief2 built; all six tests passed with Khronos sync
validation and no validation errors/VUID/sync hazards. The CPU suite also passed
ASan/UBSan; its output is retained in renderer-evidence/depth-cache-sanitized.log.
Mission 2 completed 180 frames at 1080p/PCSS. Median/p95 shadow draws stayed
29286/29727 and rendered views stayed 25/28, so this workload does not demonstrate
fewer map redraws. Median shadow-record CPU was 3.481 ms versus 3.595 previously;
frame median/p95 was 16.749/19.419 ms versus 16.922/20.244. These separate runs do
not establish a reliable frame-rate gain. Correct reuse under appearance-only
mutations is established by the regression fixtures. Runtime and validation
logs are retained under build-linux/renderer-evidence/depth-cache-*.
Stable 60 FPS and broader renderer requirements remain unfinished.
Windows/DromEd remain deferred.

Immutable GPU texture retention: the previous weak cache discarded animation
frames once neither frame slot referenced them. Reappearing frames then allocated
new images, views, memory and staging buffers. The cache now retains immutable
current-revision images under a 64 MiB default retention budget and updates their
last-used record serial. Over-budget eviction selects least-recently-used images
with no frame-slot/upload references. Required pinned images can exceed the
retention budget; they are never destroyed to satisfy it. Removed scene texture
handles are purged on subsequent Record calls. Revision changes still allocate a
separate image, preserving old images referenced by queued/submitted commands.
The budget setter applies during subsequent recording, and per-frame allocation,
staging, upload, eviction and cached-byte counters are exposed through
GetTextureWorkload and the world-textures trace.
GPU regressions verify an animation frame unused for both slots returns without
allocation/upload, forced zero-budget eviction re-uploads correctly, and a queued
slot samples its old RGB revision after another slot uploads and draws a new one.
Existing alpha-update, texture-resize and composition tests remain green. Linux
thief2 built and all six tests passed with synchronization validation and no
validation errors/VUID/sync hazards.
Mission 2 at 1080p/PCSS, 180 frames per run (171 analyzed after warmup): control
had 216 image allocations in 51 frames; retention had 73 in 18 frames, and a repeat
had 73 in 19 frames. Current-version image allocations retained by the cache
peaked at 24,909,824 bytes in both optimized runs (not total renderer VRAM).
Median/p95 upload/tiling CPU was 1.876/4.386 ms for control, 1.152/2.136 ms with
retention, and 1.190/2.383 ms in the repeat. Total recording was
6.693/9.467, 6.100/7.995, and 6.215/7.775 ms. Frame median/p95 was
16.654/19.930, 16.495/19.457, and 16.470/19.650 ms. Texture allocations explain
some, but not all, upload spikes: the largest control spike had no allocations.
This improves resource churn and measured CPU work without establishing stable
60 FPS. Traces and validation results are retained under
build-linux/renderer-evidence/texture-cache-*.
Windows and DromEd remain deferred; the full goal is still unfinished.

Matched native RenderDoc review found and fixed a sunlight normalization bug.
Mission 2 was captured at the same initial camera in legacy and modern modes
(private Xvfb, 1080p, frame 33 after scene warmup); both ran 120 frames normally.
The modern image was severely overlit. Captured descriptor data confirmed 541
lights (373 point, 167 spot, one sun), 17 shadowed lights and 376 global/unbounded
lights. More importantly, the native sun had RGB 20.94/25.41/25.41 and modern
intensity pi. Native objlight.c actually places its synthetic sunlight point at
125 feet, and mlight.c applies brightness/distance. The adapter had omitted that
distance normalization, making its directional diffuse about 125 times too strong.
The distance is now shared as OBJLIGHT_SUNLIGHT_DISTANCE in objlight.h; the native
SUNLIGHT_DISTANCE alias retains exactly its former 125.0f value. Only the copied
modern directional intensity changes to pi/125. Point/spot conversion, native
light records, AI and light-gem formulas remain unchanged. The native conversion
regression now compares directional diffuse against the 125-foot native reference.
A new capture confirms GPU intensity 0.025132744 instead of 3.141592741 with the
same sun color, camera exposure and ambient settings. The inspected corrected
image is no longer saturated across sunlit walls/roofs and is much closer to the
legacy night view; terrain, sky and HUD remain present. Exact artistic matching,
shadow-edge aliasing and broad light-reach behavior remain open. Native cell
light_indices are not yet mapped into modern receiver/light linking; the large
unbounded-light set is a potential additional reach/occlusion issue, not a proven
cause isolated by this change. This camera contains no close character/attachment
view, so that visual requirement is still outstanding.
Retained artifacts in build-linux/renderer-evidence include native-modern_frame33,
native-legacy_frame33, native-sun-fixed_frame33 .rdc files and final PNGs, helper
source/logs, and before/after light-descriptor JSON diagnostics. At corrected HDR
event 75452, 1008 sampled pixels were finite/nonnegative (not exhaustive). The
RenderDoc-injected process could not create a Wayland Vulkan surface, so only
capture runs used private Xvfb. Normal Linux Mission 2 also completed 120 frames
with synchronization validation and no validation errors/VUID/sync hazards.
Linux thief2 and native MD/MM/attachment/light-conversion tests passed. All six
renderer regressions passed with sync validation, including new 20/60-foot-offset
grazing point/spot clear-plane cases with intensity scaled to keep ambient from
masking self-shadowing. Those cases preserved clear-plane radiance; they do not
establish that all native shadow aliasing is solved. Replay resources were closed
and no private Xvfb process remained at cleanup. Windows/DromEd stay deferred;
the full renderer goal remains unfinished.

### Linux close-character and first-person sword capture (2026-11-15)

Mission 1 was run for 180 frames at 1920x1080 in modern and legacy modes on
private Xvfb :199. A PID-targeted X11 key event selected the sword in each
process after the world-model import log appeared. Both processes exited 0.
Retained evidence under build-linux/renderer-evidence:

- native-sword-modern_frame112.rdc and native-sword-modern-final.png
- native-sword-legacy_frame111.rdc and native-sword-legacy-final.png
- native-sword-modern-before-composite.png and both process logs
- capture-weapon.py and the configurable-delay native-capture.c/.so helper

Modern frame 112 contains a close animated MM character. Comparing independent
runs initially showed different poses, so the stronger comparison uses the
native raster image at event 45784 and the modern composite in the *same* frame.
The head, torso, lowered arms and hands visually align across those images.
This checks one native pose and placement; it does not establish correctness for
every motion, attachment or character. The modern composite triangle is event
45790; the subsequent 24-vertex native draw at 45797 restores the drawn sword.
The final image at event 45938 includes that sword, native sky, inventory icon,
health and light gem. The legacy final image is event 4482. Independent runs
are not a pixel-matched animation comparison because simulation timing differs.

Visible differences remain: modern direct lighting is brighter on the character
and walls, and the native lamp glow visible before compositing is absent in the
modern final image. The latter is consistent with the outstanding composition
issue for native translucent effects drawn before the modern opaque composite.
No correction to that ordering is claimed here. Windows and DromEd were neither
built nor tested in this check.

### Follow-up: lamp glow is not fixed by corona ordering (2026-11-15)

Two experimental ordering changes were tested and then fully removed. Moving
WorldSceneRender into the object queue did not restore the glow. Splitting corona
rendering into a deferred draw after the existing composite also did not restore
it: modern-deferred frame 111 ends its native world batch with the composite
triangle at event 45812 and the sword at 45819, with no intervening corona draw.
Thus the missing glow in this camera cannot be attributed to a queued corona
being covered. The earlier corona hypothesis is unproven for other scenes and
ruled out as this scene's repair. The next investigation should identify the
actual native glow draw, likely among earlier native translucent/particle draws,
before selecting an ordering change. Neither experimental change remains in
source; the original world composition/callback order is restored. Linux thief2
rebuilt successfully afterward. Native model regressions passed during this
investigation. Both experimental modern runs and the legacy comparison exited 0;
one experimental modern run also used synchronization validation without reported
VUIDs, validation errors or sync hazards. These runs do not validate a retained
corona fix because no such fix remains. Character timing changed between runs,
so absence of a nearby character in the slower validation run is not evidence of
a model-import regression. The goal remains active with translucent composition,
lighting calibration and broader shadow/performance verification outstanding.

### Native translucent composition repair (2026-11-15)

RenderDoc pixel history isolated the actual missing Mission 1 lamp glow. In the
original modern frame 112, native blended event 45560 repeatedly brightens pixel
(236,441) from about RGB .141/.114/.075 to .702/.702/.702; later blended draws
refine it before the modern triangle at 45790 covers the effect. Corona-only
reordering was consequently the wrong repair for this camera.

The native object queue now calls WorldSceneRender after opaque objects and
before its existing sorted translucent/water pass. WorldSceneRender caches its
success per camera pass, so portal cleanup remains a fallback without capturing
or compositing twice. A trace from the repaired 180-frame run contains exactly
180 world captures and 180 world records. Native translucent effects and water
continue to use the native opaque depth buffer. Imported MD/MM models suppress
only their duplicate native geometry during this alpha pass: visibility flags,
corona checks, custom render callbacks and diagnostic aftereffects retain their
native path. WorldSceneHasModel requires a current, nonempty import with every
nonempty part instantiated; missing/stale/partial imports keep native geometry.
CPU regressions exercise those cases alongside the existing MD/MM, material,
attachment, light and fog tests.

Final retained capture native-sword-modern-effects-final_frame112.rdc composites
at event 45389, followed by native effects; the inspected final image at 45992
restores both lamp glows and retains the close character, sword, sky and HUD.
The matching legacy run is native-sword-legacy-effects-final_frame111.rdc, final
image event 4490. Both 180-frame processes exited 0, and their exported final
PNGs and logs are retained in build-linux/renderer-evidence. Linux thief2 built,
native model regressions passed, and a final 60-frame Mission 1 run completed
with synchronization validation and no VUIDs, validation errors or sync hazards.
The existing unrelated missing-AI-property warning remains.

This establishes the visible lamp repair, not complete translucent correctness.
Native water is now scheduled after the composite but has not yet received a
close water-specific visual check. Modern translucent meshes and native effects
still form separate sorting groups; intersections between them can need deeper
integration. Unsupported opaque effects drawn before the composite are also not
covered by this repair. Lighting calibration, moving shadow/cascade checks and
performance work remain open. Windows and DromEd remain deferred.

### Native water boundaries excluded from modern terrain (2026-11-15)

The native hardware renderer identifies translucent medium boundaries using
PORTAL_SPLITS_OBJECT and routes them through its animated, palette-alpha water
queue. The terrain importer previously treated those same boundaries as ordinary
opaque/masked geometry. It now skips them before creating terrain materials or
meshes, keeping native water out of modern depth and shadow geometry. A trace
reports the excluded boundary count; native mapping, palettes and water drawing
are unchanged. In Mission 2, an independent debugger traversal found 60 such
render polygons (both sides included), matching native_medium_boundaries=60 in
the updated import trace.

A disposable-process debugger fixture detached the camera above the canal at
(376,-8,-8), with downward pitch 8192. Both modern and legacy runs completed 90
frames normally and drew 89 native water batches after the initial camera switch.
Captures native-water-modern_frame51.rdc and native-water-legacy_frame51.rdc,
exported final PNGs, capture-water.py, GDB scripts/logs/traces, and the independent
polygon JSON are retained in build-linux/renderer-evidence. RenderDoc injection
under GDB required startup-with-shell off; earlier noncapturing fixture runs are
not treated as GPU capture evidence.

In the modern capture, composite event 66547 precedes native water event 66561.
Pixel history at (960,540) shows water blending RGB .039/.027/.016 into
.071/.086/.031 without changing native depth .9352133. The inspected final image
at event 66688 shows the canal floor through the green animated surface. The
legacy final image at event 338 has the same surface placement/transparency with
different underlying lighting. This proves that close water remains blended over
the modern world in this view; it does not establish all underwater/fog or mixed
transparent-object cases. A final 90-frame run of this same detached-water view
with Vulkan synchronization validation exited normally, drew 89 batches and
reported no validation errors, VUIDs or sync hazards. Linux thief2 built and
whitespace checks passed. No Windows or DromEd build/test was performed.

### Model-slot reload identity (2026-11-15)

The model adapter cached source size but did not compare it when deciding whether
to rebuild geometry. Address/index/type identity also missed native reloads that
reuse the same resource allocation. Native model slots now expose a process-local
64-bit load revision, kept in a separate array so neither model files nor native
handle layouts change. Invalid/unloaded slots report zero. The adapter samples
the revision once per model index per capture and rebuilds on revision or size
changes in addition to pointer/index/type changes. New scene meshes/instances
replace the previous copies through the existing destruction path, invalidating
their shadow cache identities. A reload-only trace records observed transitions.

The new Linux x86-64 retail fixture, tools/world_scene_test/model_reload.py with
model_reload.gdb, holds object 11's resource resident and reloads its native slot
through objmodelLoadModelIntoHandle. The observed slot 36 retained exactly the
same pointer and 4218-byte extent, advanced revision 1 to 2, and retained its four
native model references. The renderer recorded exactly one rebuild for object
11 and reported a complete imported model on the following frame, with stable
revision 2. This same-pointer/same-size case would evade the former comparison.
The 12-frame fixture exited normally both without validation and with Vulkan
synchronization validation; its runner rejects validation errors, VUIDs, sync
hazards and fatal signals. Linux thief2 and native MD/MM/material/fog/light/
attachment regressions passed. Evidence logs are retained as model-reload-*.log
under build-linux/renderer-evidence. Direct byte mutation bypassing the native
model loader remains outside this revision mechanism; texture lifetime/mutation
coverage is a separate outstanding audit. Windows/DromEd remain deferred.

### Unloaded copied textures observe palette changes (2026-11-15)

The texture-lifetime audit reproduced a different stale-data case before changing
source: the native model adapter returned a cached copied texture after its
unloaded bitmap's palette changed. The native resident-texture reload callback
cannot refresh such offscreen copies. A synthetic unloaded 1x1 indexed bitmap
kept scene handle 993 and RGBA 0xff0000ff (red) even after its palette entry was
changed to blue. This is a source-level native integration test, not an inferred
visual difference.

Palettes now have per-slot revisions assigned from a process-wide serial, changed
only when their bytes change (or when renderer palettes are initialized). The
serial distinguishes different palette slots as well as successive edits. Model
texture cache entries compare that revision before returning a copied handle;
stale entries re-expand pixels through the existing snapshot replacement path.
Terrain animation frames use the same revision check, including their early
return for a stationary frame and their cached-frame selection. No pixel
conversion is added to the unchanged cache-hit path. Resident reload callbacks
remain in place.

The retained regression tools/world_scene_test/palette_snapshot.py uses a small
read-only helper and debugger calls into the production model adapter, palette
setter, and scene store. After the fix it retains handle 993 but reads
0xffff0000 (blue). An unchanged palette write retains revision 1251 in the final
validation run. The original native palette entry is restored before execution
continues. Linux thief2 and native decoder/material/attachment/fog/light tests
passed; the four-frame fixture also passed with Vulkan synchronization validation
and no validation errors/VUIDs/sync hazards. Evidence before/after/build/validation
logs are retained as palette-cache-*.log in build-linux/renderer-evidence.
This test directly covers unloaded model snapshots. Terrain's matching cache
change has normal mission runtime coverage, rather than a separate synthetic
terrain palette-edit fixture. Bitmap-address reuse with identical metadata and
pixel mutation that bypasses native upload/reload callbacks remain outstanding
lifetime cases; this palette repair does not claim to solve them. Windows and
DromEd remain deferred.
The final normal Mission 2 run completed 180 modern frames at 1080p, with exactly
180 world captures/records. Filtering to modern frames and discarding the first
eight pacing samples leaves 171 samples: median 16.211 ms, p95 18.520 ms. CPU
capture median/p95 was 2.648/3.098 ms; world recording 5.788/6.913 ms. These are
current-build observations, not a controlled before/after palette-performance
comparison. Stable 60 FPS remains unproven. The trace and runtime log are retained
as palette-cache-mission2-trace.log and palette-cache-mission2.log.

### Reusable receiver light lists (2026-11-15)

The reusable scene API now provides RenderSceneSetInstanceLights and
RenderSceneClearInstanceLights. Lists are owned copies, canonicalized and
validated against live light handles. Default instances remain unrestricted;
an explicitly empty list receives ambient/emission only. Repeating the same list
does not advance scene revision. Invalid input leaves the previous list intact.
Destroying a light removes it from receiver lists, destroying an instance removes
its list, and mission reset clears every list. Shadow casting remains independent
of receiver linking.

The world renderer translates copied handles to current enabled GPU light
indices, deduplicates identical lists, and appends them to its existing tile-index
storage buffer. Per-draw constants expand from 72 to 80 bytes to pass a list
range. The vertex stage forwards that flat range; linked receivers iterate only
that range, while unrestricted receivers retain the existing global/tile loops.
No descriptor binding or minimum Vulkan feature was added. CPU regressions cover
copying, duplicates, no-op updates, invalid input, empty/default lists and instance
cleanup. GPU readback covers empty lists, restoring direct light, excluding a
10x-brighter unlisted light, switching emitter/GPU index, destroying a linked
light, and returning to unrestricted lighting across the two frame slots.

Linux thief2 built; all six standalone regressions passed with synchronization
validation and no validation errors/VUIDs/sync hazards. The CPU scene test passed
ASan/UBSan. Native decoder/material/attachment/fog/light regressions passed, and
a 60-frame Linux Mission 2 run completed normally on the unchanged unrestricted
path with synchronization validation and no Vulkan errors. Evidence is retained
as receiver-lights-*.log in build-linux/renderer-evidence.

Native light linking is the next required integration step, not implemented by
this API/shader foundation alone. A read-only native Mission 2 debugger traversal
measured 3071 cells, 550 static lights and zero dynamic lights at the initial
capture: per-cell list minimum/median/p95/maximum 0/12/40/70, with 1555 unique
lists. These are native source lists, not counts of enabled GPU lights. Their
indices were checked against the native static-light count. The traversal script,
log and JSON are retained as inspect-cell-lights.gdb and native-cell-light-counts.*.
The adapter should preserve existing terrain chunking by taking a conservative
union of the contributing cells' static lights, rather than splitting terrain
into thousands of additional draws. Moving model receivers can use their current
cell list. Dynamic lights must remain eligible separately. Native AI/stealth
lighting and source cell lists must remain unchanged. No reduction in native
light leakage or frame time is claimed until that adapter is implemented and
measured. Windows and DromEd remain deferred.

### Native cell reach integrated with receiver lists (2026-11-15)

Native light records are copied before model import so receiver links always
refer to live scene handles. Terrain chunks retain their contributing cell IDs
and receive a conservative union of those cells' static lights; existing spatial,
material and fog chunking is unchanged (896 terrain chunks in Mission 2). Models
resolve a copied location to their current native cell, including fresh locations
for attachments. Stationary offscreen objects cache that read-only BSP lookup.
Every copied list also includes current dynamic-light handles. Invalid cells or
malformed/missing source lists retain unrestricted lighting rather than silently
darkening geometry. The adapter never edits cell lists, object locations or
native light/stealth records. `renderer_cell_lights` defaults to 1; zero is the
diagnostic unrestricted comparison path.

Cell translations use a dense cache indexed by native cell ID. They compare
copied source indices, static-light count and light-handle topology, and only
advance a cell revision when membership changes. Terrain unions and model lists
update when those revisions change. GPU list packing has a separate cache keyed
by receiver-list revision, scene identity and current enabled GPU light order;
relative list offsets are rebased after the frame's tile-index prefix. Pose,
material and camera changes do not invalidate unchanged receiver packing.

All six renderer regressions passed with synchronization validation, including
new readbacks for enabled-light remapping, changed tile-prefix size after target
resize, and a queued frame retaining its prior list after another slot changes.
The native decoder regressions passed. The new retail fixture
`tools/world_scene_test/native_cell_lights.py` independently reads native static
indices and checks all three parts of object 11: cell 70 initially has one static
light; an injected native dynamic light is eligible in every linked receiver for
one frame, then disappears from the store and all lists. Moving the object via
the native position API into cell 0 updates its three parts to that cell's three
static lights. The final four-frame fixture passed with synchronization
validation. Its changes are confined to a disposable process; no mission is
saved. Linux Mission 2 also completed 120 synchronization-validated frames with
linking enabled and no validation errors/VUIDs/sync hazards.

Retained RenderDoc capture native-cell-linked_frame33.rdc and final PNG show the
Mission 2 dock after linking: terrain, crates, window lights, sky and HUD remain
visible. Final color event is 77819. This is a native visual check, not proof that
all conservative chunk unions or shadow edges are artifact-free. Capture/logs,
native fixture logs, renderer validation and benchmark traces are retained in
build-linux/renderer-evidence under native-cell-* and native-link-*.

A final same-build diagnostic comparison used sequential normal 180-frame Linux
Mission 2 runs at 1080p/PCSS, with the first eight samples discarded. With cell
linking disabled/enabled respectively: GPU world median 11.675/4.645 ms, p95
12.714/6.295 ms; CPU capture median 2.763/3.091 ms, p95 3.198/3.499 ms; recording
median 5.796/6.163 ms, p95 6.855/8.571 ms. Modern frame intervals (171 samples)
were 16.208/16.911 ms median and 18.353/20.429 ms p95. Thus the large GPU saving
does not establish a total frame-time improvement on this CPU-bound machine.
Upload/tile CPU p95 increased from 2.124 to 3.411 ms, consistent with remaining
list-repacking work when membership changes; further profiling is required.
The original uncached implementation was worse (19.016 ms median frame interval)
and was replaced with the caches above. Stable 60 FPS remains unfinished.
Conservative terrain unions can still admit extra lights within a chunk, and
budgeted unshadowed lights can still leak around occluders. Shadow-budget
relevance, broad native visual/motion checks, mixed transparency and remaining
lifetime cases are still open. Windows and DromEd remain deferred.
