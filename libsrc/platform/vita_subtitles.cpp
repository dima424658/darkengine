#include "vita_subtitles.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <cstddef>
#include <limits>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace DarkVitaSubtitles
{
namespace
{
constexpr size_t kMaximumFileSize = 2 * 1024 * 1024;
constexpr size_t kMaximumTokenCount = 100000;
constexpr uint16_t kUndefined = 0xFFFF;

constexpr std::array<uint16_t, 128> kWindows1250 = {
   0x20AC, kUndefined, 0x201A, kUndefined, 0x201E, 0x2026, 0x2020, 0x2021,
   kUndefined, 0x2030, 0x0160, 0x2039, 0x015A, 0x0164, 0x017D, 0x0179,
   kUndefined, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
   kUndefined, 0x2122, 0x0161, 0x203A, 0x015B, 0x0165, 0x017E, 0x017A,
   0x00A0, 0x02C7, 0x02D8, 0x0141, 0x00A4, 0x0104, 0x00A6, 0x00A7,
   0x00A8, 0x00A9, 0x015E, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x017B,
   0x00B0, 0x00B1, 0x02DB, 0x0142, 0x00B4, 0x00B5, 0x00B6, 0x00B7,
   0x00B8, 0x0105, 0x015F, 0x00BB, 0x013D, 0x02DD, 0x013E, 0x017C,
   0x0154, 0x00C1, 0x00C2, 0x0102, 0x00C4, 0x0139, 0x0106, 0x00C7,
   0x010C, 0x00C9, 0x0118, 0x00CB, 0x011A, 0x00CD, 0x00CE, 0x010E,
   0x0110, 0x0143, 0x0147, 0x00D3, 0x00D4, 0x0150, 0x00D6, 0x00D7,
   0x0158, 0x016E, 0x00DA, 0x0170, 0x00DC, 0x00DD, 0x0162, 0x00DF,
   0x0155, 0x00E1, 0x00E2, 0x0103, 0x00E4, 0x013A, 0x0107, 0x00E7,
   0x010D, 0x00E9, 0x0119, 0x00EB, 0x011B, 0x00ED, 0x00EE, 0x010F,
   0x0111, 0x0144, 0x0148, 0x00F3, 0x00F4, 0x0151, 0x00F6, 0x00F7,
   0x0159, 0x016F, 0x00FA, 0x0171, 0x00FC, 0x00FD, 0x0163, 0x02D9
};

void AppendUtf8(std::string &output, uint32_t codepoint)
{
   if (codepoint <= 0x7F)
      output.push_back(static_cast<char>(codepoint));
   else if (codepoint <= 0x7FF)
   {
      output.push_back(static_cast<char>(0xC0 | (codepoint >> 6)));
      output.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
   }
   else
   {
      output.push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
      output.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
      output.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
   }
}

bool IsValidUtf8(std::string_view text)
{
   size_t i = 0;
   while (i < text.size())
   {
      const unsigned char first = static_cast<unsigned char>(text[i]);
      if (first < 0x80)
      {
         ++i;
         continue;
      }

      size_t count = 0;
      uint32_t codepoint = 0;
      if (first >= 0xC2 && first <= 0xDF)
      {
         count = 2;
         codepoint = first & 0x1F;
      }
      else if (first >= 0xE0 && first <= 0xEF)
      {
         count = 3;
         codepoint = first & 0x0F;
      }
      else if (first >= 0xF0 && first <= 0xF4)
      {
         count = 4;
         codepoint = first & 0x07;
      }
      else
         return false;
      if (i + count > text.size())
         return false;
      for (size_t j = 1; j < count; ++j)
      {
         const unsigned char continuation =
            static_cast<unsigned char>(text[i + j]);
         if ((continuation & 0xC0) != 0x80)
            return false;
         codepoint = (codepoint << 6) | (continuation & 0x3F);
      }
      if ((count == 3 && codepoint < 0x800) ||
          (count == 4 && codepoint < 0x10000) ||
          (codepoint >= 0xD800 && codepoint <= 0xDFFF) ||
          codepoint > 0x10FFFF)
         return false;
      i += count;
   }
   return true;
}

std::string LowerAscii(std::string value)
{
   for (char &c : value)
      if (c >= 'A' && c <= 'Z')
         c = static_cast<char>(c + ('a' - 'A'));
   return value;
}

enum class TokenKind
{
   Word,
   String,
   OpenBrace,
   CloseBrace,
   End,
   Invalid
};

struct Token
{
   TokenKind kind = TokenKind::End;
   std::string text;
   size_t line = 1;
};

class Lexer
{
public:
   Lexer(std::string_view source, std::vector<std::string> &errors)
      : m_source(source), m_errors(errors)
   {
   }

   Token Next()
   {
      SkipTrivia();
      const size_t tokenLine = m_line;
      if (m_offset == m_source.size())
         return {TokenKind::End, {}, tokenLine};

      const char c = m_source[m_offset];
      if (c == '{' || c == '}')
      {
         ++m_offset;
         return {c == '{' ? TokenKind::OpenBrace : TokenKind::CloseBrace,
                 std::string(1, c), tokenLine};
      }
      if (c == '"')
         return ReadString(tokenLine);

      const size_t start = m_offset;
      while (m_offset < m_source.size())
      {
         const char current = m_source[m_offset];
         if (current == '{' || current == '}' || current == '"' ||
             current == '\r' || current == '\n' || current == ' ' ||
             current == '\t' || current == '\f' ||
             StartsWith("//") || StartsWith("/*"))
            break;
         ++m_offset;
      }
      if (m_offset == start)
      {
         AddError(tokenLine, "unexpected character");
         ++m_offset;
         return {TokenKind::Invalid, {}, tokenLine};
      }
      return {TokenKind::Word,
              std::string(m_source.substr(start, m_offset - start)),
              tokenLine};
   }

private:
   bool StartsWith(std::string_view prefix) const
   {
      return m_source.substr(m_offset, prefix.size()) == prefix;
   }

   void AddError(size_t line, const std::string &message)
   {
      m_errors.push_back("line " + std::to_string(line) + ": " + message);
   }

   void SkipTrivia()
   {
      while (m_offset < m_source.size())
      {
         const char c = m_source[m_offset];
         if (c == '\r' || c == '\n' || c == ' ' || c == '\t' || c == '\f')
         {
            if (c == '\n')
               ++m_line;
            ++m_offset;
         }
         else if (StartsWith("//"))
         {
            while (m_offset < m_source.size() &&
                   m_source[m_offset] != '\n')
               ++m_offset;
         }
         else if (StartsWith("/*"))
         {
            const size_t startLine = m_line;
            m_offset += 2;
            bool closed = false;
            while (m_offset < m_source.size())
            {
               if (StartsWith("*/"))
               {
                  m_offset += 2;
                  closed = true;
                  break;
               }
               if (m_source[m_offset] == '\n')
                  ++m_line;
               ++m_offset;
            }
            if (!closed)
            {
               AddError(startLine, "unterminated block comment");
               return;
            }
         }
         else
            break;
      }
   }

   Token ReadString(size_t tokenLine)
   {
      ++m_offset;
      std::string value;
      while (m_offset < m_source.size())
      {
         const char c = m_source[m_offset++];
         if (c == '"')
            return {TokenKind::String, std::move(value), tokenLine};
         if (c == '\n')
            ++m_line;
         if (c == '\\' && m_offset < m_source.size())
         {
            const char escaped = m_source[m_offset++];
            switch (escaped)
            {
            case 'n': value.push_back('\n'); break;
            case 'r': value.push_back('\r'); break;
            case 't': value.push_back('\t'); break;
            case '\\': value.push_back('\\'); break;
            case '"': value.push_back('"'); break;
            default: value.push_back(escaped); break;
            }
         }
         else
            value.push_back(c);
      }
      AddError(tokenLine, "unterminated quoted string");
      return {TokenKind::Invalid, {}, tokenLine};
   }

   std::string_view m_source;
   std::vector<std::string> &m_errors;
   size_t m_offset = 0;
   size_t m_line = 1;
};

struct Metadata
{
   std::string type;
   std::string description;
   SoundType soundType = SoundType::Any;
   bool singleton = false;
   bool noVolume = false;
};

class Parser
{
public:
   Parser(std::string_view source, std::vector<std::string> &errors,
          std::vector<std::string> &warnings)
      : m_lexer(source, errors), m_errors(errors), m_warnings(warnings)
   {
      for (size_t i = 0; i < kMaximumTokenCount; ++i)
      {
         Token token = m_lexer.Next();
         m_tokens.push_back(std::move(token));
         if (m_tokens.back().kind == TokenKind::End)
            return;
      }
      Error(Peek(), "token limit exceeded");
      m_tokens.push_back({TokenKind::End, {}, Peek().line});
   }

   std::vector<Entry> ParseFile()
   {
      while (Peek().kind != TokenKind::End)
      {
         if (Peek().kind == TokenKind::OpenBrace)
            ParseGroup();
         else
         {
            Error(Peek(), "expected a top-level block");
            ++m_index;
         }
      }
      return std::move(m_entries);
   }

private:
   const Token &Peek() const
   {
      return m_tokens[std::min(m_index, m_tokens.size() - 1)];
   }

   Token Take()
   {
      Token token = Peek();
      if (m_index + 1 < m_tokens.size())
         ++m_index;
      return token;
   }

   void Error(const Token &token, const std::string &message)
   {
      m_errors.push_back("line " + std::to_string(token.line) + ": " +
                         message);
   }

   bool ReadString(std::string &value, const char *description)
   {
      if (Peek().kind != TokenKind::String)
      {
         Error(Peek(), std::string("expected quoted ") + description);
         return false;
      }
      value = Take().text;
      return true;
   }

   bool ReadUnsigned(uint32_t &value, const char *description)
   {
      if (Peek().kind != TokenKind::Word)
      {
         Error(Peek(), std::string("expected ") + description);
         return false;
      }
      const Token token = Take();
      uint32_t parsed = 0;
      const auto result = std::from_chars(token.text.data(),
                                          token.text.data() + token.text.size(),
                                          parsed);
      if (result.ec != std::errc{} ||
          result.ptr != token.text.data() + token.text.size())
      {
         Error(token, std::string("invalid ") + description);
         return false;
      }
      value = parsed;
      return true;
   }

   bool ParseCue(Cue &cue, bool multi)
   {
      if (Peek().kind != TokenKind::OpenBrace)
      {
         Error(Peek(), "expected subtitle cue block");
         return false;
      }
      Take();
      bool valid = true;
      while (Peek().kind != TokenKind::CloseBrace &&
             Peek().kind != TokenKind::End)
      {
         if (Peek().kind != TokenKind::Word)
         {
            Error(Peek(), "expected cue property");
            Take();
            valid = false;
            continue;
         }
         const Token property = Take();
         const std::string key = LowerAscii(property.text);
         if (key == "text")
         {
            std::string text;
            if (ReadString(text, "subtitle text"))
            {
               cue.text = std::move(text);
               cue.hasText = true;
            }
            else
               valid = false;
         }
         else if (key == "time")
         {
            if (ReadUnsigned(cue.timeMs, "subtitle time"))
               cue.hasTime = true;
            else
               valid = false;
         }
         else if (key == "length")
         {
            if (ReadUnsigned(cue.lengthMs, "subtitle length") &&
                cue.lengthMs > 0)
               cue.hasLength = true;
            else
            {
               if (cue.lengthMs == 0)
                  Error(property, "subtitle length must be positive");
               valid = false;
            }
         }
         else
            m_warnings.push_back("line " + std::to_string(property.line) +
                                 ": ignored cue property " + property.text);
      }
      if (Peek().kind != TokenKind::CloseBrace)
      {
         Error(Peek(), "unterminated subtitle cue block");
         return false;
      }
      Take();
      if (multi && (!cue.hasTime || !cue.hasLength))
      {
         Error(Peek(), "MultiSub cues require Time and positive Length");
         valid = false;
      }
      return valid;
   }

   Entry MakeEntry(const std::string &id, const Metadata &metadata,
                   bool multi) const
   {
      Entry entry;
      entry.id = id;
      entry.type = metadata.type;
      entry.description = metadata.description;
      entry.soundType = metadata.soundType;
      entry.singleton = metadata.singleton;
      entry.noVolume = metadata.noVolume;
      entry.multi = multi;
      return entry;
   }

   void AddEntry(Entry entry, const Token &origin)
   {
      const std::string key = LowerAscii(entry.id);
      if (!m_ids.insert(key).second)
      {
         m_warnings.push_back("line " + std::to_string(origin.line) +
                              ": duplicate subtitle ID ignored: " + entry.id);
         return;
      }
      m_entries.push_back(std::move(entry));
   }

   void ParseSub(const Metadata &metadata, bool multi, const Token &origin)
   {
      std::string id;
      if (!ReadString(id, "subtitle ID"))
         return;
      if (id.empty())
      {
         Error(origin, "subtitle ID must not be empty");
         return;
      }
      Entry entry = MakeEntry(id, metadata, multi);
      if (multi)
      {
         if (Peek().kind != TokenKind::OpenBrace)
         {
            Error(Peek(), "MultiSub requires a cue list");
            return;
         }
         Take();
         bool valid = true;
         while (Peek().kind != TokenKind::CloseBrace &&
                Peek().kind != TokenKind::End)
         {
            const size_t previousIndex = m_index;
            Cue cue;
            if (!ParseCue(cue, true))
            {
               valid = false;
            }
            else
               entry.cues.push_back(std::move(cue));
            if (m_index == previousIndex)
               Take();
         }
         if (Peek().kind != TokenKind::CloseBrace)
         {
            Error(Peek(), "unterminated MultiSub cue list");
            return;
         }
         Take();
         if (entry.cues.empty())
         {
            Error(origin, "MultiSub must contain at least one cue");
            valid = false;
         }
         if (valid)
            AddEntry(std::move(entry), origin);
         return;
      }

      if (Peek().kind == TokenKind::OpenBrace)
      {
         Cue cue;
         if (!ParseCue(cue, false))
            return;
         if (cue.hasText || cue.hasTime || cue.hasLength)
            entry.cues.push_back(std::move(cue));
      }
      AddEntry(std::move(entry), origin);
   }

   void ParseSrtSub(const Metadata &metadata, const Token &origin)
   {
      std::string id;
      std::string filename;
      if (!ReadString(id, "subtitle ID") ||
          !ReadString(filename, "SRT filename"))
         return;
      if (id.empty())
      {
         Error(origin, "subtitle ID must not be empty");
         return;
      }
      if (filename.empty() ||
          filename.find_first_of("/\\:") != std::string::npos ||
          filename == "." || filename == "..")
      {
         Error(origin, "SrtSub filename must be a plain filename");
         return;
      }
      Entry entry = MakeEntry(id, metadata, true);
      entry.srtFile = std::move(filename);
      AddEntry(std::move(entry), origin);
   }

   void ParseGroup()
   {
      Take();
      Metadata metadata;
      while (Peek().kind != TokenKind::CloseBrace &&
             Peek().kind != TokenKind::End)
      {
         if (Peek().kind != TokenKind::Word)
         {
            Error(Peek(), "expected subtitle declaration");
            Take();
            continue;
         }
         const Token declaration = Take();
         const std::string key = LowerAscii(declaration.text);
         if (key == "type" || key == "descr")
         {
            std::string value;
            if (!ReadString(value, key == "type" ? "subtitle type"
                                                  : "description"))
               continue;
            if (key == "type")
               metadata.type = std::move(value);
            else
               metadata.description = std::move(value);
         }
         else if (key == "singleton")
            metadata.singleton = true;
         else if (key == "novolume")
            metadata.noVolume = true;
         else if (key == "onlysoundtype")
         {
            if (Peek().kind != TokenKind::Word)
            {
               Error(Peek(), "expected 2D or 3D sound type");
               continue;
            }
            const Token value = Take();
            const std::string soundType = LowerAscii(value.text);
            if (soundType == "2d")
               metadata.soundType = SoundType::TwoD;
            else if (soundType == "3d")
               metadata.soundType = SoundType::ThreeD;
            else
               Error(value, "OnlySoundType must be 2D or 3D");
         }
         else if (key == "sub")
            ParseSub(metadata, false, declaration);
         else if (key == "multisub")
            ParseSub(metadata, true, declaration);
         else if (key == "srtsub")
            ParseSrtSub(metadata, declaration);
         else
            m_warnings.push_back("line " + std::to_string(declaration.line) +
                                 ": ignored declaration " + declaration.text);
      }
      if (Peek().kind != TokenKind::CloseBrace)
         Error(Peek(), "unterminated subtitle block");
      else
         Take();
   }

   Lexer m_lexer;
   std::vector<std::string> &m_errors;
   std::vector<std::string> &m_warnings;
   std::vector<Token> m_tokens;
   std::vector<Entry> m_entries;
   std::unordered_set<std::string> m_ids;
   size_t m_index = 0;
};

std::string DecodeWindows1250(std::string_view source,
                              std::vector<std::string> &warnings)
{
   std::string decoded;
   decoded.reserve(source.size());
   bool undefinedByte = false;
   for (unsigned char byte : source)
   {
      if (byte < 0x80)
         decoded.push_back(static_cast<char>(byte));
      else
      {
         const uint16_t codepoint = kWindows1250[byte - 0x80];
         if (codepoint == kUndefined)
         {
            AppendUtf8(decoded, 0xFFFD);
            undefinedByte = true;
         }
         else
            AppendUtf8(decoded, codepoint);
      }
   }
   if (undefinedByte)
      warnings.push_back("undefined Windows-1250 bytes replaced with U+FFFD");
   return decoded;
}
} // namespace

ParseResult Parse(std::string_view source, TextEncoding encoding)
{
   ParseResult result;
   if (source.size() > kMaximumFileSize)
   {
      result.errors.push_back("subtitle file exceeds 2 MiB limit");
      return result;
   }

   std::string decoded;
   if (encoding == TextEncoding::Utf8)
   {
      if (!IsValidUtf8(source))
      {
         result.errors.push_back("subtitle file is not valid UTF-8");
         return result;
      }
      decoded.assign(source);
   }
   else if (encoding == TextEncoding::Windows1250 ||
            !IsValidUtf8(source))
      decoded = DecodeWindows1250(source, result.warnings);
   else
      decoded.assign(source);

   if (decoded.size() < 4 || decoded.compare(0, 4, "SUB1") != 0 ||
       (decoded.size() > 4 && decoded[4] != '\r' && decoded[4] != '\n'))
   {
      result.errors.push_back(
         "subtitle file must begin with the exact SUB1 header");
      return result;
   }

   std::vector<std::string> lexErrors;
   Parser parser(std::string_view(decoded).substr(4), lexErrors,
                 result.warnings);
   result.entries = parser.ParseFile();
   result.errors.insert(result.errors.end(), lexErrors.begin(),
                        lexErrors.end());
   return result;
}
}
