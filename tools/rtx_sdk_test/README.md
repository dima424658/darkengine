# Native Vulkan NRD adapter check

DDS/PBR resource loading, optional-map defaults and suffixes are documented in
[PBR textures](../../docs/pbr-textures.md). `native_pbr.py` checks actual terrain
and model resource discovery using a private Gold overlay under validation.

The adapter follows the resource-pool and dispatch approach demonstrated by
[nvpro-samples/vk_denoise_nrd](https://github.com/nvpro-samples/vk_denoise_nrd),
reviewed at `67f28c7c6a7bd2495bd6382dac8cffbf85673858`. It uses this engine's
Vulkan ownership and NRD 4.18 descriptors, without importing nvpro_core2 or NRI.

From the repository root:

```sh
cmake -S . -B build-linux -DDARK_ENABLE_RTX_SDK=ON
cmake --build build-linux --target dark_nrd_test -j8
VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation VK_LAYER_VALIDATE_SYNC=1 \
  ./build-linux/dark_nrd_test
```

The check creates actual RELAX diffuse/specular SPIR-V pipelines and internal
image pools, binds synthetic surface guides and black lighting inputs, and
submits six denoising frames with a history reset. Scan its full output for
`Validation Error`, `VUID-`, and `SYNC-HAZARD`: validation messages can occur even
when the process exits zero. This checks GPU execution and synchronization;
it does not establish denoising quality on game lighting or moving geometry.

Caller-owned textures must have GENERAL layout before and after `Record`.
Calls execute on one ordered compute-capable queue. A frame slot must be retired
before reusing its descriptor pool. Retire GPU work before resizing/shutdown.
A failed recording must be discarded, and a rejected submission must trigger
`CompleteSubmission(false)` before retrying. Call `CompleteSubmission(true)`
after an accepted queue submission; reset accumulation explicitly with `ResetHistory`.

`dark_rtxdi_test` (root build) also executes real RTXDI and RELAX passes on
raster surface guides. GPU readback checks direct shadow rejection, light-type
coverage and preservation of converged diffuse energy through RELAX. The fixture
now reads displayed SDK HDR as well, checking emission, fog, transparent blending
and reuse of primary depth. It lives in `tools/rtx_sdk_test/lighting.cpp`.

Primary disocclusion is checked separately from offscreen shadow motion: a dark
foreground triangle replaces the receiver at the sampled pixel, verified by its
surface identity, then leaves. Raw direct lighting must return immediately and
RELAX must recover the receiver's settled radiance range within four frames.
This controlled test exercises different primary depths and identities, but does
not substitute for animated-character or broad native traversal image checks.

The same fixture now also reads the actual SHaRC indirect outputs and counters.
It tests cache hits on secondary surfaces, emission/reflected-light bounce,
wall separation, material changes, light removal and cached/reference energy.
Run it with the same validation environment:

```sh
cmake --build build-linux --target dark_rtxdi_test -j8
VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation VK_LAYER_VALIDATE_SYNC=1 \
  ./build-linux/dark_rtxdi_test build-linux/generated/vulkan
```

The cache uses fresh traced Update paths: optional update-time resampling was
found to preserve stale light after source removal and is disabled. Query
continues to reuse SHaRC and terminates eligible paths early. These tests verify the displayed composition on controlled fixtures. They do
not establish broad temporal image quality or the final performance target.

## Native RenderDoc capture (Linux)

### Native creature deformation

```sh
VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation VK_LAYER_VALIDATE_SYNC=1 \
  python3 tools/rtx_sdk_test/native_animation.py /path/to/thief_gold \
  --mission 1 --frames 240 --log /tmp/dark-creature-miss1.log
```

Repeat with `--mission 2`. The Linux x86-64 GDB observer reads the production
world scene on the engine thread and identifies creatures through the native
`CreatureGetRenderHandle` API. It compares local vertex positions across world
frames, independently of instance translation/rotation, and requires animation,
post-startup BLAS rebuilds, normal exit and clean log/trace diagnostics. A preload
observer tracks mapped buffer addresses, compares actual triangle-build input
bytes against captured creature vertices, and checks that TLAS build inputs
reference those matching BLAS addresses. It requires matching changed-vertex
builds and creature-instance references, rather than aggregate workload alone. Saves
are private temporary files. The helper changes neither creature poses nor
camera position unless the optional capture aid below is selected. GDB timings
are not performance measurements.

The checked Gold runs each contain 240 world frames. The stronger mission 1
check observes six creatures/four animated objects, 668 part changes matched to
BLAS upload bytes and 1432 creature-instance references across TLAS builds.
Mission 2 matches 3391 changed uploads and 5258 creature-instance references.
Simulation timing affects counts. `visible_changes` counts the CPU instance
visibility flag, not actual pixel coverage. Build-input inspection does not
prove a particular creature's ray intersection, motion pixels, or visual shadow
quality. Use GPU fixtures and captures for those separate checks.

### Capture setup

For the checked Gold mission 1 library-aisle creature view:

```sh
python3 tools/rtx_sdk_test/native_animation.py /path/to/thief_gold \
  --mission 1 --frames 100 --capture-prefix /tmp/dark-creature \
  --log /tmp/dark-creature.log
```

This optional aid detaches the camera at frame 32 and aims from ten engine feet
along negative Y at the nearest creature already observed animating. It logs the
object/instance/mesh identity and captures frame 80 using installed RenderDoc
and SDL X11. It requires successful API capture and a newly written nonempty
RDC. The placement is a local aid, not a general collision-free camera solver;
inspect the image before treating it as creature evidence. Creature poses and
lights are unchanged. The inspected library view contains instance 1443/object
237, with valid nonzero motion at its center pixel and finite nonnegative HDR.
It is very dark and does not establish illuminated shadow contrast or temporal
ghosting quality. Capture/observer timings are not performance results.

Add `--test-light` to this capture command for an explicitly controlled lighting
fixture. It creates one white point light near the selected creature (intensity
100, range 30, source radius 0.12), and appends that source to existing native
receiver lists while preserving their other entries. This is entirely in the
test observer; normal gameplay lighting is unchanged. The captured library view
then clearly shows the character and surrounding shelves. Its HDR is finite and
nonnegative (maximum RGB 1.0313/0.7446/0.4680), and the center pixel still matches
instance 1443. This is a useful inspection setup, not a test of original mission
lighting balance or proof of temporal shadow quality. The command checks source
creation and capture success; visual assessment remains a separate step.

Use `--capture-frames 8` to record consecutive frames in one RDC (1–16 allowed;
total `--frames` must be at least 83 plus this count). An inspected eight-frame
Gold sequence contains eight presents and visibly changing native poses, without
an obvious detached silhouette trail in its contact sheet. At the character's
center pixel, decoded normal/light cosine changes from +0.514 to -0.201 as it
passes the fixed fixture source. Pure RTXDI direct radiance becomes zero when
the cosine turns negative, explaining the darkening rather than implying a
history failure. All eight composed HDR images are finite and nonnegative.
This is local qualitative evidence under capture overhead, not a general
temporal-error bound or native performance measurement.

The installed RenderDoc build does not advertise `VK_KHR_wayland_surface`;
use SDL's X11 backend for capture. The normal game renderer still uses Wayland.
This helper captures between world frames 32 and 33 after temporal history
has settled. It counts frames that call the world renderer, excluding UI-only
frames. Override the start using GDB `-ex 'set $capture_first=45'`; the end defaults
to the following world frame (`$capture_last` can override it). It requires GDB,
RenderDoc's development header and shared library.

```sh
cc -shared -fPIC tools/rtx_sdk_test/renderdoc_native.c -ldl -o /tmp/dark-sdk-capture.so
mkdir -p /tmp/dark-sdk-capture-saves
SDL_VIDEODRIVER=x11 DARK_NATIVE_CAPTURE=/tmp/dark-sdk-native \
  gdb -q -batch \
  -ex 'set environment LD_PRELOAD /tmp/dark-sdk-capture.so:librenderdoc.so' \
  -x tools/rtx_sdk_test/renderdoc_native.gdb --args ./build-linux/thief-runner \
  game_path=/path/to/game save_path=/tmp/dark-sdk-capture-saves content=vanilla \
  quickstart=miss2.mis auto_stop=45 renderer_mode=modern renderer_gi=1 \
  game_screen_size='2560 1440' game_screen_fit=0 SlowFrame=0 min_frame_time=0 vsync=0
```

Require `Native RenderDoc API available=1`, `Native RenderDoc capture result=1`
and a new `.rdc` file; a normal process exit alone does not prove capture success.
The capture can be opened through RenderDoc MCP. The mission 2 frame inspected
for this migration contains the SDK dispatches, composed lighting, native HUD
and first-person elements. The 2560×1440 HDR target has no negative, NaN or Inf
values in that frame. A static capture is not a temporal-quality or traversal
performance check.

For an equipped-weapon capture, add GDB `-ex 'set $capture_equip_sword=1'` and
`-ex 'set $capture_first=80'`, with `auto_stop=100`. Use Gold `quickstart=miss2.mis`
for the checked starting inventory. The helper requests normal sword selection,
then requires a real arm, weapon and equipped state at capture time. It fails
instead of treating the selection command alone as proof. The resulting native
blade/HUD composition can be inspected in RenderDoc, including pixel history.

The SDK fixture also tests a back-facing source and eligible/excluded native
receiver lists after GPU index remapping. Final DI shading is fused into spatial
reuse, and submitted-frame reservoir buffers exchange roles instead of copying.
These optimizations retain the same sampling counts and output resolution.

The moving-caster regression also samples the first four submitted frames after
both revealing and hiding a light. Raw direct visibility must reject a new
occluder immediately; filtered shadow residual must fall below 5% by frame four,
and newly exposed filtered light must reach 50–150% of the settled fixture value.
Initial proposals test visibility; final hidden selections are discarded.
Ray-traced cross-surface normalization matches that rejection, using the actual
previous scene for temporal visibility. Final visibility is always retraced.
These bounds describe the controlled fixture, not arbitrary moving scenes.
Additional first-frame raw-radiance checks exercise the actual previous BVH,
same-slot deforming BLAS, previous opaque materials and previous alpha texture
revisions. Twelve alternating deformations stress recycled storage. Revealed
stochastic samples need not all equal their converged mean; blocked frames must
always have zero raw direct lighting. A recycling-disabled control has matching
readbacks. Temporal correction must be enabled before including the SDK temporal
header, whose bias-correction macro has no local default.

The direct-lighting fixture also checks steady point-source energy/variance next
to a hard shadow, and a weak visible source alongside a 100× brighter occluded
source. These catch inconsistent visibility rejection/normalization and excessive
reservoir noise that a simple shadow/no-shadow check misses. Raw SHaRC emission
and reflected-light checks use fixed sample means; the removed-light check uses
the maximum of a late sample window, avoiding a lucky zero on one stochastic path.

For a straight-edge finite-source penumbra profile, set
`DARK_LIGHTING_PROFILE_CSV=/tmp/penumbra.csv` when running `dark_rtxdi_test`.
The fixture averages 512 frames at nine columns and normalizes by the unblocked
reference at each column. It checks a bounded, monotonic profile with multiple
intermediate light levels, rather than only one partially shadowed pixel.

## Native lifecycle checks (Linux)

Use a copied game configuration with resource links and private saves, never the
retail installation as a writable test directory. The lifecycle helper creates
its own temporary save directory and requires actual modern 3D frames after a
mission-loop save/load cycle. Resize checks require two observed world extents,
including 1280×720. Both cases reject validation errors and renderer fallback.

```sh
VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation VK_LAYER_VALIDATE_SYNC=1 \
  python3 tools/rtx_sdk_test/lifecycle.py /path/to/test-game \
  --log-dir /tmp/dark-sdk-lifecycle
```

Allocation recovery has separate `--phase world`, `--phase nrd` and `--phase sharc`
injections in `tools/world_scene_test/native_allocation_failure.py`. A passing
first-allocation failure does not cover later SDK allocations.

Capability fallback in an SDK-enabled build can be tested on the current RTX device without assuming a
second GPU is available:

```sh
VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation VK_LAYER_VALIDATE_SYNC=1 \
  python3 tools/rtx_sdk_test/feature_fallback.py /path/to/test-game \
  --log-dir /tmp/dark-feature-fallback
```

The preload masks one queried feature at a time: ray queries, NRD formatless
storage writes, or SHaRC buffer int64 atomics. It requires an actual mask, native
presentation, explicit fallback and clean validation. This is capability fault
injection on the tested GPU, not a cross-vendor hardware certification.

## DLSS

See [DLSS integration and GPU checks](../../docs/dlss.md) for optional build setup, mode selection, and the `dark_dlss_test` fixture.
