#!/usr/bin/env python3
"""Build a documentation inventory; source matches are not compatibility claims."""
import argparse
import hashlib
from pathlib import Path
import re


def requirements(doc):
    text = doc.read_text(errors="replace")
    if doc.name == "new_config_vars.txt":
        for line in text.splitlines():
            if re.fullmatch(r"[a-z][a-z0-9_]+(?: .*|)", line):
                name = line.split()[0]
                if name not in {"are", "dark1"}:
                    yield name
    elif doc.name == "new_commands.txt":
        for line in text.splitlines():
            match = re.match(r"^([a-z][a-z0-9_]+)\s+:", line)
            if match:
                yield match[1]
    elif doc.name == "material-format.txt":
        lines = text.splitlines()
        for index, line in enumerate(lines[:-1]):
            if re.fullmatch(r"[a-z][a-z0-9_]*(?: .*|)", line) and re.fullmatch(r"-{4,}", lines[index + 1]):
                yield line.split()[0]
    elif doc.name == "dbmod-sample.dml":
        yield from ("DML1 syntax", "Pre/post phase annotations", "Mission patch discovery",
                    "Gamesys patch discovery", "Resource layer application order", "Savegame exclusion",
                    "Fingerprint position thresholds", "Fingerprint positive/negative precedence",
                    "Fingerprint QVAR masks/comparisons", "Fingerprint PROP/NAME/GAM rules",
                    "Fingerprint EXIST/ARCH/SCR/MOD rules", "Script inclusion/exclusion", "Schema loading",
                    "TagBlock", "ObjProp", "ObjName", "ObjArch", "MetaProp", "Link", "Receptron",
                    "StimSource", "Create/CreateArch", "Destroy/DestroyArch/DestroyArchAll",
                    "QuestVar", "Texture/TextureAll", "Duplicate field indices", "Object ID remapping",
                    "apply_dbmods game/editor defaults", "dbmod_load", "dbmod_log")
    elif doc.name == "API-reference_services.txt":
        service = None
        for line in text.splitlines():
            if re.fullmatch(r"[A-Z][A-Za-z0-9_]*", line):
                service = line
            match = re.match(r"^[ \t]+(?:[A-Za-z_][A-Za-z0-9_]*[ \t]+)?([A-Za-z_][A-Za-z0-9_]*)\(", line)
            if service and match:
                yield service + "." + match[1]
    elif doc.name == "API-reference_messages.txt":
        for match in re.finditer(r"^class (\w+)(?: extends (\w+))?", text, re.M):
            yield match[1]
    elif doc.parent.name == "script":
        for match in re.finditer(r"STDMETHOD(?:_\([^,]+,\s*|\(\s*)(\w+)", text):
            yield match[1]
    elif doc.name == "modders_notes.txt":
        lines = text.splitlines()
        for index, line in enumerate(lines[:-1]):
            if line.strip() and re.fullmatch(r"-{4,}", lines[index + 1].strip()):
                yield line.strip()
    else:
        yield doc.stem


def review(doc, name, evidence):
    status = "Unverified"
    if doc.name == "material-format.txt":
        supported = {"include", "ani_rate", "ani_frames", "ani_mode", "force_ani_settings",
                     "terrain_scale", "tile_factor", "force_opaque", "force_full_alpha",
                     "force_alpha_key", "no_mipmap", "uv_clamp", "illum_map", "force_local_pal"}
        if name in supported:
            status = "Partial" if name in {"illum_map", "force_local_pal"} else "Implemented; behavioral parity unverified"
            evidence = "`libsrc/namedres/material_parser.hpp`, `material.cpp`; see `docs/materials.md` for constraints"
        else:
            status = "Missing"
            evidence = "Material parser reports unsupported directives and skips render-pass blocks; `docs/materials.md`"
    if doc.name == "Engine.txt" and name in {"SetEnvMapZone", "SetTagBlock", "GetTagBlock", "GetFogZone", "SetFogZone", "ObjRaycast"}:
        status = "Missing" if name in {"SetEnvMapZone", "SetTagBlock", "GetTagBlock"} else "Partial"
        evidence = "`src/script/engscrpt.cpp`: explicit stubs, script-only fog table or approximate raycast exclusions"
    if doc.name == "Listener.txt":
        status = "Partial" if name in {"SubscribeLink", "UnubscribeLink", "UnsubscribeLink"} else "Missing in Squirrel adapter; native unverified"
        evidence = "`CallListener` in `libsrc/script/squirrel/squirrelhost.cpp`; synchronous link subscriptions only"
    if doc.name == "dbmod-sample.dml":
        if name in {"DML1 syntax", "Pre/post phase annotations"}:
            status = "Implemented syntax only; runtime missing"
            evidence = "`src/framewrk/dbmod/syntax.cpp`, `tools/dbmod_test`: all 88 installed patches parse; no executor"
        else:
            status = "Missing runtime DML behavior"
            evidence = "`src/framewrk/dbmod/` has syntax representation only; database load path has no patch executor"
    native_gaps = {
        "AnimTexture.txt": ({"ChangeTextureLoc", "ChangeAllTextures", "ChangeSingleTexture"},
                            "src/render/antxscpt.cpp", "not exposed by the Squirrel adapter"),
        "Property.txt": ({"PossessedSimple", "GetIndexed", "SetIndexed", "SetLocalIndexed"},
                         "src/script/propscpt.cpp", "not exposed by the Squirrel adapter"),
        "Quest.txt": ({"BinSet", "BinGet", "BinExists", "BinDelete", "GetFirst", "GetNext"},
                      "src/script/quesscpt.cpp", "Squirrel binary-variable helpers exist separately; native parity missing"),
        "Object.txt": ({"ObjectToWorld", "WorldToObject", "CalcRelTransform", "Archetype",
                        "SetArchetype", "FindDescendentsSpatial"},
                       "src/script/objscrpt.cpp", "Squirrel exposes some transforms separately; native parity missing"),
    }
    if doc.parent.name == "script" and doc.name in native_gaps:
        methods, source, qualification = native_gaps[doc.name]
        if name in methods:
            status = "Missing native method"
            evidence = f"`{source}` service implementation lacks this method; {qualification}"
    return status, evidence


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("game", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    repo = Path(__file__).resolve().parents[2]
    source_index = {}
    for directory in ("src", "libsrc", "h"):
        for path in sorted((repo / directory).rglob("*")):
            if path.suffix.lower() not in {".c", ".cpp", ".h", ".hpp", ".inc", ".l", ".y"}:
                continue
            relative = str(path.relative_to(repo))
            for token in set(re.findall(r"\b[A-Za-z_][A-Za-z_0-9]*\b", path.read_text(errors="replace"))):
                source_index.setdefault(token, []).append(relative)
    output = ["# Detailed NewDark documentation inventory", "",
              "Generated by `tools/compatibility_audit/generate.py`; baseline `82285ed`.", "",
              "This is a documentation inventory, not a verified parity score. Each row",
              "requires semantic review and a behavior check. Source tokens can be comments,",
              "declarations, stubs or unrelated identifiers. No match does not rule out aliases.", "",
              "See `newdark-compatibility-plan.md` for confirmed findings, dependencies and gates.", "",
              "IDs are content-derived and remain stable when unrelated requirements are added.", "",
              "| ID | Reference | Requirement | Source candidates / review evidence | Status |",
              "| --- | --- | --- | --- | --- |"]
    number = 0
    for folder in ("DOC", "Docs"):
        root = args.game / folder
        for doc in sorted(root.rglob("*")):
            if not doc.is_file() or doc.suffix.lower() not in {".txt", ".cfg", ".ini", ".dml", ".sub", ".rtf", ".odt", ".doc"}:
                continue
            for name in dict.fromkeys(requirements(doc)):
                number += 1
                candidates = source_index.get(name.rsplit(".", 1)[-1], [])
                evidence = ", ".join(f"`{p}`" for p in candidates[:3]) or "No exact token; inspect aliases"
                reference = str(doc.relative_to(args.game))
                status, evidence = review(doc, name, evidence)
                identity = hashlib.sha256((reference + "\0" + name).encode()).hexdigest()[:12]
                safe_name = name.replace("|", "\\|")
                output.append(f"| ND-{identity} | `{reference}` | {safe_name} | {evidence} | {status} |")
    output.extend(["", f"Inventory entries: {number}.", "",
                   "Non-text assets, bundled archives and licenses need separate sample/license review.",
                   "Binary Word/ODT entries represent workflow documents; their individual instructions",
                   "are not automatically extracted. Squirrel services/messages are individually listed,",
                   "but signatures, API-version guards and behavioral parity still require review.", ""])
    args.output.write_text("\n".join(output))
    print(f"Wrote {number} inventory entries to {args.output}")


if __name__ == "__main__":
    main()
