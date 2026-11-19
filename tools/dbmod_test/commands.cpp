#include "../../libsrc/dbmod/commands.hpp"
#include <iostream>
#include <stdexcept>

using namespace DarkDbMod;
static void Check(bool value, const char *reason)
{
   if (!value)
      throw std::runtime_error(reason);
}
int main()
{
   try
   {
      auto commands = ValidateCommandShapes(
          Parse("DML1\nFINGERPRINT { GAM test.gam }\n#script custom\n#!script bad\n#schema schema\n"
                "+ObjProp 1 Alpha = 0.5\n-ObjProp 1 Other\n"
                "Create Parent Child {\n+ObjProp Name = value\n}\n"
                "++Link 1 2 ControlDevice\n-Link 1 2 ControlDevice 01000001\n"
                "++MetaProp 1 Meta\nObjName 1 Name\n-ObjName 1\n"
                "ObjArch 1 Parent\nDestroy 2\nQuestVar flags = 7\n-QuestVar other\n"
                "TagBlock Test 1 0 { Field value }\nTexture -1 -2\nTextureAll 1 2 -3\n"));
      Check(commands.size() == 18, "validated command count");
      Check(commands[3].kind == CommandKind::ObjectProperty &&
                commands[3].operation == Operation::Ensure,
            "property ensure semantics retained");
      Check(commands[5].creationProperties.size() == 1, "creation-time property commands compiled");
      Check(commands[6].operation == Operation::Append, "append operation retained");
      for (const char *invalid :
           {"\"ObjName\" 1 X", "Unknown 1", "++ObjProp 1 X = 2", "+ObjName 1 X",
            "-ObjProp 1 X { Field value }", "ObjProp 1 X",
            "ObjProp 1 X =", "Create Parent { Destroy 1 }", "ObjName 1 X\n#script late",
            "++Link 1 2 ControlDevice 01000001", "-Link 1 2 ControlDevice { Field value }",
            "TagBlock Name", "TextureAll 1 2"})
      {
         bool failed = false;
         try
         {
            ValidateCommandShapes(Parse(std::string("DML1\n") + invalid));
         }
         catch (const SyntaxError &)
         {
            failed = true;
         }
         Check(failed, "invalid command must fail preflight");
      }
      std::cout << "PASS DML command forms, prefixes, creation properties and directive order\n";
      return 0;
   }
   catch (const std::exception &error)
   {
      std::cerr << error.what() << '\n';
      return 1;
   }
}
