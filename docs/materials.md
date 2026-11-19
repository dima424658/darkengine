# Texture material files

The raster Vulkan renderer loads NewDark-style `.mtl` sidecars from the resource
system. A material has the base image's name and canonical folder, for example
`fam/core/COBL2.dds` and `fam/core/COBL2.mtl`. Loose files and archives use the
normal resource search paths. A material in a lower-priority resource path cannot
change a higher-priority image. Missing materials retain existing rendering.

For animated textures, put the material on the root image. Its settings are
inherited by the numbered frames and terrain mipmaps.

```text
// Keep a 1024×1024 replacement at the original 64×64 terrain scale.
terrain_scale 64

// Optional: this replacement contains two original tiles in each direction.
tile_factor 2

// Optional unlit additive illumination, independent of the base lightmap.
illum_map 0.5 fam/illum_map/COBL2_glow
```

## Supported settings

| Parameter | Behavior |
| --- | --- |
| `include filename` | Textual include from the same canonical folder; nesting is limited to 16, with cycle detection. |
| `.ifdef`, `.ifndef`, `.else`, `.endif` | Nested load-time conditions. `DX6` is false for Vulkan; `$name` tests an existing nonzero config variable. `0` disables a block. |
| `terrain_scale width [height]` | Virtual terrain dimensions, independent of image storage. Dimensions must be powers of two, at most the original image size. One dimension makes a square. |
| `tile_factor u [v]` | A positive factor divides UVs; a negative factor multiplies by its absolute value. Applies to terrain and OBJ textures, excluding textures under `mesh`. |
| `ani_rate milliseconds` | Positive frame interval. The mission animation property takes precedence. |
| `ani_frames count` | Limits discovery of numbered frames, including the root. Zero retains automatic discovery. The engine frame limit still applies. |
| `ani_mode WRAP/REVERSE/PINGPONG` | Space-separated animation mode flags. |
| `force_ani_settings [0/1]` | Material rate/mode override the mission animation property. |
| `force_opaque [0/1]` | Suppresses texture alpha/keying when enabled. |
| `force_full_alpha [0/1]` | Uses continuous alpha blending instead of alpha testing. |
| `force_alpha_key [0/1]` | Thresholds texture alpha and enables alpha testing. |
| `force_local_pal` | Uses the image's palette rather than the terrain-family palette. |
| `no_mipmap [0/1]` | Disables terrain CPU mip generation. |
| `uv_clamp [0/1]` | Overrides base texture addressing on object textures; terrain continues to wrap. |
| `illum_map intensity texture` | One static 2D additive illumination layer on terrain/OBJ textures, using the base surface's UVs and vertex alpha. Texture references are resource-root relative. |

Comments use `#` or `//`; texture/include names can be quoted. Includes are
processed in order, so later settings override earlier settings. Malformed
values leave the preceding valid value or default intact. Unsupported directives
produce a material diagnostic through `mprintf`.

## Current limits

This is a base-material implementation, not the full NewDark effects system.
General `render_pass` blocks, `env_map`/cubemaps, location environment textures,
procedural color/alpha/UV functions, render-pass texture animations,
`render_material_only`, edge padding and legacy chroma-key conversion are not
implemented. Unsupported pass blocks are skipped as a unit so their properties
cannot accidentally change the base texture. A material illumination map is a
visual glow layer; it does not create a gameplay or world light.

Image decoding retains this branch's existing formats and precision. Materials
do not add PBR shading, GPU mip generation or the RTX renderer from other branches.
`force_local_pal` inheritance covers material settings; per-frame palette
reallocation for differing animation-frame palettes is not implemented.

## Parser and build

Material syntax is parsed by the reentrant Bison C++ grammar in
`libsrc/namedres/material.y`. The line lexer handles comments, quoted
paths, BOMs and line endings; the material driver applies validated settings and
shares condition/include state. Malformed quoted lines are skipped at the next
newline. Generated sources stay in the build directory. Bison 3.0 or newer is
required. On macOS, install it with `brew install bison`; CMake searches the
Homebrew prefix before the system Bison. An explicit `BISON_EXECUTABLE` override
is preserved.

The material test links the production `dark_namedres` library. It is an explicit
build target, excluded from the default build.

## Checks

```sh
cmake --build build-linux --target material_test thief-runner -j "$(nproc)"
ctest --test-dir build-linux -R '^materials$' --output-on-failure
```

The parser checks cover includes/cycles/override order, conditionals, rectangular
scales, positive/negative tiling, flags, animation, quoted paths and malformed
values. Set `DARK_RENDER_TRACE` to inspect native `material`, `material-uv` and
`material-illumination` rows. Use a private copy of writable game configuration
and saves for native testing.

The Vulkan canvas comparisons also exercise nested material-state save/restore
(texture, sampler, blending, alpha testing, depth, fog and viewport). On the local
Linux PC, the installed Gold pack loaded 146 materials and six illumination
textures in `miss2.mis`; the native run exited cleanly with synchronization
validation enabled. No DromEd or other-platform test was run.
