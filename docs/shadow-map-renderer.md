# Vulkan shadow-map replacement

## Native effects with opaque portal drawing disabled

`renderer_portal_rendering=0` now retains native water and late translucent
effects. The modern opaque pass exports a separate R32 float view-distance
attachment. Composition converts that distance to the copied native encoding
`a*d+b/d+c` and writes the native depth attachment. This supports linear and
reciprocal native depth without trying to reconstruct distance from a rounded
world-to-clip depth matrix. A translated/pitched camera exposed unacceptable
precision loss in that initial reconstruction approach; direct distance export
passes the independent depth readback checks.

The optional attachment and three world pipeline variants are created on demand;
the ordinary shader variant has no distance output. At 1080p the attachment adds
about 16 MiB across two frame slots before driver padding. R32 sampling, color
attachment and color blending support are checked; unsupported devices retain
native rendering. Native depth images keep their existing usage/store behavior.
The two color attachments use identical blend states: opaque distance has source
alpha one, while blended surfaces export alpha zero to retain opaque distance.
No independent-blend device feature is required. Target replacement is published
only after all allocations and framebuffer creation succeed.

Native drawing is enabled at the portal post-render boundary. Opaque queued
models are suppressed separately, then the water/alpha queue and chained native
callbacks retain drawing. This handles callback chains that run around the
object queue rather than assuming every late effect is inside it. Visibility,
pose, portal traversal and native gameplay lighting calculations still run.
This section supersedes older notes saying the diagnostic mode omitted water
and particle drawing; mixed modern/native translucent ordering and exhaustive
particle/weather coverage remain unverified.

Fresh GPU tests under Khronos synchronization validation pass on indirect and
scalar devices. They read the actual native depth attachment in both slots for
linear/reciprocal encodings, translated/pitched cameras, masked holes, empty
coverage, blended-over-opaque preservation, and disabling the bridge. The native
Mission 2 water fixture records 11 water batches, 22 water draw calls and 12 other
late-effect calls with no suppressed water/late draws for either portal setting.
Warm submission-rejection recovery and a partial-allocation failure fallback
also pass validation with opaque portal drawing disabled.

The final 1920x1080 RenderDoc capture
`native-water-modern-complete-no-portals_frame51.rdc` was replayed locally and
inspected. Its image differs from the portal-enabled reference
`native-water-modern-linear_frame50.png` at only one of 2,073,600 pixels in this
stationary view. These are separate capture runs, not a deterministic golden
image test. Pixel history at (1300,600) shows a water fragment behind opaque depth
failing, a front fragment passing, and the later native overlay contributing.
The resulting color matches the reference at that pixel. Artifacts, image
comparison, pixel histories and validation logs are in
`build-linux/renderer-evidence/` with prefixes `native-depth-` and
`native-water-modern-complete-no-portals_frame51`. This verifies the selected
Mission 2 view rather than all underwater/fog and transparency interactions.

Two alternating 240-frame Mission 2 Wayland pairs at 1920x1080, default shadow
settings and portal visibility off (first 60 samples discarded) measured median
frame intervals of 30.79/22.19 ms and 20.59/20.03 ms for portal drawing on/off.
Lighting GPU medians were 12.92/14.33 ms and 13.46/13.44 ms. The inconsistent
first pair does not establish a stable performance gain or sustained 60 FPS.
All samples are in `native-depth-cost-performance.json`. The default lighting
shader's SPIR-V is identical to the pre-bridge version after stripping debug
metadata and compacting IDs (`native-depth-default-shader-comparison.json`).

A fresh RenderDoc pair after the texel-center correction, camera-independent
offset and integer fetch changes rechecks this native-effects path. Captures
`native-water-modern-texel-fetch_frame51.rdc` (portal drawing enabled) and
`native-water-modern-texel-fetch-no-portals_frame51.rdc` have matching camera and
1920x1080 output. Only one of 2,073,600 pixels differs, at (1309,734), with maximum
channel difference 6/255. At (1300,600), the portal-disabled frame composites
native depth 0.92290628, rejects water depth 0.95448196, accepts front water depth
0.92284548 and retains the late translucent contribution. Companion PNG/JSON
files contain the rendered images and pixel histories;
`texel-fetch-portal-image-comparison.json` records the image comparison. These
captures verify this view after the shader changes, not every native effect or
all transparency ordering. The RenderDoc MCP server again failed replay with
missing `VK_KHR_surface`; local RenderDoc replay on a private X display succeeded.

## Optimize compiled shader bytecode outside Debug builds

The native and standalone test shader commands now pass `glslc -O` for
non-Debug configurations and `-O0` for Debug. Previously Release shader builds
used glslc's unoptimized default. The shader sources, light budgets, PCSS sample
counts and resolution settings are unchanged. Linux Release and both normal
and feature-disabled scalar test suites pass Khronos synchronization validation;
all 138 printed lighting diagnostics exactly match the pre-optimization path.

Two Mission 2 1080p camera-yaw timing pairs ran before/after and after/before,
with 180 frames, the first 30 discarded and stable-depth composition enabled.
Median lighting GPU time was 23.533/21.242 ms and 22.650/20.957 ms respectively;
shadow GPU medians remained about 3.5 ms. The isolated saved baseline has its
own shader files. This is a repeatable improvement in this fixture, not proof
of sustained 60 FPS; GDB camera control and uncontrolled clocks remain limits.
Logs and samples use the `shader-opt-*` prefix in the renderer evidence directory,
including `shader-opt-performance.json`. No Windows or DromEd build was run.

Point cube-face conversion now groups opposite faces by their common axis,
using a sign for each pair. This reduces two six-way conditional chains to two
three-way axis choices. All 138 existing printed lighting diagnostics remain
exactly equal. A new scene-rotation regression exercises all six cube directions
in both active/distant pools and both frame slots (24 comparisons), requiring a
clear reference above 0.1 and occluded radiance below 10% of that reference.
Both indirect and scalar suites pass synchronization validation. The native
180-frame camera sweep also exits normally with modern lighting enabled.

This simplification did not show a useful performance improvement: the paired
1080p sweep measured median lighting GPU 20.959/20.950 ms, shadow GPU
3.574/3.626 ms and recording CPU 7.298/7.315 ms before/after. No speedup is
claimed. The durable gain is simpler face conversion plus explicit six-face
coverage. Evidence uses `cube-axis-*`, including `cube-axis-performance.json`.

## Early depth rejection for the lighting pass

The mapped lighting shader now requests `early_fragment_tests`. Both opaque and
blended lighting pipelines test depth without writing it; opaque coverage and
cutouts are established by the separate depth pass, which retains its original
shader. Thus hidden lighting fragments can be rejected before PCSS work without
changing alpha-cutout depth writes or translucent blending. All 162 printed
lighting diagnostics match the preceding shader, and all seven tests pass
synchronization validation with indirect and scalar drawing. A native Mission 1
camera sweep with portal visibility enabled and native opaque drawing disabled
also passes validation and exits normally with modern lighting enabled.

Two Mission 2 1080p camera-yaw timing pairs, in before/after and after/before
order, measured median lighting GPU 20.939/20.472 ms and 20.950/20.776 ms.
Each run has 180 frames, discards the first 30 and enables stable-depth caching.
These small fixture gains do not establish sustained FPS. Evidence uses the
`early-depth-*` prefix, including `early-depth-performance.json`.

A separate single visibility comparison before this change measured lighting
GPU 21.232 ms off versus 18.670 ms on, but recording CPU 6.728 versus 9.006 ms.
The existing visibility default remains off pending broader visual/performance
coverage. The camera fixture now accepts `--mission`, `--visibility` and
`--portal-drawing` so these combinations can be reproduced explicitly. Trace
files are `visibility-current-off-trace.log` and
`visibility-current-on-trace.log`.

## Cache packed material tables independently of camera updates

`WorldScene::materialsRevision` now tracks successful material creation, edits,
removal and scene clearing. Identical material updates return without invalidating
the table; texture-pixel updates do not change the material revision. The Vulkan
renderer caches sorted material indices, packed material data and texture-handle
order by scene identity and this revision. Texture images are still refreshed
through their own revisions each frame. Each retired frame slot receives its
own GPU buffer data and retains its referenced texture images as before.

CPU checks cover create/edit/unchanged/remove/clear revision behavior and texture
replacement independence. Existing GPU checks exercise texture replacement,
alpha changes, animation-frame switches and emission edits. A new GPU lifecycle
case keeps the renderer alive through two full scene resets with the same
material count, then requires the replacement emission value in both frame
slots. All seven tests pass synchronization validation on indirect and scalar
devices; native Mission 2 camera motion with visibility enabled and native opaque
drawing disabled also passes validation.

Two Mission 2 1080p camera-yaw pairs with portal visibility and stable-depth
composition enabled measured preparation CPU medians 4.543/4.277 ms and
4.572/4.311 ms before/after. Total recording CPU medians were 10.073/9.629 ms
and 9.754/9.694 ms, showing the preparation saving is more stable than total
recording time. Lighting GPU stayed about 18.8–18.9 ms. Runs use 180 frames,
discard 30 initial samples and reverse pair order; they do not establish
sustained gameplay FPS. Evidence uses `material-cache-*`, including
`material-cache-performance.json`. The overall performance target remains open.

## Incremental GPU geometry updates

The copied scene now records mesh creation, vertex updates, deletion and reset
in an allocation-free 4096-entry change ring. Each retired geometry frame slot
tracks its own consumed revision and processes only subsequent changes. Reset
markers, a different scene identity or falling behind the bounded history force
full reconciliation. The slot revision advances only after a successful update.
This avoids scanning thousands of unchanged terrain meshes on every animated
frame while retaining independent GPU-buffer lifetime per slot. Scene mutations
must use the RenderScene API to maintain its revision/change history.

GPU readback checks cover pose uploads without changing the other slot,
incremental creation/removal, and history overflow with a deletion overwritten
by more than 4096 subsequent updates. Full reconciliation must remove that stale
mesh and upload exact current vertices/indices in each slot. Another case switches
the geometry cache between two scene objects with matching handles/revisions and
different vertex data, requiring exact independent readback. Scene-reset lighting
checks from the material-cache work remain enabled. All seven tests pass
synchronization validation on indirect and scalar devices. The Linux game build,
Mission 1 torch route, Mission 2 visibility camera sweep and injected allocation
failure/native fallback checks also pass.

Two Mission 2 1080p camera-yaw pairs with portal visibility and stable-depth
composition enabled measured preparation CPU medians 4.268/3.518 ms and
4.196/3.315 ms before/after. Total recording CPU medians were 9.615/8.890 ms
and 9.265/8.281 ms. Lighting GPU remained about 18.6–18.9 ms. Runs use 180
frames, discard 30 initial samples and reverse pair order; GDB-controlled timing
does not establish sustained gameplay FPS. Evidence uses `mesh-journal-*`,
including `mesh-journal-performance.json`; the final scene-identity geometry fix
does not affect the benchmark's single-scene update path. The broader renderer
and 60 FPS goals remain incomplete.

## Scene identity invalidation for retained shadows and textures

Texture cache entries now include the source scene identity as well as texture
handle/revision. The white fallback image remains scene independent. Retained
shadow maps track scene identity per retired frame slot: a change clears working
and stable-base signatures, culling memberships, caster bounds history and
stable-key ages. The existing depth images remain allocated for reuse. Each slot
commits its new scene identity only after successful shadow recording. Queued
frames retain their own texture references, geometry and shadow images.

New GPU cases use separate scene objects with matching handles and revisions.
Their blocker vertices differ while valid bounds stay identical, preventing a
changed projection/bounds key from accidentally masking stale depth reuse. The
old shadow cache fails this case; after fixing shadows, a separate changed-pixel
texture case reproduces stale image reuse. Expected failures are saved as
`scene-shadow-cache-reproduction-validation.log` and
`scene-texture-cache-reproduction-validation.log`.

The corrected cases cover active/cached point and spot shadows, directional
cascades, stable-depth composition off/on, return transitions and frames queued
before a second slot switches scenes. Texture checks require both replacement
pixels and preservation of the queued frame's old image. All seven tests pass
synchronization validation on indirect and scalar devices. Linux native camera
motion also passes validation. A warm static-cache run rejects one modern
submission after 12 successes, rebuilds the renderer and completes six recovered
modern submissions with clean shutdown (`scene-cache-submit-recovery.log`).
Artifacts use `scene-cache-*`; these functional checks make no performance claim.

## Normal-process timing and unchanged vertex submissions

`native_performance.py` runs the stationary retail mission directly, without
GDB. It checks the requested modern-record count, all timing groups, successful
exit and validation/fatal markers, then emits JSON with median/mean/p95/p99/max
submission intervals and CPU/GPU phases. Default duration is 300 frames with
60 discarded warmup samples at 1920x1080. Presentation records may be one fewer
than world records at shutdown. The derived FPS field is a submission-rate
equivalent, not a measurement of displayed frames or sustained traversal FPS.
See the test README for commands and raw trace/log options.

An initial explicit-default Mission 2 run (static cache 0, visibility 0, native
portal drawing 1) measured median interval 19.732 ms, p95 22.928 ms and mean
submission-rate equivalent 50.20 FPS (`native-default-performance.json`). Later
baseline pairs varied around 20–21 ms median, reinforcing that these samples do
not establish a fixed performance result. With static cache 1 and visibility 0,
a separate run measured median 18.391 ms, p95 21.938 ms. With both enabled it
measured median 16.143 ms, p95 20.457 ms, mean submission-rate equivalent 59.63
FPS, world GPU 13.875 ms and recording CPU 5.324 ms. Evidence is
`native-static-performance.json` and `native-static-visible-performance.json`.
Defaults are unchanged, and stable 60 FPS remains unproven.

Identical `RenderSceneUpdateVertices` submissions now preserve the mesh revision,
bounds and change history instead of forcing new uploads and shadow keys. The
comparison includes all position, normal and UV bytes; actual changes retain
the existing update path. A GPU regression requires the unchanged pose to keep
all six point shadow maps reusable. Both indirect and scalar suites pass all
seven tests under synchronization validation, and a normal 30-frame native run
with both optional optimizations enabled passes validation.

The normal-process before/after pairs do not show a reliable FPS improvement
from this no-op change: median intervals were 21.025/21.047 ms and
19.969/20.658 ms, while median shadow workload stayed unchanged. No native
speedup is claimed for this particular view. Artifacts use `unchanged-pose-*`,
including `unchanged-pose-performance-comparison.json`. Validation-run timing
samples are diagnostic only and are excluded from the performance comparisons.

## Camera-independent receiver offset

The lighting shader no longer scales its receiver lookup offset by distance to
the camera. That changed the shadow query point when only the player position
changed. It now computes one lookup point per fragment using the larger of
0.002 world units and two float machine epsilons times the largest absolute
world-position component. This depends on coordinate precision rather than eye
distance, and avoids recomputing camera distance for every contributing light.
The shading normal orientation and BRDF remain view dependent as before.

The GPU regression holds projection/geometry fixed, varies only the eye-position
uniform from z=-3 to z=-30000, and normalizes each occluded sample against its
own unoccluded BRDF response. This deliberately isolates the lookup offset;
it is not a physically consistent moving-camera visual comparison. With the
old offset restored temporarily, the point case fails with maximum visibility
difference 0.906300. With the correction, point/spot maximum differences are
0.000429/0.000388 (half-float readback), below the 0.01 requirement. All earlier
clear-plane, penumbra, contact and crossover checks remain enabled. Evidence is
recorded under `camera-independent-offset-*`; the expected old-code failure is
`camera-dependent-offset-reproduction-validation.log` in the renderer evidence
directory. This removes a demonstrated camera-dependent input without claiming
all retail lighting changes are resolved.

## Compensate local shadow taps at sampled texel centers

Local shadow receiver-plane compensation now uses the center of the actual
nearest-filtered depth texel, including the selected cube face for point lights.
Previously it compensated only the requested tap UV/direction. Quantization left
a receiver-plane depth error that required a resolution-scaled bias and changed
shadow coverage when a light moved between active and distant maps. Point taps
intersect the sampled cube ray directly with the geometric receiver plane; spot
taps use the sampled texel UV. The local numerical bias is now 0.002 world units,
independent of resolution. Directional shadow filtering is unchanged.

Reducing bias alone was rejected: it visibly self-shadowed clear point/spot
receiver planes. Tests now exercise the existing smoothed-normal clear-plane
sweep in both 512-square active and 128-square distant pools. The former distant
point case at x=6 failed the existing 99% radiance requirement (0.1015 versus
0.1046); the corrected version matches the reference in all 20 cases. All seven
GPU/CPU tests pass with validation on both indirect and scalar devices. Native
Mission 2 camera motion also passes synchronization validation.

The penumbra scan now measures point small/large widths 2/8 in both pools;
spot widths are 2/11 active and 2/12 distant, compared with 2/12 and 2/8 before.
Contact hardening and the six point/spot/filter camera-crossover combinations
still pass. This reduces the measured discrepancy without establishing seamless
retail transitions or all material/geometry cases. Logs use the
`texel-center-final-*` prefix in `build-linux/renderer-evidence/`.

The correction has a measured GPU cost. A paired Mission 2 1080p camera-yaw
sweep (180 frames, first 30 discarded, stable-depth caching enabled) measured
median lighting GPU time 19.902 ms before and 24.348 ms after; shadow-map GPU
time was 3.548/3.604 ms and recording CPU time 7.349/7.273 ms. An intermediate
implementation measured 24.039 ms lighting. The saved baseline has separate
shader files, so these comparisons do not accidentally load current shaders.
Single sequential GDB-controlled runs do not establish sustained gameplay FPS,
but indicate a material lighting cost that still needs optimization. Evidence:
`texel-center-performance.json` and matching before/final trace logs. The
correction improves shadow consistency; the overall performance goal remains
unmet.

A follow-up removes the per-tap matrix transform from point receiver-plane
intersection: the existing geometric normal is already in world space, so a
direct dot product with the sampled cube ray is equivalent. Clear-plane,
penumbra, contact-hardening and crossover assertions retain their results.
The next single 180-frame Mission 2 camera sweep measured median lighting GPU
23.604 ms, shadow GPU 3.461 ms and recording CPU 6.752 ms. This is preliminary
rather than a controlled repeated speedup claim, and remains slower than the
original uncorrected filter. Evidence uses the `texel-world-normal-*` prefix;
performance samples are in `texel-world-normal-performance.json`.

Local depth taps now use integer `texelFetch` with the same clamped texel
coordinate used for plane compensation. This avoids asking a normalized nearest
sampler to select a texel after that texel has already been computed. Sun taps
retain their existing sampling path. All 138 printed clear-plane, eye-uniform,
penumbra/contact and crossover diagnostics exactly match the preceding path;
this is a comparison of those diagnostics, not a full-image equality claim.
All seven tests pass with indirect and scalar drawing under synchronization
validation, and the Linux game build and native camera sweep pass.

Two Mission 2 1080p camera-yaw timing pairs, run before/after and after/before,
measured median lighting GPU time 23.020/22.782 ms and 24.074/22.030 ms. Shadow
GPU medians stayed around 3.6 ms. Each run has 180 frames, discards 30 initial
samples and enables stable-depth caching. The consistent direction supports
retaining the change, but the varied magnitude is not a stable speedup or FPS
claim. Artifacts use the `texel-fetch-*` prefix, including
`texel-fetch-performance.json`; the saved baseline has independent shader files.
The renderer remains short of its overall performance target.

## Distant shadow coverage enabled by default

The modern renderer now enables `renderer_shadow_cache=1` by default. The
high-resolution pool still uses 16 local lights and 48 faces; a separate
128-square pool retains shadows for up to 128 additional lights/768 faces.
This prevents the verified Mission 1 emitter from becoming unshadowed as the
camera moves outside its high-resolution selection range. The extra arrays cost
96 MiB across two frame slots. `renderer_shadow_cache=0` explicitly restores the
previous budget-only behavior. The pool remains finite: dense scenes can exceed
combined coverage, and crossing resolutions can still change shadow detail.
Historical performance sections below describe the pool as opt-in; this section
supersedes those default-setting descriptions. No sustained 60 FPS claim is made.

The native torch camera fixture now omits the configuration override by default,
so its 122-frame route tests production defaults rather than forcing the fix on.
Explicit `--cache 0` retains the original distant-shadow-loss reproduction.
The Linux game build and default-setting route pass Khronos synchronization
validation (`build-linux/renderer-evidence/distant-default-native-validation.log`):
Mission 1 fireplace emitter object 805/light 18 has cached shadows at frames
2/10 and active shadows at frames 110/122, with coverage throughout the route.
This verifies that example, rather than claiming every mission light is covered.

The GPU lighting crossover regression now covers both point and spot lights in
hard, PCF, and PCSS modes. Each of the six combinations moves the camera across
nine positions in both frame slots, checks the emitter belongs to exactly one
pool, and requires both active and cached selection during the route. All 108
occluded receiver samples remain below 20% of their matching unoccluded
references (observed maximum 0.36%). Normal indirect and feature-disabled scalar
runs pass all seven tests under Khronos synchronization validation; logs are
`crossover-filters-tests.log` and `crossover-filters-scalar-tests.log` in
`build-linux/renderer-evidence/`. This verifies fully occluded receiver coverage
through the transition, not continuity of penumbra detail at different map
resolutions or every retail torch.

A separate GPU penumbra scan now checks point and spot PCSS in both the
512-square active pool and 128-square distant pool. Increasing source radius
from 0.1 to 0.4 increases intermediate-visibility pixels from 2 to 8 for point
lights in either pool; spot lights increase from 2 to 12 active and 2 to 8
distant. Moving the blocker toward the receiver while preserving its projected
silhouette reduces intermediate pixels to zero in all four cases, verifying
contact hardening. The spot result demonstrates remaining resolution-dependent
penumbra detail; these tests do not establish seamless transitions. Both
indirect and feature-disabled scalar suites pass synchronization validation
(`pool-penumbra-tests.log`, `pool-penumbra-validation.log`, and
`pool-penumbra-scalar-tests.log`).

Verification correction: an earlier multi-command build script continued after
a depth-readback test compile failure and ran its older executable. The missing
submission helper has been fixed. `tools/world_scene_test/run_checks.py` now
requires a successful fresh build before running CTest and scanning validation
logs. Fresh normal and scalar-device runs pass all seven tests, including exact
depth-image comparisons and stable/dynamic composition and removal checks.

## Allocate unused shadow pools as descriptor placeholders

An empty pool now uses a single 1x1 depth layer rather than one layer at its
configured resolution. The production renderer also treats sun capacity as
empty when the current selection has no shadow-casting directional light.
Adding sunlight restores the configured cascades before descriptor updates;
removing it retires each slot's cascades after that slot's fence. Local and
distant configured capacities stay reserved to preserve retained layer caches.

`world-shadow-memory slot_bytes=...` reports actual Vulkan image allocation
requirements for all working and optional stable-base arrays in the current
slot, excluding other renderer resources. The GPU test with default local
capacity and no distant pool measures 50,332,672 bytes without sunlight versus
117,441,024 with sunlight per slot. Thus an absent sun saves approximately
128 MiB across both slots, or 256 MiB with stable-depth composition enabled.
The exact byte count depends on driver allocation requirements.

Tests add and remove a directional light in both slots with composition off/on,
compare unused-sun allocation against an explicitly zero sun budget, require
local occlusion to survive, and verify that removing sunlight releases the
extra allocation. All seven tests pass on indirect and scalar devices under
synchronization validation. Native 24-frame Mission 2 runs pass with composition
off/on (`lazy-shadow-pools-*-native.log`). Mission 2 has a selected directional
light and still allocates its full sun pool; these runs do not demonstrate the
no-sun saving. Its actual per-slot image allocations are 160 MiB off and 320 MiB
on, recorded in `lazy-shadow-pools-native-memory.json`.

## Adaptive sun use of stable-depth composition

With `renderer_shadow_static_cache=1`, each sun layer now compares its current
depth-affecting view header with its preceding visit in the same frame slot.
Changed projections render the complete caster set directly. Stationary or
stabilized projections retain stable-base composition when geometry changes;
unchanged complete maps still reuse their final depth. Point/spot behavior is
unchanged. No updates are deferred, and camera motion does not reallocate arrays.

The GPU regression forces fresh sun storage, checks that the first views bypass
composition, moves one caster while another remains stable, then compares the
entire composed depth image against a forced full redraw in both slots. It also
retains the existing caster removal, alpha, range and array-resize checks. All
seven tests pass with static caching enabled, disabled and scalar submission
under synchronization validation. A 30-frame native Mission 2 yaw sweep also
passes validation.

Two 180-frame Mission 2 camera sweeps at 1920x1080 (first 30 samples discarded)
measured median shadow GPU time 3.806/3.770 ms before and 3.594/3.506 ms with the
adaptive policy. Stationary controls measured 2.760/2.748 ms before and
2.709/2.789 ms after. These GDB-driven runs measure diagnostics, not gameplay FPS;
their stops affect scheduling and simulation timing. Raw traces and medians/p95
are in `sun-adaptive-performance.json` and `sun-direct-performance.json`.

An all-direct sun experiment saved 128 MiB but increased stationary shadow GPU
time to 3.183/3.240 ms, so that policy was not retained. The final adaptive policy
keeps the previous memory allocation (320 MiB per slot in this mission with
static caching enabled). The option still defaults off. The original optional
cache memory cost and sustained-performance limitations remain applicable.

## Optional stable-depth composition

`renderer_shadow_static_cache=1` retains an additional stable-caster depth base
for each shadow layer. A caster enters that base after four unchanged visits to
the same frame slot. Pose, transform, mesh, material/alpha or emitter changes
remove it immediately; removals and topology changes invalidate membership.
Dirty local views and stable sun projections copy the current stable base, then draw current changing casters
with depth load enabled. Unchanged complete views still reuse their final depth.
The stability window never delays an update to the visible shadow.

Stable membership and its depth signature are reused only while the stable set,
projection and emitter exclusion agree. Working images request transfer-destination
usage only when this option is enabled. The option defaults off: with default
active, sun and distant arrays it adds 320 MiB across two frame slots, doubling
their D32 storage. This is separate from the default distant coverage pool.

Fresh GPU tests compare complete depth images against full redraws, including
both frame slots, moving casters and removal of a previously stable caster.
The suite passes on both indirect-capable and explicitly scalar devices under
synchronization validation. The native camera fixture accepts `--static-depth`
to exercise the composition path while crossing shadow pools.
The 122-frame Mission 1 route and a 24-frame Mission 2 native run both pass
Khronos synchronization validation with composition enabled; logs are
`static-cohort-torch-validation.log` and `static-cohort-native-validation.log`
in the renderer evidence directory. The route checks actual shadow selection;
complete-image equality is established by the GPU regression rather than this
native selection fixture.

Two alternating off/on pairs of 240-frame Mission 2 stationary runs at 1920x1080
PCSS, with portal visibility enabled and the first 60 samples discarded, measured:

| Median | Composition off | Composition on |
| --- | --- | --- |
| Frame interval | 22.31 / 22.38 ms | 18.17 / 17.89 ms |
| Shadow GPU | 6.51 / 6.51 ms | 2.73 / 2.76 ms |
| World GPU | 16.99 / 16.96 ms | 13.18 / 13.20 ms |
| Command recording CPU | 11.55 / 11.47 ms | 7.50 / 7.39 ms |

Frame-interval p95 was 26.90/26.40 ms off and 22.94/22.60 ms on. All four
processes exited successfully. Raw traces and summaries are in
`build-linux/renderer-evidence/static-cohort-*-trace.log` and
`static-cohort-performance.json`. These short stationary runs do not establish
sustained 60 FPS or traversal performance.

## Skip zero-weight adjacent sun filtering

Sun visibility now filters the adjacent cascade only when its blend weight is
greater than zero. Previously that filter ran even outside the transition band,
where its result was multiplied by zero. Cascade selection, blend shape, final
distance fade and PCSS sample counts are unchanged. The existing lighting suite
checks clear and occluded receivers across eight split/blend distances, as well
as hard/PCF/PCSS and directional penumbrae. All seven fresh tests pass under
synchronization validation on indirect and scalar devices.

A 24-frame native Mission 2 run with default shadow settings also passes
synchronization validation (`cascade-blend-native-validation.log`).

Timing evidence is limited. Two alternating 240-frame X11 off/on pairs (first
60 samples discarded) gave lighting medians 12.91/11.83 ms and 12.06/12.12 ms:
mixed results with 34–35 ms of median presentation overhead. A separate Wayland
pair gave lighting 10.34/10.18 ms and world GPU 13.15/12.95 ms, while median
frame intervals were 16.78/17.15 ms. These runs used Mission 2, 1920x1080 PCSS,
portal visibility and stable-depth composition. They do not establish a frame-rate
improvement. Summaries are `cascade-blend-x11-performance.json` and
`cascade-blend-wayland-performance.json` in the renderer evidence directory.

## Shared hierarchy for fitted local shadow ranges

Unbounded local lights now query a hierarchy over the current oriented scene
bounds instead of independently scanning every bound. Node AABBs prune groups
that cannot contain a farther corner; leaves retain exact oriented-corner
evaluation and the existing rounding margin. Small scenes keep the flat path.
Active and distant plan construction share this hierarchy, avoiding a second
scene-bounds collection. It is rebuilt from current geometry/poses on every
plan call and does not retain stale bounds across frames. Visible receivers and
offscreen casters retain their previous eligibility rules; far-plane fitting
does not use a distance cutoff or approximate enclosing box as its result.

The 122-frame Mission 1 emitter camera route and a visibility-enabled native
Mission 2 run also pass synchronization validation (`bounds-tree-torch-validation.log`
and `bounds-tree-native-validation.log` in the renderer evidence directory).

The existing CPU camera oracle compares fitted ranges against brute-force
transformed corners over 160 rotated/mirrored/sheared instances and 80 light
origins, including million-unit coordinates. It passes with AddressSanitizer and
UndefinedBehaviorSanitizer. All seven fresh renderer tests pass under Vulkan
synchronization validation on indirect and scalar devices.

Two sequential before/after pairs of 240-frame Mission 2 Wayland runs at
1920x1080 PCSS, default distant shadows, stable-depth composition disabled, and
the first 60 samples discarded measured median planning CPU time:

| Portal visibility | Before | Hierarchy |
| --- | --- | --- |
| Off (default) | 1.625 / 1.607 ms | 1.042 / 1.040 ms |
| On | 2.632 / 2.600 ms | 1.869 / 1.889 ms |

Visibility-on median frame intervals changed from 20.91/20.88 to 20.03/20.24 ms;
visibility-off results were mixed (19.49/19.77 to 19.82/19.14 ms). These are short
stationary runs, not sustained traversal evidence. The before runs preceded the
after runs rather than alternating binaries. `bounds-tree-performance.json`
contains all medians, p95 values and sample counts. Portal visibility remains
opt-in: its lower GPU time did not offset its additional CPU cost in the initial
comparison, so changing the default was not justified by these measurements.

## Lazy range-bounds collection and hierarchy construction

The shared range-query object now collects bounds only when needed. Finite-range
local lights use their authored range without collecting scene bounds. Sun-only
plans collect oriented bounds for their depth fit but do not build the farthest
corner hierarchy. The first unbounded local query constructs the hierarchy once
for reuse by both pools. Its lifetime remains one plan call, so current geometry
and poses are always used.

A synthetic CPU benchmark with 8,192 instances, 128 plan calls per case and
three alternating old/new binary pairs measured median per-plan time:

| Light case | Eager bounds/hierarchy | Lazy |
| --- | --- | --- |
| Finite point | 0.7191 ms | 0.0004 ms |
| Sun only | 0.7403 ms | 0.2100 ms |
| Unbounded point | 0.7193 ms | 0.7171 ms |

The benchmark's far-depth checksums match in all cases; the existing independent
corner/range and GPU lighting regressions provide correctness coverage. All
seven fresh renderer tests pass under synchronization validation with both
indirect and scalar device features. The benchmark source, binaries, raw runs
and `bounds-lazy-performance.json` are retained in the renderer evidence
directory. These are synthetic CPU timings, not gameplay FPS measurements.
A 24-frame native Mission 2 run with default renderer settings also passes
synchronization validation (`bounds-lazy-native-validation.log`).

## Geometry arena retirement

Geometry arena free ranges now stay sorted during allocation splits. Releasing
an allocation uses an ordered insertion and joins adjacent ranges instead of
sorting and rescanning the entire free list. Live allocations and frame-slot
ownership are unchanged. The CPU range test uses 20,000 randomized allocation
and release operations against an independent byte-occupancy oracle, including
mixed vertex/index alignments, full coalescence and alignment overflow.

All seven renderer tests pass under synchronization validation, including GPU
geometry and exact scalar/indirect depth-image comparison. Matched 60-frame
Mission 2 runs with distant shadows and cell receivers enabled measured process
exit 7.192 seconds after the last present before the change and 0.275 seconds
afterward. Total process times were 11.933/4.972 seconds, with 59 recorded game
presents in each. The same test-only sampler was used for both timings. This
fixes resource retirement cost, not the remaining steady gameplay GPU budget.

## Steady-frame key and packet preparation

An isolated test helper sampled main-thread userspace CPU work, separating gameplay
from startup and shutdown using presentation timestamps. The 600-frame Mission 2 run yielded
11,542 samples between the trimmed presentation boundaries: shadow recording
accounted for 4,533 samples and planning 954. The allocator sorting that dominated
the untrimmed profile was absent from the leading steady hotspots; it is a separate
shutdown concern. Assembly at sampled recording addresses confirmed eight dependent
byte hashes of each already-hashed caster key and repeated packet construction.

Per-view signatures now combine each 64-bit caster key as one word, preserving
the same ordered set and depth-affecting dependencies. Validated indexed-draw
fields, emitter identity and emitter-surface state are prepared once per caster.
Indirect packets accumulate in a retained CPU vector and copy to coherent mapped
storage in bulk before queue submission. Per-slot ownership and scalar fallback
are unchanged. All six tests and the feature-disabled depth/lighting suites pass
under synchronization validation, including exact scalar/batched depth images.

Final quiet sequential 180-frame Mission 2 1080p PCSS runs (first eight modern
samples discarded) measured distant pool off/on median intervals 16.102/20.993 ms
and p95 19.486/25.553 ms. With the pool enabled, recording was 10.845 ms median
and shadow recording 6.054 ms, versus 13.302/8.566 ms in the earlier batched run.
Modern GPU time remained 17.340 ms and logical caster draws about 473,655 per
frame. The separate animated runs support a CPU gain, not a GPU quality/budget
reduction or sustained 60 FPS. A 24-frame native run passes synchronization
validation with the optimized path.

## Batched shadow submission

Modern device creation optionally enables the Vulkan core `multiDrawIndirect`
and `drawIndirectFirstInstance` pair. Both are required for shadow batching;
unsupported devices retain scalar draws. `renderer_shadow_indirect=0` forces
the scalar path on supported devices. No RT, device-address or shader-int64
feature is introduced.

Each retired frame slot owns a mapped instance SSBO and indirect command buffer.
Instances contain the same model/material data used by scalar push constants.
Compatible consecutive caster draws share one indexed indirect submission, split
at vertex/index buffer changes or the device's indirect-count limit. View push
constants are set once per dirty map. Array signatures, culling, alpha tests,
shadow budgets/resolutions, filters and geometry are unchanged. Buffers resize
only after slot retirement. `world-shadow-submission` records physical batches;
the existing draw count remains the number of logical caster draws.

All six Linux renderer tests pass under synchronization validation, including
the complete scalar/indirect depth-image comparison for transformed casters.
Depth and lighting suites also pass on devices created with the optional feature
pair deliberately disabled. The lighting test verifies real multi-command
batching rather than only setting the option. A 24-frame Mission 2 run passes
synchronization validation with distant shadows and cell receivers enabled.

Quiet sequential 180-frame Mission 2 1080p PCSS runs (first eight modern samples
discarded) measured scalar/batched median intervals 38.868/23.558 ms and p95
44.026/29.690 ms. Recording CPU medians were 28.706/13.302 ms; modern GPU
18.940/17.262 ms. Roughly 468–471 thousand logical caster draws became 391
physical batches at the batched median. Separate animated mission runs are not
an exact draw-count or image comparison. This reduces CPU cost but does not
meet sustained 60 FPS; distant shadows remain opt-in.

A new native Mission 2 RenderDoc capture contains 431 indirect submissions in
the recorded frame. Local replay succeeds and the final dock image retains
terrain, crates, native water, sky and HUD. MCP replay still reports the known
surface-extension environment error, so inspection used local RenderDoc replay
with the private X display instead.

## Cell-specific terrain receivers

When imported with `renderer_portal_visibility=1`, terrain now has separate
per-cell/material raster meshes and the existing merged shadow-caster meshes.
Each raster receiver uses its own native cell light list; merged-cell unions no
longer determine its lighting. Raster copies never cast shadows. The merged
copies remain casters and become the raster fallback when the visibility snapshot
is missing or the option is disabled. The default path creates no extra copies.

Mission 2 retains 896 caster meshes containing 44,348 triangles. Its 7,834 cell
receiver meshes contain the same triangles; 485 receivers are visible in the
initial fixture. Tests compare triangle multisets including vertex attributes,
material, transform and fog flags, verify exactly one caster copy, invalidate the
snapshot to restore merged drawing, and restore filtered drawing next frame.
Mission 1/2 fixtures pass synchronization validation, including native drawing
disabled in Mission 2. Model spanning-cell checks remain intact.

Quiet 180-frame Mission 2 runs at 1080p PCSS (eight modern samples discarded)
measured cache off/on median intervals 16.860/37.930 ms and p95 20.260/42.901 ms.
Recording CPU medians were 6.153/27.779 ms; modern GPU 5.057/16.877 ms. Eligible
light medians fell to 248/237, but the changed selection increased logical caster
draws to about 32,486/468,340. Thus this improves receiver precision, not frame
time. Visibility and the distant pool remain opt-in; batching shadow submission
is still needed rather than claiming this meets the frame budget.

The fresh Mission 1 capture with cell receivers was replayed locally after the
MCP replay process reported an unsupported surface extension. The exported final
image retains the nearby character, first-person sword, sky, HUD and terrain.
This is one view, not full traversal coverage.

## Mission 2 cache scale and copied object visibility

The distant pool remains opt-in. Mission 2's start view has 540 eligible lights;
128 cached slots plus the active pool cannot cover them all. A 24-frame cached
run passes synchronization validation. Sequential 180-frame 1080p PCSS runs
(first eight modern samples discarded) measured cache off/on median intervals
16.144/33.927 ms, recording CPU 5.210/23.895 ms and modern GPU 5.005/18.254 ms.
Cached shadow draws were about 396,887 per frame. Mission 1's near-60 result does
not establish acceptable Mission 2 performance.

The optional `renderer_portal_visibility=1` path now filters world models using
the union of all their native cell references. It retains objects spanning any
visible cell rather than relying on their origin cell; attachments stay
conservative. World objects with no cell refs are omitted from raster drawing,
matching native discovery, while unknown/malformed reference data falls back
to drawing. Shadow-caster flags are unchanged. In the Mission 2 start view this
reduces frustum raster candidates from 3,833 to 339 and removes an out-of-world
plaque's unrestricted receiver. Conservative terrain chunk light unions still
span 548 lights, so the cache coverage/performance problem is not resolved.

Native fixtures independently compare model flags against raw native reference
lists: 1,998 parts including 14 spanning visible/hidden cells in Mission 1, and
3,570 parts including 39 spanning cells in Mission 2. Both pass synchronization
validation, including Mission 2 with native world drawing off. Snapshot
invalidation restores all terrain and model raster visibility. Broader visual
and traversal checks remain necessary; visibility filtering still defaults off.

Quiet sequential 180-frame Mission 2 runs with the current build measured
visibility off/on without the distant pool at 16.086/16.059 ms median intervals
(p95 18.520/18.423 ms). Recording CPU decreased from 5.208 to 4.588 ms; modern GPU
time was 4.856/5.010 ms. Steady eligible-light count fell from 540 to 298. Adding
the distant pool with visibility enabled still cost 33.270 ms median / 37.788 ms
p95, with 23.672 ms recording and 21.549 ms modern GPU time. This does not solve
the Mission 2 shadow cost or meet sustained 60 FPS.

A fresh Mission 1 RenderDoc capture with copied model visibility enabled retains
the nearby character, sword, sky, HUD and expected terrain. The modern color/depth
pass contains 272 draws rather than roughly 2,593 in the earlier terrain-only
visibility capture; shadow casters remain independent of raster visibility.

## Visible retail emitter comparison

The native camera fixture now searches copied terrain and model triangles,
including alpha-mask samples, for a clear initial view of a flame emitter. It
checks native cells along the route before rendering, then verifies every
actual frame position resolves to a cell. In vanilla Mission 1 it finds a
60-to-2-foot route to fireplace flame object 805. This corrects the earlier
80-foot route whose distant camera was outside the closed door.

The fixture reads the production renderer's selection after each recording.
Cache disabled: object 805/light 18 is unshadowed at distant frames 2/10 and
active at near frames 110/122. Cache enabled: it is cached in distant frames,
active nearby, and covered in all 121 moved-camera frames. RenderDoc far/near
captures for both modes were exported and inspected: cached distant lighting
retains occlusion by the fireplace grate instead of introducing it only nearby.
Fire animation differs between runs, so this is a qualitative occlusion check,
not an exact image-difference metric. The nearby view retains the active shadow.
This verifies this fireplace case, not every wall torch or all missions.

`native_torch_camera.py` makes the camera/selection checks repeatable. It moves
only the render camera and leaves the native player/physics in place. These
GDB/RenderDoc runs are not representative traversal or performance measurements.

The 122-frame cached camera/selection fixture also passes Khronos synchronization
validation without validation errors. The final route finder includes opaque
view blockers even when their caster flag is disabled; the checked retail data
still selects the same clear 60-foot fireplace route.

## Rendered camera priority crossover

The GPU lighting regression now moves a camera across nine positions and back,
aiming at a fixed receiver while two lights exchange the single active shadow
slot. The competing light is behind the receiver, so it changes selection
priority without illuminating the measured front face. One cached slot retains
the displaced light. For each camera position and both retired frame slots,
HDR readback compares a current unoccluded reference against the shadowed
receiver and requires less than 20% of that reference. Both active and cached
states must be exercised. This production-renderer test passes synchronization
validation and logs measured radiance and selection state.

Separate retail runs completed a 120-frame camera-only route toward torch 805,
from 80 to 2 feet, with the cache off and on. Integer route samples and all actual
frame positions resolved to native cells. Native player/physics state was not
moved, so these are not player traversal/performance tests. A RenderDoc capture
of the far view shows a closed door occluding the torch; it cannot establish
retail visual continuity for that torch. That comparison remains outstanding.

## Reusing geometric caster membership

Shadow arrays now retain per-layer frustum membership in each retired frame
slot. Transformed bounds are compared once per caster; unchanged bounds and
projection reuse the exact previous geometric result. Changes to caster IDs,
blended/opaque eligibility, bounds, projection or array storage invalidate the
appropriate membership. Current pose/material/texture signatures still decide
depth redraws independently, so this does not freeze dynamic shadows or reduce
shadow resolution, filtering, budgets or caster coverage.

GPU tests verify zero repeated bounds tests for unchanged point faces, movement
into a previously excluded cube face (with analytic texel-center radial depth),
restoration and the existing caster retirement/deformation/array-resize paths.
All six Linux tests pass under synchronization validation, as does a 24-frame
retail Mission 1 run with the distant pool enabled. `world-shadow-culling` trace
exposes the number of bounds tests performed in the current frame.

Quiet sequential 180-frame 1080p Mission 1 PCSS runs (first eight modern samples
discarded) measured distant pool off/on median submission intervals of
16.059/16.507 ms and p95 17.299/17.219 ms. With the pool enabled, command recording
was 8.451 ms median (previous implementation 13.024 ms), GPU world time 5.591 ms,
and bounds tests 11,966 per frame. Shadow draws remained about 150,279 per frame.
The preceding implementation's distant-pool median interval was 21.176 ms in a
90-frame run. These separate short stationary runs support reduced CPU cost;
they do not prove sustained traversal performance or a visually verified torch
transition. The distant pool remains opt-in.

## Experimental distant local shadow pool

`renderer_shadow_cache=1` adds a separate 128-square D32 array for up to 128
additional local lights/768 faces, leaving the active default at 16 lights/48
512-square faces. This costs 96 MiB across two frame slots. Sampling uses a tagged
layer index in the existing 128-byte view ABI, with a separate sampler binding;
point cube-face remapping, spot filtering and PCSS share their existing paths.
Depth signatures validate current casters and emitter ownership in both pools.
Unchanged distant maps reuse depth; dirty ones currently redraw immediately.
There is no distant update throttle yet, so this is opt-in and not performance-ready.

All six Linux tests pass under synchronization validation. New GPU checks cover
cached point and spot occlusion with zero active lights/faces, moving caster
eligibility, both frame slots, and disabling the distant pool. CPU camera tests
cover combined active/cached/sun view offsets. The Mission 1 copied-scene torch
diagnostic now has a valid plan for all three objects at all six sampled
distances, for both active budgets. A 120-frame retail Mission 1 run at 1080p PCSS
passes synchronization validation with 13 active lights/48 faces and 63 cached
lights (all 76 eligible lights covered). This is not yet a rendered moving-player
torch comparison, and exceeding the distant capacity can still lose coverage.

Sequential 90-frame Mission 1 1920x1080 PCSS runs without validation (first eight
modern samples discarded) measured cache off/on median submission intervals
15.944/21.176 ms and p95 17.254/22.278 ms. Modern GPU medians were 3.335/6.321 ms;
command recording medians were 2.939/13.024 ms. These short stationary runs show
the distant pool's CPU recording cost and do not meet the 60 FPS target.

## Retained local shadow layers

Each retired frame slot now supplies its previous plan when assigning local
array layers. Retained lights keep contiguous ranges when possible, and new
lights use free ranges. If retained spotlights fragment the free space needed
by a six-face point light, compact allocation remains a safe fallback. Duplicate
handles and over-budget plans are rejected.

Depth cache signatures include projection, depth origin/plane/range, light type,
emitter exclusion and caster state. Sampling-only view indices, light emitter
size and filter parameters do not invalidate an unchanged depth image. Array
recreation still clears signatures. CPU tests cover retained ranges, view-order
changes, fragmented fallback and capacity rejection. GPU depth readback proves
an unchanged spotlight reuses its layer with zero redraws after a preceding
point light leaves the plan, independently in both frame slots. All six Linux
renderer tests pass under synchronization validation. This reduces cache churn;
it does not yet provide sampled shadow coverage for lights outside selection.

## Opt-in terrain visibility

`renderer_portal_visibility=1` copies the native active cell set immediately after
traversal, before native cleanup releases its render data. The copy resets for
each camera and mission; invalid/missing input falls back to all terrain visible.
Only terrain raster visibility changes. A chunk is retained if any contributing
cell is visible, and hidden chunks retain shadow casting. Native world drawing
and model visibility are controlled separately. Default visibility filtering is
off pending broader traversal/visual checks.

Linux Mission 1 fixtures pass with filtering off/on and native world drawing
on/off under synchronization validation. The enabled fixture checks 88 visible
and 332 hidden chunks, invalidates the snapshot to restore all 420, then verifies
the next frame restores 88/332. A RenderDoc frame with filtering enabled retains
the nearby character, sword, sky, HUD and expected terrain. Candidate light count
remains 76 in the start view; this is not a fix for all torch budget transitions.
The short concurrent diagnostic runs are not used as performance measurements.

## Mission 1 torch transition reproduction

The read-only `native_torch_budget.py` diagnostic copies a retail Mission 1 scene
and samples six camera distances using the current CPU selector. Torch 448's
light matches its object position exactly and correctly identifies owner 448.
With 16/48 it is unselected at 80 and 40 feet, selected at 20, 10, 5 and 2 feet.
At 64/384 it is selected throughout, but torch 805 is still unselected at 80
feet (76 eligible lights compete for 64 slots). Thus larger budgets alone do not
guarantee consistent distant occlusion. These synthetic cameras do not move the
native player and are not a rendered traversal or traversability test.

## Explicit local shadow budgets

`renderer_shadow_lights` and `renderer_shadow_faces` now reach the production
renderer settings path. Defaults remain 16/48; supported overrides are clamped
to 64 lights/384 faces. The GPU regression renders 18 emitters with 23 faces and
restores default-sized arrays in both retired frame slots. Mission 1's 90-frame
1080p PCSS synchronization-validation runs passed at 16/48 and 64/384. The former
used all 48 faces; the latter used 319 faces. This demonstrates additional shadow
coverage, not acceptable performance or a verified moving-camera torch fix.
The maximum local depth allocation is 768 MiB across both frame slots, plus sun
maps and other resources. `world-shadow-budget` trace records eligible light
count, selected light count and selected face count; it diagnoses saturation
without changing light intensity or hiding lights outside the budget.

## Visible receiver shadow eligibility

Shadow selection now unions copied light lists for visible, camera-frustum
receivers before ranking local lights. A bright unbounded light behind walls
cannot consume a slot unless a visible receiver can use it. An unrestricted
visible receiver retains conservative fallback. Directional allocation is
unchanged. Regression checks cover a bright irrelevant competitor, camera
movement, explicit empty lists, offscreen and hidden unrestricted receivers.
The Linux game build, six renderer tests and a 120-frame Mission 1 synchronization
validation run pass. This fixes wasted eligibility, not all distance-dependent
shadow changes: the 16-light/48-face resident budget still causes selected lights
to become unshadowed when displaced. Persistent distant shadow coverage remains
unfinished; merely fading that transition would not restore missing occlusion.

## Mission 2 lamppost correction and portal drawing diagnostic

The native `STRSPOT.BIN` material table uses eight-byte auxiliary records. The
adapter now reads that prefix instead of requiring the newer sixteen-byte
structure. MD object self-illumination is a multiplier for authored material
glow. Objects 228 and 234 now have five non-emitting metal parts and two emitting
lens parts, rather than emission on every part.

The light at object 234's light-source vhot previously intersected its own lens
1.178 feet down the beam. Copied lights now identify their emitter object, and
shadow maps exclude only that object's authored emitting surfaces. Housing and
other objects still cast shadows, including another light's emitter surfaces.
Emitter identity and material changes invalidate cached shadow depth.

Linux material decoder regressions and all six standalone renderer tests pass.
GPU checks cover near-camera point/spot illumination, emitter ownership, and
non-emitting housing. Mission 2 ran 120 frames with synchronization validation
for each `renderer_portal_rendering` setting, without validation errors. A
missing-descriptor-indexing shim confirmed normal legacy fallback with the flag
disabled. A Mission 1 RenderDoc capture confirms sky, character, sword and HUD
with native world draw submissions disabled. Portal visibility callbacks still
run; this diagnostic mode omits native water/particle draws. No Windows or
DromEd testing was performed. These checks do not establish the frame-rate target.

Current-build sequential Mission 2 runs at 1920x1080, PCSS, 180 frames per mode
(first eight modern samples discarded) measured median/p95 submission intervals
of 16.867/20.297 ms with portal drawing enabled and 16.218/19.123 ms disabled.
Scene capture remained about 3.0 ms median and modern command recording about
6.1 ms, including 3.4 ms shadow recording. Modern GPU time was about 5.1 ms.
These short stationary runs indicate remaining CPU costs, not sustained 60 FPS
through mission traversal. Native water/particle omission makes the disabled
mode a diagnostic rather than a visually equivalent performance comparison.

The target renderer replaces RT shadows entirely with shadow maps for point,
spot, and directional lights. PCSS is the default, with hard and PCF modes also
available. The reusable CPU scene, native adapters, HDR direct lighting,
background composition, first-person overlays, and legacy fallback remain.
Linux and Windows and the existing 1080p/60 FPS target remain in scope. Native
light records and stealth calculations must not change.

Current validation is restricted to the Linux game target by user request.
Windows and DromEd builds/runtime tests are deferred.

Native receivers now use copied per-cell static-light reach, with conservative
unions for existing terrain chunks and separate eligibility for dynamic lights.
The adapter leaves native lighting and stealth records unchanged. A tested
receiver-list packing cache avoids rebuilding unchanged lists. See the latest
native-cell integration report in [world-space renderer](world-space-renderer.md)
for GPU savings and the remaining CPU/frame-time cost; stable 60 FPS is not yet
established. `renderer_cell_lights=0` provides the unrestricted diagnostic path.

## Measured Linux performance tuning

The adapter caches object alpha, self-illumination and shadow-casting state until
a native property or
hierarchy notification invalidates them. Texture animation, replacements, poses
and other live properties remain checked each capture. On Mission 2 at 1080p
with PCSS, 180 frames and the first eight samples discarded, the configured
8192-entry trait cache now measured 17.583 ms median frame time and 2.812 ms
scene capture, versus 20.951/5.628 ms before material-property revision caching.
The 95th-percentile frame interval changed from 23.859 to 20.940 ms. The 60 FPS
target remains unmet.

Before revision caching, a launch override `trait_cache_size=16384` reduced
capture thrashing and produced roughly 18 ms median frame time. With the current
adapter, a 16384-entry run measured 18.088/2.830 ms frame/capture, so the larger
cache no longer improved this mission. Keep the configured cache size unless
measurements of another workload justify changing it. Neither the default nor
persistent game configuration has been changed by these experiments. Scene
capture that frequently receives property/hierarchy changes may have a different
cost because such changes conservatively invalidate all cached material values.

## Current implementation

The runtime now renders shadow maps and samples them through the PCSS-capable
lighting shader. `renderer_mode=modern` enables it; `modern_rt` is a compatibility
alias. Device creation requests sampled-texture indexing without RT features.
The old RT cache, feature probe, ray shader, and RT-specific tests are removed.
Legacy rendering remains the default. Set `renderer_shadow_filter=hard|pcf|pcss`
to select filtering; the default is `pcss`. Unknown values report a warning and
use PCSS.

- `VulkanWorldGeometry` owns pooled ordinary vertex/index buffers independently
  of acceleration structures, shaderInt64, or buffer device addresses. Two frame
  slots retain separate mesh revisions. The caller must retire a slot before
  updating it, and retire GPU work before shutdown.
- `VulkanWorldFeatures` probes only sampled-texture descriptor indexing. The
  feature chain contains no RT, address, or core shaderInt64 requirements. It is
  used by modern renderer device creation.
- `SelectShadowLights` budgets 16 local lights and 48 local faces by default;
  sunlight has its own four-cascade allocation. Finite influence outside the
  raster view is excluded. Priority uses intensity, distance, influence extent,
  stable handles for ties, and a 20% retention bonus against rapid reassignment.
  Selection never disables a light's direct illumination.
- Point lights cost six faces, ordinary spotlights one. Spotlights whose outer
  cone reaches or exceeds a hemisphere need six faces because a single finite
  perspective projection cannot cover them.
- The scene light API includes `sourceRadius`, `angularRadius` (radians), and
  `castsShadow`. Native lights default to 0.25 engine feet, 0.27 degrees, and
  enabled shadow casting. Zero emitter size allows hard shadows. Invalid emitter
  dimensions are rejected without changing a working light record.
- `BuildShadowPlan` creates six point faces, perspective spot cameras, and four
  texel-snapped sun cascades with overlapping receiver coverage. It fits depth
  ranges against the whole scene, including offscreen casters and visible
  non-casting receivers. Sun coverage defaults to 256 engine feet.
- `VulkanShadowMaps` owns per-frame D32 local and sun depth arrays and records
  alpha-aware, double-sided caster depth draws from the shared raster buffers.
  Point maps store radial depth; spot and sun maps store linear view depth.
  Invisible casters are retained; blended casters are excluded. Full array
  layouts remain valid for subsequent shader sampling. Unchanged views reuse cached depth; camera, caster, pose, material, and
  opacity revisions invalidate the affected view. Per-slot counters report
  rendered/reused views and draw calls.

## Remaining replacement work

Validate PCSS contact hardening, bias, cubemap seams, and cascade transitions
across retail missions. Current defaults are
512-square local maps, 2048-square sun maps, 16 blocker and 32 filtering
samples. Cache unchanged maps and invalidate affected maps for light, caster,
pose, and opacity changes. Resource ownership and atlas reassignment must remain
safe across both in-flight slots. Report actual shadow workload and GPU timing.

Complete the remaining native scene adapters/materials/lifecycle paths and
validate Windows and target performance. Per-slot GPU timestamps and runtime workload reporting are implemented; full-frame target-hardware performance remains unverified. The current GPU lighting tests cover finite emitter occlusion
and clear receivers, but do not yet establish penumbra quality across distances.

## Evidence

The standalone geometry GPU test creates a device with texture indexing only and
checks exact vertex/index buffer readback, pose updates without buffer replacement,
frame-slot independence, deletion, invalid slots, and scene reset. It passed on
an RTX 4070 Ti without enabling RT features. This does not prove non-RT hardware
compatibility or shadow-map rendering yet.

The CPU selector test runs under ASan/UBSan and covers face/light limits,
directional allocation, unused face capacity filled by spotlights, hysteresis,
finite influence visibility, stable ties, and disabled/non-shadow-casting lights.
Both the game and editor build with the replacement components present.

Shadow-camera tests cover all six axes, spot cone edges, wide-spot cubemap
fallback, cascade receiver coverage, offscreen depth range, and subtexel
stabilization under ASan/UBSan. The shadow-map GPU test renders the production
depth shaders without RT features and reads back actual D32 depth for point,
spot, and sun maps. It verifies alpha holes, blended-caster exclusion, invisible
casters, moving instances, vertex deformation, and both frame slots.

The production lighting GPU regression also grows and shrinks HDR targets through
128, 32, and 64 pixels in both frame slots, checks that offscreen-caster shadows
survive each resize, then disables all shadow budgets and restores them. Direct
illumination remains enabled at zero budget, and both slots rebuild their shadow
maps when the budget returns. Hard, PCF, and PCSS each pass occluded/clear receiver
checks with a finite emitter. These checks do not cover window surface loss or
establish visual penumbra quality.

Shadow cache signatures hash each caster pose/material once per frame. Each view
reuses its culled caster list for command recording, avoiding a second frustum
pass for dirty maps. GPU cache invalidation, depth and lighting regressions pass
after this change; a full-frame CPU performance improvement has not been measured.

A fresh offscreen RenderDoc capture of the current production path was replayed
through the MCP server on RTX 4070 Ti. It contains six point-light depth passes
(events 21–84) and the HDR pass (86–96). Written depth faces contain values in
[0.142, 1]; empty selected faces remain at 1. The fully occluded HDR receiver is
0.004997 per RGB channel, alpha 1, with no NaN/Inf/negative values. Unselected
array layers are uninitialized and are not evidence of rendered depth. The
capture helper is documented in `tools/world_scene_test/README.md`. Retail
mission composition and finite-emitter penumbra quality still need captures.

The production HDR shader now also has a normalized GPU penumbra scan: across a
16-pixel receiver span, hard shadows produce 0 intermediate-visibility pixels,
PCSS with radius 0.1 produces 1, and PCSS with radius 0.4 produces 10 on RTX 4070 Ti.
The test asserts that PCSS produces a penumbra and that a larger emitter widens
it. This establishes emitter-size response for this fixture; blocker-distance
contact hardening, seams, and retail visual quality remain separate checks.

A second normalized HDR scan keeps the emitter radius and projected blocker
silhouette fixed while moving the blocker toward the receiver. Intermediate
visibility decreases from 10 pixels for the distant blocker to 4 near contact on
RTX 4070 Ti. The test asserts the narrowing response. This validates point-light
contact hardening for the synthetic fixture, not spot/sun behavior or retail
mission bias and seam quality.

### Retail 1080p baseline (RTX 4070 Ti)

Mission 1, 1920x1080 confirmed by the renderer trace, current default budgets:

| Filter | Modern GPU median | Shadow-depth GPU median | Warm samples |
| --- | ---: | ---: | ---: |
| PCSS | 23.256 ms | 1.175 ms | 52 |
| PCF | 16.091 ms | 1.105 ms | 16 |
| Hard | 5.142 ms | 1.155 ms | 16 |

These are per-slot GPU timestamps for the modern preparation/HDR path, excluding
legacy rendering, composition and full-frame CPU time. The first eight frames
were omitted. Runs differ in simulation duration; treat comparisons as diagnostic,
not a controlled benchmark. Roughly 22–23 depth views redraw and 25–26 reuse,
with about 17,200–17,700 depth draws per frame. All runs exited successfully.
PCSS currently misses 60 FPS even on this GPU before the remaining frame costs;
filter sampling is a major optimization target. Target midrange hardware and
Windows performance remain unmeasured.

A constant golden-angle disk table removes per-tap trigonometry, and cubemap taps
retain unnormalized offset rays because perspective projection and dominant-face
selection depend only on component ratios. The same 60-frame 1080p run measured
22.412 ms median after the disk table and 21.727 ms after both changes, versus
23.256 ms initially (52 warm samples each). These uncontrolled sequential runs
suggest a useful reduction but still miss the target. All six CPU/GPU regressions
pass, including emitter-size and contact-hardening responses, without reducing
sample counts or shadow budgets.

Hoisting the receiver direction/tangent frame out of the blocker/filter loops
further reduced the sequential 60-frame 1080p modern-pass median to 20.715 ms
(52 warm samples, range 20.302–21.944 ms). All six regressions pass. Sample counts,
PCSS defaults and shadow budgets remain unchanged. This is still above 16.7 ms
before accounting for the rest of the frame.

Analytic cubemap UVs now replace per-tap matrix loads/projection for the fixed
90-degree point-light faces. CPU checks compare all six analytic mappings against
the production raster matrices over 124 directions, including face-edge ties and
corners. All six tests pass, including penumbra/contact hardening. The sequential
1080p run measured 12.874 ms modern-pass median (52 warm samples, range
12.115–14.918 ms), versus 20.715 ms before this change. Full-frame CPU/GPU time,
retail seam images and target midrange performance remain unverified; this does
not establish 60 FPS for the game.

New trace fields separate successful submission intervals (`frame-pacing
interval_ms`), wall time from frame begin through submission (`render_submit_ms`),
and modern command recording wall time (`world-shadows record_cpu_ms`). These
wall times include any blocking inside their measured regions and are not GPU
execution times. At 1080p in a 60-frame mission run, warm modern frames measured
41.589 ms median submission interval (51 samples, 38.845–44.081 ms), while modern
recording measured roughly 13.25 ms median and modern GPU roughly 12.8 ms. This
is about 24 submitted frames/s and establishes that the overall target is still
missed. Native rendering/capture, command recording and synchronization require
further profiling; shader performance alone cannot establish completion.

The bounds/frustum check now avoids general matrix multiplication for diagonal
scale plus translation, which covers terrain and many rigid poses. Rotated or
other general matrices retain the existing path. All six regressions pass. A
sequential 1080p run measured 12.178 ms median modern command-recording wall time
and 39.597 ms submission interval, compared with roughly 13.25/41.59 ms before;
GPU median remained about 12.72 ms. These runs suggest a CPU reduction, while
full-frame performance remains below target.

Additional native-adapter timing measured about 1.55 ms scene capture and 1.49 ms
from camera capture to the legacy portal cleanup/composition hook (52 warm
samples each, separate sequential runs). These measurements do not support
attributing the remaining full-frame gap mainly to scene capture or that native
portal region. Frame acquisition, canvas work and presentation must be timed
separately before assigning the bottleneck. The submission interval is wall
pacing, not proof of CPU saturation.

Frame phase timing at 1080p measured acquisition at 13.227 ms median, canvas work
at 2.032 ms, and submission/presentation CPU return at 0.071 ms (51 warm modern
frames). Acquisition currently waits both the frame-slot fence and the acquired
image's previous fence. The command buffers and transient resources are indexed
by frame slot; the image-fence host wait warrants a synchronization audit before
changing it. These timings locate a wait but do not yet prove it redundant.

The acquired-image host fence wait has been removed after auditing ownership:
command buffers and transient mapped data are frame-slot resources protected by
the slot fence; image framebuffers/depth views are immutable on the CPU. Acquire
semaphores still order GPU image reuse, and present-finished semaphores remain
indexed by swapchain image. The acquire wait now includes early depth tests;
the render-pass external dependency covers prior depth writes. This follows the
frame-slot/image separation described by the Vulkan Guide:
https://docs.vulkan.org/guide/latest/swapchain_semaphore_reuse.html

A 120-frame retail run at 1080p exited successfully. Across 111 warm modern frames,
median acquisition fell to 2.252 ms (0.017–3.333 ms), and submission interval to
26.785 ms (24.036–30.672 ms), versus 13.227/38.799 ms in the prior phase run.
This restores useful CPU/GPU overlap but still misses 60 FPS. Standalone tests
pass; they do not cover WSI synchronization. Validation-layer, resize/minimize
and Windows verification remain required.

Synchronization validation is now available locally by extracting Artix's
vulkan-validation-layers 1.4.363.1 package under `/tmp` (no system installation).
With `VK_LAYER_KHRONOS_validation` and `VK_LAYER_VALIDATE_SYNC=1`, all six
standalone tests pass and their full CTest log contains no validation messages.
A separate 60-frame 1080p modern retail mission run exits successfully with no
validation warnings, VUID errors or synchronization hazards. An initial 12-frame
run confirmed layer loading but used the deprecated enables setting; its only
validation messages were settings warnings, corrected for the subsequent run.
This covers startup, ongoing rendering and shutdown, not resize/minimize or
Windows. GPU-assisted shader validation has not been run.

A temporary SDL lifecycle driver was compiled only for a validation run and then
removed; normal game/editor binaries were rebuilt. It requested resize to
1280x720, minimize, restore at 1600x900, then 1920x1080. The run exited with no
validation/synchronization messages. Traces prove modern rendering at 1920x1080
and after resize at 1280x720, but show no submitted frames after minimization.
Consequently restore success is NOT established: the Wayland window may remain
minimized despite SDL_RestoreWindow, or lifecycle state may fail to resume.
Investigate actual window/event state before claiming minimize/restore coverage.

Inspection of bundled SDL's `Wayland_RestoreWindow` confirms that xdg-shell does
not provide an unset-minimize request; SDL_RestoreWindow only unsets maximized
state here. Thus the failed automated restore cannot establish an engine resume
bug. Actual compositor-driven minimize/restore still needs verification.

A separate 60-frame synchronization-validation run resized through 1280x720,
1600x900 and back to 1920x1080 without minimizing. It exited successfully with no
validation messages; traces show 7 modern frames at each intermediate size and
46 at 1920x1080. The temporary resize driver was removed and normal binaries
rebuilt. This establishes swapchain/modern-target resize in both directions for
this Linux Wayland run, not surface-loss/device-loss recovery.

A virtual X server (Xvfb extracted under `/tmp`) bypasses the desktop X11 MapNotify
stall. A fresh current-renderer retail capture is
`/tmp/dark-modern-capture_frame37.rdc` (74,993,861 bytes), replayed with RenderDoc
MCP: 19,840 actions, 478 textures, 4,898 buffers. Final color at event 81565 was
exported to `/tmp/dark-current-retail-final.png` and visually inspected: terrain,
rigid scene geometry, a character, native moon/cloud sky, sword overlay and HUD
are present together. This is current raster/PCSS evidence, superseding reliance
on historical RT-era screenshots. It is not proof of equipment alignment or
retail penumbra/seam quality across missions.

The capture run was intentionally bounded by its 60-second script timeout and
terminated after the capture was saved; it is not a clean-shutdown test. Xvfb was
stopped afterward.

The production HDR lighting regression now checks spot and directional lights
with an offscreen caster using hard, PCF and PCSS. For the synthetic center
receiver, the spotlight produces 1.1963 unoccluded radiance and the sun 11.2578;
all three filters reduce each to approximately 0.005 ambient with the caster.
All six tests pass under synchronization validation without validation messages.
This establishes the full lighting/shadow path for all three light types, but
spot/sun penumbra shape, cascade transition images and retail seams remain open.

Normalized production-shader penumbra scans now cover spot and sun emitters too:
spot source radii 0.1/0.4 produce 4/5 intermediate pixels, and sun angular radii
0.02/0.15 radians produce 1/3. Each test asserts that increasing emitter size
widens the soft edge. The sun sizes are deliberately exaggerated synthetic test
values, not new runtime defaults. All six tests pass under synchronization
validation with no messages. These results establish basic emitter-size response
for all light types; they do not establish retail sun quality or cascade seams.

Driver coverage now includes Mesa Lavapipe/llvmpipe (LLVM 23.1.4): all six tests
pass with the same light/penumbra results as NVIDIA, and a three-frame explicit
legacy mission run exits successfully. Lavapipe supports indexing, so it does
not establish automatic fallback by itself. A separate LD_PRELOAD test shim
hides the indexing extension: requesting modern mode then logs its missing
capability and retains legacy rendering. Three-frame normal and twelve-frame
synchronization-validation mission runs exit successfully; the latter reports
no validation messages. The shim and instructions are under world_scene_test.
This verifies simulated capability fallback, not older physical GPUs.

Shadow depth recording now prepares a compact caster record once per frame,
resolving mesh/material/GPU-buffer references before per-view loops. Blended
surfaces are excluded before culling; stable ID/hash ordering is preserved.
Per-view work reuses those records rather than repeating unordered-map lookups.
All six tests pass under synchronization validation without messages, and Linux
game/editor builds pass. A 120-frame 1080p run measured 7.447 ms median modern
recording (112 warm samples) and 23.111 ms submission interval (111), versus
11.518/26.785 ms in the previous 120-frame overlap run. GPU median stayed near
12.1 ms. Sequential runs remain diagnostic; this is still below the 60 FPS target.

Shadow culling now transforms each caster's oriented bounds once per frame and
extracts six clip planes once per view, then uses support-radius plane tests.
CPU regression compares the prepared path against the existing local-space
matrix path over camera crossings, enclosing bounds, mirrored/nonuniform scales
and 6,480 rotated/translated placements. All six tests pass under synchronization
validation and Linux game/editor builds pass. A 120-frame 1080p run measured
6.072 ms median modern command recording and 21.526 ms submission interval
(112/111 warm samples), versus 7.447/23.111 ms for the preceding compact-caster
run. GPU median stayed at 12.104 ms and depth draw median at 17,712. Full-frame
60 FPS remains unmet.

Canvas conversion queried `RenderBackendVulkanSceneActive()` once per pixel even
though the value is fixed during conversion. Hoisting it once per canvas avoids
roughly two million backend calls per 1080p frame. Linux game/editor builds and
a 120-frame retail run pass. Warm submission interval measured 19.661 ms median
(111 samples, 17.037–51.022 ms), versus 21.526 ms before; end-of-frame canvas upload
remained about 2.03 ms, since conversion occurs earlier. The remaining outlier
and median still prevent a 60 FPS claim.

RGB555/RGB565 canvas conversion now uses byte-exact lookup tables instead of
repeating channel expansion arithmetic per pixel. CPU tests exhaust all 65,536
input words for both formats against the former formula, including the unused
RGB555 high bit. Palette conversion and scene transparency behavior are unchanged.
Linux builds and all six tests pass. A 120-frame 1080p retail run measured 16.459
ms median submission interval (111 warm frames; p95 17.870, p99 18.246, max
23.517 ms), versus 19.661 ms before the table. This reaches a sub-16.7-ms median
on RTX 4070 Ti in this fixture, but does NOT establish stable 60 FPS, other
missions, target midrange hardware or Windows runtime performance.

Retail validation now also covers Missions 2 and 10 at 1080p, twelve frames each
with synchronization validation: both exit normally without validation messages,
modern disable or import failure. Mission 2 imports 916 terrain meshes / 44,538
triangles and 1,464 objects (36 MM); Mission 10 imports 730 / 50,932 and 1,110
objects (35 MM). These are startup/smoke checks, not full-mission visual audits.

A separate 120-frame Mission 2 performance run without validation exposes a much
heavier workload: 33.970 ms median submission interval, p95 37.543, max 39.272
(111 warm frames), modern GPU median 32.987 ms, recording 16.993 ms, and 29,284
depth draws. Thus Mission 1's sub-16.7-ms median does not generalize. Both CPU
submission and GPU lighting/shadows still need optimization for larger missions.

Opaque modern draws are now sorted front-to-back by transformed mesh-bounds
center, with stable handle ties. Blended geometry uses the same meaningful
center back-to-front; terrain's identity instance origin no longer makes all
chunks equidistant. This reduces expensive hidden-surface lighting work without
changing samples, budgets or geometry. All six tests pass under synchronization
validation without messages, and Linux builds pass. Mission 2's sequential
120-frame run measured 17.841 ms median modern GPU time (previously 32.987),
16.779 ms recording (16.993), and 31.683 ms submission interval (33.970). The CPU
side now limits the benefit; stable 1080p/60 FPS remains unachieved.

Geometry suballocations now share actual Vulkan arena buffers, rather than only
sharing their underlying VkDeviceMemory. Mesh queries return byte offsets;
raster draws/readback use them, and shadow views bind arena buffers once then
select each mesh with firstIndex/baseVertex offsets. Frame slots retain separate
suballocations and retire them under the existing fences. All six tests pass
under NVIDIA synchronization validation and under llvmpipe; exact vertex/index
readback exercises nonzero offsets and pose/slot independence. Linux builds pass.
Mission 2 measured 16.116 ms median recording and 31.011 ms submission interval,
versus 16.779/31.683 ms previously, with GPU time about 17.676 ms and 29,285 depth
draws. Draw-call batching remains necessary for larger CPU improvements.

A compatible-transform multi-draw-indirect experiment was implemented and tested,
but removed after Mission 2 measured no CPU benefit (about 16 ms recording) and
higher GPU time (about 19.1 ms). No optional multiDrawIndirect feature or batching
path remains from that experiment. Recording phase counters then isolated the
actual CPU bottleneck: uploads/tile construction at 10.465 ms, compared with
1.404 ms planning, 3.524 ms shadow commands and 0.754 ms HDR commands.

Full-screen lights now share one prefix in the light-index buffer instead of
being duplicated into every tile. Local tile ranges follow that prefix; the
shader processes global and local ranges separately. All enabled lights still
illuminate, with unchanged shadow selection/sample counts. A mixed local/global
GPU fixture verifies that list offsets neither duplicate nor drop direct light.
All six tests pass under synchronization validation without messages, and Linux
builds pass. Mission 2 now measures 1.752 ms uploads/tiles, 7.334 ms total recording,
19.889 ms modern GPU and 22.194 ms submission interval (112/110/111 warm samples).
This trades some GPU overhead for a large net improvement from 31.011 ms frame
pacing; GPU lighting remains above the target and needs more work.

The HDR pass now runs an opaque/masked depth prepass, using the same vertex
shader and alpha cutoff as shading. Opaque lighting then uses equal-depth tests;
blended objects retain the prior depth test and are drawn afterward. This avoids
repeating full direct/PCSS shading on hidden opaque surfaces. A GPU cutout test
checks that transparent receiver texels retain the native background and opaque
texels retain the expected image. All six tests pass on NVIDIA under sync
validation and on llvmpipe, and Linux builds pass. Mission 2 measured 11.537 ms
modern GPU median (down from 19.889), 7.665 ms CPU recording and 22.527 ms submission
interval. CPU/frame pacing now needs further investigation; stable 60 FPS is
still not established.

A GPU sun-cascade sweep moves the fixed receiver through distances 3, 7.5, 8.2,
16.8, 17.6, 34.3, 35.7 and 75 units, spanning split/blend ranges. It initially
exposed roughly 5% self-shadowing in one otherwise clear sample. Sun PCF/PCSS
now compensate each tap for the planar receiver's depth slope, including blocker
search depth alignment. All clear sweep samples now read 11.2578 radiance and
all blocked samples about 0.0050, with larger sun emitters still widening the
penumbra. The regression now requires clear brightness as well as occlusion.
All six tests pass on NVIDIA under synchronization validation and on llvmpipe;
Linux game/editor builds pass. This is synthetic transition/bias evidence, not a
complete retail sun/cascade visual audit.

A new finite-emitter local-light test exposed about 22% self-shadow dimming on a
clear receiving plane. Point and spot taps now align stored depths to the
receiver plane before blocker/filter comparisons; radial point depth includes
the offset ray length. The receiver plane comes from geometric screen derivatives
computed before alpha discard, so smoothed shading normals do not tilt it. New
checks compare shadow-enabled and unshadowed clear surfaces at three grazing
angles for point and spot emitters, including deliberately tilted shading normals.
All six tests pass under NVIDIA synchronization validation; llvmpipe also passed
the initial correction suite. Existing blockers, emitter-size response and
contact-hardening tests continue to pass. Linux game builds pass; no Windows or
DromEd work was performed for this change. A Mission 2 run measured 12.843 ms
modern GPU and 23.933 ms submission interval, reflecting extra correction cost;
performance and broader retail visual quality remain unfinished.

Linux user-only perf sampling (5,982 samples, none lost) identified canvas
presentation at 13.54% self CPU, inherited-property hash lookup at 8.54%, donor
cache drop/get/add at roughly 11% combined, shadow planning at 7.12% and prepared
bounds tests at 6.47%. Buffer free-list sorting also appeared, but includes
shutdown work and is not established as a steady-frame bottleneck. Sampling used
cpu-clock:u and --user-callchains; no kernel security settings were changed.

Sampled assembly confirmed repeated vector base reloads and separate channel
stores in canvas conversion. The conversion now caches the destination and
copies a complete four-byte RGBA pixel; exhaustive RGB555/RGB565 tests verify
opaque and zero-transparent outputs. All six tests and the Linux game build pass.
Mission 2's 120-frame run measured 22.998 ms median submission interval, versus
23.933 ms before this change. The capture profile and shader quality fixes
remain enabled; no sample/budget reduction was used.

The retail config sets trait_cache_size=8192 (engine fallback is 1024). An
experimental command-line setting of 32768 reduced Mission 2 scene capture to
4.256 ms median and submission interval to 20.438 ms. No cache default/config or
invalidation behavior has been changed based on this experiment. A renderer
query/caching policy still needs design and verification; gameplay/stealth
calculations must remain unchanged.
