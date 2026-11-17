# Vulkan canvas regression check

This standalone Linux check needs CMake, a C++17 compiler, SDL3 development
files, Vulkan headers/loader, and the built engine shaders. It tests retained
canvas updates, padding, palette-only changes, transparency changes, format
changes, clipped and partial rectangle updates, sparse clearing, removed HUD
pixels, and rejected inputs. The GPU fixtures compare the old CPU conversion
on the left against the packed-pixel shader on the right, with all 65,536 input
values for 15/16-bit formats. Twelve frames cover 8/15/16-bit canvases,
nearest/linear filtering, and opaque/transparent composition. Source rows have
padding and odd pixel widths. Each fixture also commits a one-pixel update
while changing another pixel outside the flush; the latter must remain
uncommitted, including when the pixels share a packed GPU texel.

From the repository root:

```sh
cmake --build build-linux --target thief2
cmake -S tools/vulkan_canvas_test -B /tmp/dark-canvas-test -DCMAKE_BUILD_TYPE=Release
cmake --build /tmp/dark-canvas-test -j4
/tmp/dark-canvas-test/vulkan_canvas_test --snapshot-only
mkdir -p /tmp/dark-canvas-fixtures
VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation:VK_LAYER_LUNARG_screenshot \
  VK_LAYER_VALIDATE_SYNC=1 VK_SCREENSHOT_FRAMES=all \
  VK_SCREENSHOT_DIR=/tmp/dark-canvas-fixtures \
  /tmp/dark-canvas-test/vulkan_canvas_test \
  build-linux/generated/vulkan/scene.vert.spv \
  build-linux/generated/vulkan/scene.frag.spv
python3 tools/vulkan_canvas_test/compare.py /tmp/dark-canvas-fixtures
```

Both Vulkan layers must be installed to capture the GPU fixtures. Nearest
sampling must match exactly. Linear filtering allows one channel value out of
255 for hardware/shader rounding differences. The capture comparison uses only
the Python standard library.

On the current Linux PC (i7-14700K, RTX 4070 Ti), Thief Gold Mission 2 at
3440x1440 improved from 39.1 FPS (25.6 ms) with CPU canvas conversion to
256.5 FPS (3.9 ms) with GPU decoding and retained snapshots. These are averages
over 360 frames after 60 warm-up frames, using `SlowFrame=0 min_frame_time=0
vsync=0`; MangoHud was enabled for the final run. This measures the initial
mission view, not every mission or camera position. The game, DromEd resize,
and the GPU fixtures passed Khronos synchronization validation on this PC.

Direct offscreen capture, avoiding duplicate page-flip capture, and sparse
overlay clearing subsequently raised the same view to 542.2 FPS (1.85 ms),
with a 95th-percentile frame time of 2.66 ms. The simulation clock advanced
664 ms over 664 ms of measured wall time. This used the same uncapped flags,
MangoHud, and 360-frame measurement window. Inventory changes, entering the
simulation menu, returning with Escape, and editor resize were also checked
under Vulkan synchronization validation.

## Upload and descriptor reuse stress check

Add `--stress` to render a three-stripe texture replacement check followed by
12 frames with 30,002 draws each. The first four workload frames warm up the
frame slots; the remaining eight are timed. The draws use a one-pixel viewport
to limit fill cost, change samplers/pipelines/constants, and exceed the initial
8 MiB vertex buffer. Frame-owned textures stream different colors with padded
source rows and change dimensions halfway through the run. The first reference
draw must survive vertex-buffer growth, and replacing a shared texture after
recording a draw must preserve each stripe's original color. A final frame draws an overlay
between scene draws without resetting the scene's texture, viewport, or depth
state; the final image must remain red. This detects overlay state leakage
and lost depth writes/comparison.

Use a separate capture directory because this workload produces 14 images:

```sh
mkdir -p /tmp/dark-renderer-stress
VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation:VK_LAYER_LUNARG_screenshot \
  VK_LAYER_VALIDATE_SYNC=1 VK_SCREENSHOT_FRAMES=all \
  VK_SCREENSHOT_DIR=/tmp/dark-renderer-stress \
  /tmp/dark-canvas-test/vulkan_canvas_test \
  build-linux/generated/vulkan/scene.vert.spv \
  build-linux/generated/vulkan/scene.frag.spv --stress
python3 tools/vulkan_canvas_test/compare_stress.py /tmp/dark-renderer-stress
```

For timing, run the same executable with `--stress` without the validation and
screenshot environment variables. On the current Linux PC, two runs against
the pre-reuse renderer averaged 14.2–14.4 ms per workload frame; the optimized
renderer averaged 2.6–2.7 ms. All 14 captured images matched expected colors
exactly under synchronization validation. This synthetic workload emphasizes
submission overhead: the stationary Thief Gold view remained approximately
533–534 FPS, with no meaningful average-FPS change from these optimizations.
