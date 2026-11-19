#pragma once

#include "syntax.hpp"

namespace DarkDbMod
{
enum class CommandKind
{
   Script,
   ExcludeScript,
   Schema,
   TagBlock,
   ObjectProperty,
   ObjectName,
   ObjectArchetype,
   MetaProperty,
   Link,
   Receptron,
   StimulusSource,
   Create,
   CreateArchetype,
   Destroy,
   DestroyArchetype,
   DestroyArchetypeAll,
   QuestVariable,
   Texture,
   TextureAll
};
enum class Operation
{
   Modify,
   Ensure,
   Append,
   Remove
};
struct Command
{
   CommandKind kind;
   Operation operation;
   Statement syntax;
   std::vector<Command> creationProperties;
};
// Structural command preflight only. Engine schema/value/type checks, object
// resolution, scope constraints and capabilities must be validated separately
// before executing any returned command. No changes are applied here.
std::vector<Command> ValidateCommandShapes(const std::vector<Statement> &document);
} // namespace DarkDbMod
