# DLSS Super Resolution

The modern Vulkan renderer supports NVIDIA DLSS Quality, Balanced, Performance,
and DLAA. DLSS runs on the denoised HDR world image before tone mapping and native
UI composition. The native sky coverage and depth bridge are retained separately.
The native UI stays at the display resolution.

## Build and enable

```sh
git submodule update --init 3rdparty/DLSS
cmake --preset linux -DDARK_ENABLE_DLSS=ON
cmake --build build-linux -j8
```

DLSS is optional and defaults to `DARK_ENABLE_DLSS=OFF`. It requires
`DARK_ENABLE_RTX_SDK=ON`. The CMake integration supports Linux x86-64 and MSVC
Windows x64; the Linux path is GPU tested. Other hosts retain the existing renderer.
`DARK_DLSS_ROOT` can select another SDK checkout; the bundled revision is pinned in
the submodule.

Add to the game configuration, or pass these as executable arguments:

```text
renderer_mode modern
renderer_dlss quality
```

`renderer_dlss` accepts `off` (default), `quality`, `balanced`, `performance`, and
`dlaa`. Mode selection is applied when the world renderer initializes. Quality,
Balanced, and Performance use the render resolution returned by NGX; DLAA uses
native resolution. Unavailable builds, GPUs, drivers, or runtimes log a reason and
continue at native resolution. A rendering/evaluation failure uses the existing
native renderer recovery path.

The build copies the release runtime next to the executables; installation includes
it in `bin`. Keep `libnvidia-ngx-dlss.so` (Linux) or `nvngx_dlss.dll` (Windows) beside
the executable when packaging. The SDK and runtime retain their upstream
[license](../3rdparty/DLSS/LICENSE.txt).

## Rendering and history

The integration uses Vulkan NGX directly, with feature-specific extension discovery
before device creation and a project UUID for this engine. HDR color follows
RTXDI/SHaRC and NRD. A compute pass packs normalized Vulkan depth into R32_FLOAT
and converts the engine's unjittered previous-minus-current UV motion to RG32_FLOAT
pixel motion. Halton jitter is applied to world rasterization; NRD receives the
current and preceding accepted sample jitter. NGX receives the opposite raster
displacement in the same coordinate system as pixel motion. DLSS uses automatic
exposure. RTXDI temporal reservoir reprojection compensates for the difference
between current and previous raster jitter.

Temporal state advances only after successful queue submission. Scene and camera
history invalidation reset DLSS accumulation. Output resize retires GPU work and
recreates the shared NGX feature; a rejected recording also recreates the feature
because NGX updates its internal resource bookkeeping during recording.

The pinned 310.9.16 runtime uses transformer preset K for Performance, matching the
model used by Quality/Balanced/DLAA. Its default Performance model produced Vulkan
validation errors on initial evaluation and resize on the test GPU; K passed those
checks. All modes retain their corresponding NGX resolution setting.

Blended world surfaces currently use the opaque surface motion and depth underneath
them. Native sky coverage and depth use the input-resolution images during final
composition. Moving transparency and silhouette quality need visual coverage
beyond the synthetic GPU fixture. Frame generation and ray reconstruction are
separate SDK features; this integration provides Super Resolution and DLAA.

## Verification

```sh
cmake --build build-linux --target dark_dlss_test dark_rtxdi_test -j8
VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation VK_LAYER_VALIDATE_SYNC=1 \
  ./build-linux/dark_dlss_test build-linux/generated/vulkan
VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation VK_LAYER_VALIDATE_SYNC=1 \
  ./build-linux/dark_rtxdi_test build-linux/generated/vulkan
```

The DLSS fixture checks each mode's actual render dimensions and displayed-size
HDR output, both frame slots, output resize, scene reset, rejected command
recording, and switching back to native resolution. It fails on validation errors.
It returns 77 when no compatible Vulkan RTX/DLSS device exists.

For a native game run with private saves and recorded render/output sizes:

```sh
python3 tools/world_scene_test/native_performance.py /path/to/game \
  --dlss quality --frames 120 --warmup 30 --size 1920 1080 --windowed \
  --trace /tmp/dlss.trace --log /tmp/dlss.log --output /tmp/dlss.json
```

The helper rejects requested DLSS modes that fall back during initialization.
Validation affects timing; use separate validation and timing runs. Compare total
world GPU time as well as presentation intervals. The original phase timing named
`transparency_gpu` currently includes the subsequent DLSS pass.

References: [NVIDIA DLSS SDK](https://github.com/NVIDIA/DLSS) and its
[programming guide](https://github.com/NVIDIA/DLSS/blob/main/doc/DLSS_Programming_Guide_Release.pdf).
