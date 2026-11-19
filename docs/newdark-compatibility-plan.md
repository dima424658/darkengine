# NewDark and TFix compatibility audit and implementation plan

Source baseline: `macos-support`, commit `82285ed`. Audit date: 2026-11-19.

## Objective and evidence limits

Support the installed Thief Gold/TFix content and documented NewDark behavior,
prioritizing gameplay before editor additions. Complete the compatibility matrix
before implementing the backlog below. This document records the source-level
feature-group audit; it is not an exhaustive per-method parity certification.

References are the installed `TFix_readme.txt`, `DOC` specifications and `Docs`
editor tutorials under `/home/dmitry/.local/share/Steam/steamapps/common/thief_gold`.
The installed versions define the compatibility target; later upstream features
are not implicitly included.

A symbol, registered relation, decoded property, opaque byte preservation or
successful mission load does not prove functional support. Classify each detailed
requirement as verified, implemented but unverified, partial, missing,
intentionally replaced or not applicable. Preserve uncertainty explicitly.

Only Linux game tests are authorized for this work. Do not run DromEd or test
other platforms. Use private writable configuration and save directories, and
leave retail assets unchanged. Build with `-j "$(nproc)"`. Preserve existing
untracked third-party directories. Use readable formatting with separate statements.

## Documentation inventory

| Reference | Coverage to record in the detailed matrix |
| --- | --- |
| TFix readme | Asset packs, scripts, gamesys changes, mission fixes, FM fixes and packaging requirements |
| `DOC/modders_notes.txt` | Formats, limits, properties, links, mission parameters, runtime and editor extensions |
| `DOC/dbmod-sample.dml` | DML syntax, discovery/application, fingerprints, conditions and modifications |
| `DOC/material-format.txt` | Base material settings, includes, animation, passes, environment maps and procedural effects |
| `DOC/new_config_vars.txt`, `cam_ext.cfg`, `cam_mod.ini` | Settings, defaults, precedence and equivalent platform controls |
| `DOC/new_commands.txt`, `dromed.cfg` | Command registration and actual behavior |
| `DOC/script_services.txt`, `script/` | Native services, signatures, messages, callbacks and serialization |
| `DOC/squirrel_script/` | Script language integration, messages and service adapters |
| `DOC/subtitles.txt`, `subtitles-sample.sub` | Discovery, languages, timing, presentation and controls |
| `DOC/motion_schema_syntax.txt` | Schema parser, actor declarations, tags and motion flags |
| `DOC/fm_selector_api.txt`, `fm.cfg`, `FMSel.pdf` | Content selection, exclusions and external selector contract |
| `DOC/al_reverb.ini`, keyboard definitions | Audio preset compatibility and keyboard behavior |
| `DOC/sw_cc/` | Colour controls and custom Direct3D effect overrides |
| `Docs/` menus, manuals and tutorials | Original authoring workflows, objectives, weather, sunlight and newer editor operations |

Consolidate duplicate RTF/ODT/DOC editions. Treat images as explanatory references,
archives as samples and licenses as distribution constraints, rather than engine
features. Extraction of old Word text was limited; it does not establish that
every tutorial instruction has been verified against the editor.

## Implemented and partial systems

| Feature group | Status and evidence | Remaining work |
| --- | --- | --- |
| Resource archives and mod paths | Implemented baseline: `src/framewrk/contentctx.cpp` constructs content paths | Verify full precedence, exclusions and language behavior |
| Mission/gamesys compatibility | Partial: `src/framewrk/dbfile.cpp` has version handling and preservation | Distinguish runtime codecs from retained opaque records |
| Base materials | Implemented subset: `libsrc/namedres/material.cpp`, `material_parser.hpp` | See material gaps below and `docs/materials.md` |
| DDS | Partial: `libsrc/namedres/dds.cpp` decodes DXT1/3/5 and supported RGB formats | Broader formats, cubemaps, mip chains and compressed GPU storage |
| Weather | Baseline exists in `src/render/weather.cpp`, including script access | Verify documented extensions and persistence |
| Sunlight | Baseline baked lighting: `src/editor/ged_csg.cpp` calls portal sunlight calculation | Audit extended object-shadow behavior; defer editor execution |
| OSM | Partial PE32 emulation and service adapters in `libsrc/script/osm_emu/` | Per-instruction/import/service/message/serialization coverage |
| Squirrel | Partial VM and service integration in `libsrc/script/squirrel/` | Method-by-method behavior and lifecycle verification |
| Listener | Squirrel synchronous link subscription works in source | Async link subscriptions and other listener families return `E_NOTIMPL` |
| DarkOverlay | Squirrel bitmap/drawing/overlay adapters exist | Native OSM parity, callback behavior and complete documented signatures |
| Engine service | Configuration, canvas, global fog, weather and raycast adapters exist | Explicit stubs and semantic differences listed below |
| Audio | `libsrc/sound/openal.cpp` implements spatial sound, reverb and occlusion | Preset/configuration compatibility and transitions |
| Movies | FFmpeg playback and Vulkan presentation exist | Documented crop, scale, subtitle and setting behavior |
| Editor | Shared UI, file dialogs, brushes, properties, links and database workflows exist | NewDark authoring additions; runtime verification deferred |

Previous Linux checks for commit `82285ed` include parser tests, twelve GPU canvas
comparisons exercising draw-state restoration, and a private-config Gold miss2
run loading 146 materials and six illumination textures with clean shutdown and
no Vulkan validation errors. These checks do not establish campaign or FM parity.

## Confirmed gaps

| Gap | Source evidence / consequence |
| --- | --- |
| DML patch loading | No parser/application pipeline found; TFix and FM mission/gamesys fixes cannot be assumed applied |
| General material passes | `render_pass` blocks are skipped; custom passes and procedural RGB/alpha/UV behavior are absent |
| Environment maps | Material cubemaps/location maps absent; `Engine.SetEnvMapZone` is a no-op |
| Fog zones | Engine service retains script-side zone values without connecting them to zone-aware rendering |
| Tag-block script access | `SetTagBlock` returns `E_NOTIMPL`; `GetTagBlock` returns an empty value |
| Exact object raycasting | Excluded returned hits fall back to terrain, potentially missing another object; documented flag semantics incomplete |
| Subtitles | No `.SUB` discovery, sound/movie timing or presentation implementation found |
| TTF/OTF fonts | Font resource loader accepts `.fon`/`.bin`; no documented scalable-font pipeline found |
| Extended DDS | No DX10/BC7 decoder, cubemap representation or retained mip-chain/compressed upload path found on this branch |
| Material pixel processing | Edge padding, documented legacy chroma conversion and differing per-frame local palettes incomplete |
| External FM selector | No documented `SelectFM` invocation found; configured FM selection is a separate capability |
| Custom colour effects | No `shaders/cc.fx` override loader; basic Vulkan gamma is not equivalent |
| Command/settings compatibility | Many documented names absent; equivalent modern controls must be mapped before classifying behavior as missing |

Detailed evidence for script stubs and raycasting is in `src/script/engscrpt.cpp`;
Listener restrictions are in `CallListener` in `squirrelhost.cpp`.
`DetailAttachement` registration in `src/object/linkman.cpp` preserves a relation,
but does not by itself establish documented attachment behavior.

## Unverified compatibility groups

Do not classify these groups wholesale as absent or complete:

- Extended renderer properties: order, alpha thresholds, overlays, distance alpha,
  creature scale, model LOD, room options, ambient zones and fog-zone metadata.
- Mantle, ropes, doors, player lighting, AI trace behavior and other physics changes.
- Script messages, native versus emulated ABI, timers, callbacks and save data.
- Motion-schema grammar, actor counts and optional motion flags.
- Object, texture, frame, brush and geometry limits.
- Language resources, configuration precedence and save/reload persistence.
- New editor commands and legacy tutorial workflows.

Token searches are candidate discovery only: aliases, generated parsers and
platform replacements require semantic inspection. Do not publish raw search
match counts as implemented-feature counts.

## TFix-specific interpretation

Enhanced textures/models/skies/water are supplied assets, not standalone engine
features. Interactive candles, carry-body behavior and objective notifications
combine scripts and gamesys content. Mission overhauls and FM fixups depend on
patch application. Inspect the installed dependencies and verify observable
behavior before declaring a TFix component compatible.

Windows installers, redistributables and DMM are packaging concerns. Do not
recreate them as Linux engine features. The removed native menu bars are an
intentional project decision; audit underlying editor operations independently.

## Implementation sequence

### 0. Finish the detailed parity matrix

Before production changes, create `docs/newdark-compatibility-matrix.md`. Assign
stable IDs to individual documented requirements, with document section, expected
behavior, code entry point, status, test evidence and implementation dependency.
Split native, emulated OSM and Squirrel coverage. Split loading, runtime effect
and persistence for database features. Every reference group above must have
coverage; unsupported and unverified entries remain explicit.

Use this matrix to finalize the backlog. Do not implement speculative gaps just
because a familiar NewDark command name is absent.

### 1. DML and patch lifecycle

Implement the documented parser and ordered discovery for mission/gamesys patches.
Apply fingerprints and conditions before modifications. Connect application to
the documented load lifecycle, object remapping and save/reload rules. Include
property, object, link and parameter changes used by installed TFix patches.
Do not silently apply a partially understood patch or modify retail databases.

Acceptance: installed patch inventory is classified; valid patches produce
expected changes; fingerprint mismatches and malformed input cannot affect an
unrelated mission; loading saves does not duplicate patch-created state.

### 2. Script behavior and persistence

Complete Engine tag-block access, accurate raycast exclusions/flags and Listener
families. Connect environment/fog setters only when their underlying runtime
systems exist. Verify native service availability independently of Squirrel
wrappers. Preserve documented ABI signatures and saved script state.

Acceptance: deterministic fixtures cover results, callbacks, unsubscribe/destruction,
async delivery, errors and save/reload across all supported script paths.

### 3. Mission rendering metadata

Decode and apply fog/ambient zones and the renderer-property extensions established
by the matrix. Connect room/cell/object state to rendering. Keep gameplay light
and visibility behavior distinct from visual effects.

Acceptance: camera transitions, object changes and save/reload retain the expected
appearance; preserved-but-unused records are never reported as functional support.

### 4. Material and image compatibility

Extend the reusable parsed material representation for documented passes, blending,
UV sources/transforms, functions, animation and environment maps. Add cubemap/mip
resources and complete required DDS support. Finish palette and pixel-processing
behavior. Keep missing sidecars/default materials compatible and cache resource
and render data with explicit unload/reload ownership.

Acceptance: GPU readback fixtures verify blending, alpha, UVs, animations,
cubemap orientation, mip selection, defaults and state restoration. Native checks
use unchanged installed packs; avoid synthetic replacement-floor overlays.

### 5. Subtitles and text

Implement documented `.SUB` discovery and language precedence, sound/movie cue
lifecycle and controls. Add scalable fonts and UTF-8 rendering without confusing
text support with filesystem path encoding.

Acceptance: overlapping sounds, stopped sources, movie seeks/skips, language
fallback, scaling, Unicode text and mission teardown behave correctly.

### 6. Configuration, audio and content selection

Map documented settings to existing equivalents where semantics match. Implement
remaining required controls and diagnose unsupported settings. Verify reverb
presets, room transitions, FM exclusions and content selection. Treat external
Windows DLL selector support as a separate portability requirement, not a
prerequisite for configured Linux FM loading.

Acceptance: test precedence/defaults, content switches and unavailable-device or
unsupported-option behavior without altering retail configurations.

### 7. Editor extensions

Implement missing authoring operations identified in the matrix, with command
entry points independent of native menus. Preserve unknown database content and
source brushes. Include DML authoring/inspection and material diagnostics where
required. Runtime DromEd tests remain deferred until explicitly authorized.

## Validation and completion gates

- Build the Linux game with all available CPU cores; do not run editor targets.
- Reuse parser, OSM, platform and Vulkan fixtures; add behavior-level regressions
  for actual gaps rather than tests that duplicate implementation details.
- Run Vulkan synchronization validation and GPU readbacks for rendering work.
- Test original campaign scenarios and representative FM scenarios, including
  scripts, objectives, patch effects and save/reload. Mission startup alone is
  insufficient evidence.
- Keep saves, captures and writable configuration outside retail directories.
- Record exact branch/commit, fixture, observed behavior and diagnostic logs.
- Upgrade matrix rows to verified only after their acceptance checks pass.
- Report deferred editor/platform coverage and remaining compatibility limits.
- Split implementation into reviewable subsystem commits; do not describe the
  branch as fully NewDark-compatible while partial/missing runtime entries remain.

## Implementation progress

- `7d3ed3f`: ignore Python cache directories.
- DML syntax foundation: standalone C++ parser with ordered/nested statements,
  quoted arguments, source locations, comments and pre/post phase annotations.
  Focused tests and all 88 installed TFix DML files pass syntax parsing.
- The parser is not connected to the game. No runtime compatibility status is
  upgraded by syntax success. Fingerprints, semantic validation, discovery,
  application and persistence remain pending, as does semantic matrix review.
- Matrix review: corrected material-heading extraction, added content-derived
  stable IDs, and classified material directives, Engine stubs/approximations
  and Listener restrictions from implementation evidence. Remaining rows still
  require semantic review; no runtime feature is considered complete from this.
- Native service review: documented AnimTexture location/global/single-surface
  updates, Property indexed access, Quest binary/enumeration methods and Object
  transforms/archetype/spatial methods are absent from the inspected native
  service implementations. Squirrel has separate helpers for some Object/Quest
  methods; the matrix records this distinction rather than claiming universal
  absence or native support. Runtime script ABI parity remains unverified.
- Expanded DML review into individual discovery, ordering, fingerprint, command,
  remapping and load-lifecycle requirements. Only syntax parsing and phase
  annotations exist; every runtime DML requirement remains classified missing.
- Expanded Squirrel service/message references into individual requirements.
  API version guards, signatures and semantics remain unverified; this expansion
  does not claim their implementation. Matrix now contains 1,362 inventory rows.
- Overlay/content review found additional semantic gaps: Squirrel screen
  projection and screen-bounds queries always fail, style-colour selection uses
  a constant, and native Version.GetModPath returns the complete resource path.
  These are now explicit partial/missing matrix entries. Vulkan `vsync` and SDL
  fullscreen controls are recorded as alternatives, not support for different
  NewDark configuration names/value semantics; Direct3D provider selection is
  classified as intentionally replaced.
- Added an offline read-only fingerprint evaluator with documented position
  thresholds, positive/negative precedence, quest masks/comparisons and named
  rules. Missing adapters and unsupported MOD version comparisons fail explicitly.
  Both C++ fixtures pass; all 88 installed files pass fingerprint grammar
  validation. Database adapters, discovery and application remain pending.
- Added callback-backed offline DML discovery manifests. Tests reproduce both
  documented mission/gamesys ordering examples, plus FM selection, save/disable
  skips, case handling and duplicate layers. All three C++ tests pass.
  Engine path/storage adapters and runtime patch execution remain pending.
- Added offline command-shape preflight with typed command families/operations,
  creation-time ObjProp compilation, directive-order checks and rejection of
  malformed/unsupported structural forms. All four C++ fixtures pass and all
  88 installed patches pass structural command checks. Field/schema validation,
  object resolution, database scope and execution remain pending.
- Added offline field-edit staging: quoted values and duplicate-name occurrence
  indices are preserved; selected-field bounds and encoder byte counts are
  validated; partial edits never alter the original record. All five standalone
  C++ tests pass. Strict native schema encoders, pointer-owning properties and
  runtime publication remain pending; legacy struct parsing alone is too lenient
  to establish valid DML values.
- Added strict staging encoders backed by native sStructDesc/sFieldDesc metadata
  for common scalar, enum/bit, fixed-string and floating vector types. Pointer
  types and other unsupported representations cannot be written. All six
  standalone tests pass normally and with address/undefined-behavior sanitizers.
  Live property-store ownership, deep-copy strings, remaining field types and
  runtime publication are still pending.
- Extended native field staging to fixed-point scalars/vectors, wrapped angles,
  native angle-vector order and integer points. Kept RGBA and pointer layouts
  unsupported rather than using unsafe legacy conversions. The Linux game target
  now compiles the DML modules; all six standalone tests pass normally and under
  ASan/UBSan. No database load hook or runtime patch executor is enabled yet.
- Added opt-in `dbmod_check` native resource diagnostics after successful database
  loads. The installed RESBASE gamesys patch passes (82 statements/80 commands);
  unknown-command and missing-resource runs produce expected diagnostics. All
  three Linux mission runs exit cleanly with synchronization validation. This
  checks IResMan/IStore ownership and preflight integration only: no patch is
  matched against mission state or executed, and diagnostics do not change the
  mission load result.
- Added live read-only fingerprint adapters for database gamesys identity, object
  positions/existence/names/archetypes, direct property presence, mission-only
  quest snapshots and config definitions. `dbmod_check_match=1` opts into native
  matching; script/mod adapters still fail explicitly when needed. Native
  positive/negative CFG tests and installed RESBASE matching exit cleanly under
  synchronization validation. Other adapters are implemented but not individually
  runtime-verified. No patch mutation is enabled.
- Added opt-in native property-schema diagnostics (`dbmod_check_schema=1`).
  Installed RESBASE inspection yields 63 valid property edits, five unsupported
  cases and one value rejected by current encoding. Confirmed live gaps include
  AI_Efficiency.Flags and unregistered CretHTrack/DoorStaticLight; matrix updated.
  DesignNote ownership/schema handling and a trailing-quote bit-list discrepancy
  remain unresolved. A private valid/invalid/unknown-property fixture and the
  installed-patch run exit cleanly under synchronization validation. No targets
  are resolved or modified. Field encoding errors now retain source lines;
  all six standalone tests pass normally and with ASan/UBSan.
- Added owning string-property value parsing and live IStringProperty schema
  validation. Both RESBASE DesignNote edits now validate without pointer writes;
  the report is 65 valid, three unsupported and one rejected bit-list value.
  Seven standalone tests pass normally/under sanitizers; installed and private
  native string fixtures exit cleanly with synchronization validation. Live
  property assignment and patch-wide execution/rollback remain pending.
- Added a native main-thread module-presence query backed by cScriptMan's actual
  loaded-module table, without changing guest ABI vtables. SCR fingerprints now
  use it and distinguish query failure from absence. Native tests used the
  observed MISS02.OSM module: mixed-case/suffix lookup accepts, absent-module
  lookup rejects and a matching negative block vetoes. All runs exit cleanly
  under synchronization validation; Linux build and seven C++ tests pass.
  Script directives, mod/version queries and patch execution remain pending.
