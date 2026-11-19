#include "material_parser_internal.hpp"

#include <utility>

namespace DarkMaterial::Detail
{
Lexer::Lexer(std::string_view text) : remaining(text)
{
   if (remaining.substr(0, 3) == "\xef\xbb\xbf")
      remaining.remove_prefix(3);
}

void Lexer::ReadLine()
{
   const auto end = remaining.find('\n');
   const auto line = remaining.substr(0, end);
   if (end == std::string_view::npos)
      remaining = {};
   else
      remaining.remove_prefix(end + 1);

   tokens.clear();
   nextToken = 0;
   haveLine = true;
   std::string token;
   bool quoted = false;
   for (size_t i = 0; i < line.size(); ++i)
   {
      const char c = line[i];
      if (!quoted && (c == '#' || (c == '/' && i + 1 < line.size() && line[i + 1] == '/')))
         break;
      if (c == '"')
      {
         quoted = !quoted;
         continue;
      }
      if (!quoted && (c == ' ' || c == '\t' || c == '\r' || c == '{' || c == '}'))
      {
         if (!token.empty())
         {
            tokens.push_back(std::move(token));
            token.clear();
         }
         if (c == '{' || c == '}')
            tokens.emplace_back(1, c);
      }
      else
         token += c;
   }
   if (!token.empty())
      tokens.push_back(std::move(token));
   invalidLine = quoted;
}

Lexer::Token Lexer::Next(std::string &value, bool &first)
{
   if (!haveLine)
   {
      if (remaining.empty())
         return Token::End;
      ReadLine();
   }
   if (invalidLine)
   {
      invalidLine = false;
      nextToken = tokens.size();
      return Token::Invalid;
   }
   if (nextToken == tokens.size())
   {
      haveLine = false;
      return Token::EndLine;
   }
   first = nextToken == 0;
   value = std::move(tokens[nextToken++]);
   return Token::Word;
}
} // namespace DarkMaterial::Detail
