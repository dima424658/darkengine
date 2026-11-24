# Vita content overlays and subtitles

The Vita path resolver checks loose files in `ux0:data/thiefX/mods/` before
the title's base data directory. Each non-hidden immediate subdirectory of
`mods/` is treated as an enabled mod. Mod directories are ordered by
case-insensitive name, with the first matching loose file taking precedence;
the base directory remains the final read fallback. New files are written to
the base directory, never into a mod overlay. This keeps mod assets read-only
from the engine's perspective and leaves CRF archive handling to the resource
manager.

`DARK_VITA_ASSET_ROOT` overrides the title's default data directory.
`DARK_VITA_ASSET_ROOTS` can specify multiple roots separated by `:` or `;`;
the `ux0:` device prefix is preserved when a colon is used as a separator.
Parent-directory components are rejected by the resolver.

`DarkVitaSubtitles::Parse` reads NewDark `SUB1` files, including comments,
subtitle blocks, `Sub`, timed `MultiSub` cues, sound-type flags, and `SrtSub`
references. It returns subtitle text as UTF-8. UTF-8 is detected by default;
legacy Windows-1250 files are decoded when the input is not valid UTF-8, and
callers can explicitly select either encoding. `SrtSub` filenames are retained
as references; external SRT loading and in-game subtitle presentation remain
separate work.

The isolated regression target is `tools/vita_content_test`; it exercises mod
precedence, base-root writes, traversal rejection, NewDark syntax, duplicate
IDs, and Polish Windows-1250 text.
