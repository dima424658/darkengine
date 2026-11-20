#include <types.h>
#include "property_validation.hpp"
#include "struct_fields.hpp"
#include "string_fields.hpp"
#include <appagg.h>
#include <propbase.h>
#include <propface.h>
#include <property.h>
#include <propman.h>
#include <sdestool.h>
#include <memory>

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
void Validate(const std::vector<Command> &commands, bool creating, IPropertyManager &manager,
              IStructDescTools &tools, std::vector<PropertyValidation> &results)
{
   for (const auto &command : commands)
   {
      if (command.kind == CommandKind::Create || command.kind == CommandKind::CreateArchetype)
      {
         Validate(command.creationProperties, true, manager, tools, results);
         continue;
      }
      if (command.kind != CommandKind::ObjectProperty)
         continue;
      const auto &syntax = command.syntax;
      const size_t propertyIndex = creating ? 1 : 2;
      PropertyValidation result{syntax.line, syntax.arguments.at(propertyIndex).text,
                                PropertyValidation::Status::Unsupported, ""};
      try
      {
         std::unique_ptr<IProperty, Release> property(
             manager.GetPropertyNamed(result.property.c_str()));
         if (!property || property->GetID() == PROPID_NULL)
            result.diagnostic = "property is not registered";
         else if (command.operation == Operation::Remove)
         {
            result.status = PropertyValidation::Status::Valid;
            result.diagnostic = "registered property; no value to encode";
         }
         else
         {
            const auto *type = property->DescribeType();
            const auto *description = type ? tools.Lookup(type->type) : nullptr;
            IStringProperty *rawString = nullptr;
            property->QueryInterface(IID_IStringProperty, reinterpret_cast<void **>(&rawString));
            std::unique_ptr<IStringProperty, Release> stringProperty(rawString);
            if (stringProperty)
            {
               const auto text = ReadStringPropertyValue(syntax, propertyIndex + 1);
               result.status = PropertyValidation::Status::Valid;
               result.diagnostic = text ? "owned string value validated; no property write"
                                        : "empty string-property block; no value assignment";
            }
            else if (!description)
               result.diagnostic = "property has no registered struct schema or string interface";
            else
            {
               auto schema = StructFieldSchema(*description);
               std::vector<FieldEdit> edits;
               if (syntax.hasBlock)
                  edits = ReadFieldEdits(syntax);
               else
               {
                  if (schema.size() != 1)
                     throw SyntaxError(syntax.line, "scalar edit requires a single-field schema");
                  FieldEdit edit{schema[0].name, 0, {}, syntax.line};
                  edit.values.assign(syntax.arguments.begin() + propertyIndex + 2,
                                     syntax.arguments.end());
                  edits.push_back(std::move(edit));
               }
               bool encodable = true;
               for (const auto &edit : edits)
               {
                  size_t occurrence = 0;
                  bool found = false;
                  for (const auto &field : schema)
                  {
                     if (_stricmp(field.name.c_str(), edit.name.c_str()) != 0)
                        continue;
                     if (occurrence++ != edit.occurrence)
                        continue;
                     found = true;
                     if (!field.encode)
                        encodable = false;
                     break;
                  }
                  if (!found)
                  {
                     encodable = false;
                     result.diagnostic = "native schema lacks field/occurrence: " + edit.name;
                  }
               }
               if (!encodable && result.diagnostic.empty())
                  result.diagnostic = "field representation has no strict encoder";
               if (encodable)
               {
                  // Zero bytes are scratch storage for value validation only,
                  // never a replacement for the property's live/default data.
                  StageFieldEdits(std::vector<uint8_t>(description->size), schema, edits);
                  result.status = PropertyValidation::Status::Valid;
                  result.diagnostic = "field names and encoded values validated";
               }
            }
         }
      }
      catch (const std::exception &error)
      {
         result.status = PropertyValidation::Status::Invalid;
         result.diagnostic = error.what();
      }
      results.push_back(std::move(result));
   }
}
} // namespace
std::vector<PropertyValidation> ValidateNativePropertyFields(const std::vector<Command> &commands)
{
   std::unique_ptr<IPropertyManager, Release> manager(AppGetObj(IPropertyManager));
   std::unique_ptr<IStructDescTools, Release> tools(AppGetObj(IStructDescTools));
   if (!manager || !tools)
      throw std::runtime_error("DML property/schema services unavailable");
   std::vector<PropertyValidation> results;
   Validate(commands, false, *manager, *tools, results);
   return results;
}
} // namespace DarkDbMod
