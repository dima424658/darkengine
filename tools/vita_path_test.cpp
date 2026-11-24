#include "vita_paths.h"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

static void require(bool condition)
{
   if (!condition)
   {
      std::fputs("vita path routing check failed\n", stderr);
      std::exit(1);
   }
}

int main()
{
   const std::filesystem::path temp =
      std::filesystem::temp_directory_path() /
      ("dark-vita-path-" + std::to_string(std::rand()));
   const std::filesystem::path thief1 = temp / "thief1";
   const std::filesystem::path thiefgold = temp / "thiefgold";
   std::filesystem::create_directories(thiefgold / "Resources");
   std::ofstream(thiefgold / "Resources" / "stone.bin") << "stone";

   std::string roots =
      thief1.generic_string() + ":" + thiefgold.generic_string();
   setenv("DARK_VITA_ASSET_ROOTS", roots.c_str(), 1);

   char resolved[4096] = {};
   bool exists = false;
   require(PlatformResolveVitaAssetPath(
      "Resources/stone.bin", resolved, sizeof(resolved), false, &exists));
   require(exists);
   require(std::string(resolved) ==
           (thiefgold / "Resources" / "stone.bin").generic_string());

   exists = true;
   require(!PlatformResolveVitaAssetPath(
      "Resources/missing.bin", resolved, sizeof(resolved), false, &exists));
   require(!exists);

   require(PlatformResolveVitaAssetPath(
      "Saves/new.sav", resolved, sizeof(resolved), true, &exists));
   require(std::string(resolved) ==
           (thief1 / "Saves" / "new.sav").generic_string());

   std::filesystem::remove_all(temp);
   std::puts("vita path routing check passed");
   return 0;
}
