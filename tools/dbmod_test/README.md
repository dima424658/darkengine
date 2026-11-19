# DML syntax foundation

```sh
cmake -S tools/dbmod_test -B /tmp/dark-dbmod-test -G Ninja
cmake --build /tmp/dark-dbmod-test -j "$(nproc)"
ctest --test-dir /tmp/dark-dbmod-test --output-on-failure
/tmp/dark-dbmod-test/dbmod_test /path/to/patch.dml
```

The standalone C++ parser produces ordered statements with source lines, quoted
arguments (including empty strings), nested fields and pre/post-load phase
annotations. It recognizes the exact DML1 header, comments and block structure,
limits input size and nesting, and reports malformed input without touching engine
state. Embedded quotes/braces in raw design-note values are retained.

All 88 installed TFix DML files outside DOC pass syntax parsing on the current
Linux PC. This is syntax coverage, not command validation or patch compatibility.
The parser deliberately does not evaluate fingerprints, validate command arguments,
resolve objects or mutate properties. It is not linked into the game yet.

Next stages require semantic validation of each command family, fingerprint
matching, resource-layer discovery and correct gamesys/mission/save load timing.
Never execute a partially validated patch. Keep the audit's unverified matrix
entries explicit until semantic review and behavior checks are complete.

## Offline fingerprints

`fingerprint.cpp` validates leading fingerprint blocks and evaluates them against
read-only adapters. It implements unique 8–15-object position fingerprints with
three-unit distance tolerance and the documented match counts; positive OR and
negative veto semantics; quest existence/comparisons/masks; direct property,
name, gamesys, object existence, archetype, script, config and mod presence rules.
Gamesys fingerprints are restricted to PROP/NAME/GAM. All fingerprints are
validated before evaluation, including blocks whose result would otherwise be
irrelevant. Missing adapters produce errors rather than successful negations.

Mod-version comparison syntax is explicitly rejected pending version semantics.
The query adapters must supply direct (not inherited) property presence and
current database identities. No engine adapter or patch application exists yet.
The CLI now validates fingerprint grammar as well as syntax; all 88 installed
patches pass. Evaluator fixtures test matching/mismatching snapshots independently
of those installed-file grammar checks. Neither check proves gameplay patching.

## Offline discovery manifests

`discovery.cpp` constructs ordered patch lists from caller-resolved database,
mod and resource layers using a read-only regular-file listing callback. It can
therefore later use filesystem or archive-backed IStore adapters without
changing ordering semantics. The caller selects the documented base/FM database
directory and supplies canonical layer identities in highest-priority-first order.

Named all-database patches precede database-specific patches, with base/FM first
and mods from lowest to highest priority. Directory patches follow, by resource
layer from lowest to highest; within a layer mission-global files precede
mission-specific files. Files sort case-insensitively. Gamesys discovery lists
only direct files in `dbmods`, not nested mission directories. Duplicate layer
entries do not apply the same root-relative filename twice. Non-DML entries are
ignored; ambiguous case-folded DML names and malformed adapter filenames fail.

Tests reproduce both documented load-order examples and exercise save/disable
skips, selected FM base, case handling and duplicate layers. Mission save loads
return no patches before invoking storage; gamesys loading remains a separate
caller decision. The manifest does not resolve or read engine resources yet,
evaluate fingerprints, validate commands or execute modifications.

## Command-shape preflight

`commands.cpp` turns recognized statements into command kinds and operations,
retaining their source syntax and creation-time property commands. It rejects
unknown/quoted keywords, unsupported prefixes, malformed header/data forms,
late script/schema directives and non-property commands in creation blocks.
All 88 installed patches pass this structural check. Focused fixtures exercise
both accepted variants and rejection paths.

This is deliberately not engine-semantic validation. Object names/IDs, link IDs,
tag versions, quest operators, field names/indices, numeric types, enum values,
inheritance, database scope and executor capabilities still require resolution
against engine schemas before any command can execute. Arbitrary data-block
contents are preserved, not declared valid. No runtime executor is enabled.

## Field staging

`fields.cpp` extracts named field edits and duplicate-name occurrence selectors,
retaining value tokens and quotation flags rather than prematurely coercing
strings or numbers. Schema adapters provide field offsets, sizes and strict
encoders. Staging copies the original record, updates only the selected field
bytes and returns only after all edits succeed. Unknown fields, missing encoders,
out-of-bounds storage and incorrect encoder byte counts fail without changing
the caller's original record.

This is not yet an engine sStructDesc adapter or a deep-copy property writer.
Legacy struct parsing is lenient for numeric values and unsafe to assume strict;
strict encoding and pointer-owning fields need dedicated handling before runtime
publication. Nested receptron/propagator blocks also require their command-specific
schema adapters. Tests cover repeated names, quoted empty values, occurrence
validation, unchanged surrounding bytes and failures after earlier staged edits.

## Native struct-description adapter

`struct_fields.cpp` uses the actual engine `sStructDesc`/`sFieldDesc` definitions
to build staging encoders for integer/short, boolean, enum, bitmask, bounded
string, float/double and float/double vector fields. Numeric conversions require
complete input and reject range errors; enum labels and bit labels are checked.
Unnamed bit fields retain binary notation. Strings reject truncation and NUL.
Unsupported types, including pointer-owning strings, have no encoder and cannot
be written. Enum/bit label tables must outlive their encoders.

The native-header fixture checks valid encodings, unchanged padding/pointers,
invalid labels, overflow, malformed vectors, string capacity and invalid native
schema bounds. All six standalone tests pass normally and under ASan/UBSan on
Linux. This does not yet test live property stores or publish changes to them.

The native schema adapter also encodes 32-bit 16.16 fixed-point values/vectors,
16-bit wrapped angles, heading/pitch/bank vectors in native tx/ty/tz order, and
integer points using their declared component widths. Overflow and malformed
components reject staging. The legacy fixed/angle parser's wider stores are not
used. RGBA and pointer-owning layouts remain unsupported pending explicit storage
and ownership handling.

The DML modules are compiled into the Linux game target through the shared
framework source list. This checks engine-header/build integration only: no
database-load hook invokes them, so runtime patching remains disabled. The Linux
`thief-runner` build and all six standalone tests (normal and ASan/UBSan) pass.

## Read-only native resource diagnostic

After a successful database load, the optional `dbmod_check=/absolute/patch.dml`
argument runs resource-backed syntax, fingerprint-grammar and command-shape
checks. `dbmod_check_gamesys=1` adds gamesys fingerprint restrictions. This uses
the live IResMan/IStore and binary-resource lock/release lifecycle, prints results
with mprintf, and emits `dbmod-check` trace records when DARK_RENDER_TRACE is set.
By default it does not evaluate fingerprints against the current mission. It
never validates engine property schemas or executes commands. Failures are diagnostics and do not
change the mission load result. Normal loads without the argument are unchanged.

Example with private writable game configuration and saves:

```sh
DARK_RENDER_TRACE=/tmp/dbmod-check.trace \
  ./build-linux/thief-runner game_path=/tmp/private-game \
  save_path=/tmp/private-saves content=vanilla quickstart=miss2.mis \
  auto_stop=12 SlowFrame=0 min_frame_time=0 vsync=0 \
  dbmod_check=/path/to/thief_gold/MODS/RESBASE/gamesys.dml \
  dbmod_check_gamesys=1
```

On Linux, the installed RESBASE patch passed through the native resource system
with 82 statements/80 commands. Separate runs produced the expected source-line
error for an unknown command and the expected missing-resource diagnostic.
All three mission runs exited cleanly with Vulkan synchronization validation and
private writable state. These checks establish read-only diagnostic integration,
not runtime patch application or schema validity.

Add `dbmod_check_match=1` to evaluate fingerprints against read-only adapters for
the loaded database. Gamesys identity comes from the database loader, positions
from live objects, property presence from direct relevance, names/archetypes from
the object/trait services, QVAR values from a mission-only snapshot, and CFG from
config definitions. Service references are retained for the context lifetime.
Missing mod/version adapters fail explicitly. The output distinguishes accept,
reject and unchecked; rejection is not a syntax error and never applies a patch.

Linux native checks observed positive and negative CFG fingerprint decisions and
matched the installed RESBASE gamesys patch using the loaded database. These
runs exited cleanly under Vulkan synchronization validation. They do not establish
behavioral coverage for every adapter or enable any gameplay patch execution.

## Live property-schema diagnostic

`dbmod_check_schema=1` adds read-only property-name and value-schema validation.
It inspects the live property manager and struct registry, including creation-time
ObjProp blocks. Registered fields are encoded into scratch bytes only; no object
target is resolved, no live property value is read/written, and no property is
created. Removal commands check only property registration. Other command types
are not schema-validated by this option. Scratch zeroes are never property defaults.

Results distinguish `valid`, `unsupported` (missing property/schema/field/encoder)
and `invalid` (the value cannot be encoded by the current native schema). An
invalid value can be a compatibility discrepancy or a source-content anomaly;
it is not automatically proof that an installed patch is wrong. Field encoder
errors retain their DML source line. Summary `schema=invalid` returns an error;
unsupported schemas also prevent a successful diagnostic result.

At commit afea860, the Linux miss2 RESBASE check had 63 validated
property edits, five unsupported cases and one invalid encoding. Missing cases
include AI_Efficiency.Flags, unregistered CretHTrack/DoorStaticLight, and DesignNote
string properties without a registered struct schema. The unencodable PhysAttr
bit list has a trailing quote in the installed file; it is not silently stripped.
A separate private fixture verifies valid Scale, invalid PhysAttr numeric input
and an unregistered property. Both runs exit cleanly under Vulkan synchronization
validation, without applying changes. Object resolution, strings with separate
ownership, non-property schemas and patch execution remain pending.


## Owning string-property values

`string_fields.cpp` returns owned text for IStringProperty rather than encoding
pointers into record bytes. It preserves quoted whitespace, embedded design-note
quotes and explicit empty strings. Empty blocks mean no value assignment; repeated
unnamed edits retain order. Only the unnamed singleton field/occurrence zero is
accepted; NUL is rejected to prevent native API truncation. Native schema
validation queries the string interface before considering raw struct encoding,
including when a string property also advertises struct metadata.

All seven standalone tests pass normally and under ASan/UBSan. Native RESBASE
validation now reports 65 valid edits, three unsupported cases and one rejected
bit-list encoding: both DesignNote edits now validate. A private native fixture
covers literal/empty strings, invalid field/occurrence selectors and creation-time
values (three valid, two invalid), with clean synchronization validation. No
property writes or object creation are performed. Actual IStringProperty Set,
patch-wide ownership/rollback and execution are still pending.


## Loaded script-module fingerprints

The native `ScriptModulePresence` helper queries cScriptMan's actual module table
on the main thread, not exported classes or disk files. It adds no guest COM
vtable entries. Names are compared case-insensitively using the module loader's
basename/OSM convention; both loaded and requested names normalize their suffix.
Unavailable manager and invalid input are failures, distinct from an absent
module, so a negative fingerprint cannot accept due to an unavailable query.
The manager's global reference is cleared when that manager is destroyed.

Native trace established that this Gold mission loads MISS02.OSM (142 classes),
not gen.osm. Loaded-module fingerprints accepted mixed-case MISS02 with and
without the suffix, rejected a genuinely absent module, and a matching negative
fingerprint vetoed acceptance. All three native runs exited cleanly with Vulkan
synchronization validation. The Linux game build and seven C++ tests pass. These
checks do not load/unload modules or execute patch script directives; MOD version
comparisons and complete patch application remain pending.
