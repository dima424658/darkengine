#include "object_reference.hpp"
#include <charconv>

namespace DarkDbMod
{
int ResolveObjectReference(const Argument &argument, size_t line,
                           ObjectScope scope, const ObjectReferenceContext &context)
{
   if (!context.exists)
      throw SyntaxError(line, "object existence query unavailable");
   int object = 0;
   if (argument.quoted)
   {
      if (argument.text.empty() || argument.text.find('\0') != std::string::npos)
         throw SyntaxError(line, "invalid object name");
      if (!context.named)
         throw SyntaxError(line, "object name query unavailable");
      object = context.named(argument.text);
   }
   else
   {
      const char *begin = argument.text.data();
      const char *end = begin + argument.text.size();
      const auto result = std::from_chars(begin, end, object);
      if (result.ec != std::errc() || result.ptr != end || object == 0)
         throw SyntaxError(line, "expected a nonzero object ID or quoted name");
      if (context.remap)
         object = context.remap(object);
   }
   if (object == 0 || !context.exists(object))
      throw SyntaxError(line, "referenced object does not exist");
   if ((scope == ObjectScope::Concrete && object < 0) ||
       (scope == ObjectScope::Abstract && object > 0))
      throw SyntaxError(line, "referenced object is outside the command scope");
   return object;
}
}
