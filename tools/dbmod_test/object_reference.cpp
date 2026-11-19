#include <object_reference.hpp>
#include <iostream>
#include <stdexcept>

using namespace DarkDbMod;
int main()
{
   try
   {
      int remaps = 0;
      ObjectReferenceContext context;
      context.named = [](const std::string &name) { return name == "7" ? -12 : 0; };
      context.exists = [](int object) { return object == 42 || object == -12; };
      context.remap = [&](int object) {
         ++remaps;
         return object == 7 ? 42 : object;
      };
      if (ResolveObjectReference({"7", false}, 9, ObjectScope::Concrete, context) != 42)
         throw std::runtime_error("numeric ID remapping failed");
      if (ResolveObjectReference({"7", true}, 9, ObjectScope::Abstract, context) != -12 || remaps != 1)
         throw std::runtime_error("quoted numeric names must bypass remapping");
      for (const char *value : {"0", "7junk", "2147483648", "", "name", " 7", "+7"})
      {
         bool rejected = false;
         try
         {
            ResolveObjectReference({value, false}, 9, ObjectScope::Any, context);
         }
         catch (const SyntaxError &error)
         {
            rejected = error.line == 9;
         }
         if (!rejected)
            throw std::runtime_error("malformed numeric reference accepted");
      }
      for (const auto scope : {ObjectScope::Concrete, ObjectScope::Abstract})
      {
         bool rejected = false;
         try
         {
            ResolveObjectReference({scope == ObjectScope::Concrete ? "-12" : "7", false},
                                   9, scope, context);
         }
         catch (const SyntaxError &)
         {
            rejected = true;
         }
         if (!rejected)
            throw std::runtime_error("cross-scope target accepted");
      }
      context.remap = {};
      if (ResolveObjectReference({"-12", false}, 9, ObjectScope::Any, context) != -12)
         throw std::runtime_error("live ID lookup failed");
      const auto expectFailure = [](const Argument &argument,
                                    const ObjectReferenceContext &queries) {
         try
         {
            ResolveObjectReference(argument, 9, ObjectScope::Any, queries);
         }
         catch (const SyntaxError &)
         {
            return;
         }
         throw std::runtime_error("missing object or query accepted");
      };
      expectFailure({"missing", true}, context);
      expectFailure({"", true}, context);
      expectFailure({"99", false}, context);
      context.remap = [](int) { return 0; };
      expectFailure({"7", false}, context);
      context.named = {};
      expectFailure({"7", true}, context);
      context.exists = {};
      expectFailure({"-12", false}, context);
      std::cout << "object reference tests passed\n";
   }
   catch (const std::exception &error)
   {
      std::cerr << error.what() << '\n';
      return 1;
   }
}
