# Android bring-up

The Android target is an early Thief runner bring-up. It builds the same native
engine from Windows or Linux through Gradle, CMake, and the Android NDK. DromEd
is not an Android target.

## Current scope

- Android 8.0 (API 26) or later.
- ARM64 (`arm64-v8a`) only for the first bring-up.
- SDL 2.32.10, OpenGL ES 3, and OpenAL Soft 1.25.2 using Android OpenSL.
- A folder picker that imports retail TDP, Gold, or Thief 2 files into the
  app-specific `game` directory on first launch.
- Retail movie playback through the packaged LibVLC backend.
- PNG replacement resources are not part of the first APK.

SDL and OpenAL Soft are downloaded from their official releases during the
first Gradle run and verified against pinned SHA-256 digests. They are
extracted only under the ignored `.codex-build/android-deps` directory. The
APK obtains LibVLC 3.6.5 from Maven Central through the normal Gradle dependency
resolution process.

## Toolchain

Install JDK 17 or later, Android SDK platform 36, SDK build tools 36, CMake
3.31.6, and NDK 30.0.16248370. The project intentionally pins these versions
so Windows and Linux produce the same native configuration.

From PowerShell:

```powershell
Set-Location android
.\gradlew.bat assembleDebug
```

From a Linux shell:

```sh
cd android
./gradlew assembleDebug
```

The debug APK is written below `android/app/build/outputs/apk/debug`.

Install and launch the APK. If the app-specific `game` directory does not
already contain a recognizable installation, OpenDarkEngine opens Android's
folder picker. Select the folder that directly contains `cam.cfg` and the
retail mission files. The app validates the selection, imports it without
changing the source folder, and then starts the engine. Keep the device awake
and the app in the foreground while the initial import completes.

The importer is necessary because Android's Storage Access Framework exposes
the selected tree through content URIs rather than a native filesystem path
that the engine can use as its working directory. Files are staged before the
app replaces an incomplete `game` directory.

For development, a legal installation can still be copied into the
app-specific external `game` directory with `adb`. Do not put retail data in
the repository or APK.

For example, after building the APK:

```powershell
$adb = Join-Path $env:LOCALAPPDATA 'Android\Sdk\platform-tools\adb.exe'
& $adb install -r app\build\outputs\apk\debug\app-debug.apk
& $adb shell mkdir -p /sdcard/Android/data/org.opendarkengine.thief2/files/game
& $adb push 'C:\Games\Thief2\.' /sdcard/Android/data/org.opendarkengine.thief2/files/game/
& $adb shell am start -n org.opendarkengine.thief2/.LauncherActivity
```

Substitute the source path for a legal installation. The app validates the
directory before engine startup; it does not bundle or modify the retail data.

## Direct mission debugging

Debug builds accept the engine's normal quick-start options through the
launcher, so testing a mission does not require navigating the content, main,
difficulty, briefing, and objective screens. From the repository root:

```powershell
.\tools\android_quickstart.ps1 -Mission MISS15.MIS
```

The helper stops only this application, then starts the selected mission using
the already imported game data. Optional placement and unattended-stop
arguments mirror the desktop engine options:

```powershell
.\tools\android_quickstart.ps1 `
    -Serial MQ65172511054792 `
    -Mission MISS1.MIS `
    -Position '37.43,100.15,18.97' `
    -Angles '-35,90,0' `
    -AutoStop 300
```

Use `-Build -Install` when the device also needs the latest debug APK. Release
builds ignore these intent extras; normal launcher and retail menu behavior is
unchanged.

## Gamepad requirements

Gamepads are a first-class Android input path, including built-in controls on
handhelds and Bluetooth or USB controllers. The existing SDL backend already
discovers standardized controllers, handles hot-plug and reconnect, exposes
both sticks, both triggers, the D-pad, face and shoulder buttons, and supports
rumble. Its input is routed through Dark's semantic binding layer rather than
Android key-code assumptions.

The Android acceptance pass must cover movement speed on the left stick,
right-stick look, trigger/shoulder held actions, D-pad weapon and inventory
selection, pause and automap, configurable dead zones/sensitivity/invert-Y,
remapping (including lean forward), and clean release of held actions when a
controller disconnects or the app backgrounds. Menus, books, automap,
objectives, and load/save screens also need D-pad/stick navigation plus
confirm/cancel. The touch overlay should be independently hideable for an
attached or built-in gamepad.

At the default `gamepad_look_turn_seconds 1.0`, maximum right-stick deflection
turns exactly 360 degrees per second independent of frame rate and resolution.
Lower values turn faster; `0.5` means two complete turns per second.

## Touch controls

Gameplay touch input feeds the existing semantic gamepad controls rather than
synthesizing keyboard input. All held actions are released when a finger is
cancelled, the app loses focus, or Android backgrounds the activity.

- Bottom left: tap to cycle weapons (including no weapon); hold to holster.
- Above it: circular movement stick with creep, walk, and run ranges.
- Above the movement stick: a row for lean left, crouch, and lean right, with
  a separate lean-forward button directly above crouch. Lean actions are held;
  crouch can be configured as toggle or hold.
- Bottom right: tap to cycle inventory items (including no item); hold to drop.
- Above it: circular look stick.
- Above the look stick: frob/use, jump, primary attack, and block.
- Safe top corners: automap and pause/back.
- Horizontal swipes over the weapon and inventory regions select the previous
  or next entry. Long-pressing the inventory region drops the selected item.
- Primary attack and frob/use preserve their press and release timing for bow,
  sword, world frobs, and inventory tools.
- The first bring-up uses a fixed landscape layout and opacity. The engine's
  normal gamepad dead-zone, movement-threshold, look-sensitivity, and vertical-
  inversion settings also apply to the touch sticks. Layout mirroring and
  visual customization are not implemented yet.

Menus, books, automap, objectives, and load/save screens use either stick as a
mouse cursor. A/X perform a left click, Y performs a right click, B/Menu go
back, and the D-pad retains directional navigation. The gameplay touch overlay
hides when a physical gamepad was the most recently used controller and returns
as soon as the player touches one of its controls. The content picker uses
direct Android touch hit-testing. Text fields and the command console require
the Android software keyboard.
