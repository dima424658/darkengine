#pragma once

#include "syntax.hpp"
#include <array>
#include <functional>
#include <optional>
#include <string>

namespace DarkDbMod
{
// Read-only mission snapshot adapters. An omitted adapter is an error when a
// rule needs it, never a successful negative match.
struct FingerprintContext
{
   std::string gamesys;
   std::function<std::optional<std::array<double, 3>>(int)> position;
   std::function<std::optional<int>(const std::string &)> quest;
   std::function<bool(int)> exists;
   std::function<bool(int, const std::string &)> directProperty;
   std::function<std::optional<std::string>(int)> name;
   std::function<std::optional<int>(int)> archetype;
   std::function<bool(const std::string &)> script;
   std::function<bool(const std::string &)> config;
   std::function<bool(const std::string &)> mod;
};
// Validate all fingerprints before evaluating any rule. Missing adapters,
// malformed/unsupported rules and late fingerprints throw SyntaxError.
// No engine state is mutated and no patch commands are executed.
void ValidateFingerprints(const std::vector<Statement> &document, bool gamesysPatch = false);
bool MatchFingerprints(const std::vector<Statement> &document, const FingerprintContext &context,
                       bool gamesysPatch = false);
} // namespace DarkDbMod
