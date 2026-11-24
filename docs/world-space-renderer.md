# World-space renderer

The modern Vulkan renderer displays actual NVIDIA RTXDI + RTXGI SHaRC + NRD
lighting, with raster primary visibility and Vulkan ray queries. The original
renderer remains the default and the unsupported/disabled-device fallback.
PCSS, shadow-map pools and the custom irradiance-probe implementation are removed.

Current validation is Linux game only. See [lighting](ray-traced-lighting.md),
[migration status](rtx-sdk-migration.md) and
[SDK checks](../tools/rtx_sdk_test/README.md) for configuration and evidence.

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

## Composition and lifecycle

Opaque and masked geometry writes a single surface/depth pass. RTXDI performs
initial, temporal and spatial resampling before final visibility/shading. SHaRC
runs sparse Update -> Resolve -> Query passes for diffuse and rough-specular
indirect paths. RELAX denoises demodulated signals; fullscreen HDR composition
restores material factors and adds emission, ambient light and native fog.
Transparent effects use forward direct lighting and the same opaque depth;
these effects currently use a single emitter-center visibility ray without
indirect lighting or denoising. Native first-person overlays, sky and HUD retain
their original composition path.

Previous geometry, camera, light indices and GPU surfaces follow accepted queue
submissions. A frame slot must retire before CPU uploads or descriptor reuse.
Resources shared across slots execute on one ordered graphics/compute queue.
Size/capacity changes retire all users before rebuilding persistent histories.
Rejected submissions clear temporal validity; failed recordings discard their
unsubmitted caches. Allocation failure returns to native drawing, and submission
recovery retires work before rebuilding the modern renderer.

## Diagnostics and remaining validation

`DARK_RENDER_TRACE` reports `world-ray-scene` BLAS/TLAS workload and memory,
`world-lighting` extent/GPU/CPU times, CPU recording phases, GPU lighting and
transparency phases, and texture-cache activity. Old shadow-budget traces and
controls are gone. Use the measured extent, not the configured canvas size,
when assessing the 2560×1440 performance target.

Controlled GPU fixtures and mission smoke tests pass. Final temporal image
quality, long traversal, broader lifecycle cases and the 60 FPS target remain
unverified; current evidence does not establish release readiness.
