#pragma once

#include "syntax.hpp"
#include <functional>

namespace DarkDbMod
{
// Scope describes the role of a reference, not the entire patch. Mission
// commands may still reference abstract archetypes as creation parents.
enum class ObjectScope { Any, Concrete, Abstract };
struct ObjectReferenceContext
{
   std::function<int(const std::string &)> named;
   std::function<bool(int)> exists;
   // Optional and supplied only while the loader's ID map is valid. Names
   // already identify live objects and must never pass through this map.
   std::function<int(int)> remap;
};
int ResolveObjectReference(const Argument &argument, size_t line,
                           ObjectScope scope, const ObjectReferenceContext &context);
}
