# RTXDI, SHaRC and NRD lighting

Optional DDS/PNG material maps and filename conventions are described in
[PBR textures](pbr-textures.md).

The Linux `DARK_ENABLE_RTX_SDK=ON` build uses actual NVIDIA RTXDI ReSTIR DI,
RTXGI SHaRC indirect diffuse/rough-specular lighting, and NRD RELAX when
`renderer_mode=modern` and the complete ray-query feature set are available.
`modern_rt` is an alias. Native rendering remains the default. This migration
is in progress; see [migration status](rtx-sdk-migration.md) for verified gates,
controls and outstanding work. The current target is the Linux RTX 4070 Ti.

```sh
DARK_GAME_PATH=~/.steam/steam/steamapps/common/thief_gold/ \
  ./build-linux/thief-runner quickstart=miss2.mis renderer_mode=modern \
  SlowFrame=0 min_frame_time=0 vsync=0
```

Raster primary visibility produces owned material, depth, normal, identity and
motion guides. Mesh BLAS storage is reused until geometry revisions change;
instance poses update the TLAS. Offscreen shadow casters remain available to
ray queries. Texture alpha and associated emitter glass are tested against
copied scene data, preserving native gameplay lighting and stealth calculations.

RTXDI initial/temporal/spatial resampling chooses direct-light samples from the
native receiver lists. Point, spot and directional sources use finite emitter
samples for soft shadows. SHaRC separately traces sparse updates, resolves its
spatial hash cache, and queries it only on eligible indirect continuations.
Diffuse and GGX specular contributions are demodulated before RELAX and
remodulated afterwards with the same material factors. Primary emission and
ambient light are added outside denoising; native receiver fog applies last.

HDR composition reuses primary depth for transparent forward effects and
exports opaque view distance for the native depth bridge. Uncovered pixels
retain the native sky/background. Transparency currently has forward direct
lighting with one emitter-center visibility ray, without indirect cache reuse
or denoising. The custom irradiance-probe
implementation has been removed.

History follows accepted queue submissions, independently of frame slots.
Motion includes transforms and deforming vertices. Failed submissions restart
accumulation; camera cuts, resets and resource changes reject invalid history.
Temporal bias correction queries the retained previous submitted BVH, materials
and alpha textures. Shared snapshots keep those resources alive until the
consuming slot retires; copy-on-write and retired-resource recycling preserve
both scenes during deformation and same-slot reuse.
Linux validation covers resize, genuine save/load, NRD/SHaRC allocation failure,
and a rejected submission followed by renderer rebuild and recovered frames.
These checks cover the injected failures and transitions described in the
migration notes, rather than every possible resource failure.

Shadow-map/PCSS code and custom probes are removed. SDK-disabled builds,
unsupported devices and `renderer_raytracing=0` retain the original renderer.
Measured 2560×1440 native input routes average 65.30 FPS in Gold mission 1 and
75.05 FPS in mission 2 on the current RTX 4070 Ti. Both retain frame-time spikes;
these local routes do not establish stable 60 FPS throughout the missions.
GPU fixtures cover dynamic lighting, disocclusion and finite-source penumbrae.
Native captures cover composition and a short animated-character sequence;
broader temporal quality remains under review. See the migration notes for the
test scope and measured limitations.

Validation commands and SHaRC configuration are documented in
[SDK checks](../tools/rtx_sdk_test/README.md) and
[migration status](rtx-sdk-migration.md).

Optional [DLSS Super Resolution and DLAA](dlss.md) can reduce the world rendering resolution while retaining native UI composition.
