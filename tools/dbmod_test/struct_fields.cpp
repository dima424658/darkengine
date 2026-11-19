#include "../../src/framewrk/dbmod/struct_fields.hpp"
#include <cstring>
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
      const char *labels[] = {"Off", "On"};
      sFieldDesc fields[] = {{"Count", kFieldTypeInt, 4, 0},
                             {"Enabled", kFieldTypeBool, 4, 4},
                             {"Mode", kFieldTypeEnum, 4, 8, 0, 0, 1, 2, labels},
                             {"Flags", kFieldTypeBits, 4, 12, 0, 0, 1, 2, labels},
                             {"Text", kFieldTypeString, 8, 16},
                             {"Position", kFieldTypeVector, 12, 24},
                             {"Pointer", kFieldTypeStringPtr, sizeof(void *), 40}};
      sStructDesc description = {"Fixture", 48, 0, 7, fields};
      auto schema = StructFieldSchema(description);
      const auto parse = [](const std::string &body)
      { return ReadFieldEdits(Parse("DML1\nObjProp 1 Fixture {\n" + body + "\n}")[0]); };
      const std::vector<uint8_t> original(48, 0x7a);
      auto staged = StageFieldEdits(original, schema,
                                    parse("Count -2147483648\nEnabled true\nMode on\nFlags Off, "
                                          "On\nText \"hello\"\nPosition 1.5, -2, 3"));
      int32_t count = 0;
      std::memcpy(&count, staged.data(), 4);
      Check(count == INT32_MIN && staged[4] == 1 && staged[8] == 1 && staged[12] == 3,
            "native integer, boolean, enum and named-bit encoding");
      Check(std::memcmp(staged.data() + 16, "hello\0\0\0", 8) == 0, "bounded string encoding");
      float position[3]{};
      std::memcpy(position, staged.data() + 24, sizeof(position));
      Check(position[0] == 1.5f && position[1] == -2 && position[2] == 3, "native vector encoding");
      Check(staged[39] == 0x7a && staged[40] == 0x7a && original[4] == 0x7a,
            "padding, pointers and source remain unchanged");
      for (const char *body :
           {"Count 2147483648", "Count 12garbage", "Enabled truthy", "Mode unknown", "Mode 2",
            "Flags unknown", "Text 12345678", "Position 1, 2", "Position 1, 2, 3, 4",
            "Position 1e100, 0, 0", "Pointer overwrite"})
      {
         bool failed = false;
         try
         {
            StageFieldEdits(original, schema, parse(body));
         }
         catch (const std::exception &)
         {
            failed = true;
         }
         Check(failed, "malformed or unsupported native field value rejected");
      }
      sFieldDesc unnamedBits = {"Bits", kFieldTypeBits, 4, 0};
      sStructDesc bitDescription = {"Bits", 4, 0, 1, &unnamedBits};
      auto bitSchema = StructFieldSchema(bitDescription);
      auto bitValue = StageFieldEdits(std::vector<uint8_t>(4), bitSchema, parse("Bits 10"));
      Check(bitValue[0] == 2, "unnamed bit fields retain native binary notation");
      fields[0].offset = 100;
      bool failed = false;
      try
      {
         StructFieldSchema(description);
      }
      catch (const std::exception &)
      {
         failed = true;
      }
      Check(failed, "invalid native schema bounds reject");
      std::cout << "PASS native struct schema encoders and invalid-value rejection\n";
      return 0;
   }
   catch (const std::exception &error)
   {
      std::cerr << error.what() << '\n';
      return 1;
   }
}
