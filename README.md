# Thief: Dark Engine Restored for Windows, Linux, and Android

This repository restores the historical Dark Engine sources as a portable,
working codebase for the Thief and Thief 2 games. It builds the ThiefRunner
game executable on Windows, Linux, and Android, plus a work-in-progress DromEd
mission viewer and editor on Windows and Linux. The engine core is shared
across platforms; operating-system, rendering, input, audio, movie, and
filesystem services are supplied by platform backends. The engine
experimentally implements many of the anonymously created but closed-source
NewDark extensions.

The repository does not include retail missions, movies, sounds, scripts, or
other game assets. A legally obtained installation of *Thief: The Dark
Project*, *Thief Gold*, or *Thief II: The Metal Age* is required.

> **Development disclosure:** This restoration has made extensive use of
> supervised AI coding assistants, including OpenAI Codex, for implementation,
> porting, debugging, testing, and documentation. A human maintainer directs
> the work, reviews the resulting changes, and remains responsible for what is
> accepted into the project. AI-assisted changes receive the same build and
> runtime validation as other changes, but that review cannot guarantee the
> absence of defects.

## Table of contents

- [Screenshots](#screenshots)
- [Platform status](#platform-status)
  - [Mission compatibility and goals](#mission-compatibility-and-goals)
  - [Cross-platform architecture](#cross-platform-architecture)
  - [Game runner features](#game-runner-features)
- [Building and running](#building-and-running)
  - [Windows](#windows)
  - [Linux](#linux)
  - [Android](#android)
- [Legacy OSM diagnostics](#legacy-osm-diagnostics)
- [Content selection and diagnostics](#content-selection-and-diagnostics)
- [Game saves](#game-saves)
- [Game settings](#game-settings)
- [Controls](#controls)
- [DromEd](#dromed)
  - [DromEd settings](#dromed-settings)
  - [Mission and game-system files](#mission-and-game-system-files)
- [Command console](#command-console)
- [Known limitations and remaining work](#known-limitations-and-remaining-work)
- [Preservation and open access](#preservation-and-open-access)

## Screenshots

These captures were produced during local runtime verification with external
retail game data. The data shown in the captures is not included in the
repository.

### Gameplay and inventory HUD

![Thief 2 gameplay and inventory HUD](docs/screenshots/gameplay-inventory.png)

### In-game menu

![Thief 2 in-game menu](docs/screenshots/in-game-menu.png)

### Automap

![Thief 2 automap](docs/screenshots/automap.png)

## Platform status

| Capability | Windows x86 | Windows x64 | Linux x64 | Android ARM64 |
| --- | --- | --- | --- | --- |
| Thief 2 | Playable | Playable; broader validation ongoing | Playable early port | Playable early port |
| Thief Gold/TDP | Playable; less stable than Thief 2 | Playable; less stable than Thief 2 | Playable early port; less stable than Thief 2 | Playable early port; less stable than Thief 2 |
| Thief FMs | Playable subset; less stable than Gold/TDP | Playable subset; less stable than Gold/TDP | Playable subset; less stable than Gold/TDP | Playable subset; less stable than Gold/TDP |
| System Shock 2 | Unsupported; source available | Unsupported; source available | Unsupported; source available | Unsupported; source available |
| DromEd | Viewer and playtest | Viewer and playtest; broader validation ongoing | Early viewer/playtest port | Not built |
| Graphics | D3D11 | D3D11 | SDL2/OpenGL | SDL2/OpenGL ES 3 |
| Audio | DirectSound 3D or OpenAL | DirectSound 3D or OpenAL | OpenAL Soft | OpenAL Soft/OpenSL |
| Movies | Media Foundation with legacy fallbacks | Media Foundation with legacy fallbacks | FFmpeg | LibVLC |
| Legacy `.osm` scripts | Native PE32 modules | IA-32 compatibility runtime | Partial IA-32 compatibility runtime | Partial IA-32 compatibility runtime |
| Squirrel scripts | Native Squirrel 3.2 provider | Native Squirrel 3.2 provider | Native Squirrel 3.2 provider | Native Squirrel 3.2 provider |
| Build system | Visual Studio 2022 | Visual Studio 2022 | CMake and Ninja | Gradle, CMake, and Android NDK |

The legacy `.osm` row refers to ordinary compiled mission script modules.
NewDark's `squirrel.osm` is not loaded through that compatibility path: on all
four targets it is replaced by the engine's native Squirrel 3.2 provider,
which loads the FM's loose `sq_scripts/*.nut` sources directly.

The source tree also contains historical System Shock 2 code, but this project
does not currently provide or validate a System Shock 2 build target.

The x86 builds are the established target; native x64 builds compile and run
but remain under active runtime validation. In particular, x64 relies on the
IA-32 interpreter and service bridge for 32-bit mission `.osm` binaries, while
many NewDark FMs rely on the newer Squirrel provider. Both compatibility
layers need broader mission coverage. The Linux CMake targets build native
64-bit ELF game and editor executables that can load retail missions.

The game runner retains mission loading, the game UI, inventory, automap, and
the in-game command console. On Windows it also retains the normal startup and
movie flow. DromEd can load, inspect, and playtest legacy Thief 2 missions in
a resizable window, but should still be treated as a viewer rather than a
dependable editor. Editing and mission-authoring integrity are not
comprehensively validated, and NewDark formats and features are only partially
supported.

### Mission compatibility and goals

`ThiefRunner.exe` replaces the retail `Thief2.exe`. It is more than a straight
port: the same runner supports all three Thief releases and selected NewDark
content, while continuing to read an existing installation in place. It is
not a complete NewDark implementation. Compatibility currently breaks down as
follows:

- **Thief II** is the primary and most-tested target. The stock campaign is
  playable on every supported platform; Windows x86 remains the reference
  build, with Windows x64, Linux, and Android receiving progressively broader
  mission and script validation.
- **Thief Gold and The Dark Project** stock missions load through the same
  runner, with the Dark 1 service layout selected for legacy scripts. Gold has
  passed initial runner and DromEd smoke tests. TDP and the complete Gold
  campaign and UI still need broader coverage.
- **Fan missions** using retail-era data and legacy `.osm` scripts are a core
  compatibility goal. FM-local configuration, NewDark-style path precedence,
  PNG replacement art, and Ogg Vorbis sounds are supported. Individual FMs may
  still expose missing script services, formats, or raised engine limits;
  `darkengine-diagnostics.log` and `mission_compatibility.log` record those
  failures explicitly.
- **NewDark content** is partially supported, not promised compatible. The
  runner understands selected configuration conventions, resources, database
  extensions, and loose Squirrel 3.2 sources, but it does not implement the
  complete NewDark service, renderer, object/property, or authoring surface.

Game-save coverage and format limitations are documented separately under
[Game saves](#game-saves).

The near-term goal is dependable play of all three stock Thief releases and
retail-format FMs, followed by measured expansion of NewDark FM compatibility.
NewDark-only data should be diagnosed or preserved where possible rather than
silently discarded. DromEd remains a viewer/playtest and preservation tool
until its editing and round-trip behavior is comprehensively validated.

### Cross-platform architecture

The shared renderer sends the same engine drawing commands to D3D11 on Windows
and to SDL2/OpenGL on Linux and Android; Android uses OpenGL ES. General
platform backends live under `winsrc` and `unixsrc`; Android intentionally uses
the Unix/SDL backend. Shared editor logic lives under `src/editor`, while its
platform providers live under `winsrc/editor` and `unixsrc/editor`. Shared
input interfaces and keyboard tables live under `libsrc/input`. The game and
editor continue to use Looking Glass's `lgd3d` interface.

For now CMake derives the common source inventory from the established Visual
Studio projects, then selects the game or editor platform providers explicitly.
Desktop Linux always builds `dromed` alongside `thief-runner`: a Linux DromEd
that can load a retail mission, construct its editor UI, and render a portal
viewport is a portability requirement, not an optional future target. Android
shares the Unix/SDL runtime but does not build the desktop editor.

Core clocks, dynamic-library loading, memory queries, assertions, clipboard,
synchronization, and filesystem operations cross `platform_services.h` and
`platform_io.h`. The shared engine retains its historical data layouts while
the compatibility headers isolate Windows and LP64 ABI differences.

Native 64-bit builds include a compatibility runtime for legacy 32-bit `.osm`
script modules. It reads PE32 modules, interprets IA-32 code, and bridges the
supported script-manager and engine-service calls into the host process.
Every game build provides an in-process Squirrel 3.2 script module for
NewDark-style `sq_scripts/*.nut` sources. It uses the normal Dark script
manager and service interfaces in place of the game-specific NewDark
`squirrel.osm` binary. Both bridges are still being expanded and validated
against third-party scripts.

Audio follows the same provider model. Windows uses DirectSound 3D by default,
with native I3DL2 environmental reverb. An OpenAL provider supplies positional
audio, EFX reverb, and per-source occlusion when `OpenAL32.dll` is available;
set `sfx_backend openal` in `cam.cfg` to select it explicitly. The default
`auto` setting tries DirectSound first and uses OpenAL only if native audio
cannot initialize, so the standard executable has no OpenAL dependency.

Movie playback is exposed through the platform-neutral engine movie API. The
Windows Media Foundation, DirectShow, and legacy DirectDraw implementation is
isolated under `winsrc/movie`; Linux uses FFmpeg for video/audio decoding and
the existing SDL/OpenGL presentation surface. Android uses LibVLC on a native
Android surface while retaining the engine's synchronous movie lifecycle.

Legacy PKWARE DCL-compressed resources are decoded in-process with Mark
Adler's small `blast` decoder under `3rdparty/blast`; newly written resources
use the engine's existing LZW codec. No external compression DLL is required.

### Game runner features

The runner currently provides:

- use of an external retail Thief installation without copying its data;
- automatic TDP, Gold, Thief II, and current GOG Gold collection-layout
  detection from the working directory;
- native display-resolution defaults when the user has not configured a
  resolution;
- borderless fullscreen and resizable windowed presentation;
- the standard Windows splash, startup, movie, and main-menu flow;
- direct mission startup and unattended frame-limited smoke tests; and
- a configurable command console, defaulting to `:` and `;` in every mode.

## Building and running

### Windows

#### Prerequisites

- 64-bit Windows 10 or Windows 11.
- Visual Studio 2022 or Visual Studio 2022 Build Tools with the **Desktop
  development with C++** workload, MSVC v143 x86/x64 tools, and a Windows 10
  or 11 SDK.
- Git, used by the dependency bootstrap.
- A retail TDP, Thief Gold, or Thief II installation containing `cam.cfg` and
  the stock mission data.
- Optional: `ffmpeg.exe` on `PATH` for stock Indeo 5 AVIs when Windows has no
  compatible DirectShow decoder. H.264 MP4 siblings do not require it.

Normal game builds include the native Squirrel 3.2 provider, and normal DromEd
builds include the KCTools v1.3.4 sidecar. Both are external source checkouts;
bootstrap the pinned versions with:

```powershell
.\tools\bootstrap-dependencies.ps1
```

The default locations are `../../darkengine-deps/squirrel` and
`../../darkengine-deps/kctools`. Use `-Target Game` or `-Target DromEd` to
bootstrap only one variant. DromEd also requires the .NET 10 SDK to publish its
self-contained KCTools sidecar. Override the checkouts with
`/p:SquirrelSourceDir=...` or `/p:KCToolsSourceDir=...`, and override `dotnet`
with `/p:DotNetExecutable=...`.

Both integrations can be omitted only by opting out explicitly:
`/p:EnableSquirrel=false` for the game or `/p:EnableKCTools=false` for DromEd.
Omitting those properties enables the corresponding integration.

Ogg Vorbis FM sounds are loaded through libsndfile at runtime. To package it on
Windows, place the official architecture-matched libsndfile 1.2.2 release in
the default `../../darkengine-deps/libsndfile-1.2.2-win32` or `-win64` folder
and build with `/p:EnableSndFileRuntime=true`; use
`/p:SndFileRuntimeDir=C:\path\to\bin` for another location.

#### Build

In Visual Studio, open `thief2.sln`, select `Release` and `x86`, then choose
**Build > Build Solution**. From a regular PowerShell prompt, the equivalent
explicit commands are:

```powershell
$msbuild = 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe'
& $msbuild src\dromed.vcxproj /m /t:Build /p:Configuration=Release /p:Platform=Win32 /p:BuildThief2=true
& $msbuild src\dromed.vcxproj /m /t:Build /p:Configuration=Release /p:Platform=Win32 /p:BuildThief2=false
```

Use `Platform=x64` for native 64-bit builds. Outputs are separated by
architecture and application:

```text
build\Release\Win32\ThiefRunner\ThiefRunner.exe
build\Release\Win32\DromEd\DromEd.exe
build\Release\x64\ThiefRunner\ThiefRunner.exe
build\Release\x64\DromEd\DromEd.exe
```

The game and editor share `src\dromed.vcxproj`; `BuildThief2` selects their
different source sets. Static libraries are shared within an architecture and
must not vary with that property.

#### Running

Run from the retail directory, or launch elsewhere and choose the installation
when prompted. For the current GOG Gold package, either the common **Thief
Gold** collection folder or one of its runnable child game folders can be
selected:

```powershell
Set-Location 'C:\Games\Thief Gold'
& 'C:\src\darkengine\build\Release\Win32\ThiefRunner\ThiefRunner.exe'
```

The chosen game directory is used in place; retail data is not copied. The picker
remembers only a starting-folder hint under
`HKCU\Software\DarkEngineRestored\Launcher`. Use Dark-style assignments such
as `quickstart=MISS1.MIS` or `auto_stop=60`; there are no GNU `--` prefixes.

### Linux

#### Prerequisites

- A 64-bit Linux distribution, GCC, CMake 3.20 or later, and Ninja.
- Git, used to bootstrap Squirrel 3.2.
- SDL2, OpenGL, and libpng development packages.
- OpenAL Soft and libsndfile at runtime. Both are loaded dynamically; libsndfile
  enables Ogg Vorbis sounds used by many FMs.
- FFmpeg development libraries (`libavformat`, `libavcodec`, `libavutil`,
  `libswscale`, and `libswresample`) for movie playback. The build succeeds
  without them but reports movies unavailable.
- A retail TDP, Thief Gold, or Thief II installation.

For Ubuntu/Debian:

```sh
sudo apt update
sudo apt install build-essential cmake ninja-build libsdl2-dev libgl-dev \
  libpng-dev libopenal1 libsndfile1 libavformat-dev libavcodec-dev \
  libavutil-dev libswscale-dev libswresample-dev
```

#### Build

```sh
sh tools/bootstrap-dependencies.sh game
cmake -S . -B build-linux -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-linux -j "$(nproc)"
```

The results are `build-linux/thief-runner` and `build-linux/dromed`. Desktop
OpenGL 3.3 is the default. Install GLES development headers and configure with
`-DDARK_OPENGL_ES=ON` to exercise the OpenGL ES 3 path. Normal builds include
the native Squirrel provider. Use `-DDARK_SQUIRREL_SOURCE=/path/to/squirrel`
for a non-default checkout, or explicitly pass `-DDARK_ENABLE_SQUIRREL=OFF`
for a reduced build without NewDark Squirrel FM support.

#### Running

The launcher accepts the current directory, `DARK_GAME_PATH`, or
`game_path=<directory>`:

```sh
cd "$HOME/Games/Thief 2"
/path/to/darkengine/build-linux/thief-runner quickstart=MISS1.MIS
```

Run DromEd from the same retail directory. It accepts the same editor command
line used on Windows, including deterministic viewport inspection:

```sh
/path/to/darkengine/build-linux/dromed file=MISS1.MIS
/path/to/darkengine/build-linux/dromed render_test=MISS1.MIS \
  render_test_pos=37.43,100.15,18.97 render_test_angles=-35,90,0
```

The Linux editor uses SDL for its window and input, SDL message boxes for
editor prompts, and `zenity` when available for graphical Open/Save dialogs;
it falls back to a terminal path prompt. The in-engine editor panes, command
console, and keyboard bindings work, but the Unix provider does not yet supply
a desktop application menu bar. Use the command console and shortcuts for
commands that are menu-only on Windows.

An interactive launch from another directory opens the folder chooser. SDL
uses relative-pointer input during gameplay and absolute coordinates in menus,
with native Wayland preferred when available. See [Game saves](#game-saves) for
the save location and fallback rules.

### Android

#### Prerequisites

- JDK 17 or later.
- Android SDK platform and build tools 36.
- Android SDK CMake 3.31.6 and NDK 30.0.16248370.
- Git, used to bootstrap Squirrel 3.2.
- An ARM64 device running Android 8.0 (API 26) or later and a retail game
  folder available to Android's system folder picker.

#### Build

Bootstrap Squirrel, then use the checked-in Gradle wrapper:

```powershell
.\tools\bootstrap-dependencies.ps1 -Target Game
Set-Location android
.\gradlew.bat assembleDebug
```

On a Linux host, use `sh tools/bootstrap-dependencies.sh game`, then
`cd android && ./gradlew assembleDebug`. Gradle downloads SDL 2.32.10, OpenAL
Soft 1.25.2, libpng 1.6.59, libogg 1.3.5, and libvorbis 1.3.7 from their
official releases and verifies pinned SHA-256 digests. LibVLC 3.6.5 is resolved
through Maven. The APK is written under
`android/app/build/outputs/apk/debug`.

#### Running

Install the APK and select the folder that directly contains `cam.cfg` and the
retail missions. The launcher can use that folder in place after **All files
access** is granted, or import an optional copy into app-specific storage.
Long-press the app icon and choose **Choose game folder** to switch or refresh
content. Touch controls, keyboard, mouse, and SDL-compatible Bluetooth, USB,
or built-in gamepads remain active together.

See [`docs/android.md`](docs/android.md) for installation, storage modes,
direct-mission debugging, and touch-control details.

## Legacy OSM diagnostics

The portable OSM loader selects a fixed guest ABI when each module is loaded:
`gold` and `tdp` use the Dark 1 service layout, while `thief2` uses Dark 2.
Set `DARK_OSM32_TRACE=1` to log the selected ABI, service calls, and imports
that are not implemented by the guest runtime.

`osm32_probe` validates a PE32 script module without executing it. Use
`--imports` to list its imports or `--strict-imports` to fail when any import
is outside the runtime's supported manifest:

```powershell
.\build\Release\x64\osm32_probe.exe --strict-imports `
  'C:\Games\Thief Gold\SCRIPT.OSM'
```

The probe checks the PE image, required `_ScriptModuleInit@20` export,
relocations, and import surface. It does not replace an in-mission behavior
test because service calls and message traffic depend on retail mission data.

## Content selection and diagnostics

The runner identifies TDP, Gold, or Thief II from the installed stock mission
set. It also recognizes GOG's current Gold collection layout: a common folder
containing complete `Thief Gold` and `Thief The Dark Project` child installs
plus a shared `FMs` folder. Selecting that common folder presents Gold and TDP,
each in base-resource and configured-mod form, followed by the shared fan
missions in the engine-owned Thief content menu. Shared FMs always run against
the Gold child; they are not duplicated as speculative TDP variants.

```text
Thief Gold\                         selectable collection folder
|-- Thief Gold\cam.cfg             selectable Gold game folder
|-- Thief The Dark Project\cam.cfg selectable TDP game folder
`-- FMs\                            shared fan missions (Gold runtime)
```

Either layout may be selected directly. Choosing the `Thief Gold` child of a
recognized GOG collection still discovers the parent's shared `FMs` folder;
choosing the TDP child is an explicit TDP campaign selection. The engine keeps
the selected runnable child as its working directory and the shared FM folder
as a separate content root. The common folder is also the final lookup location
for collection-level mission dependencies such as GOG's `TFIX.GAM`; it never
overrides a child, mod, or FM file. The engine does not copy files between the
folders and does not run from the non-game collection folder. A non-interactive
command launched from the collection folder also defaults to Gold, including
an explicit `fm=<folder>` launch.

If the initial working directory is not recognizable, the runner prompts for a
game or collection folder. The picker starts at the last successfully selected
folder, stored under `HKCU\Software\DarkEngineRestored\Launcher`; that value is
only a picker hint and is never used to choose or launch a game silently.
Retail data is read in place and is not copied.

A game-runner launch with no arguments opens the shared engine-owned, themed
content menu. For a single game folder it lists the two original-campaign
modes followed by every recognized immediate child of the installation's
configured `fm_path` directory (`FMs` by default). For the GOG collection it
lists those two modes for Gold and TDP, then the shared Gold FMs. Display names
and descriptions come from each FM's `fm.ini` when present. For example,
select **Thief: The Black Parade v1.0** and press **Play** to play the installed
Black Parade campaign. The menu is shown afresh on every ordinary interactive
launch; a previous campaign or FM selection is not applied automatically.

The available content modes are:

- **Original Missions (recommended)** suppresses both mod layers and uses the last existing
  entry in each base path, normally `RES` for resources and the installation
  root for missions. This is the most compatible mode for the original
  campaigns. Gold is listed first when a GOG Gold collection is selected.
- **Original Missions - Modded** honors `uber_mod_path`, `mod_path`, and the full
  paths in the installation configuration. Installation-wide `cam_ext.cfg`
  defaults are read before `cam.cfg`, and `cam_mod.ini` supplies NewDark-style
  mod/FM settings. Use it for an installed enhancement pack, with the caveat
  that it can expose NewDark features this engine does not support yet.
- **Fan mission** selects one immediate child directory of the installation's
  `FMs` folder. Its files are inserted between `uber_mod_path` and `mod_path`,
  matching NewDark path precedence. An FM's `DARK.CFG`, `fm.cfg`, and
  `cam_ext.cfg` are loaded when Play is chosen. Before the themed picker is
  shown, a narrow bootstrap pass scans those files only for fixed-capacity
  limits and reserves the largest installed reference and sound pools. Play
  before the first database is built. The picker pre-reserves the largest
  installed object, reference, and sound capacities, then Play applies the FM
  configuration and refreshes resource and script paths in place. The logical
  mode changes to the main menu while the top-level window and presentation
  surface remain persistent.

Dark model materials identify logical texture names rather than requiring one
image codec. The runner removes a material's historical image extension and,
within the highest-priority content directory, tries DDS, PNG, TGA, GIF, PCX,
CEL, and BMP in that order. This is why an FM can intentionally ship a PNG
replacement with the same basename as retail TGA, GIF, or PCX art; installed
NewDark FMs use that pattern. Explicit image-resource requests retain their
extension. Decoders also inspect the file signature so a supported image with
a misleading historical extension is decoded by its actual format. Indexed
animations retain their own palettes, arbitrary GIF transparency indices are
normalized to Dark's index-zero convention, and RGBA animation frames are
alpha-composited into the engine's 16-bit UI canvas.

Set `DARK_RESOURCE_TRACE=1` to include each resolved resource name, type, and
source path in `monolog`; this is useful for distinguishing a decoder problem
from an FM/mod/base path-precedence problem.

Every run also creates `darkengine-diagnostics.log` in the selected game
directory. It records the executable, architecture, content/FM selection,
mission database compatibility warnings, assertions, and every distinct
resource probe that could not be resolved when `DARK_RESOURCE_TRACE=1` or
`DARK_RESOURCE_MISS_TRACE=1`. Raw misses are opt-in because the engine normally
probes absent animation frames and alternate model formats while resolving a
valid asset. Immediately after mission post-load the
engine prints a conspicuous summary to `monolog` and writes a detailed
`texture-audit.log` covering terrain surfaces, object models, base textures,
and per-object replacements. Any texture that would reach the renderer's
checkerboard fallback is treated as an error. A missing texture resource is
therefore included in both the missing-resource and checkerboard-fallback
totals; a loaded resource whose decoded frame is already the dummy
checkerboard is only in the latter. Missing resources requested later during
play are appended to the same diagnostics report. Set `DARK_DIAGNOSTICS_PATH` to an
absolute filename to redirect the main report for unattended tests, and
`DARK_TEXTURE_AUDIT_PATH` to redirect the detailed texture inventory.

On Windows, an unhandled exception, pure virtual call, invalid C runtime
parameter, allocation failure, or C++ termination writes
`darkengine-crash.log` plus a timestamped `darkengine-crash-*.dmp`. The crash
report includes launch/content context, memory state, registers, a symbolized
stack when symbols are available, and the last 64 KiB of `monolog.txt`.
`DARK_CRASH_OUTPUT_DIR` redirects the crash log and dump for automated tests;
setting the environment-only `DARK_CRASH_REPORT_TEST=1` deliberately raises a
startup exception to verify the reporter without loading a mission.
Diagnostics deliberately do not attempt to continue after memory-corruption
or runtime-fatal failures; doing so would risk corrupting saves and obscure the
original fault.

Mission, FM, resource, property, and script data are treated as untrusted
input. Tag-file directories are validated completely before dispatch (bounded
counts, terminated tags, unique names, checked ranges, and matching block
headers), and decoders report required-data failures through one database-load
transaction. A failed transaction skips post-load, rebuilds a safe empty
database, and returns the retail game to its menu; optional asset and script
failures are written as `ERROR` diagnostics and use their subsystem fallback.
File sizes, allocation sizes, record ranges, and loading-progress calculations
use checked 64-bit arithmetic. Assertions remain for internal invariants, not
for values read from content files.

`tools\mission_load_regression.ps1` generates malformed synthetic missions
under `.codex-build` (no retail data is copied) and sends them through the
normal loading-panel and database-loader path. It fails unless each case is
rejected without entering gameplay and records a `mission-regression` PASS in
the redirected diagnostics log:

```powershell
.\tools\mission_load_regression.ps1 -GameDirectory 'C:\Games\Thief 2 1.29'
```

For shortcuts, scripts, and tests, bypass the chooser with explicit Dark-style
assignments (there are no GNU `--` prefixes):

```powershell
& $thief content=configured
& $thief content=vanilla
& $thief fm='TheDarkMansion'
```

PLAYTEST builds can exercise the complete mission-loop loading path without
manual input. Unlike `quickstart`, this retains the loading panel and database
progress proxy, mission preparation, player creation, and the transition into
gameplay; it skips only the briefing and loadout screens:

```powershell
& $thief content=configured `
  'fm=Reflections in Red [2025-11-14] T2 FM v1.1b (EN)' `
  mission_loop_test=20 auto_stop=120
```

Interactive launches require a fresh choice from the content menu. The
remembered game folder is used only to position the folder dialog; it never
selects a game, campaign mode, or FM on a later launch.

## Game saves

The in-game Save and Load panels use ordinary Dark tag-file saves. Numbered
slots are named `game0001.sav`, `game0002.sav`, and so on; the quick-save slot
is `quick.sav`, and the mission-start checkpoint is `startmis.sav`. A save is a
snapshot of campaign and current-mission state, but it still depends on the
same game, campaign or FM, gamesys, scripts, and external resources being
installed when it is loaded. Moving a save between Windows, Linux, and Android
is intended to work when that content context is identical, but cross-platform
and cross-version save compatibility is not yet comprehensively validated.

Save locations are deliberately isolated by detected game and selected
content:

- On Windows, new saves are written beneath
  `%LOCALAPPDATA%\DarkEngineRestored\Profiles\<game>\<content>\saves`. Existing
  saves in the retail installation's `saves` directory remain visible when a
  slot is absent from the active profile, but new saves do not appear there.
- On Linux and Android, the runner prefers the selected game directory's
  existing `saves` or `SAVES` directory when it is writable. Otherwise it uses
  `$XDG_DATA_HOME/DarkEngineRestored/Profiles/<game>/<content>/saves`, or the
  corresponding `$HOME/.local/share` location. An Android installation copied
  for faster loading therefore keeps its saves in the app-specific game copy;
  direct-folder mode keeps them beside that installation.
- `save_path=<directory>` or the `DARK_SAVE_PATH` environment variable selects
  an explicit save directory for launches and tests.

The retail tag-file structure is retained rather than replaced by a
project-specific save format. Retail Thief saves and the tested subset of
NewDark saves can be loaded, and newer source blocks that this engine cannot
interpret are retained where possible during a load/save cycle. This is not
complete NewDark compatibility: each block is versioned independently, and a
supported save can still contain one unsupported subsystem block.

In particular, NewDark's `VISITED` 4.0 campaign block uses an undocumented
encoding for its expanded 32-page by 256-location automap state. The current
engine does not decode that block, so explored automap locations may be reset
when loading a NewDark save. Saves written by this engine use the understood
retail `VISITED` 1.0 layout, limited to 8 pages by 64 locations. This automap
limitation does not by itself prevent the rest of a NewDark save from loading.
Do not yet assume that saves written here will round-trip every NewDark-only
feature or load in an original retail executable.

## Game settings

Configuration is resolved from the selected installation and content context.
`cam_ext.cfg` supplies installation-wide defaults, `cam.cfg` supplies the
user's base-game overrides, and `cam_mod.ini` supplies NewDark-style mod and FM
paths. Included game/editor configuration and the selected FM's `DARK.CFG`,
`fm.cfg`, and `cam_ext.cfg` are then applied as appropriate. Dark-style
command-line assignments such as `game_full_screen=0` are parsed last and take
priority for that launch.

On Windows, the game runner stores the Video Options resolution, display mode,
Fit choice, color depth, and gamma below
`HKCU\Software\DarkEngineRestored\Thief2`; this keeps runner-specific display
preferences out of the retail installation. Other writable configuration
continues to use the selected installation's `cam.cfg`. Control customization
is stored in `user.bnd`; see [Controls](#controls) for bindings and gamepad
settings.

If `screen_size` or `game_screen_size` is already defined in `cam.cfg`, that
size is retained when the display driver supports it. Otherwise, the game uses
the closest supported mode to the native resolution of the display under the
mouse pointer at startup. The Video Options list merges the active monitor's
current display modes with compatibility render sizes, including 640x480,
800x600, and widescreen equivalents that fit on that display. Resolutions are
listed with the highest first; use the arrow buttons or mouse wheel to reach
lower entries. Unsupported 24/32-bit NewDark settings are normalized to the
engine's 16-bit internal canvas. That canvas is converted to 32-bit color for
presentation; Windows is not switched into an obsolete 16-bit display mode.

The Video Options **Display Mode** toggle selects a borderless **Fullscreen**
window or a normal resizable **Windowed** window. Display-mode changes take
effect immediately; resolution changes are applied when **Done** is selected.
In Windowed mode, a selected resolution is the fixed internal render size;
resizing the window scales that canvas. Select **Fit** to make the 3D scene and
software overlay follow the window's actual client-pixel size. This keeps HUD,
tutorial, and console text pixel-aligned. The equivalent `cam.cfg` settings are
`game_full_screen 0|1` and `game_screen_fit 0|1`.

On a multi-monitor system, the display containing the game window determines
both the resolution list and the target for borderless fullscreen. To change
displays, switch to Windowed, move the window, reopen Video Options, and then
select a resolution or Fullscreen. The list is queried again for that display.
The engine's gamma setting is applied in the Windows D3D11 presentation shader
using the legacy palette-correction power curve.

The retail game writes `skip_intro` after the intro has played. Add
`always_play_intro` to `cam.cfg` to show it on every launch. When both movie
formats exist, the Windows runner prefers H.264 MP4 through Media Foundation.
If only the original Indeo 5 AVI exists and DirectShow cannot decode it, the
runner uses `ffmpeg.exe` to prepare an H.264/AAC copy beneath
`%LOCALAPPDATA%\DarkEngineRestored\MovieCache`; it never changes the retail
movie. Set `DARK_MOVIE_CACHE` to override that cache directory.

## Controls

This project's game runner replaces the original game's legacy DirectInput
joystick polling with a platform-neutral gamepad layer: runtime-loaded XInput
on Windows and SDL GameController on Linux and Android. SDL accepts compatible
controllers regardless of branding; the default bindings merely use the familiar
Xbox-style button names and layout. One controller owns gameplay input at a
time and can be connected, disconnected, or reconnected while the game runs.
The selected modern controller is excluded from the legacy joystick fallback,
avoiding duplicate input. Keyboard and mouse remain active on every platform,
and Android touch controls remain active alongside them and the gamepad.

The default layout follows the modern Thief radial-menu scheme. Left stick
moves and right stick looks; A jumps, B crouches, X uses the selected item, RT
attacks, and LT blocks. Tap Y to equip or holster the last weapon, or hold Y to
drop the selected item. Hold LB and point the left stick to lean left, right,
or forward. Hold RB for the native radial: the left stick selects weapons and
the right stick selects items, and both rings include a none slot. D-pad
up/down cycles weapons and left/right cycles items. L3 toggles the movement
speed mode, R3 forces sneak speed, View opens the automap, and Menu pauses.
L3+Y quicksaves and L3+B quickloads.

The radial menu currently uses text labels rather than the inventory artwork.
Those labels use the same density-aware readable-overlay font selection as
messages, `show_stats`, and the console. Windows obtains the scale from the
window's current monitor, Linux uses the SDL drawable/compositor scale with a
display-DPI fallback, and Android uses `scaledDensity`, including the user's
system font-size preference. Presentation magnification is removed before a
font is selected so text is not enlarged twice. The Windows compositor
point-samples the overlay at exact integer scales and retains linear filtering
for fractional scales and the 3D scene. Switching from fullscreen to windowed
defaults the resolution choice to `Fit`, making the logical canvas match the
settled client size; a fixed windowed resolution can still be selected
afterward. Set `gamepad_radial_font` to a font resource name to override it on
any platform; `overlay_font` overrides the automatic choice for all readable
overlays.

In menus, either stick moves the mouse pointer, A/X left-click, Y right-click,
and B or Menu goes back; the D-pad remains directional navigation. These
controls use semantic names such as `pad_a`, `pad_rt`, and `pad_right_x` in
`user.bnd`; existing `joy1` and `joy_axis*` bindings remain valid as fallbacks.

Gamepad behavior can be adjusted in `cam.cfg`:

```text
gamepad_enable 1
gamepad_backend auto
gamepad_index 0
gamepad_left_deadzone 0.18
gamepad_right_deadzone 0.14
gamepad_left_response 1.0
gamepad_right_response 1.35
gamepad_run_threshold 0.78
gamepad_fast_run_threshold 0.35
gamepad_lean_threshold 0.55
gamepad_trigger_press 0.15
gamepad_trigger_release 0.10
gamepad_look_sensitivity 1.0
gamepad_look_turn_seconds 1.0
gamepad_menu_cursor_speed 16.0
gamepad_hold_msec 400
gamepad_radial_time_scale 0.15
gamepad_look_invert_y 0
gamepad_rumble 1
```

`gamepad_backend` accepts `auto`, `xinput`, `legacy`, or `off` on Windows and
`auto`, `sdl`, `legacy`, or `off` on Linux, while Android uses SDL. One
configured controller owns gameplay input at a time; `gamepad_index` selects
it, with automatic failover to another connected controller if it disconnects. The
`gamepad_status` console command prints the selected backend and normalized
state. DromEd editor input remains unchanged; gamepad input is currently enabled
only in the game-runner target.

## DromEd

Windows provides the complete native application menu and common-dialog
provider. Linux uses the SDL/Unix provider described in the Linux running
instructions; editor logic and mission behavior are shared.

On Windows, DromEd uses the same working-directory detection as the game runner:

```bat
cd /d "C:\Games\Thief Gold"
"C:\src\darkengine\build\Release\Win32\DromEd\DromEd.exe"
```

The first prompt accepts either a named game's base folder (the runnable root
containing `cam.cfg`) or the recognized GOG Gold collection folder, but not an
`FMs` child. The next dialog is headed
**DromEd <game> - Select mission set or FM to work with** and chooses either
the base/modded game or one installed FM. DromEd then opens without prompting
for a mission. Its Open/Save dialogs default to that selected base or FM
directory; use **File > Open** to select the `.mis` or `.cow` to edit. For an
FM, the selected content context stays active while the mission opens, so its
referenced `.gam` and its `obj`, `mesh`, `fam`, `snd`, `strings`, and other
resource directories remain ahead of ordinary mods and retail data.

### DromEd settings

DromEd reads the selected installation's `cam_ext.cfg` and `cam.cfg`, followed
by its game and editor includes and any selected FM configuration. Put
editor-specific overrides in the editor configuration included by that stack;
command-line assignments still take priority for the current launch.

The DromEd viewer uses the same hardware scene API as the game: D3D11 on
Windows and OpenGL on Linux. Its legacy editor canvas remains 16-bit internally,
while the platform presenter converts it to the resizable desktop window.
When an editor window resize finishes, DromEd recreates that canvas at the
client area's actual dimensions and scales the four viewports, console,
status bar, and brush controls independently in each axis. This fills
widescreen and portrait windows without imposing a 4:3 aspect ratio.
`edit_screen_size` in `cam.cfg` selects the initial canvas size; it defaults
to 1024x768. The editor always uses a windowed desktop surface, regardless of
legacy fullscreen flags. Game preview mode stays in the same window and
recreates the D3D11 render context for the preview; returning to edit mode
restores the editor viewport and menu state. Linux uses the corresponding
SDL/OpenGL context and editor provider.

Use **Help > Keyboard Shortcuts** to open a resizable, human-readable input
reference. It includes every active editor binding (including the complete
built-in fallback set), non-conflicting shortcut annotations from the loaded
`menus.cfg`, and direct mouse/viewport gestures. Controls are grouped and
ordered by purpose, with modifier keys shown before the base key.
Editor binding overrides share the selected installation's `user.bnd`, using
the editor input context rather than replacing gameplay bindings.

DromEd keeps its scrollable command console visible in the lower-right editor
pane. Press `:` or `;` to give its command line keyboard focus; **Escape**
releases focus without hiding the pane. See [Command console](#command-console)
for its original behavior, restoration enhancements, controls, and added
commands.

The built-in historical editing command registry is present. KCTools lighting
is exposed directly as `kctools_light`, `kctools_portalize`, and
`kctools_optimize`; these commands perform a checked save/run/import
transaction without NewDark's command script or `kctools.osm`. They first use
the published sidecar beside DromEd, then fall back to the historical
`Tools\KCTools\KCTools.exe` location. `kctools_path` remains an explicit
override. Other companion-tool workflows and some familiar external commands
are still unavailable.

### Mission and game-system files

This build is therefore not yet a dependable mission-authoring environment.
Work from backups and do not rely on it as the only editor or copy of a
mission. DromEd inventories the version and size of every source chunk and
preserves that per-chunk target by default. WREXT 0.30, LM_PARAM 0.1, extended
RENDPARAMS layouts, CELL_MOTION 0.2, and NewDark's 28-face BRLIST records have
writable paths; untouched raw WREXT data is still replayed exactly. Unknown
chunks and unowned suffixes remain preservation-only, and AIPATH 3.4 remains
raw-preserved rather than regenerated, so this is not yet comprehensive
NewDark authoring support.

Use `db_format_report` to print the current source inventory. The
`db_write_profile preserve|retail|newdark` command selects an explicit output
policy; `preserve` is the default and is the only policy intended for normal
round-trip editing.

`save_mission` writes the mission partitions to `.mis`; `save_gamesys` writes
the abstract object/game-system partitions to `.gam`; and `save_cow` writes
both into an editor working file. None of them packages external FM resources.
Custom models, textures, samples, strings, movies, scripts, and motion data
remain files in their normal FM directories and must be distributed by the FM
author.

Sound schema source (`schema\*.sch`, `*.spc`, and `*.arc`) is likewise not
embedded as source text. `reload_schemas` parses those files into schema
objects and sample mappings; saving the gamesys persists that compiled object
data, while the audio samples themselves remain under `snd`. Keep custom
schema source with an authoring project so it can be rebuilt, and ship the
custom `.gam` plus every referenced sample. Motion schema source under
`mschema` is compiled separately into `motiondb.bin`, which is also an
external FM file.

For a copy-only save/load regression, run:

```powershell
.\tools\dromed_roundtrip.ps1 `
  -GameRoot 'C:\Games\Thief 2 1.29' `
  -FanMission 'Reflections in Red [2025-11-14] T2 FM v1.1b (EN)' `
  -Mission miss20.mis `
  -GameSys lakeside.gam
```

The harness writes only beneath `.codex-build\dromed-roundtrip`. It loads the
installed source, saves `.mis` and `.gam` copies with the `preserve` profile,
reloads and saves each again, then launches a second DromEd process for a full
reload of the resulting mission. `roundtrip-report.json` compares every tag's
presence, version, size, and SHA-256. Structural compatibility and exact byte
stability are reported separately because decoded hash/property tables may be
serialized in a different order without changing their runtime meaning.

Serializer read ceilings are independent from writer versions. Unknown chunks,
newer read-only chunks, and unowned record suffixes are carried through an
editor save; this does not imply that their fields are editable.

Engine and file-format capacities are inventoried in `h/darklimit.h`. It keeps
the retail and NewDark values distinct from the runtime capacity: runtime code
uses the broadest supported representation, while WR, WREXT, AIPATH, automap,
and other codecs validate against their explicit on-disk limits. Subsystem
headers may retain historical names as aliases, but do not own the values.

Every mission and gamesys load appends a chunk-by-chunk report to
`mission_compatibility.log` in the selected game directory. Each entry is
classified as `decoded`, `prefix-decoded`, `raw-preserved`, or `unsupported`.
The report records the stored version and size so unsupported FM data can be
identified without raising the writer version or silently discarding it.

## Command console

The command system and command terminal are original Dark Engine features, not
new facilities invented by this project. Original DromEd kept a one-line
command entry in the lower-right editor pane. The game player used by DromEd's
play mode created the same terminal as a hidden one-line pop-up: focusing it
paused simulation, **Enter** ran the command and dismissed the pop-up, and
**Escape** cancelled it. Command results were routed through the status line or
the engine's monochrome diagnostic output rather than retained in the terminal.

The original terminal already provided a 256-command history, saved in
`loc_hist.hst` and seeded from `pre_hist.hst`. **Ctrl+P** and **Ctrl+N** moved
through that history, while **Tab** completed from both command history and the
registered command names. It executed the engine's existing context-sensitive
command registry, including the original `help`, `run`, and `dump_cmds`
commands. Those mechanisms and the historical DromEd command set remain in
use.

This restoration turns that terminal into a practical console in DromEd, its
game player, and the standalone Thief runner. It adds a retained 4,096-line
output view, scrollbar and mouse-wheel scrolling, **Page Up**, **Page Down**,
**Home**, and **End** navigation, clipboard operations, resizable fonts, and
capture of command output that formerly went only to diagnostic destinations.
The DromEd pane remains visible and uses a compact font; the gameplay console
is a larger pop-up and pauses play while open. **Enter** now runs a command
without closing the gameplay console, while **Escape** closes it and resumes
play.

The **Console** entry under **Options > Controls > Customize Controls** is the
bindable launcher shortcut for the console. It invokes the original
`edit_command` action and saves its binding in `user.bnd`. The restoration
assigns both `:` and `;` in DromEd, DromEd play mode, and the Thief runner when
those keys do not already have explicit bindings.

Click the command-entry row to give it keyboard focus. **Ctrl+V** pastes into
the command line. **Ctrl+C** copies the command line when it is nonempty, or
the visible output otherwise; **Ctrl+Shift+C** copies all retained output.
Hold **Ctrl** and turn the mouse wheel over the console to change its font size
for the current run.

The existing `help` command has also been expanded. In gameplay, plain `help`
prints a short index of command groups, `help <group>` lists one group,
`help <keyword>` searches the names, descriptions, and argument types available
in the current mode, and `help all` prints the complete active registry. In
DromEd, plain `help` opens the searchable **Command Reference** window; use
`help <text>` to print matching commands in the console.

Commands added by this restoration for direct console use are:

- Mission and player: `open_mission [number/shortname]`, `list_missions`,
  `immunity [on/off]`, `flying [on/off]`, `player_physics [on/off]`, and
  `invisible [on/off]`. The first two select or inventory installed missions;
  the remaining commands expose explicit player-state and movement controls.
- Rendering and diagnostics: `reticle [on/off]`, `reticle_info [on/off]`,
  `audit_textures [output file]`, `list_scripts`, and
  `debug_scripts [on/off]`. With no argument, `reticle_info` reports the
  terrain, object, or mesh under the reticle once; `on` displays it
  continuously. Script diagnostics are written to `script-debug.log`.
- Console and accessibility: `console_copy [visible/all/command]`,
  `console_diagnostics [on/off]`, and `copy_screen_text`. The last command
  copies the visible tutorial or reticle message and `show_stats` lines.
- Input: `gamepad_status`, which reports the active gamepad backend and its
  normalized state.
- DromEd database diagnostics: `db_format_report` and
  `db_write_profile preserve|retail|newdark`, which inspect loaded chunk
  versions and select the database writer's compatibility target.
- DromEd tools: `world_size <extent>`, `kctools_light`,
  `kctools_portalize`, `kctools_optimize`, `show_command_help`, and
  `show_keyboard_shortcuts`. These set the portalization extent, run the
  integrated KCTools workflows, or open the two searchable editor references.

Controller and touch bindings also use new internal action commands, but they
are not intended as the primary typed console interface. The lists produced by
`help` remain the authoritative inventory because command availability depends
on whether the runner, DromEd editor, or DromEd game player is active. Long
mission and script listings are also appended to `thief-console.log`; generated
console logs are ignored by Git.

## Known limitations and remaining work

- The x64 targets build and pass initial smoke tests, but are not yet as fully
  runtime-validated as Release/x86, including mission transitions and
  save/load. Their embedded compatibility runtime covers the IA-32
  instructions, imports, and engine services exercised by tested legacy
  `.osm` modules; broader third-party script coverage remains ongoing.
- Game-save compatibility is not yet universal. Retail and representative
  NewDark saves load, but repeated in-game reloads and saves written by this
  engine remain under active validation. Gold, TDP, FM, x64, Linux, and
  Android coverage remains uneven.
  NewDark `VISITED` 4.0 automap state is not decoded, and saves written here
  use the smaller retail `VISITED` 1.0 layout. Retaining unknown source blocks
  does not make their behavior understood or guarantee that another Dark
  executable can load the resulting save; see [Game saves](#game-saves).
- The renderer still uses a 16-bit internal compatibility canvas despite
  presenting through 32-bit D3D11 or OpenGL output. Full 32-bit scene
  rendering remains to be implemented.
- Native-resolution, window resizing, ultrawide presentation, and the new
  cross-platform high-DPI text scaling need testing across more GPUs, desktop
  scales, Android font-size settings, and display configurations.
- Gamma correction is implemented in the Windows D3D11 presentation path.
  Equivalent Linux/OpenGL behavior and broader visual comparison of shadow
  detail, lightmaps, and emissive-looking light-source textures remain to be
  completed.
- The first-run picker and startup path need clean-machine testing against the
  common CD, GOG, and Steam directory layouts.
- NewDark compatibility is selective. PNG replacement art, FM-local
  configuration, principal lighting chunks, and several expanded database
  layouts are supported, but the complete renderer, world, AI, property,
  service, replacement-model, and uncommon-resource surface is not. A mission
  being readable or raw-preservable does not imply that every feature works.
- Stock Thief Gold mission/world and AI-path formats pass initial runner and
  DromEd smoke tests. TDP and broader Gold campaign/UI behavior still need
  validation; System Shock 2 is not a supported game target.
- DromEd is currently a viewer and playtest tool with partial editing support,
  not yet a validated general-purpose mission-authoring environment. Direct
  KCTools lighting and the principal NewDark lighting chunks are implemented,
  but the complete external DromEd/NewDark command environment is not. NewDark
  reading and preservation coverage still does not imply universal authoring
  or save compatibility.
- Linux DromEd can load a retail mission and render the editor viewport, but
  its Unix platform provider does not yet implement the desktop application
  menu bar. SDL prompts and `zenity`/terminal file selection are available;
  menu-only commands must currently be invoked through the editor console or
  keyboard bindings. Linux play-preview transitions need broader validation.
- DromEd mission loads currently report phases in the status field but do not
  show the game's animated loading progress. Play preview remains unstable:
  returning to the editor can still crash, and a resolution selected in the
  gameplay Video Options menu may not take effect in the preview window.
- Environmental audio now maps the original EAX room presets onto DirectSound
  I3DL2 or OpenAL EFX. DirectSound provides room reverb while the optional
  OpenAL provider also supplies per-source low-pass occlusion; both paths need
  broader in-mission acoustic comparison against the original game.
- Game builds load NewDark-style loose
  `sq_scripts/*.nut` sources with the native Squirrel 3.2 provider. Core
  object, property, link, physics, Act/React, door, lock, damage, containment,
  quest, data, and light calls are bridged, but the complete NewDark service
  surface, overlay callbacks, and packaged Squirrel-source archives still need
  broader implementation and mission coverage.
- There is no installer or redistributable package. The executable is run
  directly from the build output and uses separately installed retail data.
- Keyboard-layout behavior for the console itself and default `:` and `;`
  console bindings needs broader testing on non-US layouts.
- Raising warning levels and enabling additional static analysis remain future
  work.
- Automated runtime tests are limited because licensed retail data cannot be
  included in the repository or continuous-integration environment.

## Preservation and open access

This repository contains and modifies Dark Engine source code originating from
the unauthorized 2010 source leak. The historical source remains copyrighted
and was never released under an open-source license.

The repository exists for preservation, research, and modernization of the
engine. It does not include retail game assets. Users and contributors are 
expected to use game data from their own lawfully acquired copies of the relevant 
Dark Engine games when testing their code contributions.

Contributors freely license only their own original additions and modifications.
No rights are claimed in the underlying Dark Engine source or other third-party
material, and no license to that material is express or implied.
