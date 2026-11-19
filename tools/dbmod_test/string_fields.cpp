#include "../../src/framewrk/dbmod/string_fields.hpp"
#include <iostream>
#include <stdexcept>

using namespace DarkDbMod;
static void Check(bool condition, const char *reason)
{
   if (!condition)
      throw std::runtime_error(reason);
}
int main()
{
   try
   {
      const auto parse = [](const std::string &text) { return Parse("DML1\n" + text).at(0); };
      auto value = ReadStringPropertyValue(parse("+ObjProp 1 DesignNote = \" spaced  text \""), 3);
      Check(value && *value == " spaced  text ", "quoted whitespace retained");
      value = ReadStringPropertyValue(
          parse("+ObjProp 1 DesignNote {\n\"\" X=\"{target}\"; Y=\"two words\";\n}"), 3);
      Check(value && *value == "X=\"{target}\"; Y=\"two words\";",
            "embedded design-note quotes retained");
      value = ReadStringPropertyValue(parse("+ObjProp 1 DesignNote {\n\"\" \"\"\n}"), 3);
      Check(value && value->empty(), "explicit empty value is an edit");
      value = ReadStringPropertyValue(parse("+ObjProp 1 DesignNote {}"), 3);
      Check(!value, "empty block is not an empty-string assignment");
      value =
          ReadStringPropertyValue(parse("+ObjProp 1 DesignNote {\n\"\" first\n\"\"[0] last\n}"), 3);
      Check(value && *value == "last", "repeated singleton edits retain order");
      value = ReadStringPropertyValue(parse("+ObjProp DesignNote = value"), 2);
      Check(value && *value == "value", "creation-time property header supported");
      auto statement = parse("+ObjProp 1 DesignNote = value");
      statement.arguments.back().text = std::string("bad\0value", 9);
      bool failed = false;
      try
      {
         ReadStringPropertyValue(statement, 3);
      }
      catch (const SyntaxError &)
      {
         failed = true;
      }
      Check(failed, "NUL cannot silently truncate a string property");
      for (const char *text :
           {"+ObjProp 1 DesignNote =", "+ObjProp 1 DesignNote {\nNamed value\n}",
            "+ObjProp 1 DesignNote {\n\"\"[1] value\n}", "+ObjProp 1 DesignNote extra {}"})
      {
         failed = false;
         try
         {
            ReadStringPropertyValue(parse(text), 3);
         }
         catch (const SyntaxError &)
         {
            failed = true;
         }
         Check(failed, "invalid string field form rejected");
      }
      std::cout << "PASS DML string-property ownership, empties, quotes and singleton validation\n";
      return 0;
   }
   catch (const std::exception &error)
   {
      std::cerr << error.what() << '\n';
      return 1;
   }
}
