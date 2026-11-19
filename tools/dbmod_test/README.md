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
