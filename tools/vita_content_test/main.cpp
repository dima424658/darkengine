#include "vita_paths.h"
#include "vita_subtitles.hpp"
#include "vita_video_settings.hpp"
#include "vita_controls.hpp"
#include "vita_texture_limits.hpp"
#include "vita_acoustics.hpp"
#include "vita_power.hpp"

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace
{
void Check(bool condition, const char *message)
{
   if (!condition)
   {
      std::cerr << "FAIL: " << message << '\n';
      std::exit(1);
   }
}

void SetEnvironment(const char *name, const std::string &value)
{
#ifdef _WIN32
   _putenv_s(name, value.c_str());
#else
   setenv(name, value.c_str(), 1);
#endif
}

void WriteFile(const std::filesystem::path &path, const std::string &text)
{
   std::filesystem::create_directories(path.parent_path());
   std::ofstream output(path, std::ios::binary);
   Check(output.good(), "open temporary test file");
   output.write(text.data(), static_cast<std::streamsize>(text.size()));
   Check(output.good(), "write temporary test file");
}

void TestSubtitleParser()
{
   using namespace DarkVitaSubtitles;
   const std::string source =
      "SUB1\r\n"
      "// comment\n"
      "{ Type \"convo\" Descr \"Woman\" Singleton NoVolume "
      "OnlySoundType 3D\n"
      "  Sub \"sg1\" { Text \"She said \\\"hello\\\".\\nNext line\" }\n"
      "  Sub \"SG1\" { Text \"duplicate\" }\n"
      "  MultiSub \"movie/intro\" { "
      "{ Time 0 Length 2000 Text \"Start\" } "
      "{ Time 2000 Length 1000 Text \"End\" } }\n"
      "  SrtSub \"movie/b01\" \"intro.srt\"\n"
      "}\n";
   ParseResult parsed = Parse(source);
   Check(parsed.ok(), "parse NewDark SUB1 declarations");
   Check(parsed.entries.size() == 3, "ignore duplicate ID case-insensitively");
   const Entry &single = parsed.entries[0];
   Check(single.id == "sg1" && single.type == "convo" &&
         single.description == "Woman", "copy group metadata to Sub");
   Check(single.singleton && single.noVolume &&
         single.soundType == SoundType::ThreeD, "parse sound flags");
   Check(single.cues.size() == 1 &&
         single.cues[0].text == "She said \"hello\".\nNext line",
         "decode quoted text escapes");
   const Entry &multi = parsed.entries[1];
   Check(multi.multi && multi.cues.size() == 2 &&
         multi.cues[1].timeMs == 2000 &&
         multi.cues[1].lengthMs == 1000, "parse timed MultiSub cues");
   Check(parsed.entries[2].srtFile == "intro.srt",
         "retain external SRT filename");
   Check(parsed.warnings.size() == 1, "report duplicate subtitle ID");

   const std::string cp1250 =
      "SUB1\n{ Type \"convo\" Sub \"pl\" { Text \"Za\xBF\xf3\xb3\xe6\" } }\n";
   ParseResult polish = Parse(cp1250, TextEncoding::Windows1250);
   Check(polish.ok() && polish.entries.size() == 1 &&
         polish.entries[0].cues[0].text == "Zażółć",
         "decode Polish Windows-1250 subtitle text to UTF-8");

   ParseResult malformed = Parse(" SUB1\n{}");
   Check(!malformed.ok(), "reject header with leading whitespace");
   ParseResult missingLength =
      Parse("SUB1\n{ Type \"movie\" MultiSub \"movie/x\" "
            "{ { Time 0 Text \"missing length\" } } }");
   Check(!missingLength.ok(), "require time and length for MultiSub cues");
   ParseResult emptyId =
      Parse("SUB1\n{ Sub \"\" { Text \"invalid ID\" } }");
   Check(!emptyId.ok(), "reject empty subtitle identifiers");
   ParseResult invalidUtf8 = Parse("SUB1\n{}", TextEncoding::Utf8);
   Check(invalidUtf8.ok(), "accept valid UTF-8 input");
}

void TestModOverlay()
{
   const std::filesystem::path root =
      std::filesystem::temp_directory_path() / "thief-vita-content-test";
   const std::filesystem::path secondaryRoot = root / "secondary";
   std::error_code error;
   std::filesystem::remove_all(root, error);
   error.clear();

   WriteFile(root / "mods/00-core/Resources/stone.bin", "first");
   WriteFile(root / "mods/10-variant/Resources/stone.bin", "second");
   WriteFile(root / "Resources/base-only.bin", "base");
   WriteFile(secondaryRoot / "Resources/secondary-only.bin", "secondary");

   SetEnvironment("DARK_VITA_ASSET_ROOTS",
                  root.generic_string() + ":" + secondaryRoot.generic_string());
   SetEnvironment("DARK_VITA_ASSET_ROOT", root.generic_string());

   char resolved[4096] = {};
   bool exists = false;
   Check(PlatformResolveVitaAssetPath("Resources/stone.bin", resolved,
                                     sizeof(resolved), false, &exists) &&
         exists && std::string(resolved).find("mods/00-core/") !=
         std::string::npos, "alphabetical mod overlay has highest priority");
   Check(PlatformResolveVitaAssetPath("Resources/base-only.bin", resolved,
                                     sizeof(resolved), false, &exists) &&
         exists && std::string(resolved).find("mods/") == std::string::npos,
         "fall back to the retail asset root");
   Check(PlatformResolveVitaAssetPath("Resources/secondary-only.bin",
                                     resolved, sizeof(resolved), false,
                                     &exists) &&
         exists && std::string(resolved).find("secondary/") !=
         std::string::npos, "search additional configured data roots");
   Check(PlatformResolveVitaAssetPath("Saves/new.sav", resolved,
                                     sizeof(resolved), true, &exists) &&
         !exists && std::string(resolved) ==
         (root / "Saves/new.sav").generic_string(),
         "write new files to the base root, not a mod overlay");
   Check(!PlatformResolveVitaAssetPath("../outside.bin", resolved,
                                      sizeof(resolved), false, &exists),
         "reject parent traversal");
   Check(!PlatformResolveVitaAssetPath("Resources/stone.bin", resolved, 4,
                                      false, &exists),
         "reject undersized output buffer");

   std::filesystem::remove_all(root, error);
   Check(!error, "remove temporary mod overlay tree");
}

void TestVideoSettings()
{
   using namespace DarkVitaVideo;
   Settings settings;
   Check(BrightnessOffset(settings) == 0.0f && GammaValue(settings) == 1.0f,
         "neutral Vita LCD settings");
   const Color neutral = ApplyCorrection({0.25f, 0.5f, 0.75f}, settings);
   Check(std::abs(neutral.red - 0.25f) < 0.0001f &&
         std::abs(neutral.green - 0.5f) < 0.0001f,
         "neutral shader correction leaves colors unchanged");

   settings.brightnessSlider = 100;
   const Color brighter = ApplyCorrection({0.25f, 0.5f, 0.75f}, settings);
   Check(brighter.red > neutral.red, "brightness slider lifts dark values");
   settings.brightnessSlider = 50;
   settings.gammaSlider = 100;
   const Color gammaCorrected =
      ApplyCorrection({0.25f, 0.5f, 0.75f}, settings);
   Check(gammaCorrected.red > neutral.red,
         "gamma slider separately adjusts LCD shadow response");
   Check(std::string(FragmentShaderSource()).find("uBrightness") !=
         std::string::npos &&
         std::string(FragmentShaderSource()).find("uGamma") !=
         std::string::npos, "shader exposes separate brightness and gamma");

   const std::filesystem::path root =
      std::filesystem::temp_directory_path() / "thief-vita-video-test";
   std::error_code filesystemError;
   std::filesystem::remove_all(root, filesystemError);
   std::filesystem::create_directories(root);
   const std::string path = (root / "vita_options.cfg").string();
   std::string error;
   Check(Save(path, settings, &error), "save Vita video options");
   settings.brightnessSlider = 5;
   settings.gammaSlider = 5;
   Check(Save(path, settings, &error), "replace Vita video options");
   const LoadResult loaded = Load(path);
   Check(loaded.found && loaded.error.empty() &&
         loaded.settings.brightnessSlider == 5 &&
         loaded.settings.gammaSlider == 5,
         "load independently persisted video sliders");

   WriteFile(root / "vita_options.cfg", "version=1\nbrightness=999\ngamma=50\n");
   const LoadResult invalid = Load(path);
   Check(!invalid.found && !invalid.error.empty(),
         "report invalid persisted slider values");
   std::filesystem::remove_all(root, filesystemError);
   Check(!filesystemError, "remove temporary video settings");
}

void TestTextureLimits()
{
   using namespace DarkVitaTextures;
   Check(BoundDimensions(Profile::Size256, 1024, 768).width == 256 &&
         BoundDimensions(Profile::Size256, 1024, 768).height == 256,
         "256 profile safely bounds large mod textures");
   Check(BoundDimensions(Profile::Size512, 300, 700).width == 512 &&
         BoundDimensions(Profile::Size512, 300, 700).height == 512,
         "512 profile rounds texture dimensions into its VRAM bound");
   Check(BoundDimensions(Profile::Size1024, 1600, 900).width == 1024 &&
         BoundDimensions(Profile::Size1024, 1600, 900).height == 1024,
         "1024 profile protects Vita VRAM");
   const Dimensions unlimited =
      BoundDimensions(Profile::Unlimited, 1600, 900);
   Check(unlimited.width == 1600 && unlimited.height == 900,
         "unlimited profile preserves source dimensions");
   Check(EstimatedRgbaBytes(512, 512) == 1048576,
         "RGBA VRAM estimate is byte-accurate");

   const std::filesystem::path root =
      std::filesystem::temp_directory_path() / "thief-vita-texture-test";
   std::error_code filesystemError;
   std::filesystem::remove_all(root, filesystemError);
   std::filesystem::create_directories(root);
   const std::string path = (root / "vita_texture.cfg").string();
   std::string error;
   Check(Save(path, {Profile::Size1024}, &error),
         "save Vita texture profile");
   Check(Load(path).profile == Profile::Size1024,
         "load Vita texture profile");
   std::filesystem::remove_all(root, filesystemError);
   Check(!filesystemError, "remove temporary Vita texture settings");
}

void TestAcoustics()
{
   using namespace DarkVitaAcoustics;
   const Preset generic = ForRoomType(0);
   Check(generic.decay > 1.0f && generic.gainHF > 0.8f,
         "generic EAX room maps to a valid EFX reverb preset");
   const Preset padded = ForRoomType(1);
   Check(padded.decay < generic.decay,
         "small padded-room preset remains short and dry");
   const Preset occluded =
      ForPropagation(12.0f, 1.0f, 0.0f);
   Check(occluded.gain < generic.gain &&
         occluded.decay >= 0.1f,
         "occlusion and distance reduce propagation safely");
   const Preset underwater =
      ForPropagation(5.0f, 0.0f, 1.0f);
   Check(underwater.gainHF < generic.gainHF &&
         underwater.decay > generic.decay,
         "underwater propagation attenuates HF and lengthens decay");
   Check(HeadphoneWetMix(1.0f) < 1.0f &&
         HeadphoneWetMix(1.0f) > 0.7f,
         "headphone mode applies conservative wet-mix compensation");
}

void TestPowerProfiles()
{
   using namespace DarkVitaPower;
   Settings settings;
   Check(settings.profile == Profile::Fps60,
         "Vita power profile defaults to 60 FPS");
   const Clock low = ApplyProfile(Profile::Fps30);
   const Clock high = ApplyProfile(Profile::Fps60);
   Check(low.armMegahertz < high.armMegahertz &&
         low.busMegahertz < high.busMegahertz &&
         low.gpuMegahertz < high.gpuMegahertz,
         "30 FPS profile lowers ARM, bus, and GPU clocks");

   const std::filesystem::path root =
      std::filesystem::temp_directory_path() / "thief-vita-power-test";
   std::error_code filesystemError;
   std::filesystem::remove_all(root, filesystemError);
   std::filesystem::create_directories(root);
   const std::string path = (root / "vita_power.cfg").string();
   std::string error;
   settings.profile = Profile::Fps30;
   Check(Save(path, settings, &error),
         "save Vita 30 FPS power profile");
   Settings loaded;
   Check(Load(path, loaded) && loaded.profile == Profile::Fps30,
         "load Vita 30 FPS power profile");
   std::filesystem::remove_all(root, filesystemError);
   Check(!filesystemError, "remove temporary Vita power settings");
}

void TestControlLayouts()
{
   using namespace DarkVitaControls;
   Check(LayoutName(ClampLayout(-1)) == std::string("A - Dual Analog FPS"),
         "layout A is the safe default");
   Check(LayoutName(ClampLayout(3)) == std::string("C - Custom"),
         "layout C clamps from out-of-range values");

   Check(PointInTriggerCorner(0.05f, 0.90f, Trigger::Left),
         "left LB icon occupies lower-left corner");
   Check(PointInTriggerCorner(0.95f, 0.90f, Trigger::Right),
         "right RB icon occupies lower-right corner");
   Check(!PointInTriggerCorner(0.50f, 0.90f, Trigger::Left),
         "center touch is not an LB/RB trigger");

   const CustomBindings defaults;
   Check(MapButton(Layout::DualAnalogFps, PhysicalButton::Cross, defaults) ==
         Action::Attack &&
         MapAxis(Layout::DualAnalogFps, AnalogAxis::LeftX, defaults) ==
         AxisAction::MoveHorizontal &&
         MapAxis(Layout::DualAnalogFps, AnalogAxis::RightY, defaults) ==
         AxisAction::LookVertical,
         "Layout A maps face buttons and both analog sticks");
   Check(MapButton(Layout::RetroDpadMove, PhysicalButton::DpadUp, defaults) ==
         Action::MoveForward &&
         MapButton(Layout::RetroDpadMove, PhysicalButton::DpadLeft, defaults) ==
         Action::StrafeLeft &&
         MapAxis(Layout::RetroDpadMove, AnalogAxis::LeftX, defaults) ==
         AxisAction::None,
         "Layout B moves with the D-pad and preserves right-stick look");

   CustomBindings custom;
   Check(custom.SetButton(PhysicalButton::Cross, Action::Use) &&
         custom.SetAxis(AnalogAxis::LeftX, AxisAction::LookHorizontal) &&
         MapButton(Layout::Custom, PhysicalButton::Cross, custom) ==
         Action::Use &&
         MapAxis(Layout::Custom, AnalogAxis::LeftX, custom) ==
         AxisAction::LookHorizontal,
         "Layout C accepts physical button and axis remapping");
   Check(!custom.SetButton(static_cast<PhysicalButton>(99), Action::Use) &&
         !custom.SetAxis(static_cast<AnalogAxis>(99),
                         AxisAction::LookHorizontal),
         "reject out-of-range custom binding identifiers");

   TouchTrigger left;
   Check(UpdateTouchTrigger(Layout::DualAnalogFps, Trigger::Left, left,
                            7, true, 1000) == Action::None,
         "touch trigger does not emit on press");
   Check(UpdateTouchTrigger(Layout::DualAnalogFps, Trigger::Left, left,
                            8, false, 1100) == Action::None && left.down,
         "ignore another finger releasing an active trigger");
   Check(UpdateTouchTrigger(Layout::DualAnalogFps, Trigger::Left, left,
                            7, false, 1250) == Action::Block,
         "short LB press maps to block");
   Check(UpdateTouchTrigger(Layout::DualAnalogFps, Trigger::Left, left,
                            7, true, 2000) == Action::None,
         "LB hold begins without immediate action");
   Check(UpdateTouchTrigger(Layout::DualAnalogFps, Trigger::Left, left,
                            7, true, 2399) == Action::None &&
         UpdateTouchTrigger(Layout::DualAnalogFps, Trigger::Left, left,
                            7, true, 2400) == Action::LeanForward,
         "long LB press triggers exactly once at the hold threshold");
   Check(UpdateTouchTrigger(Layout::DualAnalogFps, Trigger::Left, left,
                            7, false, 2500) == Action::None,
         "long press release does not also trigger the short action");

   TouchTrigger wrapping;
   Check(UpdateTouchTrigger(Layout::DualAnalogFps, Trigger::Right, wrapping,
                            9, true, 0xfffffff0u) == Action::None &&
         UpdateTouchTrigger(Layout::DualAnalogFps, Trigger::Right, wrapping,
                            9, true, 0x00000180u) == Action::CycleWeapon,
         "long-press timing remains correct across SDL tick rollover");

   Check(ShortTriggerAction(Layout::RetroDpadMove, Trigger::Left) ==
         Action::LeanForward &&
         LongTriggerAction(Layout::RetroDpadMove, Trigger::Right) ==
         Action::DropItem,
         "retro layout has distinct trigger mappings");
   Check(ShortTriggerAction(Layout::Custom, Trigger::Right) ==
         Action::UseWeapon &&
         LongTriggerAction(Layout::Custom, Trigger::Right) ==
         Action::QuickLoad,
         "custom layout exposes alternate RB behavior");

   const std::filesystem::path root =
      std::filesystem::temp_directory_path() / "thief-vita-controls-test";
   std::error_code filesystemError;
   std::filesystem::remove_all(root, filesystemError);
   std::filesystem::create_directories(root);
   const std::string path = (root / "vita_controls.cfg").string();
   std::string error;
   Check(Save(path, {Layout::RetroDpadMove}, &error),
         "save Vita control layout");
   Settings loaded = Load(path);
   Check(loaded.layout == Layout::RetroDpadMove,
         "load Vita control layout");
   Settings customSettings;
   customSettings.layout = Layout::Custom;
   customSettings.custom.SetButton(PhysicalButton::Cross, Action::Use);
   customSettings.custom.SetAxis(AnalogAxis::LeftX,
                                 AxisAction::LookHorizontal);
   Check(Save(path, customSettings, &error),
         "save custom physical input bindings");
   loaded = Load(path);
   Check(loaded.layout == Layout::Custom &&
         loaded.custom.GetButton(PhysicalButton::Cross) == Action::Use &&
         loaded.custom.GetAxis(AnalogAxis::LeftX) ==
         AxisAction::LookHorizontal,
         "restore custom physical input bindings");
   std::filesystem::remove_all(root, filesystemError);
   Check(!filesystemError, "remove temporary Vita controls settings");
}
}

int main()
{
   TestSubtitleParser();
   TestModOverlay();
   TestVideoSettings();
   TestTextureLimits();
   TestAcoustics();
   TestPowerProfiles();
   TestControlLayouts();
   std::cout << "Vita content tests passed\n";
   return 0;
}
