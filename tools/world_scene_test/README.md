# World-scene and renderer checks

The standalone suite covers copied scene ownership, generic geometry bounds,
submitted-frame history and reusable Vulkan geometry buffers. The retired
shadow-map/PCSS suites and custom probe test are removed with their implementations.
Actual RTXDI + SHaRC + NRD lighting is tested separately in the root Linux build.

```sh
cmake -S tools/world_scene_test -B /tmp/dark-world-tests -G Ninja -DCMAKE_BUILD_TYPE=Release
VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation VK_LAYER_VALIDATE_SYNC=1 \
  python3 tools/world_scene_test/run_checks.py /tmp/dark-world-tests
```

This runs six tests: dds, material_metadata, scene, ranges, history and geometry.
The material metadata check covers square/rectangular terrain scales, invalid
dimensions, comments and conditional blocks. The DDS check
covers BC1/2/3/4/5/7, exact channels, pitch and malformed inputs; initialize
`3rdparty/bcdec` before building. Geometry skips with
code 77 when its Vulkan descriptor-indexing requirements are unavailable.
`run_checks.py` builds first and rejects validation errors in the CTest log,
including messages emitted by a process which exits zero.

For SDK lighting and displayed HDR readback:

```sh
cmake --build build-linux --target dark_rtxdi_test dark_nrd_test -j8
VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation VK_LAYER_VALIDATE_SYNC=1 \
  ./build-linux/dark_rtxdi_test build-linux/generated/vulkan
VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation VK_LAYER_VALIDATE_SYNC=1 \
  ./build-linux/dark_nrd_test
```

See [SDK checks](../rtx_sdk_test/README.md) for their precise coverage and limits,
and [migration status](../../docs/rtx-sdk-migration.md) for the remaining gates.

## Native Linux recovery

These helpers use private temporary save directories. Pass the copied test game
configuration or installed game directory as `game_path`; do not edit retail data.

```sh
VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation VK_LAYER_VALIDATE_SYNC=1 \
  python3 tools/world_scene_test/native_allocation_failure.py /path/to/game --log /tmp/allocation.log
VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation VK_LAYER_VALIDATE_SYNC=1 \
  python3 tools/world_scene_test/native_submit_failure.py /path/to/game --after 3 --equip-sword --log /tmp/submission.log
```

The allocation fixture rejects one allocation during world recording and checks
that native terrain drawing resumes without suppression. The submission fixture
rejects one queue submission, checks renderer rebuild and resumed modern
submissions, and optionally selects the sword through the native command path.
They require Linux x86-64, GDB and debug symbols. They reject validation errors;
the intentional injected failure itself is expected.

A separate build with `DARK_ENABLE_RTX_SDK=OFF` exercises the original renderer
without RTX SDK compilation. Runtime `renderer_raytracing=0` also retains the
original renderer. Neither selects a shadow-map fallback.

## Native timing and camera sweep

```sh
python3 tools/world_scene_test/native_performance.py /path/to/game \
  --mission 2 --frames 300 --warmup 60 --size 2560 1440 \
  --trace /tmp/lighting.trace --log /tmp/lighting.log --output /tmp/lighting.json
```

This runs uncapped, fullscreen by default (`--windowed` overrides it), validates
the actual world extent, and summarizes CPU/GPU phases and frame-budget misses.
It rejects fallback and incomplete runs. The default measures a stationary view;
traversal performance cannot be inferred from that default. Use no concurrent GPU tasks
for meaningful comparisons. Validation runs and performance runs serve separate
purposes and should be reported as such.

For Linux movement timing without GDB, add `--route` and use complete 480-frame
cycles after warmup, for example `--frames 600 --warmup 120 --route`. The helper
resolves the tested executable's `DarkInputKey` symbol and preloads a small
`vkQueueSubmit` wrapper. On the render thread after successful world submissions,
it sends balanced W/S and left/right key presses into the ordinary native input
queue. Native simulation handles motion and collisions; it does not teleport or
replace the camera. It requires observed horizontal camera displacement and
turning, complete trace coverage and the expected input count. The final
`auto_stop` frame may be recorded without presentation. Temporary saves and the
preload exist only inside the test process. Do not rebuild the tested executable
while the helper runs.

This is a local scripted route near the mission start, not full-mission coverage.
Its frame-timed input means speed, simulation and collisions affect the precise
trajectory. Compare GPU phases as well as FPS; do not treat different routes as
an isolated before/after performance experiment. Longer runs can use
`--frames 2040 --warmup 120 --route` (four cycles).

`native_shadow_camera.py` performs a render-camera-only yaw sweep under GDB.
It checks modern rendering and clean shutdown, using a private save directory;
GDB overhead prevents frame-rate claims. Other retained diagnostics cover model
reload, copied texture palettes, cell-light lists, material invalidation and
native water. Their earlier shadow-map performance results are obsolete; rerun
relevant checks before claiming coverage of the SDK path.

`native_water.py` uses a fixed Thief 2 miss2 water location; those coordinates do
not reach water in Gold miss2. It tracks bound water textures through deferred
vertex flushes, verifies water/late draws remain unsuppressed, and uses private
temporary saves. Pass `--log /tmp/native-water.log` to preserve its native log.
