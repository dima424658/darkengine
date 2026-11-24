#include "vita_paths.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

namespace
{
const std::vector<std::string> DefaultVitaRoots()
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

std::vector<std::string> VitaRoots()
{
   const char *roots = std::getenv("DARK_VITA_ASSET_ROOTS");
   if (roots && roots[0])
   {
      std::vector<std::string> result;
      std::string current;
      for (const char *p = roots; ; ++p)
      {
         if (*p == ':' || *p == '\0')
         {
            while (!current.empty() && current.back() == '/')
               current.pop_back();
            if (!current.empty())
               result.push_back(current);
            current.clear();
            if (*p == '\0')
               break;
         }
         else
         {
            current.push_back(*p);
         }
      }
      if (!result.empty())
         return result;
   }

   const char *root = std::getenv("DARK_VITA_ASSET_ROOT");
   if (root && root[0])
   {
      std::string value(root);
      while (!value.empty() && value.back() == '/')
         value.pop_back();
      if (!value.empty())
         return {value};
   }
   return DefaultVitaRoots();
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
   if (!path[0] || std::filesystem::path(path).is_absolute())
      return false;

   const std::string relative(path);
   for (const std::string &root : VitaRoots())
   {
      const std::filesystem::path candidate =
         std::filesystem::path(root) / std::filesystem::path(relative);
      std::error_code error;
      const bool candidateExists = std::filesystem::exists(candidate, error);
      if (candidateExists)
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
      const std::vector<std::string> roots = VitaRoots();
      if (!roots.empty())
      {
         const std::filesystem::path candidate =
            std::filesystem::path(roots.front()) /
            std::filesystem::path(relative);
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
