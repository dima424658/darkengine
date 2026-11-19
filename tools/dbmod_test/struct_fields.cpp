#include <struct_fields.hpp>
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
      sFieldDesc geometricFields[] = {{"Fixed", kFieldTypeFix, 4, 0},
                                      {"FixedVec", kFieldTypeFixVec, 12, 4},
                                      {"Angle", kFieldTypeAng, 2, 16},
                                      {"Angles", kFieldTypeAngVec, 6, 18},
                                      {"Point", kFieldTypePoint, 4, 24}};
      sStructDesc geometry = {"Geometry", 32, 0, 5, geometricFields};
      auto geometricSchema = StructFieldSchema(geometry);
      const auto geometryBytes = StageFieldEdits(
          std::vector<uint8_t>(32, 0x5a), geometricSchema,
          parse(
              "Fixed -1.5\nFixedVec 1, -2, 0.5\nAngle -90\nAngles 90, 180, -90\nPoint -123, 456"));
      int32_t fixed[4]{};
      std::memcpy(fixed, geometryBytes.data(), sizeof(fixed));
      Check(fixed[0] == -98304 && fixed[1] == 65536 && fixed[2] == -131072 && fixed[3] == 32768,
            "32-bit fixed point and fixed vector encoding");
      uint16_t angles[4]{};
      std::memcpy(angles, geometryBytes.data() + 16, sizeof(angles));
      Check(angles[0] == 49152 && angles[1] == 49152 && angles[2] == 32768 && angles[3] == 16384,
            "wrapped angles and heading/pitch/bank storage order");
      int16_t point[2]{};
      std::memcpy(point, geometryBytes.data() + 24, sizeof(point));
      Check(point[0] == -123 && point[1] == 456 && geometryBytes[28] == 0x5a,
            "point width and surrounding padding preserved");
      for (const char *body :
           {"Fixed 32768", "FixedVec 0, 0, -32769", "Angle nan", "Point 40000, 0"})
      {
         bool rejected = false;
         try
         {
            StageFieldEdits(std::vector<uint8_t>(32), geometricSchema, parse(body));
         }
         catch (const std::exception &)
         {
            rejected = true;
         }
         Check(rejected, "invalid fixed/angle/point value rejected");
      }
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
