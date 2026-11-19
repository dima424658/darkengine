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
    elif doc.name == "subtitles.txt":
        yield from ("Language/resource-layer SUB discovery", "Sound subtitle lifecycle",
                    "Movie subtitle lifecycle", "Positional sound distance indicators")
        for line in text.splitlines():
            if re.match(r"^(?:enable_subtitles|subtitles_[a-z_<>A-Z]+|movsubtitles_[a-z_]+)(?:\s|$)", line):
                yield line.split()[0]
    elif doc.name == "subtitles-sample.sub":
        yield from ("SUB1 header and comments", "Resource priority and duplicate entries",
                    "Type and Descr blocks", "Singleton", "OnlySoundType", "NoVolume",
                    "Sub sound identifiers", "Text and escaped quotes", "Time and Length",
                    "MultiSub sequences", "Movie identifiers")
    elif doc.name == "font-sample.fon":
        yield from ("FON1 header", "resource", "antialias", "height", "width", "dpi", "dynamic")
    elif doc.name == "fm_selector_api.txt":
        yield from ("SelectFM export and packed ABI", "Selection/cancel/exit return values",
                    "Mutable FM root and selection buffers", "Run selector after game exit",
                    "Mod exclude paths", "FM language fallback and forced language")
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
        for match in re.finditer(r"STDMETHOD\s*(?:_\s*\([^,]+,\s*|\(\s*)(\w+)", text):
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
    if doc.name in {"subtitles.txt", "subtitles-sample.sub"}:
        status = "Missing subtitle pipeline"
        evidence = "No SUB discovery/parser or subtitle sound/movie lifecycle; `src/ui/movie.cpp` only invokes playback backends"
    if doc.name == "font-sample.fon" or (
            doc.name == "modders_notes.txt" and name == "TTF/OTF Fonts and UTF-8 Support"):
        status = "Missing vector font pipeline"
        evidence = "`libsrc/namedres/fonrstyp.cpp` binds .fon/.bin as raw resources; no FON1 definition parsing or vector rasterizer"
    if doc.name == "fm_selector_api.txt":
        status = "Missing external selector API; configured selection exists separately"
        evidence = "`contentctx.cpp` reads fm/fm_path and builds resource paths; no SelectFM dynamic-library invocation"
    if doc.name in {"kbd_00000407.ini", "kbd_0000041d.ini"}:
        status = "Missing custom keyboard table loading; native layouts exist"
        evidence = "`gen_bind.cpp` reads kb_country as int and passes it to kb_set_country; `kbcntry.c` uses compiled tables"
    if doc.name == "al_reverb.ini":
        status = "Missing preset-file loading; EFX reverb exists"
        evidence = "`libsrc/sound/openal.cpp` UpdateEffect uses compiled six-parameter presets and fixed reflection values; no INI loader"
    engine_method = name.rsplit(".", 1)[-1]
    native_engine = doc.parent.name == "script" and doc.name == "Engine.txt"
    squirrel_engine = doc.name == "API-reference_services.txt" and name.startswith("Engine.")
    if native_engine or squirrel_engine:
        engine_review = {
            "ConfigIsDefined": ("Source implementation reviewed; runtime/ABI parity unverified",
                                "engscrpt.cpp delegates to config_is_defined with null guard"),
            "ConfigGetInt": ("Source implementation reviewed; runtime/ABI parity unverified",
                             "engscrpt.cpp delegates typed output to config_get_int"),
            "ConfigGetFloat": ("Source implementation reviewed; runtime/ABI parity unverified",
                               "engscrpt.cpp delegates typed output to config_get_float"),
            "ConfigGetRaw": ("Source implementation reviewed; runtime/ABI parity unverified",
                             "engscrpt.cpp copies bounded config text into owned script string; missing value clears output"),
            "BindingGetFloat": ("Source implementation reviewed; runtime/ABI parity unverified",
                                "engscrpt.cpp queries binder echo and parses float, matching inputbnd.cpp GetVarValue query mechanism"),
            "FindFileInPath": ("Source implementation reviewed; runtime/ABI parity unverified",
                               "engscrpt.cpp builds Datapath from named config value, disables implicit current directory and returns owned path"),
            "IsRunningDX6": ("Intentionally false on SDL/Vulkan backend",
                             "engscrpt.cpp returns FALSE; backend has no legacy DX6 rendering mode"),
            "GetCanvasSize": ("Partial: configured resolution instead of actual canvas",
                              "engscrpt.cpp reads game_screen_size with 640x480 fallback, without querying the active 2D surface"),
            "GetAspectRatio": ("Partial: configured resolution instead of actual canvas",
                               "engscrpt.cpp computes ratio from GetCanvasSize game_screen_size values"),
            "GetFog": ("Source implementation reviewed; persistence/visual parity unverified",
                       "engscrpt.cpp reads global portal_fog_color/dist used by portal.c"),
            "SetFog": ("Source implementation reviewed; persistence/visual parity unverified",
                       "engscrpt.cpp writes global portal_fog_color/dist; effective drawing also depends on portal_fog_on"),
            "GetWeather": ("Source implementation reviewed; lifecycle/visual parity unverified",
                           "engscrpt.cpp delegates to WeatherGetScriptParams over live g_Weather fields"),
            "SetWeather": ("Source implementation reviewed; lifecycle/visual parity unverified",
                           "weather.cpp copies parameters into g_Weather and calls Update; non-rain type coerced to snow, texture bounded"),
            "PortalRaycast": ("Source implementation reviewed; boundary cases unverified",
                              "engscrpt.cpp inverts terrain PortalRaycast clear result, returning hit or endpoint position"),
        }
        if engine_method in engine_review:
            status, evidence = engine_review[engine_method]
            evidence = "`src/script/engscrpt.cpp`, `src/render/weather.cpp`: " + evidence
            if squirrel_engine:
                evidence += "; CallEngine forwards to native service, Squirrel output/ABI behavior still unverified"
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
        elif name in {"Mission patch discovery", "Gamesys patch discovery",
                      "Resource layer application order", "Savegame exclusion"}:
            status = "Offline ordering implemented; storage/game integration missing"
            evidence = "`src/framewrk/dbmod/discovery.cpp`, `tools/dbmod_test/discovery.cpp`; callback-backed manifests, no executor"
        elif name in {"Fingerprint position thresholds", "Fingerprint positive/negative precedence",
                      "Fingerprint QVAR masks/comparisons", "Fingerprint PROP/NAME/GAM rules"}:
            status = "Evaluator and opt-in native inspection implemented; automatic application missing"
            evidence = "`fingerprint.cpp`, `engine_fingerprint.cpp`, `inspection.cpp`; native read-only adapters, no patch executor"
        else:
            status = "Missing runtime DML behavior"
            evidence = "`commands.cpp` validates command shapes; database load path has no patch executor"
        if name == "ObjProp":
            evidence = "`property_validation.cpp` checks native schemas/owned strings; `object_validation.cpp` inspects live targets; no writes"
        elif name == "Duplicate field indices":
            status = "Staged encoding implemented; runtime application missing"
            evidence = "`fields.cpp`, `struct_fields.cpp`; occurrence selection and strict encoders tested without property-store writes"
        elif name == "Object ID remapping":
            status = "Resolver implemented; loader lifecycle integration missing"
            evidence = "`object_reference.cpp` tests explicit numeric remapping and quoted names; post-load inspection uses live IDs only"
        elif name == "Fingerprint EXIST/ARCH/SCR/MOD rules":
            status = "Partial evaluator/native inspection; MOD adapter and automatic application missing"
            evidence = "`engine_fingerprint.cpp` queries objects, traits and loaded script modules; MOD query/version semantics unavailable"
    overlay_method = name.rsplit(".", 1)[-1]
    is_overlay = (doc.parent.name == "script" and doc.name == "DarkOverlay.txt") or (
        doc.name == "API-reference_services.txt" and name.startswith("DarkOverlay."))
    if is_overlay and overlay_method in {"WorldToScreen", "GetObjectScreenBounds"}:
        status = "Missing projection behavior in Squirrel; native unverified"
        evidence = "`CallDarkOverlay` in `squirrelhost.cpp` always returns false and zero screen coordinates"
    if is_overlay and overlay_method == "SetTextColorFromStyle":
        status = "Partial Squirrel behavior; native unverified"
        evidence = "`CallDarkOverlay` sets hardcoded 0xffff and ignores the requested style colour"
    if doc.parent.name == "script" and doc.name == "Version.txt" and name == "GetModPath":
        status = "Partial"
        evidence = "`src/script/engscrpt.cpp` returns the full resource search path, including FM/base entries, rather than just mod paths"
    if doc.name == "new_config_vars.txt":
        replacements = {
            "vsync_mode": ("Partial alternative; NewDark key unsupported", "`libsrc/lgd3d/vulkan.cpp` reads boolean `vsync`; documented mode values are not equivalent"),
            "force_windowed": ("Alternative controls; NewDark key unsupported", "`src/framewrk/init.cpp` persists `game_full_screen`; SDL display provider owns window modes"),
            "use_d3d_display": ("Intentionally replaced", "Shared SDL/Vulkan display path replaces Direct3D provider selection; no DX9 provider"),
        }
        replacements.update({
            "snd_oal_device": ("Missing device override; default device works",
                               "`openal.cpp` Init always calls alcOpenDevice(NULL)"),
            "disable_oal_eax_reverb": ("EFX-only backend; selector key unsupported",
                                       "`openal.cpp` InitEffects uses AL_EFFECT_REVERB; no native EAX path to suppress"),
            "snd_decompress_on_load": ("Missing mode selection; OGG decoding exists",
                                       "`auxsnd.cpp` PrepareSoundBuffer decodes OGG to PCM without reading the documented mode"),
            "snd_speaker_config": ("Missing configurable output layout",
                                   "`openal.cpp` uses automatic output selection with Linux mono-to-stereo retry, no documented mode mapping"),
            "force_underwater_reverb": ("Partial fallback; forced/configurable override missing",
                                        "`appsfx.cpp` chooses UnderWater only when room acoustics are unset, before mission/gamesys fallback"),
            "submerged_sound_occlusion": ("Missing configurable submerged occlusion",
                                          "`appsfx.cpp` handles underwater room fallback but does not read submerged_sound_occlusion"),
        })
        if name in replacements:
            status, evidence = replacements[name]
    if doc.name == "modders_notes.txt":
        rendering_gaps = {
            "Fog Zones": ("Missing zone rendering; metadata/script storage partial",
                          "`portal.c`/`portdraw.c` consume global fog only; `engscrpt.cpp` keeps eight script-only presets"),
            "Ambient Light Zones": ("Missing runtime zone lighting; metadata retained",
                                    "`wrloop.cpp` retains zone bytes; portal_cell_ambient_zone has no callers; missrend.h has global ambient only"),
            "Max View Distance": ("Partial global control; mission minimum missing",
                                  "`render.cpp` owns global zfar/set_zfar; sMissionRenderParams has no Required View Dist field"),
            'Changed Prop: Weather->"Local Weather"': ("Partial legacy weather schema",
                                                      "`weather.h`/`weather.cpp` expose boolean fog and precipitation, no fog/ambient zone selectors"),
            'New Var: Mission Variables->"Fog Zones"': ("Missing mission zone variable",
                                                      "`engscrpt.cpp` presets are script-only storage; renderer consumes global portal_fog values"),
        }
        if name in rendering_gaps:
            status, evidence = rendering_gaps[name]
        native_property_gaps = {
            'New Prop: Door->"Static Light Pos"': ("Missing in tested Linux game", "DoorStaticLight is not registered"),
            'New Prop: Creature->"Disable Head Track"': ("Missing in tested Linux game", "CretHTrack is not registered"),
            'Changed Prop: AI->"Efficiency settings"': ("Partial native schema", "AI_Efficiency has no Flags field in the live schema"),
        }
        if name in native_property_gaps:
            status, detail = native_property_gaps[name]
            evidence = "Native dbmod_check_schema RESBASE inspection: " + detail + "; see tools/dbmod_test/README.md"
    property_method = name.rsplit(".", 1)[-1]
    native_property = doc.parent.name == "script" and doc.name == "Property.txt"
    squirrel_property = doc.name == "API-reference_services.txt" and name.startswith("Property.")
    if native_property or squirrel_property:
        property_review = {
            "Get": "GetPropField follows direct/inherited donor; schema lookup selects first matching field; cleanup defects remain",
            "Set": "native scalar/field overloads route through SetField network message; typed field/string writes exist; ownership cleanup defects remain",
            "SetSimple": "CallProperty forwards three-argument form to native scalar Set overload",
            "SetLocal": "native four-argument form invokes SetField directly, bypassing owner routing; lifecycle/THIEF1 ABI order unverified",
            "Add": "calls property Create on nonzero object; missing-property rejection exists but acquired property reference is not released",
            "Remove": "calls property Delete; acquired property reference is not released",
            "CopyFrom": "calls property Copy(target, source); acquired property reference is not released",
            "Possessed": "uses IsRelevant including inheritance, not IsSimplyRelevant; acquired property reference is not released",
        }
        if property_method in property_review:
            status = "Source behavior reviewed; lifecycle/runtime parity unverified"
            evidence = "`src/script/propscpt.cpp`: " + property_review[property_method]
            if squirrel_property:
                evidence += "; CallProperty forwarding reviewed, Squirrel conversion/version guards unverified"
        if squirrel_property and property_method in {"PossessedSimple", "GetIndexed", "SetIndexed", "SetLocalIndexed"}:
            status = "Missing Squirrel method and native backing method"
            evidence = "CallProperty has no branch; propscpt.h/cpp lacks method; documented API 11/12 additions"
    quest_method = name.rsplit(".", 1)[-1]
    native_quest = doc.parent.name == "script" and doc.name == "Quest.txt"
    squirrel_quest = doc.name == "API-reference_services.txt" and name.startswith("Quest.")
    if native_quest or squirrel_quest:
        quest_review = {
            "SubscribeMsg": ("Partial named subscriptions; wildcard missing",
                             "quest.cpp SubscribeMsg looks up/creates literal name, not all-variable subscription; quesscpt.cpp always returns TRUE"),
            "UnsubscribeMsg": ("Partial named subscriptions; wildcard missing",
                               "quest.cpp removes listener from literal named node; quesscpt.cpp ignores returned status and returns TRUE"),
            "Set": ("Source implementation reviewed; runtime/scope parity unverified",
                    "quesscpt.cpp updates existing qvar without changing type, creates missing qvar with requested scope"),
            "Get": ("Source implementation reviewed; runtime parity unverified",
                    "quesscpt.cpp delegates integer read to QuestData"),
            "Exists": ("Source implementation reviewed; runtime parity unverified",
                       "quesscpt.cpp delegates existence query to QuestData"),
            "Delete": ("Source implementation reviewed; runtime/network parity unverified",
                       "quesscpt.cpp delegates deletion to QuestData; no native binary store method"),
        }
        if quest_method in quest_review:
            status, evidence = quest_review[quest_method]
            evidence = "`src/script/quesscpt.cpp`, `src/engfeat/quest.cpp`: " + evidence
            if squirrel_quest:
                evidence += "; CallQuest forwards to native service"
        if squirrel_quest:
            if quest_method == "GetAllVars":
                status = "Source implementation reviewed; scope/version parity unverified"
                evidence = "CallQuest builds a fresh table from QuestData Iter(type) and releases iterator/service"
            elif quest_method in {"BinSet", "BinGet", "BinExists", "BinDelete"}:
                status = "Partial VM-only data; campaign persistence missing"
                evidence = "CallQuest keeps HSQOBJECT references in gQuestBinaryValues; no binary serialization/65000-byte enforcement, cleared at VM teardown"
            elif quest_method in {"BinSetTable", "BinGetTable"}:
                status = "Incorrect table API and missing campaign persistence"
                evidence = "CallQuest treats argument 1 as string key and argument 2 as value, unlike documented (name, table)/(name); no table serialization"
            elif quest_method in {"GetFirst", "GetNext"}:
                status = "Missing Squirrel method and native backing method"
                evidence = "CallQuest has no branch; quesscpt.cpp lacks native iterator methods"
    object_method = name.rsplit(".", 1)[-1]
    native_object = doc.parent.name == "script" and doc.name == "Object.txt"
    squirrel_object = doc.name == "API-reference_services.txt" and name.startswith("Object.")
    if native_object or squirrel_object:
        object_groups = [
            ({"BeginCreate", "Create"}, "creates concrete object from existing archetype/clone; invalid parent guarded by assertion"),
            ({"EndCreate", "Destroy", "Exists"}, "delegates to ObjectSystem lifecycle/existence methods"),
            ({"SetName", "GetName", "Named"}, "delegates naming and reverse name lookup to ObjectSystem"),
            ({"SetTransience", "IsTransient"}, "delegates transience state to ObjectSystem"),
            ({"AddMetaProperty", "RemoveMetaProperty"}, "uses TraitManager or owner-routed ChangeMeta message, depending on network build"),
            ({"HasMetaProperty"}, "uses ObjHasDonorIntrinsically, distinct from inherited donor query"),
            ({"InheritsFrom"}, "uses TraitManager ObjHasDonor"),
            ({"Position", "Facing"}, "reads ObjPos; missing position returns zero vector, facing converted to degrees"),
            ({"Teleport"}, "converts facing degrees to engine angles and delegates GameTools TeleportObject with reference frame"),
            ({"IsPositionValid"}, "delegates to IsCreaturePositionValid; general-object/version boundary behavior unverified"),
            ({"AddMetaPropertyToMany", "RemoveMetaPropertyFromMany"}, "parses target set and returns parsed count, not count of successful changes"),
            ({"RenderedThisFrame"}, "delegates to rendobj_object_visible; frame timing semantics unverified"),
        ]
        for methods, detail in object_groups:
            if object_method in methods:
                status = "Source implementation reviewed; runtime/ABI parity unverified"
                evidence = "`src/script/objscrpt.cpp`: " + detail
                if squirrel_object:
                    evidence += "; CallObject forwards to native service"
                break
        if object_method == "FindClosestObjectNamed":
            status = "Partial source behavior; query cleanup and invalid-target cases unresolved"
            evidence = "objscrpt.cpp queries named archetype descendants and compares squared distance; query not released and candidate position not null-checked"
        if squirrel_object and object_method == "Archetype":
            status = "Squirrel-only implementation reviewed; native method missing"
            evidence = "CallObject directly calls TraitManager GetArchetype; native service has no corresponding method"
        elif squirrel_object and object_method in {"ObjectToWorld", "WorldToObject"}:
            status = "Squirrel-only transform; runtime parity unverified"
            evidence = "CallObject combines native Position/Facing with rotation or transpose; native service method absent"
        elif squirrel_object and object_method == "CalcRelTransform":
            status = "Incorrect relative transform; native method missing"
            evidence = "CallObject ignores rel_type/sub-index, subtracts Euler angles instead of composing orientations, and always returns true"
        elif squirrel_object and object_method in {"SetArchetype", "FindDescendentsSpatial", "FindScriptDonor", "AnyExist", "GetAll", "GetOne"}:
            status = "Missing Squirrel method and native backing method"
            evidence = "CallObject has no matching branch; native Object service lacks the documented method"
    link_method = name.rsplit(".", 1)[-1]
    native_link = doc.parent.name == "script" and doc.name == "Link.txt"
    squirrel_link = doc.name == "API-reference_services.txt" and name.startswith("Link.")
    if native_link or squirrel_link:
        link_review = {
            "Create": "delegates source/destination/kind to LinkManager Add",
            "Destroy": "delegates link ID to LinkManager Remove",
            "AnyExist": "queries source/destination/kind and checks Done with scoped query ownership",
            "GetAll": "returns owned linkset over matching LinkManager query",
            "GetOne": "returns first matching ID or zero; ordering unspecified",
            "CreateMany": "parses source/destination sets and adds their Cartesian product; individual add results ignored",
            "DestroyMany": "parses Cartesian product, removes links while iterating each query; mutation/iteration safety unverified",
            "GetAllInherited": "calls QueryInheritedLinksUncached and wraps query in linkset",
            "GetAllInheritedSingle": "calls QueryInheritedLinksSingleUncached and wraps query in linkset",
            "BroadcastOnAllLinks": "posts owner-routed messages to linked destinations; data overload filters by first struct field when relation has multiple fields",
            "BroadcastOnAllLinksData": "CallLink uses native data overload, which filters links using first data field and posts matching owner-routed messages",
        }
        if link_method in link_review:
            status = "Source implementation reviewed; runtime/ABI parity unverified"
            evidence = "`src/script/linkscpt.cpp`: " + link_review[link_method]
            if squirrel_link:
                evidence += "; CallLink forwarding/PushLinkSet snapshot reviewed; iterable/version semantics unverified"
        if link_method == "BroadcastOnAllLinksMany":
            status = "Missing native method" if native_link else "Missing Squirrel method and native backing method"
            evidence = "linkscpt.h/cpp and CallLink lack documented many-source broadcast method"
    if doc.name == "API-reference_services.txt" and name.startswith("LinkTools."):
        status = "Source implementation reviewed; invalid-ID/field behavior incomplete"
        details = {
            "LinkKindNamed": "resolves relation name through native linkkind",
            "LinkKindName": "asserts relation exists before dereferencing descriptor",
            "LinkGet": "native Verify on nonexistent ID then S_OK; Squirrel failure check cannot reliably detect invalid ID",
            "LinkGetData": "selects first matching schema field, missing fields guarded by assertions",
            "LinkSetData": "missing data allocates uninitialized bytes before setting one field; absent schema leaks that allocation; assertion-based validation",
        }
        evidence = "`linkscpt.cpp`, CallLinkTools: " + details.get(link_method, "requires further review")
    service_method = name.rsplit(".", 1)[-1]
    native_version = doc.parent.name == "script" and doc.name == "Version.txt"
    squirrel_version = doc.name == "API-reference_services.txt" and name.startswith("Version.")
    if native_version or squirrel_version:
        version_review = {
            "GetAppName": ("Partial hardcoded identity; capability parity incomplete",
                           "engscrpt.cpp reports Thief 2 1.28 (or DromEd) even with Gold content; many 1.28 service additions are absent"),
            "GetVersion": ("Partial hardcoded identity; capability parity incomplete",
                           "engscrpt.cpp returns 1,28 regardless of missing documented 1.28 APIs"),
            "IsEditor": ("Game branch implemented; editor mode distinction missing",
                         "engscrpt.cpp returns zero in game, always one under EDITOR without checking edit/game mode"),
            "GetGame": ("Source implementation reviewed; runtime parity unverified",
                        "engscrpt.cpp returns game config string with dark fallback"),
            "GetGamsys": ("Partial config value; loaded database identity not queried",
                          "engscrpt.cpp reads gamesys config rather than dbfile.cpp gCurrGameFile"),
            "GetMap": ("Partial config value; loaded database identity not queried",
                       "engscrpt.cpp reads quickstart then file config rather than current loaded mission identity"),
            "GetCurrentFM": ("Source implementation reviewed; runtime parity unverified",
                             "engscrpt.cpp derives final path component from DarkContentFMPath, empty/S_FALSE without FM"),
            "GetCurrentFMPath": ("Source implementation reviewed; runtime parity unverified",
                                 "engscrpt.cpp returns DarkContentFMPath and S_FALSE if empty"),
            "FMizeRelativePath": ("Source implementation reviewed; path boundary cases unverified",
                                  "engscrpt.cpp prefixes active FM path with bounded JoinContentPath, otherwise passes through"),
            "FMizePath": ("Source implementation reviewed; path boundary cases unverified",
                          "engscrpt.cpp preserves slash/backslash/drive absolute paths, otherwise prefixes active FM"),
            "GetModPath": ("Partial",
                           "engscrpt.cpp returns full resource search path including FM/base entries, rather than mod paths only"),
        }
        if service_method in version_review:
            status, evidence = version_review[service_method]
            evidence = "`src/script/engscrpt.cpp`: " + evidence
            if squirrel_version:
                evidence += "; CallVersion forwards to native service; output/version gates unverified"
    native_animation = doc.parent.name == "script" and doc.name == "AnimTexture.txt"
    squirrel_animation = doc.name == "API-reference_services.txt" and name.startswith("AnimTexture.")
    if native_animation or squirrel_animation:
        if service_method == "ChangeTexture":
            status = "Partial legacy implementation; input safety unresolved"
            evidence = "antxscpt.cpp resolves texture IDs and calls DynTexture; parser uses unchecked strcpy, dyntex.cpp dereferences missing object position"
        else:
            status = "Missing native method" if native_animation else "Missing Squirrel method and native backing method"
            evidence = "antxscpt.cpp and CallAnimTexture implement only object-centred ChangeTexture"
    ai_method = name.rsplit(".", 1)[-1]
    native_ai = doc.parent.name == "script" and doc.name == "AI.txt"
    squirrel_ai = doc.name == "API-reference_services.txt" and name.startswith("AI.")
    if native_ai or squirrel_ai:
        ai_review = {
            "MakeGotoObjLoc": ("Source implementation reviewed; action/callback parity unverified",
                               "uses cAIBasicScript GotoObjLoc with speed/priority conversion and callback data, false without basic script"),
            "MakeFrobObj": ("Source implementation reviewed; action/callback parity unverified",
                            "native overloads call FrobObj with optional tool, priority conversion and callback data"),
            "MakeFrobObjWith": ("Source implementation reviewed; action/callback parity unverified",
                                "CallAI forwards explicit-tool form to native FrobObj overload"),
            "GetAlertLevel": ("Source implementation reviewed; runtime parity unverified",
                              "reads AI state alertness, returns kNoAlert without basic script"),
            "SetMinimumAlert": ("Source implementation reviewed; property effects unverified",
                                "updates minLevel in AI alert-cap property"),
            "ClearGoals": ("Missing behavior: explicit no-op",
                           "aiscrpt.cpp ClearGoals body contains only a comment"),
            "SetScriptFlags": ("Partial: speech flag only; motion flag ignored",
                               "sets sound-enactor speech-stop and halts speech; source explicitly excludes motion support"),
            "ClearAlertness": ("Source implementation reviewed; AI-state parity unverified",
                               "sets lowest alert level and removes outgoing awareness links"),
            "Signal": ("Source implementation reviewed; network/runtime parity unverified",
                       "sends AISignal message to owner via g_pAISignalMsg"),
            "StartConversation": ("Source implementation reviewed; conversation lifecycle unverified",
                                  "delegates to AIManager StartConversation"),
        }
        if ai_method in ai_review:
            status, evidence = ai_review[ai_method]
            evidence = "`src/ai/aiscrpt.cpp`: " + evidence
            if squirrel_ai:
                evidence += "; CallAI forwarding exists, Squirrel conversion/version guards unverified"
        if ai_method in {"Stun", "IsStunned", "UnStun", "Freeze", "IsFrozen", "UnFreeze", "SetFaceVisage"}:
            status = "Missing native AI method" if native_ai else "Missing Squirrel AI method and native backing method"
            evidence = "aiscrpt.cpp service and CallAI lack documented AI service additions; ShockAI capability is not equivalent exposure"
    physics_method = name.rsplit(".", 1)[-1]
    native_physics = doc.parent.name == "script" and doc.name == "Physics.txt"
    squirrel_physics = doc.name == "API-reference_services.txt" and name.startswith("Physics.")
    if native_physics or squirrel_physics:
        physics_review = {
            "SubscribeMsg": "calls subscription service with event mask and returns literal TRUE as HRESULT",
            "UnsubscribeMsg": "calls unsubscription service with event mask and returns literal TRUE as HRESULT",
            "LaunchProjectile": "delegates launcher/archetype/power/flags/additional velocity to launchProjectile",
            "SetVelocity": "destroys active model contacts before PhysSetVelocity, returns S_OK",
            "GetVelocity": "calls PhysGetVelocity and always returns S_OK; absent-model output behavior requires verification",
            "SetGravity": "calls PhysSetGravity and returns S_OK",
            "GetGravity": "delegates to PhysGetGravity",
            "HasPhysics": "delegates to PhysObjHasPhysics",
            "IsSphere": "checks first submodel for sphere, point or sphere-hat; false without model",
            "IsOBB": "checks first submodel for OBB; false without model",
            "ControlVelocity": "calls PhysControlVelocity and returns S_OK; game-specific vtable placement unverified",
            "StopControlVelocity": "calls PhysStopControlVelocity and returns S_OK; game-specific vtable placement unverified",
            "DeregisterModel": "calls PhysDeregisterModel for nonzero ID, E_FAIL for zero",
            "PlayerMotionSetOffset": "delegates submodel and offset to PlayerMotionSetOffset",
            "Activate": "activates existing model, E_FAIL without model",
            "ValidPos": "delegates to PhysObjValidPos",
            "IsRope": "delegates to PhysObjIsRope; native addition is appended to service interface",
            "GetClimbingObject": "sets output from PhysGetClimbingObj; native addition is appended to service interface",
        }
        if physics_method in physics_review:
            status = "Source implementation reviewed; runtime/ABI parity unverified"
            evidence = "`src/physics/phscrpt.cpp`: " + physics_review[physics_method]
            if squirrel_physics:
                evidence += "; CallPhysics forwards method, output/argument conversion still unverified"
        elif physics_method in {"ControlCurrentLocation", "ControlCurrentRotation", "ControlCurrentPosition"}:
            status = "Partial implementation; missing-position safety unresolved"
            evidence = "phscrpt.cpp checks nonzero ID then dereferences ObjPosGet without null check; position combines both controls and sends owner message"
    if doc.name == "API-reference_services.txt":
        interaction_review = {
            "Door.CloseDoor": "drscrpt.cpp rejects non-door/missing property and already closed/closing state, then DoorPhysActivate CLOSE",
            "Door.OpenDoor": "drscrpt.cpp rejects non-door/missing property and already open/opening state, then DoorPhysActivate OPEN",
            "Door.GetDoorState": "drscrpt.cpp returns kDoorNoDoor for invalid door, otherwise GetDoorStatus",
            "Door.ToggleDoor": "drscrpt.cpp returns BOOL activation through HRESULT; invalid door returns FALSE (zero), not failure HRESULT",
            "Door.SetBlocking": "drscrpt.cpp adds/removes door blocking, returns TRUE through HRESULT or FALSE for invalid door",
            "Door.GetSoundBlocking": "doorphys.cpp derives bool from room-to-room acoustic blocking factor, with room validity guards",
            "Locked.IsLocked": "lockscpt.cpp uses ObjComputeLockedState, which reads computed lock count > 0",
            "Key.TryToUseKey": "keysys.cpp checks key/lock properties and KeyWorksOnLock; check mode is non-mutating, other modes change self-lock state",
            "Damage.Damage": "dmgscrpt.cpp calls DamageObject with amount/kind for nonzero victim, always returns S_OK",
            "Damage.Slay": "dmgscrpt.cpp calls SlayObject for nonzero victim, always returns S_OK",
            "Damage.Resurrect": "dmgscrpt.cpp calls ResurrectObject for nonzero victim, always returns S_OK",
            "Container.Add": "contscrs.cpp forwards (container,obj,type,flags), reversing script object/container order",
            "Container.Remove": "contscrs.cpp forwards (container,obj); default/null-container semantics need runtime verification",
            "Container.MoveAllContents": "contscrs.cpp forwards (target,source,flags), reversing script source/target order",
            "Container.StackAdd": "contscrs.cpp delegates quantity change to ContainSys StackAdd",
            "Container.IsHeld": "contscrs.cpp delegates container/containee type query to ContainSys IsHeld",
        }
        if name in interaction_review:
            status = "Source implementation reviewed; runtime/return semantics unverified"
            evidence = "`src/script/` and native engfeat/sim services: " + interaction_review[name]
            evidence += "; matching Squirrel Call adapter forwards to native service"
    sound_method = name.rsplit(".", 1)[-1]
    native_sound = doc.parent.name == "script" and doc.name == "Sound.txt"
    squirrel_sound = doc.name == "API-reference_services.txt" and name.startswith("Sound.")
    if native_sound or squirrel_sound:
        sound_groups = [
            ({"Play", "PlayAtLocation", "PlayAtObject", "PlayAmbient"},
             "native raw-sound overloads use PlayAndRememberSound with position/ambient flags and completion records"),
            ({"PlaySchema", "PlaySchemaAtLocation", "PlaySchemaAtObject", "PlaySchemaAmbient"},
             "native schema overloads use PlayAndRememberSchema with object/location flags and completion records"),
            ({"PlayEnvSchema"}, "native implementation parses tags and uses environmental schema selection"),
            ({"PlayVoiceOver"}, "delegates to VoiceOverSys Play and returns whether handle is non-null; callback argument unused in wrapper"),
            ({"Halt", "HaltSchema"}, "delegates target/name/callback filters to HaltActiveSounds with raw/schema selector"),
            ({"HaltSpeech"}, "calls SpeechHalt and returns S_OK"),
            ({"PreLoad"}, "delegates to SFX_PreLoad"),
        ]
        for methods, detail in sound_groups:
            if sound_method in methods:
                status = "Source implementation reviewed; callback/save/runtime parity unverified"
                evidence = "`src/script/sndscrpt.cpp`: " + detail
                if squirrel_sound:
                    evidence += "; CallSound forwards to native overload"
                break
        if sound_method in {"SongChange", "SongSetTheme"}:
            status = "Missing native Sound method" if native_sound else "Missing Squirrel Sound method and native backing method"
            evidence = "sndscrpt.cpp service and CallSound lack song methods; underlying music system does not imply script exposure"
        elif squirrel_sound and sound_method.endswith("Net"):
            status = "Missing named Squirrel network method; native network parameter exists"
            evidence = "newdark_api.nut.inc and CallSound omit documented *Net names; ordinary native playback accepts eSoundNetwork"
        elif native_sound and sound_method.endswith("Legacy"):
            status = "THIEF1-specific entry point absent; restored host uses Thief II ABI"
            evidence = "sndscrpt.h/cpp expose network-parameter overloads, not named legacy overloads; Gold guest bridging needs separate ABI verification"
    camera_method = name.rsplit(".", 1)[-1]
    native_camera = doc.parent.name == "script" and doc.name == "Camera.txt"
    squirrel_camera = doc.name == "API-reference_services.txt" and name.startswith("Camera.")
    if native_camera or squirrel_camera:
        camera_review = {
            "StaticAttach": "calls CameraView and hides player arm/clears weapon in THIEF builds",
            "DynamicAttach": "calls CameraRemote and hides player arm/clears weapon in THIEF builds",
            "CameraReturn": "reattaches player only if supplied object matches current camera parent; otherwise S_FALSE",
            "ForceCameraReturn": "reattaches camera to PlayerObject unconditionally",
            "GetCameraParent": "queries CameraGetObjID(PlayerCamera())",
            "IsRemote": "compares actual camera parent against PlayerObject",
            "GetPosition": "reads current CameraGetLocation position",
            "GetFacing": "reads current CameraGetLocation orientation and converts engine angles to degrees",
            "CameraToWorld": "rotates local vector with camera orientation matrix and adds camera position",
            "WorldToCamera": "subtracts camera position then applies transpose of orientation matrix",
        }
        if camera_method in camera_review:
            status = "Source implementation reviewed; runtime/ABI parity unverified"
            evidence = "`src/script/camscrpt.cpp`: " + camera_review[camera_method]
            if squirrel_camera:
                evidence += "; CallCamera forwards to native service"
        elif camera_method == "LockMovement":
            status = "Missing behavior: explicit no-op"
            evidence = "camscrpt.cpp ignores boolean argument and returns S_OK; CallCamera forwards without implementing input lock"
    native_ui = doc.parent.name == "script" and doc.name == "DarkUI.txt"
    squirrel_ui = doc.name == "API-reference_services.txt" and name.startswith("DarkUI.")
    if native_ui or squirrel_ui:
        ui_review = {
            "TextMessage": "delegates to DarkMessageParams with default timeout translation",
            "ReadBook": "enters SwitchToDarkBookMode with text/art resources",
            "InvItem": "reads Inventory Selection(kCurrentItem)",
            "InvWeapon": "reads Inventory Selection(kCurrentWeapon)",
            "InvSelect": "delegates to Inventory Select",
            "IsCommandBound": "queries GetCmdBinds with one-result capacity",
            "DescribeKeyBinding": "joins up to sixteen bindings with localized separator or returns localized no-bindings text",
        }
        method = name.rsplit(".", 1)[-1]
        if method in ui_review:
            status = "Source implementation reviewed; UI/runtime parity unverified"
            evidence = "`src/dark/drkuiscr.cpp`: " + ui_review[method]
            if squirrel_ui:
                evidence += "; CallDarkUI forwarding exists"
        elif method == "TextMessageFormatted":
            status = "Missing native method" if native_ui else "Missing Squirrel method and native backing method"
            evidence = "drkuiscr.cpp and CallDarkUI lack font/margins/alignment formatted-message method"
    networking_method = name.rsplit(".", 1)[-1]
    native_networking = doc.parent.name == "script" and doc.name == "Networking.txt"
    squirrel_networking = doc.name == "API-reference_services.txt" and name.startswith("Networking.")
    if native_networking or squirrel_networking:
        networking_review = {
            "Broadcast": "owner/local-only checks followed by cNetMsg broadcast when networking is compiled",
            "SendToProxy": "directed cNetMsg with object-valued data mapping, local dispatch in non-network build",
            "TakeOver": "requests ObjTakeOver while networking; returns TRUE if inactive",
            "GiveTo": "requests ObjGiveWithoutObjID while networking; returns TRUE if inactive",
            "IsPlayer": "delegates to IsAPlayer",
            "IsMultiplayer": "queries NetManager IsNetworkGame",
            "SetProxyOneShotTimer": "queues proxy-enabled one-shot script timer after seconds-to-milliseconds conversion",
            "FirstPlayer": "starts NetManager player iterator and returns local PlayerObject",
            "NextPlayer": "advances NetManager iterator or returns OBJ_NULL",
            "Suspend": "delegates to NetManager SuspendMessaging",
            "Resume": "delegates to NetManager ResumeMessaging",
            "HostedHere": "queries ObjectNetworking ObjHostedHere",
            "IsProxy": "queries ObjectNetworking ObjIsProxy",
            "LocalOnly": "queries ObjectNetworking ObjLocalOnly",
            "IsNetworking": "queries NetManager Networking",
            "Owner": "returns PlayerObject for local-only object, otherwise ObjHostPlayer",
        }
        if networking_method in networking_review:
            status = "Native source path reviewed; multiplayer/runtime parity unverified"
            evidence = "`src/script/netscrpt.cpp`: " + networking_review[networking_method]
            if squirrel_networking:
                evidence += "; CallNetworking forwards to native service"
        if squirrel_networking and networking_method in {"IsMultiplayer", "IsNetworking", "FirstPlayer", "NextPlayer", "Suspend", "Resume", "HostedHere", "IsProxy", "LocalOnly", "Owner"}:
            status = "Local Squirrel fallback; multiplayer semantics absent"
            evidence = "CallNetworking hardcodes local-player/boolean/success results rather than querying native network state"
        if networking_method in {"CreateContentProxy", "AmHost", "NumPlayers", "MyPlayerNum", "ObjToPlayerNum", "PlayerNumToObj", "GetPlayerName"}:
            status = "Missing native Networking addition" if native_networking else "Missing Squirrel method and native backing method"
            evidence = "netscrpt.h/cpp and CallNetworking lack documented 1.27/API-11 methods"
    if doc.name == "API-reference_services.txt" and name in {"CD.SetBGM", "CD.SetTrack"}:
        status = "Missing behavior: success-returning no-op"
        evidence = "CallCD ignores track/flags and returns S_OK; streamed Sound schemas are a separate capability"
    act_method = name.rsplit(".", 1)[-1]
    native_act = doc.parent.name == "script" and doc.name == "ActReact.txt"
    squirrel_act = doc.name == "API-reference_services.txt" and name.startswith("ActReact.")
    if native_act or squirrel_act:
        act_review = {
            "React": "validates reaction descriptor, fills reaction struct from up to eight multi-parameters and calls Reactions React",
            "Stimulate": "looks up sensor, optionally adds a source descriptor, stimulates sensor or returns S_FALSE if absent",
            "GetReactionNamed": "delegates reaction-name lookup to Reactions",
            "SubscribeToStimulus": "requires concrete sensor and abstract stimulus, adds script-message receptron",
            "UnsubscribeToStimulus": "requires concrete sensor and abstract stimulus, removes script-message receptron",
            "BeginContact": "delegates to ScriptContact periodic propagator after nonzero assertions",
            "EndContact": "delegates to ScriptContact periodic propagator after nonzero assertions",
            "SetSingleSensorContact": "delegates exclusive sensor contact to periodic propagator after nonzero assertions",
        }
        if act_method in act_review:
            status = "Source implementation reviewed; event/parameter/runtime parity unverified"
            evidence = "`src/script/arscript.cpp`: " + act_review[act_method]
            if squirrel_act:
                evidence += "; CallActReact forwarding/name conversion exists"
        elif act_method == "GetReactionName":
            status = "Partial implementation; invalid-ID safety unresolved"
            evidence = "arscript.cpp dereferences DescribeReaction(id) without null check; CallActReact forwards"
        elif act_method == "StimulateLegacy":
            status = "THIEF1-specific entry point absent; restored host uses Thief II ABI"
            evidence = "arscript.cpp exposes source-aware Stimulate; legacy guest signature needs ABI verification"
    light_method = name.rsplit(".", 1)[-1]
    native_light = doc.parent.name == "script" and doc.name == "Light.txt"
    squirrel_light = doc.name == "API-reference_services.txt" and name.startswith("Light.")
    if native_light or squirrel_light:
        light_review = {
            "SetMode": "changes mode/countdown and activates animation; equal brightness limits make TimeLeft divide by zero",
            "Activate": "sets animation.inactive FALSE on resolved animated-light property",
            "Deactivate": "sets animation.inactive TRUE on resolved animated-light property",
            "Subscribe": "sets notify_script to light object's own ID",
            "Unsubscribe": "clears notify_script",
            "GetMode": "returns animation mode or -1 when animated-light property is absent",
        }
        if light_method in light_review:
            status = "Source implementation reviewed; property/callback/visual parity unverified"
            evidence = "`src/script/litscrpt.cpp`: " + light_review[light_method]
            if squirrel_light:
                evidence += "; CallLight forwards to native service"
        elif light_method == "Set":
            status = "Partial: supplied brightness range not stored"
            evidence = "litscrpt.cpp SetCore uses min/max for countdown but never assigns animation.min_brightness/max_brightness; equal limits divide by zero"
    dark_method = name.rsplit(".", 1)[-1]
    native_dark = doc.parent.name == "script" and doc.name == "DarkGame.txt"
    squirrel_dark = doc.name == "API-reference_services.txt" and name.startswith("DarkGame.")
    if native_dark or squirrel_dark:
        dark_review = {
            "KillPlayer": "sets player mode dead; PLAYTEST cheat/no_endgame can suppress it",
            "EndMission": "calls UnwindToMissionLoop; PLAYTEST cheat/no_endgame can suppress it",
            "FadeToBlack": "starts black fade-out after seconds-to-milliseconds conversion; cheat guard can suppress it",
            "FoundObject": "increments secret count and clears hidden-stat bit once",
            "ConfigIsDefined": "delegates to config_is_defined without Engine service null guard",
            "ConfigGetInt": "delegates to config_get_int",
            "ConfigGetFloat": "delegates to config_get_float",
            "GetAutomapLocationVisited": "delegates to DarkAutomapGetLocationVisited",
            "SetAutomapLocationVisited": "calls DarkAutomapSetLocationVisited and returns S_OK",
            "SetNextMission": "delegates to global SetNextMission",
            "GetCurrentMission": "returns GetMissionData num or zero if unavailable",
            "FadeIn": "starts player-mode fade-in after seconds-to-milliseconds conversion",
        }
        if dark_method in dark_review:
            status = "Source implementation reviewed; mission/UI/runtime parity unverified"
            evidence = "`src/dark/drkscrs.cpp`: " + dark_review[dark_method]
            if squirrel_dark:
                evidence += "; CallDarkGame forwards to native service"
        elif dark_method == "BindingGetFloat":
            status = "Partial implementation; unsafe input/result handling"
            evidence = "drkscrs.cpp sprintf into 255-byte buffer and strtof on unchecked binder result; CallDarkGame forwards"
        elif dark_method == "RespawnPlayer":
            status = "Single-player false fallback; multiplayer behavior absent"
            evidence = "drkscrs.cpp explicitly returns FALSE; documented effect is multiplayer-only"
    if doc.name == "API-reference_services.txt":
        data_review = {
            "Data.GetString": "datascrs.cpp delegates table/name/default/relative path to GameStrings FetchString",
            "Data.GetObjString": "datascrs.cpp delegates object/table lookup to GameStrings FetchObjString",
            "Data.DirectRand": "datascrs.cpp returns engine Rand output",
            "Data.RandFlt0to1": "datascrs.cpp scales masked 15-bit random value to [0,1), excluding upper endpoint",
            "Data.RandFltNeg1to1": "datascrs.cpp scales masked 15-bit random value to [-1,1), excluding upper endpoint",
        }
        if name in data_review:
            status = "Source implementation reviewed; resource/random runtime parity unverified"
            evidence = "`src/script/`: " + data_review[name] + "; CallData forwarding exists"
        elif name == "Data.RandInt":
            status = "Partial implementation; wide-range integer overflow unresolved"
            evidence = "datascrs.cpp computes signed high-low+1 and modulo range, low>=high returns low; CallData forwards"
        elif name == "Puppet.PlayMotion":
            status = "Partial implementation; success does not prove motion started"
            evidence = "pupscrpt.cpp uses AI script ability or cerebellum; returns TRUE even without either; CallPuppet forwards"
        elif name == "PGroup.SetActive":
            status = "Source implementation reviewed; missing-group result ambiguous"
            evidence = "pgrpscpt.cpp changes pg->active if group exists, always returns TRUE through HRESULT; CallPGroup forwards"
    native_overlay = doc.parent.name == "script" and doc.name == "DarkOverlay.txt"
    squirrel_overlay = doc.name == "API-reference_services.txt" and name.startswith("DarkOverlay.")
    if native_overlay:
        status = "Missing native DarkOverlay service/handler interface"
        evidence = "No native service registration or OSM guest bridge; Squirrel CallDarkOverlay is separate and does not implement native ABI"
    if squirrel_overlay:
        overlay_review = {
            "AddHandler": ("Partial callback implementation; UI lifecycle incomplete",
                           "retains callback handle and sets UI dirty; resize/menu-resume invalidation not wired"),
            "RemoveHandler": ("Partial callback implementation; UI lifecycle incomplete",
                              "removes handle from handler vector and sets UI dirty"),
            "GetBitmap": ("Partial resource loading; freed slots not reused",
                          "binds/locks image and retains resource, but 128 limit counts vector slots including flushed entries"),
            "FlushBitmap": ("Partial resource release; slot reuse missing",
                            "releases resource and clears pointer without reclaiming handle slot"),
            "GetBitmapSize": ("Source implementation reviewed; runtime/output parity unverified",
                              "returns retained dimensions or zeros for invalid handle"),
            "CreateTOverlayItem": ("Partial bitmap item; alpha/transparency semantics missing",
                                   "allocates bitmap canvas with BMF_TRANS, ignores trans_bg, stores alpha without drawing it; 64 vector-slot cap"),
            "CreateTOverlayItemFromBitmap": ("Partial bitmap item; alpha/transparency semantics missing",
                                             "copies bitmap into new canvas, ignores trans_bg, stored alpha unused by drawing"),
            "DestroyTOverlayItem": ("Partial resource release; slot reuse missing",
                                    "closes/frees canvas bitmap and clears pointer, leaves consumed vector slot"),
            "UpdateTOverlayAlpha": ("Missing visible alpha behavior",
                                    "updates item.alpha but RenderOverlays never consumes it"),
            "UpdateTOverlayPosition": ("Source implementation reviewed; visual parity unverified",
                                       "updates item x/y used by RenderOverlays"),
            "UpdateTOverlaySize": ("Source implementation reviewed; visual parity unverified",
                                   "clamps draw dimensions and uses gr_scale_bitmap without resizing backing canvas"),
            "DrawBitmap": ("Source implementation reviewed; phase/visual parity unverified",
                           "locks image, draws into current software canvas and unlocks"),
            "DrawSubBitmap": ("Source implementation reviewed; phase/visual parity unverified",
                              "clamps source rectangle, creates sub-bitmap and draws current canvas"),
            "SetTextColor": ("Source implementation reviewed; font/visual parity unverified",
                             "packs RGB into gr_make_screen_fcolor and sets current canvas colour"),
            "GetStringSize": ("Source implementation reviewed; font/output parity unverified",
                              "queries gr_string_size into short dimensions"),
            "DrawString": ("Source implementation reviewed; phase/font parity unverified",
                           "draws text using gr_string on current canvas"),
            "DrawLine": ("Source implementation reviewed; phase/visual parity unverified",
                         "draws using gr_int_line and current canvas colour"),
            "FillTOverlay": ("Partial fill; per-pixel alpha ignored",
                             "clears update canvas using palette colour, never reads alpha argument"),
            "BeginTOverlayUpdate": ("Partial canvas update; lifecycle safety unverified",
                                     "pushes item canvas and keeps pointer into growable item vector; creation during update can invalidate pointer"),
            "EndTOverlayUpdate": ("Source implementation reviewed; lifecycle parity unverified",
                                  "pops current update canvas and clears pointer; RenderOverlays also closes unfinished update"),
            "DrawTOverlayItem": ("Partial software drawing; transparent 3D overlay behavior missing",
                                 "marks item then gr_bitmap/gr_scale_bitmap draws retained canvas, no item alpha blending into 3D buffer"),
        }
        method = name.rsplit(".", 1)[-1]
        if method in overlay_review:
            status, evidence = overlay_review[method]
            evidence = "`squirrelhost.cpp` CallDarkOverlay/RenderOverlays: " + evidence
    if doc.parent.name == "script" and doc.name == "IScriptMan.txt":
        manager_review = {
            "SendMessage": "sets dispatch time, handles proxy/owner flags, active targets/broadcast and deferred actions",
            "SendMessage2": "wraps three data parameters in sScrMsg and calls synchronous SendMessage",
            "PostMessage": "AddRefs persistent message and appends posted queue",
            "PostMessage2": "wraps sender/target/data/flags and calls PostMessage",
            "SetTimedMessage": "AddRefs message, stores deadline/period and monotonic timer ID",
            "SetTimedMessage2": "builds sScrTimerMsg and delegates timer scheduling",
            "KillTimedMessage": "finds ID, releases message/timer and removes entry",
            "PumpMessages": "guards reentrancy, snapshots/refcounts timers and detaches posted queue; returns zero regardless of deliveries",
            "IsScriptDataSet": "checks tagged script-data hash entry",
            "GetScriptData": "copies stored multi-parameter, returns S_FALSE when absent",
            "SetScriptData": "creates tagged entry if missing and assigns owned multi-parameter value",
            "ClearScriptData": "copies old value then removes tagged entry, S_FALSE if absent",
        }
        manager_review.update({
            "GameInit": "exposes complete service table before Init calls, then acquires ScriptNet",
            "GameEnd": "ends services, releases networking, clears modules/objects and frees module datapath",
            "SetModuleDatapath": "copies supplied Datapath for module lookup",
            "AddModule": "normalizes .osm name, disconnects objects, replaces module, reloads classes/reconnects and sends BeginScript; rejects script reentry by assertion",
            "RemoveModule": "disconnects/reloads/reconnects after freeing named module, S_FALSE if absent; script reentry guarded by assertion",
            "ClearModules": "disconnects objects, clears descriptors and frees modules; script reentry guarded by assertion",
            "ExposeService": "AddRefs service into GUID table; duplicate GUID guarded by assertion",
            "GetService": "AddRefs matching service or falls back to NullScript service for unknown GUID",
            "GetFirstClass": "allocates hash iterator and returns first class descriptor",
            "GetNextClass": "advances class hash iterator",
            "EndClassIter": "deletes iterator and clears opaque handle",
            "GetClass": "searches class descriptors by name",
            "SetObjScripts": "defers changes for actively dispatched object, reconnects class list and dispatches lifecycle/clears removed-class data",
            "ForgetObj": "defers self-removal during dispatch; otherwise sends EndScript and removes target queues/data",
            "ForgetAllObjs": "defers during dispatch, otherwise forgets objects, releases remaining queues/data and resets timer IDs",
            "BeginScripts": "guards duplicate begin, enables and broadcasts BeginScript",
            "EndScripts": "guards duplicate end, disables and broadcasts EndScript",
            "AddTrace": "adds/replaces object/message trace hash entry",
            "RemoveTrace": "removes matching object/message trace entry",
            "GetTraceLineMask": "returns current integer trace-line mask",
            "SetTraceLineMask": "replaces trace-line mask",
            "GetFirstTrace": "allocates trace hash iterator",
            "GetNextTrace": "advances trace hash iterator",
            "EndTraceIter": "deletes iterator and clears opaque handle",
            "PostLoad": "clears BeginScript filtering state and pending filter list",
        })
        if name in manager_review:
            status = "Source implementation reviewed; lifecycle/reentrancy/runtime parity unverified"
            evidence = "`libsrc/script/scrptman.cpp`: " + manager_review[name]
        elif name == "WantsMessage":
            status = "Partial query: object-level script presence only"
            evidence = "scrptman.cpp ignores message-name argument and checks active object table"
        elif name in {"GetTraceLine", "SetTraceLine"}:
            status = "Partial trace control; unchecked shift index"
            evidence = "scrptman.cpp shifts 1 by supplied trace-line index without validating integer bit range"
        elif name == "SaveLoad":
            status = "Persistence implementation present; input/lifecycle parity unverified"
            evidence = "scrptman.cpp serializes queues, timers and script data; loaded counts resize arrays without visible bounds checks or propagated IO status"
        elif name in {"GetQueuedMessageCount", "GetQueuedMessageInfo", "DeleteQueuedMessage",
                      "GetTimedMessageCount", "GetTimedMessageInfo", "GetFirstScriptData",
                      "GetNextScriptData", "EndScriptDataIter", "GetFirstScriptObject",
                      "GetNextScriptObject", "EndScriptObjectIter", "GetModuleCount", "GetModule",
                      "GetAllScriptParams", "HasScriptParam", "GetScriptParam"}:
            status = "Missing IScriptMan interface addition"
            evidence = "scrptapi.h/scrptman.h/cpp lack documented 1.28 methods; internal helpers do not expose these vtable entries"
    native_gaps = {
        "AnimTexture.txt": ({"ChangeTextureLoc", "ChangeAllTextures", "ChangeSingleTexture"},
                            "src/render/antxscpt.cpp", "not exposed by the Squirrel adapter"),
        "Property.txt": ({"PossessedSimple", "GetIndexed", "SetIndexed", "SetLocalIndexed"},
                         "src/script/propscpt.cpp", "not exposed by the Squirrel adapter"),
        "Quest.txt": ({"BinSet", "BinGet", "BinExists", "BinDelete", "GetFirst", "GetNext"},
                      "src/script/quesscpt.cpp", "Squirrel binary-variable helpers exist separately; native parity missing"),
        "Object.txt": ({"ObjectToWorld", "WorldToObject", "CalcRelTransform", "Archetype",
                        "SetArchetype", "FindDescendentsSpatial", "FindScriptDonor", "AnyExist", "GetAll", "GetOne"},
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
              "See `newdark-script-contracts.md` for full overloads, guards and version annotations;",
              "those documented declarations still require ABI/behavioral review.", "",
              "| ID | Reference | Requirement | Source candidates / review evidence | Status |",
              "| --- | --- | --- | --- | --- |"]
    number = 0
    for folder in ("DOC", "Docs"):
        root = args.game / folder
        for doc in sorted(root.rglob("*")):
            if not doc.is_file() or (doc.name != "font-sample.fon" and doc.suffix.lower() not in {".txt", ".cfg", ".ini", ".dml", ".sub", ".rtf", ".odt", ".doc"}):
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
