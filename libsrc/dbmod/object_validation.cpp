#include <types.h>
#include "object_validation.hpp"
#include "object_reference.hpp"
#include <appagg.h>
#include <iobjsys.h>
#include <memory>

namespace DarkDbMod
{
namespace
{
struct Release
{
   void operator()(IObjectSystem *objects) const
   {
      if (objects)
         objects->Release();
   }
};
}
std::vector<ObjectValidation> InspectNativePropertyTargets(const std::vector<Command> &commands,
                                                          bool gamesysPatch)
{
   std::unique_ptr<IObjectSystem, Release> objects(AppGetObj(IObjectSystem));
   if (!objects)
      throw std::runtime_error("DML object service unavailable");
   ObjectReferenceContext context;
   context.exists = [&](int object) { return bool(objects->Exists(object)); };
   context.named = [&](const std::string &name) { return objects->GetObjectNamed(name.c_str()); };
   // This diagnostic runs after loading, not inside the object-load map's
   // lifetime. Numeric IDs deliberately refer to the current database here.
   std::vector<ObjectValidation> results;
   for (const auto &command : commands)
   {
      if (command.kind != CommandKind::ObjectProperty)
         continue;
      ObjectValidation result{command.syntax.line};
      try
      {
         result.object = ResolveObjectReference(command.syntax.arguments.at(1), result.line,
             gamesysPatch ? ObjectScope::Abstract : ObjectScope::Concrete, context);
         result.resolved = true;
         result.diagnostic = "live property target exists within patch scope";
      }
      catch (const std::exception &error)
      {
         result.diagnostic = error.what();
      }
      results.push_back(std::move(result));
   }
   return results;
}
}
