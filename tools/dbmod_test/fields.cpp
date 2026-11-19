#include "../../libsrc/dbmod/fields.hpp"
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
      const auto document = Parse("DML1\nObjProp 1 Test {\n\"Joint\"[1] second\n\"\" \"\"\n}\n");
      auto edits = ReadFieldEdits(document[0]);
      Check(edits.size() == 2 && edits[0].occurrence == 1 && edits[0].name == "Joint",
            "duplicate named field occurrence");
      Check(edits[1].values[0].quoted && edits[1].values[0].text.empty(),
            "empty string value preserved");
      const auto encode = [](const std::vector<Argument> &values)
      {
         if (values.size() != 1 || values[0].text != "second")
            throw std::runtime_error("invalid enum token");
         return std::vector<uint8_t>{42};
      };
      const std::vector<uint8_t> original{1, 2, 3, 4};
      const std::vector<FieldSchema> schema = {{"joint", 1, 1, encode}, {"Joint", 2, 1, encode}};
      edits.resize(1);
      auto staged = StageFieldEdits(original, schema, edits);
      Check(staged == std::vector<uint8_t>({1, 2, 42, 4}), "only selected field changes");
      Check(original == std::vector<uint8_t>({1, 2, 3, 4}), "original remains untouched");
      edits.push_back({"missing", 0, {{"second", false}}, 10});
      bool failed = false;
      try
      {
         StageFieldEdits(original, schema, edits);
      }
      catch (const SyntaxError &)
      {
         failed = true;
      }
      Check(failed && original[2] == 3, "later error does not publish earlier edit");
      edits.resize(1);
      for (const auto &badSchema :
           std::vector<std::vector<FieldSchema>>{{{"Joint", 100, 1, encode}},
                                                 {{"Joint", 0, 100, encode}},
                                                 {{"Joint", 0, 1, {}}},
                                                 {{"Joint", 0, 2, encode}}})
      {
         edits[0].occurrence = 0;
         failed = false;
         try
         {
            StageFieldEdits(original, badSchema, edits);
         }
         catch (const SyntaxError &)
         {
            failed = true;
         }
         Check(failed, "invalid field schema fails before publication");
      }
      edits[0].occurrence = 0;
      edits[0].line = 99;
      edits[0].values = {{"invalid", false}};
      failed = false;
      try { StageFieldEdits(original, schema, edits); }
      catch (const SyntaxError &error) { failed = error.line == 99; }
      Check(failed, "encoder errors retain DML source line");
      for (const char *body : {"Field[-1] value", "Field[99999999999999999999999999] value",
                               "Field[1]", "Field", "Field value { Nested x }"})
      {
         failed = false;
         try
         {
            ReadFieldEdits(Parse(std::string("DML1\nObjProp 1 X {\n") + body + "\n}")[0]);
         }
         catch (const SyntaxError &)
         {
            failed = true;
         }
         Check(failed, "invalid field form rejects");
      }
      std::cout << "PASS DML field occurrences, staging, bounds and late failures\n";
      return 0;
   }
   catch (const std::exception &error)
   {
      std::cerr << error.what() << '\n';
      return 1;
   }
}
