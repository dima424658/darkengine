#pragma once

#include <functional>
#include <string>
#include <vector>

namespace DarkDbMod
{
enum class PatchKind
{
   Mission,
   Gamesys
};
struct DiscoveryRequest
{
   PatchKind kind = PatchKind::Mission;
   std::string databaseName;
   // Caller resolves the documented base-versus-FM database selection first.
   std::string databaseDirectory;
   std::vector<std::string> modsHighToLow;
   std::vector<std::string> resourcesHighToLow;
   bool enabled = true;
   bool loadingSave = false;
};
struct PatchFile
{
   std::string directory;
   std::string relativeName;
};
// Lists regular filenames directly inside root/relativeDirectory. Implementations
// may use filesystem or IStore archives; filenames must not contain separators.
// Missing directories return an empty list; read failures must throw.
using ListPatchFiles = std::function<std::vector<std::string>(
    const std::string &root, const std::string &relativeDirectory)>;
std::vector<PatchFile> DiscoverPatches(const DiscoveryRequest &request, const ListPatchFiles &list);
} // namespace DarkDbMod
