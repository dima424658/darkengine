#include "../../src/framewrk/dbmod/syntax.hpp"
#include "../../src/framewrk/dbmod/fingerprint.hpp"
#include "../../src/framewrk/dbmod/commands.hpp"
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>

static void Check(bool value, const char *message)
{
   if (!value)
      throw std::runtime_error(message);
}
int main(int argc, char **argv)
{
   try
   {
      if (argc > 1)
      {
         for (int index = 1; index < argc; ++index)
         {
            std::ifstream file(argv[index], std::ios::binary);
            Check(bool(file), "cannot open input");
            std::string text((std::istreambuf_iterator<char>(file)), {});
            const auto statements = DarkDbMod::Parse(text);
            DarkDbMod::ValidateFingerprints(statements);
            DarkDbMod::ValidateCommandShapes(statements);
            std::cout << argv[index] << ": " << statements.size() << " statements\n";
         }
         return 0;
      }
      auto statements = DarkDbMod::Parse(
          "DML1\r\n!FINGERPRINT { GAM tfix.gam }\n"
          "#script \"CustomScripts\"\n//@pre\n"
          "+ObjProp 42 \"Position\" { \"Location\" 1, 2, 3 }\n"
          "//@post\nCreate \"Parent\" \"Child\"\n{\n+ObjProp \"Label\" = \"\"\n}\n");
      Check(statements.size() == 4, "top-level statements");
      Check(statements[2].phase == DarkDbMod::Phase::Pre, "pre-load phase");
      Check(statements[3].phase == DarkDbMod::Phase::Post, "post-load phase");
      Check(statements[2].fields[0].arguments.size() == 4, "vector value components");
      Check(statements[3].fields[0].arguments.back().quoted &&
                statements[3].fields[0].arguments.back().text.empty(), "quoted empty scalar");
      Check(statements[2].line == 5, "source line accounting");
      statements = DarkDbMod::Parse("DML1\n/* { ignored } */\nObjName 1 \"// { name }\"\n");
      Check(statements.size() == 1 && statements[0].arguments.back().text == "// { name }",
            "comments and quoted syntax");
      statements = DarkDbMod::Parse(
          "DML1\n+ObjProp 1 DesignNote {\n\"\" Target=\"^{1.7:Candle\"; Message=\"Frob World\";\n}\n");
      Check(statements.size() == 1 && statements[0].fields.size() == 1,
            "embedded quoted braces in raw design notes");
      for (const char *invalid : {" DML1", "DML10", "DML1\n}", "DML1\nObjProp 1 X {",
                                  "DML1\n/*", "DML1\nObjName 1 \"unfinished"})
      {
         bool failed = false;
         try
         {
            DarkDbMod::Parse(invalid);
         }
         catch (const DarkDbMod::SyntaxError &)
         {
            failed = true;
         }
         Check(failed, "malformed syntax rejected");
      }
      std::cout << "PASS DML syntax, phases, nested fields, comments and diagnostics\n";
      return 0;
   }
   catch (const std::exception &error)
   {
      std::cerr << error.what() << '\n';
      return 1;
   }
}
