# macos-support branch review

Reviewed the branch against fetched `origin/ai-rework` (`3d84942`), starting at
`f1aa1a8`: 82 commits and 1,009 changed files. Most deleted lines belong to the
retired Windows/platform implementations, assembly, and project files. The
Vulkan-only renderer, shared SDL platform library, shared editor UI, and removed
menu bar are intentional parts of this migration.

## Findings fixed

- A mid-scene texture-cache purge ended and restarted the renderer scene. Restart
  cleared color/depth, discarding already recorded world rendering. Texture
  eviction now flushes legacy geometry and releases textures without restarting
  the scene. The unused driver callback table and synchronization callback were
  removed.
- Canvas and movie draws changed scene textures, samplers, viewport, depth,
  blending, and fog without restoring them. The legacy state cache could then
  skip required setters. Both paths now use `DrawOverlay`, which saves/restores
  the renderer's intended state while preserving the actual command-state cache.
- The hardware lightmap cache retained pointers into freed world cells. Reloading
  a saved game crashed in `porthw_shutdown`. World teardown now clears the
  hardware cache while the cells are still alive, retaining an initialized cache
  for subsequent world loads/editor rebuilds.
- Movie audio EOF requested a deferred stop, but synchronous playback never ran
  the mixer update that completes it. Playback now updates the mixer while
  draining. Unavailable audio streams no longer leave playback waiting forever.
- `fix_make` changed negative fractional values and still performed undefined
  signed arithmetic. It now assembles the original 16.16 bit pattern with
  unsigned arithmetic, evaluating each argument once.
- Windows documentation now distinguishes the supplied VS 2026 preset (CMake
  4.2+) from a manual VS 2022 configuration.

## Renderer structure

`libsrc/lgd3d` retains the legacy engine-facing API and the Vulkan backend:

- `legacy_driver.cpp`: device enumeration, scene lifecycle, legacy state adapter.
- `legacy_primitives.c`: geometry conversion and batching.
- `legacy_textures.cpp`: palette conversion, texture uploads, texture binding.
- `legacy_texture_manager.c`: texture residency, aging, and eviction.
- `legacy_texture_driver.h`: private direct bridge between those components.

Removed the unused clipping implementation, obsolete alternate texture-manager
interface, unused multitexture aliases, duplicate white texture, and no-op
lightmap selector, dithering, antialiasing, shading, linear-filter and blend
switches. Texture ownership is private; the texture audit uses read-only bitmap
and residency queries. `RenderBackendBeginScene` now names the scene clear
boundary explicitly; the empty backend end-frame hook is gone.

## Linux verification

- Release game and DromEd builds.
- Exhaustive fixed-point construction test under UBSan.
- Platform I/O regression tool.
- Twelve GPU canvas comparisons: nearest exact; linear within one channel value.
- Fourteen renderer stress captures, including upload growth, texture replacement,
  and a new overlay-between-scene-draws state-restoration check.
- Thief Gold mission 2: forced texture eviction while rendering, short video with
  audio through EOF, inventory/menu transitions, and shutdown.
- Saved and reloaded a game in a private directory, then rendered 120 more frames.
  The quickstart test explicitly creates its missing mission-start checkpoint
  before saving.
- DromEd interactive resize and mission/gamesys export; mission reload/export
  preserves all 4,423 brush records byte for byte.
- Linux installation into a temporary prefix and installed game smoke check.

GPU/runtime checks used Khronos synchronization validation without reported
validation errors. Retail assets were read from private test setups; saves and
editor exports went to temporary directories. Windows, macOS, and Android were
reviewed statically only. These checks do not establish compatibility with every
mission, mod, or historical save format.
