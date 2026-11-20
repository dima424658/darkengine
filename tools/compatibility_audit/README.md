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

Matrix IDs now derive from the reference path and requirement name, so adding
unrelated entries does not renumber reviewed requirements. Material extraction
uses directive headings rather than prose words. Materials, Engine stub methods
and Listener adapter restrictions have explicit source-reviewed classifications;
these classifications do not imply native runtime or full parity verification.

The inventory now expands DML lifecycle/command requirements and Squirrel service
methods/message classes. Native and Squirrel requirements use different reference
keys. Squirrel API-version conditions and overloaded signatures still need review;
method-name inventory must not be interpreted as API parity.

Generate the companion declaration inventory with:

```sh
python3 tools/compatibility_audit/script_contracts.py /path/to/thief_gold \
  --output docs/newdark-script-contracts.md
```

This retains every documented native/Squirrel overload, source line, conditional
branch history and nearby version annotation. Guards are literal documentation,
not evaluated platform availability; notes are not inferred minimum versions.
The declaration inventory closes an extraction gap, not the semantic ABI audit.
Use it alongside the method-level matrix when checking vtable order, defaults,
argument/output shape and API-version gating.

Native declaration extraction includes standalone `DECLARE_INTERFACE` and
`DECLARE_INTERFACE_` blocks as well as script services. Spaced `STDMETHOD (` and
`STDMETHOD_ (` forms are included; excluding them previously omitted manager
methods. The current installed-doc snapshot yields 1,432 matrix requirements and
864 detailed declarations. These are inventory counts, not compatibility scores.
