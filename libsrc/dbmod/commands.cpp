#include "commands.hpp"
#include "fingerprint.hpp"
#include <algorithm>
#include <map>

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
[[noreturn]] void Fail(const Statement &statement, const std::string &message)
{
   throw SyntaxError(statement.line, message);
}
void Exact(const Statement &statement, size_t arguments)
{
   if (statement.arguments.size() != arguments || statement.hasBlock)
      Fail(statement, "unexpected arguments or block");
}
void Data(const Statement &statement, size_t header, bool required)
{
   const auto &args = statement.arguments;
   if (statement.hasBlock)
   {
      if (args.size() != header)
         Fail(statement, "unexpected arguments before data block");
      return;
   }
   if (!required && args.size() == header)
      return;
   if (args.size() < header + 2 || args[header].text != "=")
      Fail(statement, "expected data block or = value");
}
Command Compile(const Statement &statement, bool creationProperty)
{
   if (statement.arguments.empty())
      Fail(statement, "empty command");
   if (statement.arguments[0].quoted)
      Fail(statement, "command keywords cannot be quoted");
   std::string key = Lower(statement.arguments[0].text);
   Operation operation = Operation::Modify;
   if (key.substr(0, 2) == "++")
   {
      operation = Operation::Append;
      key.erase(0, 2);
   }
   else if (!key.empty() && (key[0] == '+' || key[0] == '-'))
   {
      operation = key[0] == '+' ? Operation::Ensure : Operation::Remove;
      key.erase(0, 1);
   }
   static const std::map<std::string, CommandKind> kinds = {
       {"#script", CommandKind::Script},
       {"#!script", CommandKind::ExcludeScript},
       {"#schema", CommandKind::Schema},
       {"tagblock", CommandKind::TagBlock},
       {"objprop", CommandKind::ObjectProperty},
       {"objname", CommandKind::ObjectName},
       {"objarch", CommandKind::ObjectArchetype},
       {"metaprop", CommandKind::MetaProperty},
       {"link", CommandKind::Link},
       {"receptron", CommandKind::Receptron},
       {"stimsource", CommandKind::StimulusSource},
       {"create", CommandKind::Create},
       {"createarch", CommandKind::CreateArchetype},
       {"destroy", CommandKind::Destroy},
       {"destroyarch", CommandKind::DestroyArchetype},
       {"destroyarchall", CommandKind::DestroyArchetypeAll},
       {"questvar", CommandKind::QuestVariable},
       {"texture", CommandKind::Texture},
       {"textureall", CommandKind::TextureAll}};
   const auto found = kinds.find(key);
   if (found == kinds.end())
      Fail(statement, "unknown DML command: " + key);
   const CommandKind kind = found->second;
   if (creationProperty && kind != CommandKind::ObjectProperty)
      Fail(statement, "creation blocks may contain only ObjProp commands");
   const bool mutableRecord =
       kind == CommandKind::ObjectProperty || kind == CommandKind::ObjectName ||
       kind == CommandKind::MetaProperty || kind == CommandKind::Link ||
       kind == CommandKind::Receptron || kind == CommandKind::StimulusSource ||
       kind == CommandKind::QuestVariable;
   const bool appendable = kind == CommandKind::MetaProperty || kind == CommandKind::Link ||
                           kind == CommandKind::Receptron || kind == CommandKind::StimulusSource;
   if ((operation != Operation::Modify && !mutableRecord) ||
       (operation == Operation::Append && !appendable) ||
       (kind == CommandKind::ObjectName && operation == Operation::Ensure))
      Fail(statement, "unsupported command prefix");
   Command command{kind, operation, statement, {}};
   const auto &args = statement.arguments;
   const bool remove = operation == Operation::Remove;
   if (kind == CommandKind::ObjectProperty)
   {
      const size_t header = creationProperty ? 2 : 3;
      if (args.size() < header || args[header - 1].text.empty())
         Fail(statement, "expected object/property name");
      if (remove)
         Exact(statement, header);
      else
         Data(statement, header, true);
   }
   else if (kind == CommandKind::TagBlock)
   {
      if (!statement.hasBlock || (args.size() != 2 && args.size() != 4))
         Fail(statement, "expected tag name, optional version pair and data block");
   }
   else if (kind == CommandKind::Link || kind == CommandKind::Receptron ||
            kind == CommandKind::StimulusSource)
   {
      size_t header = kind == CommandKind::Link ? 4 : 3;
      if (args.size() < header)
         Fail(statement, "missing record target/type");
      if (args.size() > header && args[header].text != "=")
      {
         if (operation == Operation::Append)
            Fail(statement, "append record commands cannot specify a link ID");
         ++header;
      }
      if (remove)
         Exact(statement, header);
      else if (kind == CommandKind::Link)
         Data(statement, header, false);
      else if (args.size() != header || !statement.hasBlock)
         Fail(statement, "expected record data block");
   }
   else if (kind == CommandKind::Create || kind == CommandKind::CreateArchetype)
   {
      if (args.size() != 2 && args.size() != 3)
         Fail(statement, "expected archetype and optional object name");
      for (const auto &property : statement.fields)
         command.creationProperties.push_back(Compile(property, true));
   }
   else if (kind == CommandKind::QuestVariable)
      Exact(statement, remove ? 2 : 4);
   else if (kind == CommandKind::ObjectName)
      Exact(statement, remove ? 2 : 3);
   else if (kind == CommandKind::ObjectArchetype || kind == CommandKind::MetaProperty ||
            kind == CommandKind::Texture)
      Exact(statement, 3);
   else if (kind == CommandKind::TextureAll)
      Exact(statement, 4);
   else
      Exact(statement, 2);
   return command;
}
} // namespace
std::vector<Command> ValidateCommandShapes(const std::vector<Statement> &document)
{
   ValidateFingerprints(document);
   std::vector<Command> result;
   bool regular = false;
   for (const auto &statement : document)
   {
      const auto key = Lower(statement.arguments.at(0).text);
      if (key == "fingerprint" || key == "!fingerprint")
         continue;
      Command command = Compile(statement, false);
      const bool directive = command.kind == CommandKind::Script ||
                             command.kind == CommandKind::ExcludeScript ||
                             command.kind == CommandKind::Schema;
      if (directive && regular)
         Fail(statement, "script/schema directives must precede regular commands");
      regular = regular || !directive;
      result.push_back(std::move(command));
   }
   return result;
}
} // namespace DarkDbMod
