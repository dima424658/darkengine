#include "../../libsrc/dbmod/discovery.hpp"
#include <iostream>
#include <map>
#include <stdexcept>

using namespace DarkDbMod;
static void Check(bool value, const char *reason)
{
   if (!value)
      throw std::runtime_error(reason);
}
int main()
{
   try
   {
      std::map<std::pair<std::string, std::string>, std::vector<std::string>> files = {
          {{"base", ""},
           {"miss_all.dml", "miss1.mis.dml", "gamesys.dml", "README.txt", "readme.txt"}},
          {{"A", ""}, {"miss_all.dml"}},
          {{"B", ""}, {"miss_all.dml", "miss1.mis.dml"}},
          {{"B", "dbmods/miss_all"}, {"gen.dml"}},
          {{"B", "dbmods/miss1.mis"}, {"torchfix.dml", "skyfix.dml", "notes.txt"}},
          {{"A", "dbmods/miss1.mis"}, {"barrelfix.dml"}},
          {{"A", "dbmods"}, {"weakerWeapons.dml", "tougherAI.dml"}},
          {{"B", "dbmods"}, {"tougherAI.dml", "cheaperLoot.dml"}}};
      auto list = [&](const std::string &root, const std::string &directory)
      { return files[{root, directory}]; };
      auto paths = [](const std::vector<PatchFile> &patches)
      {
         std::vector<std::string> result;
         for (const auto &patch : patches)
            result.push_back(patch.directory + "/" + patch.relativeName);
         return result;
      };
      DiscoveryRequest request;
      request.databaseName = "miss1.mis";
      request.databaseDirectory = "base";
      request.modsHighToLow = {"A", "B"};
      request.resourcesHighToLow = {"A", "B", "base"};
      const std::vector<std::string> expected = {"base/miss_all.dml",
                                                 "B/miss_all.dml",
                                                 "A/miss_all.dml",
                                                 "base/miss1.mis.dml",
                                                 "B/miss1.mis.dml",
                                                 "B/dbmods/miss_all/gen.dml",
                                                 "B/dbmods/miss1.mis/skyfix.dml",
                                                 "B/dbmods/miss1.mis/torchfix.dml",
                                                 "A/dbmods/miss1.mis/barrelfix.dml"};
      Check(paths(DiscoverPatches(request, list)) == expected,
            "documented mission ordering example");
      request.loadingSave = true;
      Check(DiscoverPatches(request, {}).empty(), "save mission skips all patch discovery");
      request.loadingSave = false;
      request.enabled = false;
      Check(DiscoverPatches(request, {}).empty(), "disabled discovery does not touch storage");
      request.enabled = true;
      request.kind = PatchKind::Gamesys;
      request.databaseName = "dark.gam";
      const std::vector<std::string> gamesys = {"base/gamesys.dml", "B/dbmods/cheaperLoot.dml",
                                                "B/dbmods/tougherAI.dml", "A/dbmods/tougherAI.dml",
                                                "A/dbmods/weakerWeapons.dml"};
      Check(paths(DiscoverPatches(request, list)) == gamesys,
            "documented gamesys ordering example");
      request.modsHighToLow.push_back("base");
      request.resourcesHighToLow.push_back("A");
      Check(paths(DiscoverPatches(request, list)).size() == gamesys.size(),
            "duplicate roots do not reapply files");
      request.databaseDirectory = "fm";
      files[{"fm", ""}] = {"gamesys.dml", "DARK.GAM.DML"};
      auto selected = paths(DiscoverPatches(request, list));
      Check(selected.front() == "fm/gamesys.dml", "caller-selected FM base takes precedence");
      Check(selected[2] == "fm/DARK.GAM.DML", "case-insensitive database sidecar name");
      files[{"fm", ""}] = {"gamesys.dml", "GAMESYS.DML"};
      bool failed = false;
      try
      {
         DiscoverPatches(request, list);
      }
      catch (const std::runtime_error &)
      {
         failed = true;
      }
      Check(failed, "ambiguous case-folded files reject discovery");
      files[{"fm", ""}] = {"../escape.dml"};
      failed = false;
      try
      {
         DiscoverPatches(request, list);
      }
      catch (const std::runtime_error &)
      {
         failed = true;
      }
      Check(failed, "directory adapter cannot inject paths");
      request.databaseName = "../miss1.mis";
      failed = false;
      try
      {
         DiscoverPatches(request, list);
      }
      catch (const std::runtime_error &)
      {
         failed = true;
      }
      Check(failed, "database argument must be a basename");
      std::cout
          << "PASS DML mission/gamesys discovery order, skips, case handling and duplicates\n";
      return 0;
   }
   catch (const std::exception &error)
   {
      std::cerr << error.what() << '\n';
      return 1;
   }
}
