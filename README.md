# Dark Engine / Thief 2 runner and DromEd viewer

This repository restores the historical Dark Engine sources as a Visual
Studio 2022 project. The shared executable project builds both a Thief 2
runner with an in-game command console and a preliminary DromEd mission
viewer.

The repository does not include Thief 2 missions, movies, sounds, scripts, or
other retail assets. A legally obtained installation of *Thief II: The Metal
Age* is required to run the game.

## Current status

The Release/x86 Thief 2 and DromEd targets build successfully with the Visual
Studio 2022 v143 toolset. Release/x64 builds also compile and run, but remain
under runtime validation. Both use the modern D3D11 renderer and presentation
path. Thief 2 can use an external retail installation and retains the standard
startup sequence, mission loading, game UI, inventory, and automap. The DromEd
build can load, inspect, and playtest legacy Thief 2 missions in a normal
resizable window, using the full client area when the window size changes.
Treat it as a viewer, not a reliable editor: editing and saving workflows are
not yet stable or comprehensively validated, and NewDark formats and features
are not supported.

Native x64 builds include a compatibility runtime for legacy 32-bit `.osm`
script modules. It reads PE32 modules, interprets the IA-32 code, and bridges
the supported script-manager and engine-service calls into the 64-bit process.
This is separate from NewDark's Squirrel support and is still being expanded
and validated against third-party OSMs.

Modern scene submission now crosses the platform-neutral
`h/render_backend.h` boundary. The Windows display and D3D11 providers live
under `winsrc/dispdev` and `winsrc/render`; engine, UI, movie, and `lgd3d` code no longer call a
D3D11-named API directly. This is the renderer seam for a future OpenGL
provider, but an OpenGL implementation and a complete non-Windows build do
not exist yet.

Core clocks, dynamic-library loading, memory queries, assertions, clipboard,
synchronization, and filesystem conveniences now cross `platform_services.h`
and `platform_io.h`. Windows providers live under `winsrc`; Unix providers
live under `unixsrc`. On Linux, CMake builds a native 64-bit `Thief2` executable
with SDL2/OpenGL presentation and runtime-loaded OpenAL audio. The Linux target
can load a retail mission and run gameplay, but remains an early port: movie
playback, complete OSM service bridging, and DromEd are not yet implemented.

Audio follows the same provider model. Windows uses DirectSound 3D by default,
with native I3DL2 environmental reverb. An OpenAL provider supplies positional
audio, EFX reverb, and per-source occlusion when `OpenAL32.dll` is available;
set `sfx_backend openal` in `cam.cfg` to select it explicitly. The default
`auto` setting tries DirectSound first and uses OpenAL only if native audio
cannot initialize, so the standard executable has no OpenAL dependency.

Movie playback is exposed through the platform-neutral engine movie API. The
Windows Media Foundation, DirectShow, and legacy DirectDraw implementation is
isolated under `winsrc/movie` so another platform can supply its own backend.

Legacy PKWARE DCL-compressed resources are decoded in-process with Mark
Adler's small `blast` decoder under `3rdparty/blast`; newly written resources
use the engine's existing LZW codec. No external compression DLL is required.

The runner currently provides:

- first-run selection and validation of a Thief 2 data directory;
- reuse of the selected directory on later launches;
- native display-resolution defaults when the user has not configured a
  resolution;
- borderless fullscreen and resizable windowed presentation;
- the standard splash/startup, intro movie, and main-menu flow; and
- a configurable command console, defaulting to `:` and `;` in every mode.

## Requirements

### Linux / WSL2

- A 64-bit Linux distribution with GCC, CMake, and Ninja
- SDL2 and OpenGL development headers
- A retail Thief 2 installation
- OpenAL Soft at runtime for audio; the executable has no hard OpenAL link

On Ubuntu under WSL2, the minimal build packages are:

```sh
sudo apt update
sudo apt install build-essential cmake ninja-build libsdl2-dev libgl-dev
```

Gameplay uses SDL's native relative-pointer mode and consumes motion deltas;
menus use absolute window coordinates. The provider prefers the compositor's
native Wayland path when it is advertised. WSLg may not confine its Windows
host cursor as reliably as a native Linux Wayland or X11 session.

WSLg audio uses its PulseAudio socket. If WSLg stops accepting audio clients,
close running WSL applications and run `wsl --shutdown` from PowerShell before
starting the game again.

### Windows

- 64-bit Windows 10 or Windows 11
- Visual Studio 2022 or Visual Studio 2022 Build Tools
- The **Desktop development with C++** workload, including:
  - MSVC v143 x86/x64 build tools
  - a Windows 10 or Windows 11 SDK
- A retail Thief 2 installation containing `cam.cfg` and either `DARK.GAM` or
  `MISS1.MIS`

The Windows SDK supplies the DirectX declarations used by the Windows input,
audio, and rendering providers. Current Windows output uses a D3D11 swap chain
and Windows Media Foundation. No assembler is required by the active x86 or
x64 build.

## Build instructions

### Native Linux / WSL2

Keep the source and build trees in the Linux filesystem for good build
performance. From a WSL shell:

```sh
rsync -a --delete --exclude=.git --exclude=build --exclude=.codex-build \
  /mnt/c/path/to/darkengine/ ~/darkengine-src/
cmake -S ~/darkengine-src -B ~/darkengine-build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release
cmake --build ~/darkengine-build -j "$(nproc)"
```

The result is `~/darkengine-build/Thief2`, a native 64-bit ELF executable.
Run it with the retail installation as the working directory because the
engine resolves configuration and resource paths relative to that directory:

```sh
cd "/mnt/c/Games/Thief 2 - The Metal Age"
~/darkengine-build/Thief2 quickstart=MISS1.MIS
```

For an unattended smoke test, add `auto_stop=60`. Paths on `/mnt/c` work but
load more slowly than native Linux filesystems because Dark performs many
small, case-insensitive resource lookups.

### Thief 2 in Visual Studio

1. Open `thief2.sln` in Visual Studio 2022.
2. Select the `Release` configuration and `x86` solution platform.
3. Build the solution with **Build > Build Solution**.
4. The resulting executable is
   `build\Release\Win32\Thief2\Thief2.exe`.

`Directory.Build.props` selects the Thief 2 target by default and applies the
packing and C-language compatibility settings required by the legacy code.
The game and editor intentionally use the same `src\dromed.vcxproj`; the
`BuildThief2` property selects which source set and executable name it emits.

### Developer Command Prompt

From an **x86 Native Tools Command Prompt for VS 2022**, run:

```bat
msbuild thief2.sln /m /t:Build /p:Configuration=Release /p:Platform=x86
```

The command should finish with
`build\Release\Win32\Thief2\Thief2.exe`.

To build DromEd from the same project, run:

```bat
msbuild thief2.sln /m /t:dromed /p:Configuration=Release /p:Platform=x86 /p:BuildThief2=false
```

The command should finish with
`build\Release\Win32\DromEd\DromEd.exe`. The editor switch also
works for a direct project build; `Directory.Build.props` supplies the solution
root needed by the legacy include and library paths:

```bat
msbuild src\dromed.vcxproj /m /t:Build /p:Configuration=Release /p:Platform=Win32 /p:BuildThief2=false
```

Use `Platform=x64` for a native 64-bit direct build. Final executables and
their object files are separated by architecture, configuration, and target:

```text
build\Release\Win32\Thief2\Thief2.exe
build\Release\Win32\DromEd\DromEd.exe
build\Release\x64\Thief2\Thief2.exe
build\Release\x64\DromEd\DromEd.exe
```

Static libraries remain shared between Thief2 and DromEd within a given
architecture. They must not vary with `BuildThief2`.

## Run instructions

1. Start `build\Release\Win32\Thief2\Thief2.exe`.
2. On the first launch, select the root of the Thief 2 installation. This is
   the directory containing `cam.cfg` and `DARK.GAM` or `MISS1.MIS`.
3. The runner changes the working directory to the selected installation and
   starts the engine. Retail data is read in place and is not copied into this
   repository.
4. The normal startup flow continues through the splash/startup screen, intro
   movie, and main menu.

The selected directory is stored for the current Windows user at:

```text
HKCU\Software\OpenDarkEngine\Thief2\GameDataPath
```

If the stored directory no longer contains valid game data, the runner asks
for a new directory. To choose a different directory while the old one is
still valid, remove the `GameDataPath` value with Registry Editor and launch
the executable again.

### Run the DromEd viewer

DromEd uses the same Thief 2 data-directory selection as the game runner. Start
it directly from the build output:

```bat
build\Release\Win32\DromEd\DromEd.exe
```

The viewer reuses the `GameDataPath` registry value shown above. If no valid
directory has been selected yet, it prompts for the root of a legally installed
Thief 2 data directory before loading its configuration and mission resources.

The DromEd viewer uses the same D3D11 scene renderer and swap-chain presenter
as the game. Its legacy editor canvas remains 16-bit internally, while the D3D11
presenter converts it to the resizable 32-bit desktop window. When an editor
window resize finishes, DromEd recreates that canvas at the client area's
actual dimensions and scales the four viewports, console, status bar, and
brush controls independently in each axis. This fills widescreen and portrait
windows without imposing a 4:3 aspect ratio. `edit_screen_size` in `cam.cfg`
still selects the initial canvas size. Game preview mode stays in the same
window and recreates the D3D11 render context for the preview; returning to
edit mode restores the editor viewport and menu.

Use **Help > Keyboard Shortcuts** to open a resizable, human-readable input
reference. It includes every active editor binding (including the complete
built-in fallback set), non-conflicting shortcut annotations from the loaded
`menus.cfg`, and direct mouse/viewport gestures. Controls are grouped and
ordered by purpose, with modifier keys shown before the base key.

DromEd keeps its scrollable command console visible in the lower-right editor
pane. Press `:` or `;` to give its command line keyboard focus; **Escape**
releases focus without hiding the pane. It uses the same scrollback, command
history, completion, clipboard controls, and output path as the pop-up console
in gameplay and play preview.

The historical editing commands are present, but this build is not yet a
dependable mission-authoring environment. Work from backups and do not rely on
it as the only editor or copy of a mission. In particular, it cannot load or
save NewDark-specific mission and data formats.

If `screen_size` or `game_screen_size` is already defined in `cam.cfg`, that
size is retained when the display driver supports it. Otherwise, the game
uses the closest supported mode to the native resolution of the display under
the mouse pointer at startup. The Video Options list merges the active
monitor's current Windows display modes with compatibility render sizes,
including 640x480, 800x600, and widescreen equivalents that fit on that
display. Resolutions are listed with the highest first; use the arrow buttons
or mouse wheel to reach lower entries such as 640x480 and 800x600.
Unsupported 24/32-bit NewDark settings are normalized to the engine's 16-bit
internal canvas. That canvas is converted to 32-bit color and presented by
D3D11; Windows is not switched into an obsolete 16-bit display mode. The
engine's gamma setting is applied by the D3D11 presentation shader using the
same power curve as the legacy palette correction.

The Video Options screen includes a **Display Mode** toggle. **Fullscreen**
uses a borderless window on the current display. **Windowed** uses a normal,
resizable window. Display-mode changes take effect immediately; resolution
changes are applied when **Done** is selected. In Windowed mode, the selected
resolution is the fixed internal render size, not a maximum window size.
Entering Windowed mode opens the client area at that size (clamped to the
monitor work area); resizing the window afterward only resizes the swap-chain
output and scales that fixed render canvas. Select **Fit** instead of a fixed
resolution to render the 3D scene directly across the full client area in
Windowed mode or across the display's native pixel area in Fullscreen mode.
During an interactive window resize, the last completed scene is scaled; the
single scene/depth pair is resized once the drag ends. The settings can also
be changed in `cam.cfg` with `game_full_screen 1` or `game_full_screen 0` and
`game_screen_fit 1` or `game_screen_fit 0`.

On a multi-monitor system, the display containing the game window determines
both the resolution list and the target for borderless fullscreen. To change
displays, switch to Windowed, move the game window to the other display,
reopen Video Options, and then select a resolution or Fullscreen. The list is
queried again for that display; it is not assumed that every connected
display supports the same modes.

The retail game writes `skip_intro` after the intro has played. Add
`always_play_intro` to `cam.cfg` to show the intro on every launch. When the
retail movie directory contains both formats, the runner uses the H.264 MP4
through Windows Media Foundation instead of requiring the obsolete Indeo 5
codec used by the original AVI.

## Xbox controller support

Thief2 supports one active Xbox-style controller through a platform-neutral
gamepad layer and a runtime-loaded XInput backend. Controllers can be connected,
disconnected, and reconnected while the game is running. Keyboard and mouse
remain active, and an XInput controller is not also polled through the legacy
DirectInput joystick path.

The default layout follows a NewDark-compatible Thief control scheme: left
stick moves and strafes, right stick looks, A jumps, B crouches, X/Y use or
frob, the triggers block and attack, the shoulders and D-pad cycle inventory or
weapons, View opens the automap, and Menu opens the pause menu. Stick clicks run
and recenter the view. Menus accept D-pad navigation, A to accept, and B or Menu
to go back. These controls use semantic names such as `pad_a`, `pad_rt`, and
`pad_right_x` in `user.bnd`; existing `joy1` and `joy_axis*` bindings remain
valid as fallbacks.

Gamepad behavior can be adjusted in `cam.cfg`:

```text
gamepad_enable 1
gamepad_backend auto
gamepad_index 0
gamepad_left_deadzone 0.18
gamepad_right_deadzone 0.14
gamepad_left_response 1.0
gamepad_right_response 1.35
gamepad_trigger_press 0.15
gamepad_trigger_release 0.10
gamepad_look_sensitivity 1.0
gamepad_look_invert_y 0
gamepad_rumble 1
```

`gamepad_backend` accepts `auto`, `xinput`, `legacy`, or `off`. The
`gamepad_status` console command prints the selected backend and normalized
state. DromEd editor input remains unchanged; gamepad input is currently enabled
only in the Thief2 target.

## Command console

The console appears as **Console** under **Options > Controls > Customize
Controls**. It defaults to both `:` and `;` in DromEd, DromEd play preview,
and Thief2 gameplay when those keys are free. Existing commands assigned to
either key are preserved. The console can be rebound like any other control,
and the selection is saved in `user.bnd`.

The console pauses gameplay while open. Press **Enter** to run a command and
keep the console open. Use the **mouse wheel**, drag anywhere on the scrollbar
track, or use **Page Up** and **Page Down** to move through its 4,096-line
scrollback; **Home** jumps to the oldest retained output and **End** returns to
the newest.
Hold **Ctrl** and turn the mouse wheel over the console to change its font
size. The permanently visible DromEd pane starts with the compact editor font,
while the pop-up gameplay console starts with a larger font; each profile
remembers its adjustment for the current run. Clicking the command-entry row
gives it keyboard focus.
Press **Ctrl+V** to paste into the command line. **Ctrl+C** copies the command
line when it is nonempty, or the visible output otherwise; **Ctrl+Shift+C**
copies all retained output. Press **Escape** to close the console and resume
play.

`help <keyword>` searches the names, descriptions, and argument types of the
commands available in the current mode. Plain `help` opens DromEd's searchable
command-reference window; in gameplay it prints the available command names.

Available commands:

- `help [keyword]` - search the currently available command help
- `open_mission [number/shortname]` - start an installed mission
- `list_missions` - list installed missions and their short names
- `immunity [on/off]` - prevent player damage
- `flying [on/off]` - disable or restore player gravity
- `player_physics [on/off]` - toggle player collision and gravity for noclip
  free-flight while leaving world physics active
- `invisible [on/off]` - toggle the existing player-invisibility property
- `reticle [on/off]` - show or hide the center reticle
- `reticle_info` - describe the object and mesh under the reticle
- `list_scripts` - list loaded script classes
- `debug_scripts [on/off]` - log dispatched script messages to
  `script-debug.log`
- `console_copy [visible/all/command]` - copy console text to the clipboard
- `console_diagnostics [on/off]` - optionally include general `mprintf`
  diagnostic output in the console

In DromEd, bare `help` opens the searchable **Command Reference** window
instead of printing the complete editor command registry. Use `help <text>`
to print only matching commands in the console.

The earlier spellings `openmission`, `listmissions`, `playerphysics`,
`retinfo`, `listscripts`, and `debugscripts` remain compatibility aliases.

Long mission and script listings are appended to `thief-console.log`. Both log
files are ignored by Git.

## Screenshots

These captures were produced during local runtime verification with external
retail game data. The data shown in the captures is not included in the
repository.

### In-game menu

![Thief 2 in-game menu](docs/screenshots/in-game-menu.png)

### Gameplay and inventory HUD

![Thief 2 gameplay and inventory HUD](docs/screenshots/gameplay-inventory.png)

### Automap

![Thief 2 automap](docs/screenshots/automap.png)

## Known limitations and remaining work

- The x64 targets build and pass initial smoke tests, but are not yet as fully
  runtime-validated as Release/x86. The embedded compatibility runtime covers
  the IA-32 instructions, imports, and engine services exercised by tested
  legacy `.osm` modules; broader third-party script coverage remains ongoing.
- Modern rendering is still exposed through the historical `lgd3d` API as a
  compatibility facade over the renderer-neutral backend. The unusable
  Direct3D 2 implementation and its dormant build sources have been removed;
  callers still need migration away from the facade itself.
- The native 64-bit build still needs broader mission, save/load, and editing
  validation. The renderer also still uses a 16-bit internal canvas despite
  presenting a 32-bit D3D11 image, so full 32-bit scene rendering remains to
  be implemented.
- Native-resolution, high-DPI, window resizing, and ultrawide behavior need
  testing across more GPUs and display configurations.
- Gamma correction is implemented in the D3D11 presentation path. Visual
  equivalence of shadow detail, lightmaps, and emissive-looking light-source
  textures still needs comparison against the retail renderer on additional
  displays.
- The first-run picker and startup path need clean-machine testing against the
  common CD, GOG, and Steam directory layouts.
- NewDark data formats are not supported. This affects both the runner's mod
  compatibility and the DromEd viewer's ability to open or preserve
  NewDark-specific mission data.
- Thief Gold and System Shock 2 game targets are not yet supported or
  validated by this runner and modern presentation path.
- DromEd is currently a viewer and playtest tool, not a reliable editor. It
  builds with the shared D3D11 renderer in a resizable window, but stability,
  editing and save integrity, and complete mission-authoring workflows still
  require substantial validation. NewDark missions and data are not
  supported.
- Environmental audio now maps the original EAX room presets onto DirectSound
  I3DL2 or OpenAL EFX. DirectSound provides room reverb while the optional
  OpenAL provider also supplies per-source low-pass occlusion; both paths need
  broader in-mission acoustic comparison against the original game.
- Squirrel scripting support required by NewDark-era fan missions and mods,
  including *The Black Parade*, is not implemented. The x64 legacy-OSM
  compatibility runtime does not provide Squirrel support.
- Broader NewDark mod compatibility remains to be implemented and tested,
  including replacement models, textures, and other art assets.
- There is no installer or redistributable package. The executable is run
  directly from the build output and uses separately installed retail data.
- Console scrollback retains the latest 4,096 output lines; command recall is
  a separate 256-command history. Long mission and script listings are also
  written to `thief-console.log`.
- `flying` disables gravity while preserving collision. Use `player_physics off`
  for noclip free-flight; its movement behavior still needs broader mission
  testing.
- `invisible` uses the engine's existing invisibility/render property. AI
  perception behavior has not been exhaustively verified for every mission.
- Keyboard-layout behavior for the default `:` and `;` console bindings needs
  broader testing on non-US layouts.
- The four supported Release targets currently compile and link without
  warnings at the configured warning level. Raising that level and enabling
  additional static analysis remain future work.
- Automated runtime tests are limited because licensed retail data cannot be
  included in the repository or continuous-integration environment.

## Preservation and open access

This repository contains and modifies Dark Engine source code originating from
the unauthorized 2010 source leak. The historical source remains copyrighted
and was never released under an open-source license.

The repository exists for preservation, research, and modernization of the
engine. It does not include retail game assets. Contributors are expected to
use game data from their own lawfully acquired copies of the relevant Dark
Engine games when testing their code contributions.

Contributors freely license only their own original additions and modifications.
No rights are claimed in the underlying Dark Engine source or other third-party
material, and no license to that material is express or implied.
