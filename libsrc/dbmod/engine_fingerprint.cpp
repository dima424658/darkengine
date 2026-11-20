#include <types.h>
#include "engine_fingerprint.hpp"
#include <appagg.h>
#include <config.h>
#include <iobjsys.h>
#include <objpos.h>
#include <propman.h>
#include <property.h>
#include <questapi.h>
#include <script_module_query.h>
#include <traitman.h>
#include <memory>
#include <stdexcept>
#include <unordered_map>

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
std::string Lower(std::string name)
{
   for (char &c : name)
      if (c >= 'A' && c <= 'Z')
         c += 'a' - 'A';
   return name;
}
struct Services
{
   std::unique_ptr<IObjectSystem, Release> objects{AppGetObj(IObjectSystem)};
   std::unique_ptr<IPropertyManager, Release> properties{AppGetObj(IPropertyManager)};
   std::unique_ptr<ITraitManager, Release> traits{AppGetObj(ITraitManager)};
   std::unordered_map<std::string, int> missionQuest;
};
} // namespace
FingerprintContext MakeEngineFingerprintContext(const std::string &gamesysName)
{
   auto services = std::make_shared<Services>();
   if (!services->objects || !services->properties || !services->traits)
      throw std::runtime_error("DML fingerprint object/property/trait services unavailable");
   std::unique_ptr<IQuestData, Release> quest(AppGetObj(IQuestData));
   if (!quest)
      throw std::runtime_error("DML fingerprint quest service unavailable");
   std::unique_ptr<IQuestDataIter, Release> iterator(quest->Iter(kQuestDataMission));
   if (!iterator)
      throw std::runtime_error("DML fingerprint mission quest iterator unavailable");
   while (!iterator->Done())
   {
      services->missionQuest[Lower(iterator->Name())] = iterator->Value();
      iterator->Next();
   }
   FingerprintContext context;
   context.gamesys = gamesysName;
   context.exists = [services](int object) { return bool(services->objects->Exists(object)); };
   context.position = [services](int object) -> std::optional<std::array<double, 3>>
   {
      if (object <= 0 || !services->objects->Exists(object))
         return {};
      const auto *position = ObjPosGetLocVector(object);
      if (!position)
         return {};
      return std::array<double, 3>{position->x, position->y, position->z};
   };
   context.quest = [services](const std::string &name) -> std::optional<int>
   {
      const auto found = services->missionQuest.find(Lower(name));
      return found == services->missionQuest.end() ? std::nullopt
                                                   : std::optional<int>(found->second);
   };
   context.directProperty = [services](int object, const std::string &name)
   {
      if (!services->objects->Exists(object))
         return false;
      std::unique_ptr<IProperty, Release> property(
          services->properties->GetPropertyNamed(name.c_str()));
      return property && property->GetID() != PROPID_NULL && property->IsSimplyRelevant(object);
   };
   context.name = [services](int object) -> std::optional<std::string>
   {
      if (!services->objects->Exists(object))
         return {};
      const char *name = services->objects->GetName(object);
      return name ? std::optional<std::string>(name) : std::nullopt;
   };
   context.archetype = [services](int object) -> std::optional<int>
   {
      if (!services->objects->Exists(object))
         return {};
      return services->traits->GetArchetype(object);
   };
   context.script = [](const std::string &name)
   {
      const HRESULT result = ScriptModulePresence(name.c_str());
      if (FAILED(result))
         throw std::runtime_error("DML script-module query unavailable or invalid: " + name);
      return result == S_OK;
   };
   context.config = [](const std::string &name) { return bool(config_is_defined(name.c_str())); };
   return context;
}
} // namespace DarkDbMod
