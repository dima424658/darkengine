#include "vita_paths.h"
#include "vita_subtitles.hpp"

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
}

int main()
{
   TestSubtitleParser();
   TestModOverlay();
   std::cout << "Vita content tests passed\n";
   return 0;
}
