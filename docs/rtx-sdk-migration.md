# RTX SDK migration status

The accepted target is raster primary visibility + NVIDIA RTXDI ReSTIR DI +
RTXGI SHaRC diffuse/rough-specular indirect lighting + NRD RELAX. The Linux
RTX 4070 Ti target is 60 FPS at a measured 2560 x 1440 world-lighting extent.
This migration is **not complete**. The displayed ray-enabled path now uses the
actual SDK outputs. Custom probes are removed. Shadow-map fallback code is removed;
the measured native routes exceed 60 FPS on average at the target extent, with
remaining frame-time spikes. Broader temporal visual coverage remains pending.

## Implemented foundation

- Pinned SDK sources; DXC download with SHA256 verification; native NRD SPIR-V
  builds without NRI or nvpro_core2.
- A native Vulkan RELAX adapter creates internal pools, pipelines, descriptor
  sets, uniform uploads, dispatches, and inter-pass dependencies. Its standalone
  six-frame test executes actual GPU work and tests history reset.
- World history advances only on successful submission, owns immutable previous
  geometry, and detects scene reset, resize, and camera cuts. Material matching
  rejects individual changed surfaces without clearing unrelated geometry.
- Actual RTXDI SDK initial, temporal and combined spatial/final-shading dispatches
  consume the surface guides, native receiver lists and alpha-tested ray scene.
  Accepted light handles map indices between current and previous frames. Previous
  surfaces and light/draw/link data are GPU-owned copies, independent of frame slots.
- RTXDI emits demodulated diffuse/specular radiance and hit distances into NRD
  RELAX. The live renderer executes denoising with conventional camera matrices,
  linear view depth and screen-space motion. A fullscreen HDR pass remodulates
  diffuse/specular factors and adds primary emission, ambient light and native fog.
  Its shared primary depth rejects hidden transparent surfaces; opaque geometry
  is no longer rasterized again for depth and lighting.
- Actual SHaRC 1.8.3 Update -> Resolve -> Query passes use SDK GLSL buffer
  references with explicit inter-pass barriers. Sparse updates cover jittered
  pixels across the full view; query traces diffuse and GGX specular continuations.
  Cache reuse excludes primary surfaces, short segments and sharp specular
  footprints. Material demodulation and separate emissive reconstruction are on.
- SHaRC buffers use 8-byte keys, 16-byte accumulation and 16-byte resolved entries
  (40 MiB at the default 1,048,576 entries). All are zero-cleared initially and
  after scene/history, capacity, scale, material or enabled-light-set changes.
  The old custom probe update, sampling code, buffers and controls are removed.
  Update-time cache resampling is disabled: its stale-radiance feedback failed a
  dynamic-light test. Updates trace fresh paths; Query still terminates on hits.
- The live modern renderer now rasterizes opaque/alpha-masked surface guides,
  including submitted-frame transform and vertex-deformation motion. Transparent
  effects remain outside this pass.

## Surface ABI

All surface images finish in GENERAL layout and are frame-slot owned. Slots must
be retired before reuse. The MRTs are:

| Attachment | Format | Content |
| --- | --- | --- |
| 0 | RGBA32F | World XYZ and positive linear view Z; uncovered Z is 1,000,000. |
| 1 | RGB10A2 UNORM | Official NRD normal/roughness packing, material category 0. |
| 2 | RGBA16F | Linear base RGB and metallic. |
| 3 | RGBA16F | Linear emission RGB; alpha = native fog flag + (1 − AO) / 4. |
| 4 | RGBA32F | Previous UV minus current UV (invalid motion points outside history), previous view Z, history validity. |
| 5 | RGBA32 UINT | Stable 64-bit instance handle, surface-draw index; W bit 0 = history validity, bits 1–30 = geometric normal (two 15-bit octahedral coordinates). |

An alpha-as-red sampled view of attachment 0 exposes its view Z to NRD
without a separate depth-copy pass. Surface-draw metadata retains the material
index and native receiver-light list range for the RTXDI application bridge.
The two surface camera matrices are unjittered and use Vulkan screen UVs.

Optional DDS/PNG PBR companions now feed raster guides, transparent forward
shading and secondary GI hits through shared material sampling. GPU materials
use a 96-byte layout with six optional texture indices and a fast path for
materials without maps. Color/emissive use sRGB; scalar/normal/ORM use UNORM.
AO affects ambient and primary indirect diffuse, preserving direct illumination.
Missing maps retain native defaults; changing to an unregistered animation frame
clears old optional maps. See [texture conventions](pbr-textures.md).

Primary GI ray offsets and hemisphere checks now use a separately retained
geometric normal, independent of smooth interpolation or a normal-map tilt.
The raster pass packs it into existing identity metadata; motion validity remains
in bit 0. A grazing normal-map fixture exposed 0.514679 units of indirect light
from an emissive panel behind the opaque receiver. With geometric offsets and
diffuse/specular hemisphere rejection, both raw GI outputs are zero; a direct
source between the receiver and panel exposed a separate 1.272461-unit direct
leak. RTXDI target/visibility checks now use geometric normals, including previous
submitted surfaces for temporal visibility; direct leakage is now zero as well.
Secondary direct-light rays also use their traced face normal. The full GPU fixture passes
synchronization validation. This addresses a measured backface leak, rather than
reducing indirect sampling or disabling authored normal maps.
GPU readback checks both a rotated geometric normal and independence from mapped
shading normals. A 40-frame native Gold miss2 smoke (41 modern records including
the final record) also exits cleanly with original textures and no validation or
fallback diagnostics.
The final regression run uses 512 penumbra samples at nine columns, retaining
the same bounds and monotonicity checks; relative filtered visibility falls
from 0.8432 to 0.1059. The increased measurement width/sample count avoids
classifying a noisy interior penumbra sample as the fully lit endpoint.

The transparent forward path now makes the same geometric/shading distinction.
A normal-mapped foreground regression previously produced HDR 0.3872 with a
source behind its face, against an expected ambient/background blend of 0.0470.
After geometric hemisphere clipping and offsets, it reports 0.0469. The full
SDK fixture, including native-depth preservation and transparency blending,
passes synchronization validation. Transparent shading remains forward direct
lighting; this fix does not add indirect lighting or NRD to that path.

The shader uses the SDK's GLSL front end for normal packing. NRD 4.18 contains
an HLSL-only `[flatten]` hint in that header; CMake removes that optimization
hint from a generated compatibility copy while retaining SDK source and notices.
The submodule is unchanged. Both frontend and NRD use normal encoding 2 and
linear roughness encoding 1. Encoding 4 was investigated but failed SDK shader
compilation with the pinned compiler; it is not enabled.

## Verified so far

- Linux game build and Thief Gold mission 2 smoke under synchronization validation.
- GPU readback verifies surface world/view depth, NRD packed normals/roughness,
  instance identity, first-frame invalidity, static zero motion, moving-instance
  velocity sign, and deformation velocity.
- The SDK fixture now tests displayed HDR, emission bypass, native fog, transparent
  source-over blending, opaque occlusion and preservation of exported native depth.
  Rough-specular indirect radiance is also read back after RELAX (1.682 in the
  emissive fixture).
- Native RELAX test covers rejection before submission, including first pool-image
  initialization, and passes synchronization validation.
- RTXDI GPU readback verifies occluded/lit direct radiance, new occlusion after
  temporal reuse, and point/spot/directional sources. RELAX preserves nonzero
  converged radiance within 25% of the raw result in this fixture. This is an
  execution/energy check, not proof of final temporal image quality.
- SHaRC GPU checks prove successful cache hits, emissive and point-light bounce,
  wall separation, emission edits, the uncached reference toggle and light removal.
  An emissive fixture averages 1.920 cached vs 1.922 uncached over 24 frames;
  this fixture is an energy check, not a general image-error bound.
- Linux Gold mission 2 completes a 60-frame validation smoke with SHaRC executing.
  Mission 1 also passes 40 frames with a 65536-entry cache, downscale 3 and
  scene scale 25, exercising non-default allocation/configuration.
- SHaRC requires shaderInt64, buffer int64 atomics, shaderFloat16, 16-bit storage
  and compute subgroup ballot support. These are probed and explicitly enabled.
- NRD requires shaderStorageImageWriteWithoutFormat for the pinned DXC kernels;
  the SDK ray path now probes and enables that feature.

## Required remaining work

1. GPU readback now covers RTXDI dynamic visibility, primary disocclusion and
   finite-source penumbrae. Broaden native temporal visual checks beyond the
   inspected local animated-character sequence and traversal captures.
2. SHaRC response tests now cover enabled-source intensity/position edits,
   geometry occlusion, alpha-only visibility changes and rough-specular camera
   motion. Native performance and rejected-submission recovery include the
   adaptive update. The current-build resize/save-load recheck also passes.
3. Representative native water and equipped-sword/HUD captures now verify actual
   pixel contribution and depth composition. Broader multi-frame/mission coverage
   remains separate from those local views and from synthetic readback.
4. Broaden denoised disocclusion and moving-scene history coverage. Native resize,
   save/load, NRD/SHaRC allocation failure and rejected submission checks pass.
5. Capability fault injection now verifies ray-query, NRD storage and SHaRC atomic
   requirements independently. PCSS/maps and obsolete tests are removed; the
   original renderer is the disabled/unsupported fallback. Other physical GPU
   vendors remain outside the Linux RTX 4070 Ti validation scope.
6. Native missions 1/2 now have measured 2040-frame input routes at 2560×1440,
   and native creature deformation matches actual BLAS/TLAS build inputs. Tail
   stalls remain; broader visual coverage and stability conclusions must stay
   within the evidence described below.

## Requirement audit

The PBR addition changes the material/shader path after the long performance
runs below. A subsequent 600-frame mission 1 route (120 warmup, 479 intervals)
averages 47.75 FPS, with a 20.94 ms mean and 49.95 ms p95; world GPU mean is
20.16 ms. This shorter run is not an isolated before/after comparison, and its
cause remains unresolved. The older 65.30/75.05 FPS results must not be claimed
as verified current-build performance. The performance goal remains open.

A repeat of that short route averages 50.31 FPS, with GI/NRD median GPU times
2.59/4.93 ms and means 3.39/6.22 ms. Independent GPU activity was observed in
the same investigation: a post-run `nvidia-smi pmon` sample reports 34% SM use
by a Python compute process and 18% by the compositor. This is not an isolated
attribution of every slow frame, but prevents treating these timings as a clean
PBR before/after comparison. User processes were left running. A no-map GI
specialization trial did not show a measurable gain and was reverted.

The retained GPU regression now switches a secondary emissive panel from scalar
emission to an equivalent scalar-plus-texture material and back, comparing raw
indirect energy. The first validated run measures 1.8383 versus 1.8396. This
checks that PBR emission maps participate in secondary GI, independently of
primary emission composition and missing-map defaults.

| Goal requirement | Current evidence | Scope/remaining limit |
| --- | --- | --- |
| Actual RTXDI, SHaRC and RELAX | Pinned SDK calls in production shaders/adapter; real GPU readback and inspected SDK dispatches | Linux RTX 4070 Ti; no cross-vendor certification |
| Raster visibility and reusable world/AS | Surface MRT readback; unchanged-mesh BLAS reuse; native creature upload/TLAS matching | Native capture identity/motion checked locally |
| Submitted-frame motion/history | Object/deformation/queued-slot tests; retained previous scene/material/alpha data; rejected-submission recovery | Broader native temporal visual coverage remains open |
| Native gameplay lighting and fallback | Gameplay/stealth code unchanged; ray-query/NRD/SHaRC feature masking; SDK-off game build and native fallback | Injected capability cases, not every physical GPU |
| Remove PCSS and custom probes | Implementations/obsolete tests deleted; no runtime references in renderer/build files | Native original renderer retained as requested |
| Correctness and synchronization | Full SDK fixture, native missions 1/2, alpha/emitter/culling, dynamic-light and GI response tests | Quantitative fixture bounds are local, not general image error bounds |
| Resource recovery | Current adaptive build: resize and genuine save/load pass; rejected submission resumes six modern frames; separate NRD/SHaRC allocation failures retain native terrain | Failure tests inject the documented allocation/submission cases |
| 60 FPS target at 2560×1440 | 2040-frame native routes: 65.30/75.05 mean FPS in missions 1/2 | 75/56 of 1919 intervals exceed 16.67 ms; not stable 60 FPS throughout missions |
| Document limitations | This audit, measured routes, fixture/capture descriptions and rendering guide | Broader native temporal inspection remains pending |

The final current-build lifecycle recheck contains 50 world frames through actual
3440×1380 → 1600×900 → 1280×720 resize transitions, and 239 world frames through
a genuine successful private save/load (`saved=0`, `loaded=0`). Both logs and
traces are clean under synchronization validation. These are correctness checks,
not performance runs. The game and both GPU test targets rebuild successfully.

## Adaptive indirect-light response

Native Gold creature observation now covers 240 world frames in each of missions
1 and 2 under synchronization validation. The observer uses the native creature
API and compares captured local vertices without altering poses or camera.
Mission 1 has six creatures/four animated objects with 663 changing part updates;
mission 2 has 22/19 with 3194 updates. Both continue BLAS rebuilding after startup
and exit cleanly. A stronger mission 1 rerun additionally matches 668 changing
creature part uploads byte-for-byte at the Vulkan BLAS build call and observes
1432 references to matching BLAS addresses in TLAS build inputs. This verifies
the native deformation-to-acceleration data path. The stronger mission 2 rerun
matches 3391 changed uploads and 5258 creature-instance references, also clean
under synchronization validation. CPU visibility flags and build
inputs do not establish per-creature intersections, motion pixels or native
visual quality. Counts vary with native simulation timing.
The reproducible helper is `tools/rtx_sdk_test/native_animation.py`.

Its optional RenderDoc camera aid now provides an inspected Gold mission 1
library-aisle character view. The logged object 237/instance 1443 matches the
actual center-pixel surface identity. The current-to-previous motion is
(-0.0000094, -0.0028645), with valid history and previous view depth 7.2303.
The captured frame contains 15 SDK compute dispatches and finite nonnegative
3440×1380 HDR. This demonstrates native animated-character raster/motion
composition in one frame. The aisle is very dark, so this is insufficient
evidence of illuminated shadow contrast or multi-frame ghosting quality. The
capture uses a detached camera and is separate from the 2560×1440 performance
routes. An earlier camera placement faced nearby geometry and is not counted
as character evidence.

The same capture helper now accepts `--test-light`, adding an explicitly
controlled point source through the scene API and preserving native receiver
lists while appending that source. The inspected result clearly shows the
character's textured body and nearby shelves, with instance 1443 at the center
pixel. HDR remains finite and nonnegative, with RGB maxima
1.0313/0.7446/0.4680. The added light is test-only (intensity 100, range 30,
radius 0.12); this is not evidence of original mission lighting balance or
multi-frame ghosting quality. No production lighting settings change.

An additional eight-frame capture with that source now provides local temporal
evidence. The RDC contains eight presents; the character's pose changes across
them and no obvious detached silhouette trail appears in the inspected contact
sheet. The character moves past the fixed light: at its center pixel the decoded
normal/light cosine changes +0.514, +0.463, +0.292, +0.112, -0.027, -0.063,
-0.063, -0.201. RTXDI direct red radiance, read immediately after spatial shading
and before GI, follows 0.276, 0.232, 0.174, 0.079, then zero for four frames.
Thus the visible darkening has a geometric explanation. RELAX also includes
indirect illumination, so its remaining nonzero signal is not a pure direct-light
residual measurement. All eight full HDR images are finite and nonnegative.
This local sequence addresses native pose/motion composition under capture
overhead; it does not establish a general temporal error bound or performance.

The GPU fixture also verifies primary-surface disocclusion, independently of
offscreen shadow movement. A foreground triangle's actual surface identity is
read back before it moves away. After revealing the receiver, RTXDI raw diffuse
returns on the first frame; RELAX reports 0.7563 on that frame and 0.7549 on
frame four, against a previously settled 0.7510. The dark foreground also rejects
old background lighting. The full fixture passes synchronization validation.
This establishes one controlled depth/identity transition, not general native
animated-character quality.

Light, material, texture or ray-scene geometry changes temporarily shorten SHaRC
Resolve accumulation to one frame and stale-entry retention to at most eight
frames. This window lasts 16 accepted submissions after the last change; a static
scene then returns to its configured accumulation/staleness limits. Global light
scale changes also trigger the window. Rejected submissions do not advance it.
Unchanged native light updates preserve their revision, so ordinary per-frame
resubmission does not continually restart the window. Texture revisions include
alpha-only edits, which can change visibility without changing a transform.
This preserves the spatial cache while trading temporal smoothing for faster
response during changes; moving scenes can remain in the shorter window.

The synchronization-validated GPU fixture keeps a source enabled while reducing
its intensity from 80 to 10. Displayed diffuse drops from 0.2452 to 0.0428 over
frames 9–16 and settles at 0.0354, versus 0.0358 without cache reuse. Rough
specular settles at 0.0559 versus a 0.0568 reference. Source movement and a new
occluder both return to the 0.0050 background. An alpha-only opening restores
0.1171 against a 0.1186 reference; a global light-scale edit settles at 0.0354
against 0.0358. These are controlled fixture measurements, not general error
bounds. The fixture uses an explicit 60 Hz NRD clock; native NRD timing remains
automatic. All displayed-lighting checks pass without validation diagnostics.
The adaptive build also survives an injected failed submission after three
accepted modern frames, rebuilds the renderer and submits six recovered modern
frames with sword selection requested, under synchronization validation. This
checks rejection/recovery; the separate equipped-sword capture establishes its
visible composition. All four core scene/range/history/geometry tests pass.

## SHaRC controls (intermediate integration)

`DARK_ENABLE_RTX_SDK=ON` builds the actual SDK path. `renderer_gi=1` executes
SHaRC in the ray-enabled renderer. Its outputs feed RELAX and displayed HDR
composition. Transparent effects currently use forward direct lighting without
SHaRC or denoising.

| Setting | Default | Meaning |
| --- | --- | --- |
| `renderer_sharc` | 1 | 0 traces the same indirect integrator without cache reuse, for comparison. |
| `renderer_sharc_entries` | 1048576 | Rounded down to a power of two, range 65536–4194304; size changes recreate the cache. |
| `renderer_sharc_downscale` | 5 | One jittered update root per tile, range 1–10. |
| `renderer_sharc_scene_scale` | 50 | World-space voxel density, range 1–100. |
| `renderer_sharc_accumulation` | 8 | Static-scene Resolve window, range 1–100 frames; changes temporarily use 1. |
| `renderer_sharc_stale_frames` | 64 | Static-scene eviction age, range 8–100 frames; changes temporarily cap it at 8. |
| `renderer_gi_strength` | 1 | Indirect-radiance multiplier. |

Diagnostic C++ callers can enable `RaySettings.sharcStatistics` and read
`IndirectStatistics` only after the slot fence retires. Counters are disabled
by default to avoid global atomic contention in normal rendering.

The RELAX prefilter is deliberately smaller than the SDK defaults (2-pixel
diffuse / 4-pixel specular radius, hit-distance minimum weight 0.01, 16-frame
main / 3-frame fast history, four A-trous iterations). The default 30/50-pixel
prefilter visibly filled an occluded directional-light fixture with radiance.
This tuning passes the current shadow test; final temporal/penumbra image
quality still needs broader inspection.

## PCSS removal and fallback checks

The modern renderer has no shadow-map, cascade, light-budget or PCSS branches.
Map-only pipelines/descriptors/uploads and their old test fixtures are removed.
`DARK_ENABLE_RTX_SDK=OFF` compiles a small renderer stub; its shader build contains
only the original scene vertex/fragment shaders and requires no RTX SDK headers
or DXC. Original terrain/model/overlay rendering remains available.

Verified on Linux: SDK ON and OFF game builds, the four core scene/range/history/
geometry tests, actual SDK GPU fixtures, runtime `renderer_raytracing=0`, and a
40-frame SDK-disabled native fallback smoke. Native frame traces report modern=0.
Injected allocation failure retains 105 native terrain draws without suppressing
native drawing. A rejected submission after three accepted modern frames rebuilds
and resumes six modern submissions with clean synchronization validation.
Separate allocation injections during NRD initialization and after NRD succeeds
(SHaRC allocation) also retain native rendering with clean validation. These
checks do not replace visual inspection or the final performance run.

The save/load regression enters through the actual mission loop and uses private
temporary saves. It requires resumed modern 3D submissions after loading, rather
than merely successful save/load API returns or UI frames. The corrected run
counted 239 modern 3D frames with clean validation. An earlier quickstart-based
check did not prove this: it saved campaign state and returned to mission UI.

## Preliminary full-resolution baseline

A short, stationary Gold mission 2 run after PCSS removal rendered a verified
2560×1440 world extent: 60 frames, 20 warmup, 39 measured presentation intervals.
It averaged 56.81 FPS (17.60 ms interval; p95 18.43 ms). World GPU time averaged
17.27 ms and CPU recording 4.28 ms. This does **not** meet the 60 FPS gate and is
not a traversal result. Per-pass GPU timestamps now locate the cost before optimization; final sustained
measurements remain required. A matching short profiled run averaged 56.63 FPS:
RTXDI 7.26 ms, NRD 4.36 ms, SHaRC 2.63 ms, primary surfaces 1.28 ms, history
copies 1.16 ms and composition 0.35 ms. These values are provisional and identify
optimization work; they do not establish final quality or sustained performance.

## Full-resolution optimizations and current timing

The estimator sample counts remain unchanged: 16 initial candidates, four spatial
neighbors (eight for disocclusion), 20-frame RTXDI history and the same RELAX
settings. No resolution reduction was used.

- Previous motion is no longer allocated/copied; only current-frame motion is
  consumed. Final reservoirs exchange current/history slots after accepted
  submissions instead of copying a full reservoir image every frame.
- Spatial reuse and final visibility/shading execute in one dispatch. Spatial
  neighbors read only temporal input, so this removes an intermediate store,
  barrier and reread without changing estimator dependencies.
- Initial proposals already belong to the receiver list and skip redundant
  membership scans. Reused samples still validate links using sorted GPU-private
  indices and binary search. Native scene/gameplay lists are unchanged.
- Back-facing lights exit before GGX evaluation, including the degenerate V+L=0
  case. GPU tests check zero diffuse/specular output and native-link exclusion of
  a much brighter ineligible light.
- Initial sampling tests the selected proposal's visibility, and final shading
  discards hidden selections. Temporal/spatial normalization uses ray-traced
  cross-surface visibility; temporal queries use the previous submitted scene.
  Visibility shortcuts remain disabled. This keeps proposal rejection and
  normalization consistent. With the corrected macro/include order and previous
  scene, revealed raw diffuse reaches 0.735 on frame one versus about 0.757
  settled. Filtered diffuse reaches 0.673 on frame one. New occlusion is immediate
  in raw direct lighting, with RELAX residual below 5% by frame four.
  These are controlled-fixture observations, not general quality bounds.

The fixture checks an unoccluded point-source pixel beside a hard shadow:
clear/edge means are 0.75695/0.75851, edge standard deviation 0.01701. The earlier
hybrid of untested proposals and ray-traced normalization produced edge standard
deviation 0.29809. The regression now requires energy within 5% and standard
deviation below 5% of the clear value. A second fixture adds a fully occluded
source 100 times brighter than a visible weak source: weak-only mean 0.007548,
mixed mean 0.007716, standard deviation 0.002068. The tested Basic/unshadowed
alternative had standard deviation 0.066419 and failed this quality bound;
the visibility-tested/ray-traced variant remains selected. No candidate-count,
resolution or denoiser-quality reduction was made.

Raw SHaRC energy tests now average fixed frame windows rather than requiring
one stochastic path to be nonzero. Disabled-light residual uses the maximum of
16 late samples as well as the displayed-energy decay check.

A straight offscreen half-plane and finite disk emitter now test a spatial
penumbra profile, normalizing each column by its own unobstructed reference.
In an earlier 128-frame measurement at seven columns, RELAX relative light fell monotonically
through 0.842, 0.739, 0.620, 0.520, 0.395, 0.270 and 0.154. The fixture requires
bounded, monotonic coverage and at least three intermediate levels; it can
export CSV with `DARK_LIGHTING_PROFILE_CSV`. This verifies the controlled soft
edge, not every moving-light/geometry case in a mission.

The native water regression now tracks texture binding through deferred legacy
vertex flushes, rather than assuming every draw occurs inside the water call.
At its Thief 2 miss2 fixture location it observes 11 water draws and 12 late-effect
draws, all unsuppressed, under clean synchronization validation. The same fixed
coordinates do not reach water in Gold miss2 and are not evidence for that map.
The helper now uses private temporary saves and optionally preserves its log.
A separate 40-frame RenderDoc run at that location records 39 water draws and
40 late draws. MCP pixel history identifies the water draw after HDR composition:
water changes visible pixels at (2400,300) and (2800,800), while depth rejects
water behind the shore at (2100,800). This verifies actual pixel contribution
and depth rejection, beyond merely reaching the backend. The captured HDR
target remains finite and nonnegative. It is one local Thief 2 view, not all
water/underwater or Gold mission coverage.

The Gold miss2 first-person capture explicitly selects a sword and checks native
state before capture (`owner=1092`, `arm=1114`, `weapon=432`, `equipped=1`). The
image shows the blade, inventory, compass and light gem over the SDK-lit scene.
Pixel history at (2250,1100) confirms the native blade changes the composed
world pixel and writes closer depth (about 0.529 versus 0.951 for the world).
This proves the selected current first-person path rather than merely requesting
selection. The helper's optional `$capture_equip_sword` mode enforces that check.

Temporal bias correction now traces the last accepted ray scene. Immutable
snapshots retain TLAS/BLAS and triangle/instance metadata until the consuming
slot retires; changed resources use copy-on-write. Previous material data and
texture images are retained as well, including old alpha texture revisions.
This also works when a caller repeatedly retires and reuses the same frame slot.
Rejected submissions do not publish a snapshot.

The shader explicitly defines `RTXDI_ALLOWED_BIAS_CORRECTION` before including
the temporal header. Previously the spatial header supplied its default too
late, silently compiling temporal correction out despite the runtime setting.
Earlier timing/quality observations below predate this correction. GPU tests now
require first-frame revealed radiance above 50% of settled energy for a moved
caster, same-slot mesh deformation, opaque-to-cutout material changes and alpha
texture revisions. Rapid alternating deformation also tests current visibility
and recycling, without assuming every stochastic radiance sample equals its mean.

Snapshot storage recycles only after its last snapshot reference retires, with a
64 MiB limit on idle AS/metadata allocations. BLAS reuse checks both primitive
and maximum-vertex capacity as required by the
[Vulkan build-size contract](https://docs.vulkan.org/refpages/latest/refpages/source/vkGetAccelerationStructureBuildSizesKHR.html).
CPU triangle/instance mirrors avoid reading mapped device memory during cloning
and comparisons. A recycling-disabled control produces identical rapid-caster
readbacks to the recycled path.

Mission 1 measurements during this change (600 frames, 120 warmup, 2560×1440)
show ray-scene CPU recording at 7.38 ms with uncached snapshots, 5.76 ms after
resource recycling, and 2.03 ms after replacing mapped-device reads with CPU
mirrors. These are separate runs. The last run averages 58.96 FPS with a 16.62 ms
world GPU mean and 141/479 presentation intervals over budget; it does not pass
the target. GPU timing variability and traversal performance remain unresolved.
The final CPU-mirror build passes the full SDK GPU fixture under synchronization
validation, native resize (3440×1380 → 1600×900 → 1280×720), genuine save/load
with 239 modern 3D frames, and a rejected submission after three accepted frames
followed by six recovered modern submissions. These validate lifecycle ownership,
not broad traversal image quality.

Stationary Gold runs on the current RTX 4070 Ti, verified 2560×1440, 180 frames /
60 warmup / 119 measured intervals, uncapped and without validation or GDB:

| Stage | Mission 1 FPS | Mission 2 FPS |
| --- | ---: | ---: |
| After history-copy removal | — | 59.20 |
| After dispatch fusion | 55.99 | 60.61 |
| After receiver-link optimization | 59.77 | 67.75 |

These are short stationary measurements with simulation and hardware variability,
not sustained traversal results. Mission 1 remains below target, and budget
misses remain in both runs. The performance gate is still open.

After the moving-shadow history correction, longer stationary runs (600 frames,
120 warmup, 479 measured intervals) average 60.95 FPS in mission 1 and 63.17 FPS
in mission 2. Mission 1 misses the 16.67 ms interval budget 120 times, mission 2
71 times. World GPU means are 16.09 and 15.49 ms respectively. These runs clear
60 FPS on average, but do not prove stable frame pacing or traversal performance.
They use different run lengths from the table above, so they are not an isolated
before/after timing comparison of the history correction.

A receiver-membership bitset trial saved only about 0.08 ms of DI time on mission
1, within short-run variability. It was reverted to retain the simpler sorted
lookup. The GPU regression still covers receiver links at indices 31, 32 and 64.

RenderDoc capture/replay now works through MCP using SDL X11 (the installed
RenderDoc lacks Wayland surface support). The captured mission 2 frame contains
three DI dispatches plus SHaRC/RELAX, displayed lighting, native HUD and first-
person elements. Its 2560×1440 HDR target reports no negative, NaN or Inf values.
The reusable capture helper is documented in `tools/rtx_sdk_test/README.md`.
This establishes one static-frame check, not temporal image quality.

## Native input route and ray traversal

`native_performance.py --route` now measures local gameplay motion without GDB.
A preload helper feeds the ordinary native key queue after world submission;
physics and collisions remain active. Camera traces prove horizontal movement
and turning. Each 480-frame cycle includes short forward/back movements and
turns, with complete trace/extent/input-count checks and private saves. This is
near-start coverage, not full-mission exploration; frame timing affects the
precise trajectory. Validation and performance runs are reported separately.

Safe opaque instances now use Vulkan's hardware opaque override and back-face
culling. Double-sided instances disable face culling; cutouts, blended coverage
and emitter glass retain shader candidate tests. BLAS geometry remains reusable
across material changes because the override is per instance. Tests include
single/double-sided opaque and masked back faces, opaque emitter lenses,
alpha revisions and all existing visibility checks. This follows the
[Vulkan ray traversal rules](https://docs.vulkan.org/spec/latest/chapters/raytraversal.html).

The trace also reports separate initial, temporal and spatial/shading DI GPU
phases. The current visibility-tested variant completes a 2040-frame miss1 input
route at verified 2560×1440 (120 warmup, 1919 intervals), averaging 64.95 FPS with
15.40 ms mean, 16.58 ms p95 and 22.28 ms p99 intervals. It travels 54.7 engine
feet, emits 64 input events, and misses the 16.67 ms budget in 82 intervals.
World GPU mean is 15.09 ms; DI initial/temporal/spatial means are 1.39/1.18/4.45 ms.
This clears 60 FPS on average for this route, with remaining tail stalls and
broader gameplay/visual coverage still pending.
The matching miss2 run averages 72.19 FPS (13.85 ms mean, 16.44 ms p95,
18.37 ms p99), with 65/1919 intervals over budget. It travels 70.4 engine feet
and emits 64 input events. A separate 600-frame miss2 route passes synchronization
validation; its slower validation timing is not a performance result.

The selected visibility-tested build was captured while native input moved the
miss2 camera from (-21.40,-163.47,-1.40) to approximately
(-15.95,-158.12,-2.67). RenderDoc MCP inspection finds the expected 15 SDK
dispatches, nonzero valid motion, native HUD and intact wall/lamppost composition.
The 2560×1440 HDR target has no negative, NaN or Inf values. A strong vertical
lighting boundary coincides with a world-space corner and different surface
normals, rather than a screen-space split. First-person weapon coverage and
multi-frame visual quality are separate checks; this capture does not prove them.

`feature_fallback.py` passes under synchronization validation with injected
missing ray queries, NRD formatless writes and SHaRC int64 buffer atomics. Each
case confirms feature masking, explicit native fallback, native presentations
and no modern submissions. The SDK-disabled Linux game also rebuilds successfully,
including the adaptive-GI revision counters and shared settings.

After the adaptive-GI changes, separate 2040-frame native routes at verified
2560×1440 (120 warmup, 1919 measured intervals, 64 input events each) report:

| Route | Mean FPS | Mean interval | p95 | p99 | Intervals over 16.67 ms |
| --- | ---: | ---: | ---: | ---: | ---: |
| Gold mission 1 | 65.30 | 15.31 ms | 16.53 ms | 21.68 ms | 75/1919 |
| Gold mission 2 | 75.05 | 13.33 ms | 15.45 ms | 19.90 ms | 56/1919 |

World GPU means are 15.00 and 13.01 ms. The routes travel 52.5 and 67.9 engine
feet respectively. These runs exclude validation, GDB and concurrent builds.
They exceed 60 FPS on average locally, but retain tail stalls and do not establish
a stable 60 FPS throughout either mission. Frame-timed trajectories vary, so the
numbers are not an isolated before/after comparison of adaptive accumulation.
