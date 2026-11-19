#include "discovery.hpp"
#include <algorithm>
#include <map>
#include <set>
#include <stdexcept>

namespace DarkDbMod
{
namespace
{
std::string Lower(std::string value)
{
   for (char &c : value)
      if (c >= 'A' && c <= 'Z')
         c += 'a' - 'A';
   return value;
}
bool Basename(const std::string &value)
{
   return !value.empty() && value != "." && value != ".." &&
          value.find_first_of("/\\:") == std::string::npos;
}
bool Dml(const std::string &value)
{
   return value.size() > 4 && Lower(value.substr(value.size() - 4)) == ".dml";
}
class Discovery
{
   const ListPatchFiles &list;
   std::map<std::pair<std::string, std::string>, std::vector<std::string>> listings;
   std::set<std::pair<std::string, std::string>> seen;
   std::vector<PatchFile> result;
   const std::vector<std::string> &Files(const std::string &root, const std::string &directory)
   {
      const auto key = std::make_pair(root, directory);
      auto found = listings.find(key);
      if (found != listings.end())
         return found->second;
      auto files = list(root, directory);
      files.erase(std::remove_if(files.begin(), files.end(),
                                 [](const std::string &file) { return !Dml(file); }),
                  files.end());
      std::set<std::string> names;
      for (const auto &file : files)
      {
         if (!Basename(file))
            throw std::runtime_error("DML directory adapter returned a non-filename: " + file);
         if (!names.insert(Lower(file)).second)
            throw std::runtime_error("ambiguous case-insensitive DML directory entry: " + file);
      }
      std::sort(files.begin(), files.end(),
                [](const std::string &a, const std::string &b) { return Lower(a) < Lower(b); });
      return listings.emplace(key, std::move(files)).first->second;
   }
   void Add(const std::string &root, const std::string &directory, const std::string &name)
   {
      const std::string relative = directory.empty() ? name : directory + "/" + name;
      if (seen.emplace(root, Lower(relative)).second)
         result.push_back({root, relative});
   }
   void Named(const std::string &root, const std::string &name)
   {
      if (root.empty())
         return;
      for (const auto &file : Files(root, ""))
         if (Lower(file) == Lower(name))
            Add(root, "", file);
   }
   void Directory(const std::string &root, const std::string &directory)
   {
      if (root.empty())
         return;
      for (const auto &file : Files(root, directory))
         if (Dml(file))
            Add(root, directory, file);
   }

 public:
   explicit Discovery(const ListPatchFiles &reader) : list(reader)
   {
   }
   std::vector<PatchFile> Run(const DiscoveryRequest &request)
   {
      if (!request.enabled || (request.kind == PatchKind::Mission && request.loadingSave))
         return {};
      if (!list)
         throw std::runtime_error("missing DML directory adapter");
      if (!Basename(request.databaseName))
         throw std::runtime_error("DML discovery requires a database basename");
      const std::string all = request.kind == PatchKind::Mission ? "miss_all.dml" : "gamesys.dml";
      for (const auto &name : {all, request.databaseName + ".dml"})
      {
         Named(request.databaseDirectory, name);
         for (auto root = request.modsHighToLow.rbegin(); root != request.modsHighToLow.rend();
              ++root)
            Named(*root, name);
      }
      // Directory patches preserve layer priority. Within each layer, global
      // mission patches precede specific mission patches, each alphabetically.
      for (auto root = request.resourcesHighToLow.rbegin();
           root != request.resourcesHighToLow.rend(); ++root)
      {
         if (request.kind == PatchKind::Mission)
         {
            Directory(*root, "dbmods/miss_all");
            Directory(*root, "dbmods/" + request.databaseName);
         }
         else
            Directory(*root, "dbmods");
      }
      return result;
   }
};
} // namespace
std::vector<PatchFile> DiscoverPatches(const DiscoveryRequest &request, const ListPatchFiles &list)
{
   return Discovery(list).Run(request);
}
} // namespace DarkDbMod
