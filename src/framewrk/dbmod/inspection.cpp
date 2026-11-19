#include <types.h>
#include "inspection.hpp"
#include "commands.hpp"
#include "fingerprint.hpp"
#include "engine_fingerprint.hpp"
#include "property_validation.hpp"
#include <appagg.h>
#include <binrstyp.h>
#include <mprintf.h>
#include <render_backend.h>
#include <resapi.h>
#include <storeapi.h>
#include <memory>
#include <stdexcept>

namespace DarkDbMod
{
namespace
{
struct Release
{
   template <class T> void operator()(T *value) const
   {
      if (value)
         value->Release();
   }
};
struct Unlock
{
   IRes *resource;
   ~Unlock()
   {
      resource->Unlock();
   }
};
} // namespace
bool InspectPatch(const char *filename, bool gamesysPatch, const char *gamesysName,
                  bool matchCurrentDatabase, bool validatePropertyFields)
{
   const std::string path = filename ? filename : "";
   try
   {
      if (path.empty())
         throw std::runtime_error("expected a DML filename");
      std::unique_ptr<IResMan, Release> manager(AppGetObj(IResMan));
      if (!manager)
         throw std::runtime_error("resource manager is unavailable");
      const auto slash = path.find_last_of("/\\");
      const std::string directory = slash == std::string::npos ? "." : path.substr(0, slash + 1);
      const std::string name = slash == std::string::npos ? path : path.substr(slash + 1);
      if (name.empty())
         throw std::runtime_error("expected a filename, not a directory");
      std::unique_ptr<IStore, Release> store(manager->GetStore(directory.c_str()));
      if (!store || !store->Exists() || !store->StreamExists(name.c_str()))
         throw std::runtime_error("patch resource does not exist");
      std::unique_ptr<IRes, Release> resource(
          manager->BindSpecific(name.c_str(), RESTYPE_BINARY, store.get(), nullptr, 0));
      if (!resource)
         throw std::runtime_error("cannot bind patch resource");
      const long size = resource->GetSize();
      if (size < 4 || size > 4 * 1024 * 1024)
         throw std::runtime_error("patch resource size is invalid or exceeds 4 MiB");
      std::string text;
      {
         const auto *data = static_cast<const char *>(resource->Lock());
         if (!data)
            throw std::runtime_error("cannot lock patch resource");
         Unlock unlock{resource.get()};
         text.assign(data, size);
      }
      const auto syntax = Parse(text);
      ValidateFingerprints(syntax, gamesysPatch);
      const auto commands = ValidateCommandShapes(syntax);
      std::string schemaResult = "unchecked";
      if (validatePropertyFields)
      {
         const auto properties = ValidateNativePropertyFields(commands);
         unsigned valid = 0;
         unsigned unsupported = 0;
         unsigned invalid = 0;
         for (const auto &property : properties)
         {
            const char *status = property.status == PropertyValidation::Status::Valid ? "valid"
                                 : property.status == PropertyValidation::Status::Unsupported
                                     ? "unsupported"
                                     : "invalid";
            valid += property.status == PropertyValidation::Status::Valid;
            unsupported += property.status == PropertyValidation::Status::Unsupported;
            invalid += property.status == PropertyValidation::Status::Invalid;
            mprintf("DML property check %s: %s:%u %s: %s\n", status, path.c_str(),
                    unsigned(property.line), property.property.c_str(),
                    property.diagnostic.c_str());
            RenderBackendTrace("dbmod-property-check status=%s line=%u property=%s diagnostic=%s",
                               status, unsigned(property.line), property.property.c_str(),
                               property.diagnostic.c_str());
         }
         schemaResult = invalid ? "invalid" : (unsupported ? "unsupported" : "valid");
         RenderBackendTrace("dbmod-property-summary valid=%u unsupported=%u invalid=%u", valid,
                            unsupported, invalid);
      }
      std::string match = "unchecked";
      if (matchCurrentDatabase)
      {
         const auto context = MakeEngineFingerprintContext(gamesysName ? gamesysName : "");
         match = MatchFingerprints(syntax, context, gamesysPatch) ? "accept" : "reject";
      }
      const char *result = schemaResult == "invalid"       ? "error"
                           : schemaResult == "unsupported" ? "unsupported"
                                                           : "ok";
      mprintf("DML check %s: %s (%u statements, %u commands; fingerprint %s; schema %s; no changes "
              "applied)\n",
              result, path.c_str(), unsigned(syntax.size()), unsigned(commands.size()),
              match.c_str(), schemaResult.c_str());
      RenderBackendTrace(
          "dbmod-check result=%s path=%s statements=%u commands=%u gamesys=%d match=%s schema=%s",
          result, path.c_str(), unsigned(syntax.size()), unsigned(commands.size()),
          int(gamesysPatch), match.c_str(), schemaResult.c_str());
      return schemaResult != "invalid" && schemaResult != "unsupported";
   }
   catch (const std::exception &error)
   {
      mprintf("DML check FAILED: %s: %s (no changes applied)\n", path.c_str(), error.what());
      RenderBackendTrace("dbmod-check result=error path=%s diagnostic=%s", path.c_str(),
                         error.what());
      return false;
   }
}
} // namespace DarkDbMod
