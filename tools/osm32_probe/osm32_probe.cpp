#include <osm_emu/osm32img.h>
#include <osm_emu/osm32imports.h>

#include <stdio.h>
#include <string.h>
#include <string>

int main(int argc, char **argv)
{
   bool showImports = false;
   bool strictImports = false;
   int firstModule = 1;
   while (firstModule < argc && argv[firstModule][0] == '-')
   {
      if (!strcmp(argv[firstModule], "--imports")) showImports = true;
      else if (!strcmp(argv[firstModule], "--strict-imports"))
      {
         showImports = true;
         strictImports = true;
      }
      else
      {
         fprintf(stderr, "unknown option: %s\n", argv[firstModule]);
         return 2;
      }
      ++firstModule;
   }
   if (firstModule >= argc)
   {
      fprintf(stderr, "usage: osm32_probe [--imports|--strict-imports] "
                      "module.osm [module.osm ...]\n");
      return 2;
   }

   int failures = 0;
   for (int i = firstModule; i < argc; ++i)
   {
      osm32::cImage image;
      std::string error;
      if (!image.Load(argv[i], &error))
      {
         fprintf(stderr, "%s: FAIL: %s\n", argv[i], error.c_str());
         ++failures;
         continue;
      }

      const uint32_t init = image.FindExport("_ScriptModuleInit@20");
      if (!init)
      {
         fprintf(stderr, "%s: FAIL: no _ScriptModuleInit@20 export\n", argv[i]);
         ++failures;
         continue;
      }

      if (!image.Relocate(image.PreferredBase() + 0x10000000u, &error))
      {
         fprintf(stderr, "%s: FAIL: relocation: %s\n", argv[i], error.c_str());
         ++failures;
         continue;
      }

      unsigned unsupported = 0;
      const std::vector<osm32::sImport> &imports = image.Imports();
      for (size_t j = 0; j < imports.size(); ++j)
      {
         const bool supported = osm32::IsSupportedImport(imports[j]);
         if (!supported) ++unsupported;
         if (showImports)
            printf("  %s!%s: %s\n", imports[j].module.c_str(),
                   imports[j].byOrdinal ? "<ordinal>" :
                                           imports[j].name.c_str(),
                   supported ? "supported" : "UNSUPPORTED");
      }

      if (strictImports && unsupported)
      {
         fprintf(stderr, "%s: FAIL: %u unsupported import(s)\n", argv[i],
                 unsupported);
         ++failures;
         continue;
      }

      printf("%s: PE32 base=%08X size=%08X entry=%08X init=%08X "
             "imports=%u unsupported=%u PASS\n",
             argv[i], image.PreferredBase(), image.ImageSize(), image.EntryPoint(),
             image.FindExport("_ScriptModuleInit@20"),
             static_cast<unsigned>(image.Imports().size()), unsupported);
   }
   return failures ? 1 : 0;
}
