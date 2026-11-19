#include "fingerprint.hpp"
#include <charconv>
#include <cmath>
#include <cstdint>
#include <locale>
#include <set>
#include <sstream>

namespace DarkDbMod
{
namespace
{
std::string Lower(std::string value)
{
   for (char &c : value)
      if (c >= 'A' && c <= 'Z')
         c += 'a' - 'A';
   return value;
}
int Integer(const std::string &text, size_t line)
{
   int value = 0;
   const auto parsed = std::from_chars(text.data(), text.data() + text.size(), value);
   if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size())
      throw SyntaxError(line, "expected signed decimal integer: " + text);
   return value;
}
struct Rule
{
   std::string kind;
   std::string text;
   std::string comparison;
   bool negative = false;
   int object = 0;
   int value = 0;
   int mask = -1;
   std::array<double, 3> position{};
   size_t line = 0;
};
struct Fingerprint
{
   bool negative = false;
   std::vector<Rule> rules;
   size_t positions = 0;
};
bool IsComparison(const std::string &op)
{
   return op == "==" || op == "!=" || op == "<" || op == ">" || op == "<=" || op == ">=";
}
std::vector<Fingerprint> Compile(const std::vector<Statement> &document, bool gamesys)
{
   std::vector<Fingerprint> result;
   bool commands = false;
   for (const auto &statement : document)
   {
      const auto key = Lower(statement.arguments.at(0).text);
      if (key != "fingerprint" && key != "!fingerprint")
      {
         commands = true;
         continue;
      }
      if (commands || statement.arguments.size() != 1 || !statement.hasBlock ||
          statement.fields.empty())
         throw SyntaxError(statement.line, "fingerprints must be nonempty leading blocks");
      Fingerprint fingerprint;
      fingerprint.negative = key.front() == '!';
      std::set<int> positions;
      for (const auto &field : statement.fields)
      {
         const auto &args = field.arguments;
         Rule rule;
         rule.line = field.line;
         rule.kind = Lower(args.at(0).text);
         rule.negative = rule.kind.front() == '!';
         if (rule.negative)
            rule.kind.erase(0, 1);
         if (field.hasBlock)
            throw SyntaxError(field.line, "fingerprint rules cannot have nested blocks");
         if (!rule.kind.empty() && rule.kind.front() >= '0' && rule.kind.front() <= '9')
         {
            rule.object = Integer(rule.kind, field.line);
            if (rule.negative || rule.object <= 0 || !positions.insert(rule.object).second)
               throw SyntaxError(field.line, "position IDs must be unique concrete objects");
            std::string coordinates;
            for (size_t index = 1; index < args.size(); ++index)
               coordinates += args[index].text + " ";
            std::istringstream input(coordinates);
            input.imbue(std::locale::classic());
            char open = 0;
            char close = 0;
            if (!(input >> open >> rule.position[0] >> rule.position[1] >> rule.position[2] >>
                  close) ||
                open != '[' || close != ']' || !(input >> std::ws).eof())
               throw SyntaxError(field.line, "expected [x y z] position");
            for (double component : rule.position)
               if (!std::isfinite(component))
                  throw SyntaxError(field.line, "position must be finite");
            rule.kind = "position";
            ++fingerprint.positions;
         }
         else if (rule.kind == "qvar")
         {
            if (args.size() != 2 && args.size() != 4 && args.size() != 6)
               throw SyntaxError(field.line, "invalid QVAR rule");
            rule.text = args[1].text;
            if (args.size() > 2)
            {
               size_t comparison = 2;
               if (args.size() == 6)
               {
                  if (args[2].text != "&")
                     throw SyntaxError(field.line, "expected QVAR mask operator");
                  rule.mask = Integer(args[3].text, field.line);
                  comparison = 4;
               }
               rule.comparison = args[comparison].text;
               if (!IsComparison(rule.comparison))
                  throw SyntaxError(field.line, "invalid comparison operator");
               rule.value = Integer(args[comparison + 1].text, field.line);
            }
         }
         else if (rule.kind == "prop" || rule.kind == "name" || rule.kind == "arch")
         {
            if (args.size() != 3)
               throw SyntaxError(field.line, "expected object and fingerprint value");
            rule.object = Integer(args[1].text, field.line);
            rule.text = args[2].text;
            if (rule.kind == "arch")
               rule.value = Integer(rule.text, field.line);
         }
         else if (rule.kind == "exist")
         {
            if (args.size() != 2)
               throw SyntaxError(field.line, "expected object ID");
            rule.object = Integer(args[1].text, field.line);
         }
         else if (rule.kind == "gam" || rule.kind == "scr" || rule.kind == "cfg" ||
                  rule.kind == "mod")
         {
            if (args.size() != 2)
               throw SyntaxError(field.line,
                                 "unsupported or malformed fingerprint rule: " + rule.kind);
            rule.text = args[1].text;
         }
         else
            throw SyntaxError(field.line, "unknown fingerprint rule: " + rule.kind);
         if (gamesys && rule.kind != "gam" && rule.kind != "prop" && rule.kind != "name")
            throw SyntaxError(field.line, "rule is not allowed in gamesys fingerprints");
         fingerprint.rules.push_back(std::move(rule));
      }
      if (fingerprint.positions && (fingerprint.positions < 8 || fingerprint.positions > 15))
         throw SyntaxError(statement.line, "position fingerprints require 8 to 15 objects");
      result.push_back(std::move(fingerprint));
   }
   return result;
}
template <class Callback, class... Args>
auto Query(const Callback &callback, const Rule &rule, Args &&...args)
{
   if (!callback)
      throw SyntaxError(rule.line, "missing fingerprint adapter: " + rule.kind);
   return callback(std::forward<Args>(args)...);
}
bool Compare(int a, int b, const std::string &op)
{
   if (op == "==")
      return a == b;
   if (op == "!=")
      return a != b;
   if (op == "<")
      return a < b;
   if (op == ">")
      return a > b;
   if (op == "<=")
      return a <= b;
   return a >= b;
}
bool Evaluate(const Rule &rule, const FingerprintContext &context)
{
   bool match = false;
   if (rule.kind == "position")
   {
      const auto actual = Query(context.position, rule, rule.object);
      if (!actual)
         return false;
      double squared = 0;
      for (size_t index = 0; index < 3; ++index)
      {
         const double delta = (*actual)[index] - rule.position[index];
         squared += delta * delta;
      }
      return std::isfinite(squared) && squared <= 9;
   }
   if (rule.kind == "qvar")
   {
      const auto value = Query(context.quest, rule, rule.text);
      if (!value)
         return rule.negative;
      if (rule.comparison.empty())
         return !rule.negative;
      return Compare(*value & rule.mask, rule.value, rule.comparison);
   }
   if (rule.kind == "gam")
   {
      if (context.gamesys.empty())
         throw SyntaxError(rule.line, "missing current gamesys name");
      match = Lower(context.gamesys) == Lower(rule.text);
   }
   else if (rule.kind == "prop")
      match = Query(context.directProperty, rule, rule.object, rule.text);
   else if (rule.kind == "name")
   {
      const auto name = Query(context.name, rule, rule.object);
      match = name && Lower(*name) == Lower(rule.text);
   }
   else if (rule.kind == "arch")
   {
      const auto archetype = Query(context.archetype, rule, rule.object);
      match = archetype && *archetype == rule.value;
   }
   else if (rule.kind == "exist")
      match = Query(context.exists, rule, rule.object);
   else if (rule.kind == "scr")
      match = Query(context.script, rule, rule.text);
   else if (rule.kind == "cfg")
      match = Query(context.config, rule, rule.text);
   else if (rule.kind == "mod")
      match = Query(context.mod, rule, rule.text);
   return rule.negative ? !match : match;
}
} // namespace
void ValidateFingerprints(const std::vector<Statement> &document, bool gamesysPatch)
{
   Compile(document, gamesysPatch);
}

bool MatchFingerprints(const std::vector<Statement> &document, const FingerprintContext &context,
                       bool gamesysPatch)
{
   const auto fingerprints = Compile(document, gamesysPatch);
   bool hasPositive = false;
   bool positiveMatch = false;
   bool excluded = false;
   for (const auto &fingerprint : fingerprints)
   {
      size_t positionMatches = 0;
      bool rulesMatch = true;
      for (const auto &rule : fingerprint.rules)
      {
         const bool match = Evaluate(rule, context);
         if (rule.kind == "position")
            positionMatches += match;
         else
            rulesMatch = rulesMatch && match;
      }
      const size_t required = fingerprint.positions < 10 ? 7 : fingerprint.positions - 3;
      const bool match = rulesMatch && (!fingerprint.positions || positionMatches >= required);
      if (fingerprint.negative)
         excluded = excluded || match;
      else
      {
         hasPositive = true;
         positiveMatch = positiveMatch || match;
      }
   }
   return !excluded && (!hasPositive || positiveMatch);
}
} // namespace DarkDbMod
