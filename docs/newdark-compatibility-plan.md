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
| DML patch loading | Parser, offline discovery/evaluation and opt-in native diagnostics exist; automatic discovery/application and persistence are unfinished |
| General material passes | `render_pass` blocks are skipped; custom passes and procedural RGB/alpha/UV behavior are absent |
| Environment maps | Material cubemaps/location maps absent; `Engine.SetEnvMapZone` is a no-op |
| Fog zones | Engine service retains script-side zone values without connecting them to zone-aware rendering; no mission fog-zone variable |
| Ambient zones | WREXT cell bytes are retained, but object lighting reads global ambient and the zone accessor has no callers |
| Mission view-distance minimum | Global near/far controls exist; the mission Required View Dist setting is not consumed |
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

- Added a read-only object-reference resolver with strict signed numeric IDs,
  quoted names (including numeric names), existence queries and per-reference
  concrete/abstract scope checks. An explicit optional loader remap callback
  applies only to numeric IDs; name lookups already return live IDs. Standalone
  tests cover remapping, scope rejection, malformed IDs, missing objects and
  unavailable queries. This is preparatory code, not an engine execution hook:
  remap lifetime, command-specific reference roles, runtime mutation and full
  semantic matrix review remain unfinished.

- `dbmod_check_objects=1` now inspects standalone ObjProp targets against the
  loaded native object system. With `dbmod_check_gamesys=1`, targets must be
  abstract; otherwise they must be concrete. This optional diagnostic reports
  resolved/unresolved live snapshots independently of syntax/schema results;
  it does not simulate earlier creations, renames or destruction, inspect
  creation-time property targets, or use the expired loader remapping table.
  A private miss2 Linux fixture resolved both -1 and "Object" to archetype -1,
  rejected missing numeric/named targets, and reported concrete object 1 as
  outside gamesys scope. The run exited normally with Vulkan validation enabled
  and no validation errors. No object/property writes were added.
  The same fixture in mission mode resolved concrete object 1, rejected both
  abstract references, and again exited normally without validation errors.

- Source-level rendering metadata review checked DOC/modders_notes.txt against
  weather.h/cpp, missrend.h/cpp, objlight.c, render.cpp, portal.c/portdraw.c and
  sim/wrloop.cpp. Five more matrix rows now distinguish retained metadata,
  legacy behavior and missing runtime behavior. In particular, objlight.c
  copies global ambient directly, and the WREXT ambient-zone accessor has no
  consumers. RENDPARAMS preserves extended source bytes for round-tripping;
  that does not implement the additional fields. Global zfar defaults to 256
  and has manual controls, but no mission minimum field is consumed. These
  are source findings, not new visual acceptance results; editor testing
  remains deferred. The complete semantic audit is still unfinished.
- Refreshed stale DML matrix evidence to include the native fingerprint,
  property-schema and object diagnostics, staged duplicate-field selection,
  and the explicit object resolver. These remain preparation for an executor,
  not proof that installed patches are applied. Twelve audit-tool tests pass.

- Resource-loading audit expanded subtitle discovery, sound/movie lifecycle,
  SUB1 grammar and all documented subtitle settings into individual matrix
  requirements. Added the previously omitted textual font-sample.fon spec
  (FON1/resource/antialias/height/width/dpi/dynamic), plus individual external
  selector ABI/lifecycle requirements. Inventory now contains 1,406 entries;
  this expansion does not mean the entire semantic audit is finished.
  Source review found no subtitle pipeline or FON1/vector font rasterization.
  fonrstyp.cpp binds legacy font bytes as raw resources; movie.cpp delegates
  playback without subtitle coordination. contentctx.cpp provides configured
  FM selection but does not invoke the documented SelectFM plugin API.
  gen_bind.cpp reads numeric kb_country and kbcntry.c uses compiled layouts;
  the documented custom INI table loader is missing. Existing compiled keyboard
  layouts and configured FM selection remain separate supported capabilities.
  Fourteen audit-tool tests pass. No game behavior changed in this audit commit;
  runtime/visual acceptance for these missing systems remains outstanding.

- Audio source audit distinguishes existing OpenAL EFX reverb and per-source
  wet mixes from missing NewDark controls. openal.cpp Init opens the default
  device and uses automatic layout selection (with the existing Linux stereo
  retry); snd_oal_device/snd_speaker_config are not consumed. UpdateEffect uses
  compiled six-value presets and fixed reflection parameters, not al_reverb.ini.
  The backend is EFX-only, so disable_oal_eax_reverb has no EAX path to suppress.
  auxsnd.cpp already decodes OGG buffers to PCM but does not implement the
  documented snd_decompress_on_load modes. appsfx.cpp selects UnderWater only
  when room acoustics are unset; force_underwater_reverb and configurable
  submerged_sound_occlusion remain missing. These seven source-reviewed rows
  are not listening-test or complete audio parity claims. Fifteen audit tests
  pass; no audio behavior changed in this review.

- Reviewed fourteen native Engine service methods against DOC/script/Engine.txt
  and traced CallEngine forwarding for the matching Squirrel entries. Typed
  config reads, owned raw strings, binder echo queries, Datapath searches,
  global fog, live weather parameter access and terrain raycast implementations
  are now marked source-reviewed rather than merely token-matched. Runtime ABI,
  version guards, lifecycle and visual acceptance are still explicitly open.
  GetCanvasSize reads configured game_screen_size rather than the actual main
  2D canvas, and GetAspectRatio inherits that discrepancy; both are partial.
  IsRunningDX6 intentionally returns false for the SDL/Vulkan renderer.
  Existing tag-block/environment-map stubs and fog-zone/raycast limitations
  retain their missing/partial classifications. Sixteen audit-tool tests pass;
  this review does not establish full script-service compatibility.

- Property service source audit reviewed Get, both native Set overloads,
  SetLocal, Add, Remove, CopyFrom and Possessed plus Squirrel SetSimple/forwarding.
  Reads follow property donors; Possessed calls IsRelevant (inheritance included),
  while SetLocal bypasses the SetField owner-routing message. NewDark's
  PossessedSimple/indexed API additions have neither native implementations nor
  CallProperty branches. API-version gates and THIEF1-dependent native vtable
  ordering remain unverified, and the inventory still groups Set overloads.
  Confirmed source cleanup defects for the script-correctness phase: propman.cpp
  GetPropertyNamed AddRefs its result, but several Property methods do not
  release it; get_prop_donor does not release its queried ITrait; GetPropField
  and SetPropField have early returns with store/ops references still held.
  No runtime leak magnitude or ABI correctness is inferred from these findings.
  Seventeen audit tests pass. Script behavior changes remain pending.

- Quest source audit traced quesscpt.cpp, engfeat/quest.cpp and CallQuest.
  Named subscriptions exist, but "*" is a literal node name, not the documented
  all-qvar subscription. Native wrappers also discard subscription HRESULTs and
  always report true. Set updates existing qvars without changing their scope;
  a missing variable is created with the requested scope. Basic integer methods
  are source-reviewed with runtime/network verification still open.
  Squirrel GetAllVars builds a fresh scoped table. Binary helpers instead retain
  HSQOBJECT references in a VM-global vector, without campaign serialization or
  the documented native size limit. BinSetTable/BinGetTable use an extra string
  key/value argument convention incompatible with documented name/table calls.
  GetFirst/GetNext are missing in both adapters. These findings supersede any
  assumption that the presence of binary helpers establishes campaign storage.
  Eighteen audit tests pass; no gameplay/script mutation was introduced here.

- Object service review traced lifecycle, naming, transience, trait queries,
  position/facing, teleport, batch metaproperty edits and visibility against
  DOC/script/Object.txt and CallObject. Existing methods now have source-level
  evidence with runtime/ABI gates still open. Batch edits report parsed target
  counts rather than proven successful mutations. FindClosestObjectNamed leaks
  its trait query and dereferences candidate positions without a null check.
  Squirrel's Archetype and object/world transforms are separate implementations,
  not native ABI additions. CalcRelTransform ignores rel_type and sub-index,
  always returns true and subtracts Euler angles instead of composing relative
  rotations; joint/vhot/subobject/subphysics support is absent. SetArchetype,
  FindDescendentsSpatial, FindScriptDonor and AnyExist/GetAll/GetOne are missing
  from both native service and CallObject. Nineteen audit-tool tests pass.
  No object lifecycle or transform behavior changed in this audit commit.

- Link/LinkTools source review traced legacy operations, inherited queries,
  native broadcast overloads and CallLink/CallLinkTools/PushLinkSet. Existing
  operations now carry source evidence with runtime/ABI and iterable semantics
  still open. CreateMany ignores per-link failures; DestroyMany removes while
  iterating a query and still needs behavioral verification. Data broadcasts
  filter on the first field for multi-field schemas. BroadcastOnAllLinksMany
  is absent from both adapters. LinkTools relies on assertion-based invalid-ID
  and field checks; LinkGet reports S_OK after Verify, and LinkSetData allocates
  uninitialized data for absent records and leaks it if no schema exists.
  These are recorded for script/link correctness work, not reported as fixed.
  Twenty audit-tool tests pass. No runtime link behavior changed.

- Version/AnimTexture source audit reviewed all documented methods and Squirrel
  forwarding. Version reports hardcoded 1.28 despite absent 1.28 APIs; this is
  host identity behavior, not evidence of full NewDark capability. GetMap reads
  quickstart/file config and GetGamsys reads gamesys config instead of the
  loader's identity. IsEditor cannot distinguish editor game mode (editor tests
  remain deferred). FM path helpers have implementations; GetModPath includes
  FM/base resource entries and remains partial. Legacy ChangeTexture resolves
  names and calls DynTexture, but unchecked parser copies and missing-position
  dereferences remain correctness work. Loc/global/single-polygon additions
  are absent from native and Squirrel adapters. Twenty-one audit tests pass;
  no version reporting or texture behavior changed in this review.

- AI service source audit traced aiscrpt.cpp and CallAI against native/Squirrel
  docs. ClearGoals is an explicit no-op; SetScriptFlags controls speech only,
  ignoring motion flags. Stun/IsStunned/UnStun, Freeze/IsFrozen/UnFreeze and
  SetFaceVisage are absent from both AI adapters. Existing ShockAI capabilities
  would not establish exposure through the documented AI service. Movement and
  frobbing delegate to cAIBasicScript with speed/priority/callback data; alerts,
  owner-routed signals and conversations have implementations but need action,
  callback, state and runtime verification. No AI behavior changed. The existing
  twenty-one audit-tool tests still pass; full semantic review remains open.

- Physics source review checked phscrpt.h/cpp and CallPhysics against native
  and Squirrel docs. Rope/climbing and velocity controls are present, alongside
  legacy launch, gravity, model, subscription and motion-offset operations.
  These now have source evidence instead of name matches; runtime output,
  argument conversion and game-dependent ABI ordering remain unverified.
  ControlCurrentLocation/Rotation dereference ObjPosGet after only a nonzero-ID
  check; ControlCurrentPosition invokes both. Subscription methods return literal
  TRUE as HRESULT; GetVelocity and several setters report S_OK without surfacing
  underlying failure. These are acceptance/correctness follow-ups, not a claim
  that all valid-input physics behavior is missing. Existing twenty-one audit
  tests pass; no physics behavior changed in this audit commit.

- Reviewed sixteen Squirrel Door/Locked/Key/Damage/Container requirements by
  tracing adapters into drscrpt, lockscpt, keysys, dmgscrpt and contscrs. Door
  state checks and room acoustic blocking, computed lock-count queries and
  mode-specific key matching exist. Door HRESULT methods return boolean values,
  including zero for invalid door; damage wrappers always return S_OK even for
  zero victims. Container wrappers deliberately reverse object/container or
  source/target order to the native API. Source behavior is now recorded;
  return interpretation, save/lifecycle and action outcomes remain runtime
  acceptance work. No new native docs were inferred where only Squirrel docs
  enumerate these services. Existing twenty-one audit tests pass; gameplay
  behavior is unchanged in this review.

- Sound source audit reviewed raw/schema/environment/voiceover playback,
  halting and preload in sndscrpt.cpp and CallSound. Active callback records,
  SoundDone/SchemaDone messages and SndScript save/load/restart paths exist;
  callback timing and restored playback are still runtime acceptance gates.
  Failure without active audio posts asynchronous completion for non-loop raw
  sounds and schemas; this is not evidence of audible playback. The voiceover
  wrapper does not use its callback-object argument. SongChange/SongSetTheme
  are absent from native Sound and Squirrel. Squirrel *Net names are neither
  installed nor dispatched, despite native playback accepting eSoundNetwork.
  THIEF1 Legacy entry-point layout differs from the restored Thief II host and
  needs guest ABI verification, not a blanket claim of absent Gold audio.
  Existing twenty-one audit tests pass; no sound behavior changed here.

- Camera/DarkUI review traced live camera pose, attachment/return and matrix
  transforms plus Squirrel forwarding. LockMovement is an explicit no-op that
  ignores its argument and returns S_OK. CameraToWorld/WorldToCamera have actual
  orientation-matrix implementations; input locking is a separate missing
  behavior. DarkUI basic text, book mode, inventory selection and localized
  binding queries exist; TextMessageFormatted has no native/adapter method for
  custom fonts, margins or alignment. Source-reviewed status does not close
  visual, state-restoration, callback or ABI gates. Existing twenty-one audit
  tests pass; no camera/input/UI behavior changed in this review.

- Networking/CD source audit checked netscrpt.cpp, CallNetworking and CallCD.
  Current Linux compile commands define NEW_NETWORK_ENABLED; native network
  methods cannot be classified as absent simply because Squirrel uses local
  fallback answers. Native broadcast/ownership/iteration/query paths exist but
  multiplayer behavior has not been tested. Squirrel hardcodes local state for
  multiplayer/networking/player iteration/suspend/ownership queries, rather than
  forwarding those calls. New content-proxy, host/player-number/name additions
  are missing from both adapters. CD SetBGM/SetTrack ignore arguments and return
  S_OK; Sound schema playback is separate. Existing twenty-one audit tests pass.
  No network session or CD behavior was tested or changed in this review.
