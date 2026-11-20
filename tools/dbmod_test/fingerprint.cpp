#include <fingerprint.hpp>
#include <iostream>
#include <stdexcept>

using namespace DarkDbMod;
static void Check(bool value, const char *reason)
{
   if (!value)
      throw std::runtime_error(reason);
}
static std::string Positions(int count)
{
   std::string result = "DML1\nFINGERPRINT\n{\n";
   for (int index = 1; index <= count; ++index)
      result += std::to_string(index) + " [0 0 0]\n";
   return result + "}\n";
}
int main()
{
   try
   {
      FingerprintContext context;
      context.gamesys = "TFIX.GAM";
      auto match = [&](const std::string &source)
      { return MatchFingerprints(Parse(source), context); };
      Check(match("DML1\nObjProp 1 X = 2\n"), "no fingerprint accepts");
      Check(match("DML1\nFINGERPRINT { GAM tfix.gam }\n"), "gamesys case folding");
      Check(!match("DML1\n!FINGERPRINT { GAM tfix.gam }\n"), "negative fingerprint veto");
      Check(match("DML1\nFINGERPRINT { GAM other.gam }\nFINGERPRINT { GAM tfix.gam }"),
            "positive fingerprints use OR");
      Check(!match("DML1\nFINGERPRINT { GAM tfix.gam }\n!FINGERPRINT { GAM tfix.gam }"),
            "negative veto wins over positive match");
      context.quest = [](const std::string &key) -> std::optional<int>
      {
         if (key == "flags")
            return 15;
         return {};
      };
      Check(match("DML1\nFINGERPRINT { QVAR flags & 7 == 7 }"), "masked qvar");
      Check(match("DML1\nFINGERPRINT { !QVAR absent < 2 }"), "negative missing qvar comparison");
      Check(!match("DML1\nFINGERPRINT { QVAR absent < 2 }"), "positive missing qvar comparison");
      Check(match("DML1\nFINGERPRINT { !QVAR flags >= 15 }"),
            "negative existing qvar still compares normally");
      Check(!match("DML1\nFINGERPRINT { !QVAR flags }"), "negative presence check");
      int available = 7;
      double distance = 3;
      context.position = [&](int object) -> std::optional<std::array<double, 3>>
      {
         if (object <= available)
            return std::array<double, 3>{distance, 0, 0};
         return {};
      };
      Check(match(Positions(8)), "eight positions require seven within three units");
      Check(match(Positions(9)), "nine positions require seven");
      Check(match(Positions(10)), "ten positions permit three failures");
      available = 12;
      Check(match(Positions(15)), "fifteen positions permit three failures");
      available = 11;
      Check(!match(Positions(15)), "fifteen positions reject four failures");
      available = 6;
      Check(!match(Positions(8)), "too few matching objects reject");
      available = 15;
      distance = 3.001;
      Check(!match(Positions(15)), "out of tolerance rejects");
      context.exists = [](int object) { return object == 1; };
      context.directProperty = [](int object, const std::string &property)
      { return object == 1 && property == "direct"; };
      context.name = [](int object) -> std::optional<std::string>
      { return object == 1 ? std::optional<std::string>("Object") : std::nullopt; };
      context.archetype = [](int object) -> std::optional<int>
      { return object == 1 ? std::optional<int>(-3) : std::nullopt; };
      context.script = [](const std::string &name) { return name == "script"; };
      context.config = [](const std::string &name) { return name == "configured"; };
      context.mod = [](const std::string &name) { return name == "mod"; };
      Check(match("DML1\nFINGERPRINT {\nEXIST 1\n!EXIST 2\nPROP 1 direct\n"
                  "!PROP 1 inherited\nNAME 1 object\n!NAME 2 absent\nARCH 1 -3\n"
                  "SCR script\nCFG configured\nMOD mod\n}"),
            "combined named rules use AND");
      for (const std::string &source :
           {"DML1\nFINGERPRINT { UNKNOWN x }", "DML1\nFINGERPRINT {}",
            "DML1\nObjProp 1 X = 2\nFINGERPRINT { GAM tfix.gam }",
            "DML1\nFINGERPRINT { QVAR flags && 1 == 1 }", "DML1\nFINGERPRINT { MOD mod >= 1.0 }",
            "DML1\nFINGERPRINT { GAM tfix.gam }\n!FINGERPRINT { UNKNOWN x }"})
      {
         bool failed = false;
         try
         {
            match(source);
         }
         catch (const SyntaxError &)
         {
            failed = true;
         }
         Check(failed, "malformed/unsupported fingerprints cannot accept a patch");
      }
      bool failed = false;
      try
      {
         MatchFingerprints(Parse("DML1\n!FINGERPRINT { CFG missing }"), FingerprintContext{});
      }
      catch (const SyntaxError &)
      {
         failed = true;
      }
      Check(failed, "missing adapter must not turn a negative rule into acceptance");
      failed = false;
      try
      {
         ValidateFingerprints(Parse("DML1\nFINGERPRINT { QVAR flags }"), true);
      }
      catch (const SyntaxError &)
      {
         failed = true;
      }
      Check(failed, "gamesys fingerprint restrictions");
      std::cout << "PASS DML fingerprints, tolerance, precedence, queries and rejection\n";
      return 0;
   }
   catch (const std::exception &error)
   {
      std::cerr << error.what() << '\n';
      return 1;
   }
}
