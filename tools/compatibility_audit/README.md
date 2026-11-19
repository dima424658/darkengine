# Compatibility audit tools

Run from the repository root:

```sh
python3 tools/compatibility_audit/generate.py /path/to/thief_gold \
  --output docs/newdark-compatibility-matrix.md
python3 -m unittest discover -s tools/compatibility_audit -v
python3 tools/compatibility_audit/dml_inventory.py /path/to/thief_gold \
  > /tmp/dark-dml-inventory.json
```

The matrix is a reproducible discovery inventory. Source candidates are exact
token matches, never inferred proof of implementation. Workflow documents and
Squirrel reference files need manual expansion; those limitations are recorded
in the generated file. Human-reviewed conclusions belong in the compatibility
plan until the detailed matrix has semantic review and behavioral evidence.

The DML inventory recognizes top-level commands and skips nested bodies, comments
and quoted text. It does not validate command argument grammar, evaluate
fingerprints, resolve resource precedence or execute patches. Raw field values
can contain a trailing quote (present in installed TFix files); such quotes are
not interpreted as opening a quoted token. Malformed block/comment structure
still fails the inventory with a file-specific diagnostic.

At baseline `82285ed`, the installed pack contains 88 DML files outside DOC. All
88 pass structural inventory. Counts include 2,002 `+ObjProp`, 559 `ObjProp`,
505 `-ObjProp`, 167 `+Link`, 60 `+MetaProp`, 143 `+Receptron`, 41 `+StimSource`,
12 `Create`, 12 `CreateArch`, 92 positive fingerprints, two negative fingerprints
and 18 script directives. Other modification/deletion variants also occur.

This demonstrates that a property-only patch executor would be insufficient:
fingerprints, links, metaproperties, creation, receptrons, stimulus sources and
script dependencies must be supported or rejected before any patch is applied.
No runtime patch loader is enabled by these tools.
