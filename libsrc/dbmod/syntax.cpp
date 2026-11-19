#include "syntax.hpp"
#include <cctype>

namespace DarkDbMod
{
namespace
{
enum class Kind { Word, Open, Close, Newline, Pre, Post, End };
struct Token
{
   Kind kind;
   Argument argument;
   size_t line;
};
class Parser
{
   std::string_view source;
   size_t offset = 4;
   size_t line = 1;
   Token token{Kind::End, {}, 1};
   Phase phase = Phase::Post;

   [[noreturn]] void Fail(const std::string &message)
   {
      throw SyntaxError(line, message);
   }
   Token Next()
   {
      while (offset < source.size())
      {
         char c = source[offset];
         if (c == '\n')
         {
            ++offset;
            return {Kind::Newline, {}, line++};
         }
         if (std::isspace(static_cast<unsigned char>(c)))
         {
            ++offset;
            continue;
         }
         if (source.substr(offset, 2) == "//")
         {
            const size_t end = source.find('\n', offset);
            auto comment = source.substr(offset, end == source.npos ? source.size() - offset : end - offset);
            if (!comment.empty() && comment.back() == '\r')
               comment.remove_suffix(1);
            offset = end == source.npos ? source.size() : end;
            if (comment == "//@pre")
               return {Kind::Pre, {}, line};
            if (comment == "//@post")
               return {Kind::Post, {}, line};
            continue;
         }
         if (source.substr(offset, 2) == "/*")
         {
            const size_t end = source.find("*/", offset + 2);
            if (end == source.npos)
               Fail("unterminated block comment");
            bool newline = false;
            while (offset < end + 2)
            {
               if (source[offset++] == '\n')
               {
                  ++line;
                  newline = true;
               }
            }
            if (newline)
               return {Kind::Newline, {}, line};
            continue;
         }
         if (c == '{' || c == '}')
         {
            ++offset;
            return {c == '{' ? Kind::Open : Kind::Close, {}, line};
         }
         if (c == '"')
         {
            const size_t start = ++offset;
            while (offset < source.size() && source[offset] != '"')
            {
               if (source[offset] == '\n' || source[offset] == '\r')
                  Fail("unterminated quoted argument");
               ++offset;
            }
            if (offset == source.size())
               Fail("unterminated quoted argument");
            std::string text(source.substr(start, offset - start));
            ++offset;
            return {Kind::Word, {std::move(text), true}, line};
         }
         const size_t start = offset;
         bool embeddedQuote = false;
         while (offset < source.size())
         {
            c = source[offset];
            if (c == '\n' || c == '\r')
               break;
            if (!embeddedQuote && (std::isspace(static_cast<unsigned char>(c)) || c == '{' ||
                                   c == '}' || source.substr(offset, 2) == "//" ||
                                   source.substr(offset, 2) == "/*"))
               break;
            if (c == '"')
               embeddedQuote = !embeddedQuote;
            ++offset;
         }
         return {Kind::Word, {std::string(source.substr(start, offset - start)), false}, line};
      }
      return {Kind::End, {}, line};
   }
   void Advance()
   {
      token = Next();
   }
   void Newlines()
   {
      while (token.kind == Kind::Newline)
         Advance();
   }
   std::vector<Statement> Block(unsigned depth)
   {
      if (depth > 64)
         Fail("block nesting exceeds 64");
      std::vector<Statement> result;
      Newlines();
      while (token.kind != Kind::End && token.kind != Kind::Close)
      {
         if (token.kind == Kind::Pre || token.kind == Kind::Post)
         {
            if (depth)
               Fail("load phase directive inside a block");
            phase = token.kind == Kind::Pre ? Phase::Pre : Phase::Post;
            Advance();
            Newlines();
            continue;
         }
         Statement statement;
         statement.line = token.line;
         statement.phase = phase;
         while (token.kind == Kind::Word)
         {
            statement.arguments.push_back(std::move(token.argument));
            Advance();
         }
         if (statement.arguments.empty())
            Fail("expected statement before block");
         Newlines();
         if (token.kind == Kind::Open)
         {
            statement.hasBlock = true;
            Advance();
            statement.fields = Block(depth + 1);
            if (token.kind != Kind::Close)
               Fail("unterminated block");
            Advance();
         }
         result.push_back(std::move(statement));
         Newlines();
      }
      return result;
   }
public:
   explicit Parser(std::string_view text) : source(text)
   {
      if (source.size() > 4 * 1024 * 1024)
         Fail("file exceeds 4 MiB");
      if (source.substr(0, 4) != "DML1" ||
          (source.size() > 4 && !std::isspace(static_cast<unsigned char>(source[4]))))
         Fail("expected DML1 at byte zero");
   }
   std::vector<Statement> Run()
   {
      Advance();
      auto result = Block(0);
      if (token.kind != Kind::End)
         Fail("unmatched closing brace");
      return result;
   }
};
}
std::vector<Statement> Parse(std::string_view source)
{
   return Parser(source).Run();
}
}
