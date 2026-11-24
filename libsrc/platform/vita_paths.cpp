#include "vita_paths.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

namespace
{
struct VitaRootSet
{
   std::vector<std::string> baseRoots;
   std::vector<std::string> readRoots;
};

std::string TrimRoot(std::string root)
{
   while (!root.empty() && root.back() == '/')
      root.pop_back();
   return root;
}

std::vector<std::string> SplitRoots(const char *roots)
{
   std::vector<std::string> result;
   std::string current;
   if (!roots)
      return result;

   for (size_t i = 0; ; ++i)
   {
      const char c = roots[i];
      const bool vitaVolumeColon = c == ':' && current == "ux0";
      if (c == '\0' || ((c == ':' || c == ';') && !vitaVolumeColon))
      {
         current = TrimRoot(std::move(current));
         if (!current.empty())
            result.push_back(std::move(current));
         current.clear();
         if (c == '\0')
            break;
      }
      else
      {
         current.push_back(c);
      }
   }
   return result;
}

std::vector<std::string> DefaultVitaRoots()
{
   const char *titleId = std::getenv("DARK_VITA_TITLE_ID");
   if (titleId)
   {
      const std::string id(titleId);
      if (id == "THFG00001")
         return {"ux0:data/thief1"};
      if (id == "THFG00002")
         return {"ux0:data/thiefgold"};
      if (id == "THFG00003")
         return {"ux0:data/thief2"};
   }
   return {
      "ux0:data/thief1",
      "ux0:data/thiefgold",
      "ux0:data/thief2"
   };
}

std::vector<std::string> BaseVitaRoots()
{
   const char *roots = std::getenv("DARK_VITA_ASSET_ROOTS");
   if (roots && roots[0])
   {
      std::vector<std::string> result = SplitRoots(roots);
      if (!result.empty())
         return result;
   }

   const char *root = std::getenv("DARK_VITA_ASSET_ROOT");
   if (root && root[0])
   {
      std::string value = TrimRoot(root);
      if (!value.empty())
         return {value};
   }
   return DefaultVitaRoots();
}

std::string FoldName(std::string name)
{
   for (char &c : name)
      if (c >= 'A' && c <= 'Z')
         c = static_cast<char>(c + ('a' - 'A'));
   return name;
}

std::vector<std::string> DiscoverModRoots(const std::string &baseRoot)
{
   std::vector<std::string> mods;
   const std::filesystem::path modDirectory =
      std::filesystem::path(baseRoot) / "mods";
   std::error_code error;
   std::filesystem::directory_iterator it(modDirectory, error), end;
   for (; !error && it != end; it.increment(error))
   {
      const std::string name = it->path().filename().string();
      if (name.empty() || name.front() == '.' || name == "." || name == "..")
         continue;
      std::error_code entryError;
      if (it->is_symlink(entryError) || entryError ||
          !it->is_directory(entryError) || entryError)
         continue;
      mods.push_back(it->path().generic_string());
   }
   std::sort(mods.begin(), mods.end(), [](const std::string &left,
                                          const std::string &right)
   {
      const std::string leftName = FoldName(
         std::filesystem::path(left).filename().string());
      const std::string rightName = FoldName(
         std::filesystem::path(right).filename().string());
      return leftName == rightName ? left < right : leftName < rightName;
   });
   return mods;
}

const VitaRootSet &VitaRoots()
{
   static const VitaRootSet roots = []
   {
      VitaRootSet result;
      result.baseRoots = BaseVitaRoots();
      for (const std::string &baseRoot : result.baseRoots)
      {
         const std::vector<std::string> mods = DiscoverModRoots(baseRoot);
         result.readRoots.insert(result.readRoots.end(), mods.begin(),
                                 mods.end());
         result.readRoots.push_back(baseRoot);
      }
      return result;
   }();
   return roots;
}
}

bool PlatformResolveVitaAssetPath(const char *path, char *resolved,
                                  size_t capacity, bool for_write,
                                  bool *exists)
{
   if (!path || !resolved || capacity == 0)
      return false;
   resolved[0] = '\0';
   if (exists)
      *exists = false;
   if (!path[0])
      return false;

   std::string relativeText(path);
   std::replace(relativeText.begin(), relativeText.end(), '\\', '/');
   const std::filesystem::path relative(relativeText);
   if (relative.is_absolute() || relative.has_root_path())
      return false;
   for (const auto &part : relative)
      if (part == "..")
         return false;

   const VitaRootSet &roots = VitaRoots();
   for (const std::string &root : roots.readRoots)
   {
      const std::filesystem::path candidate = std::filesystem::path(root) /
                                               relative;
      std::error_code error;
      const bool candidateExists = std::filesystem::exists(candidate, error);
      if (!error && candidateExists)
      {
         const std::string native = candidate.generic_string();
         if (native.size() >= capacity)
            return false;
         std::copy(native.begin(), native.end(), resolved);
         resolved[native.size()] = '\0';
         if (exists)
            *exists = true;
         return true;
      }
   }

   if (for_write)
   {
      if (!roots.baseRoots.empty())
      {
         const std::filesystem::path candidate =
            std::filesystem::path(roots.baseRoots.front()) / relative;
         const std::string native = candidate.generic_string();
         if (native.size() >= capacity)
            return false;
         std::copy(native.begin(), native.end(), resolved);
         resolved[native.size()] = '\0';
         return true;
      }
   }
   return false;
}
