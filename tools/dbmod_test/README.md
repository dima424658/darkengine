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
