#pragma once

#include "commands.hpp"

namespace DarkDbMod
{
struct ObjectValidation
{
   size_t line;
   int object = 0;
   bool resolved = false;
   std::string diagnostic;
};
// Snapshot checks for standalone property targets only. Creation-time property
// edits have no live target yet and are not included. This is not an execution
// simulation: earlier Create/ObjName/Destroy commands are never applied.
std::vector<ObjectValidation> InspectNativePropertyTargets(const std::vector<Command> &commands,
                                                          bool gamesysPatch);
}
