%skeleton "lalr1.cc"
%require "3.0"
%define api.namespace {DarkMaterial::Detail}
%define api.parser.class {Grammar}
%define api.value.type variant
%define api.token.constructor
%define parse.error verbose
%parse-param {DarkMaterial::Parser &driver}
%parse-param {DarkMaterial::Detail::Lexer &lexer}
%lex-param {DarkMaterial::Detail::Lexer &lexer}

%code requires {
#include "material_parser_internal.hpp"
}

%code {
static DarkMaterial::Detail::Grammar::symbol_type
yylex(DarkMaterial::Detail::Lexer &lexer)
{
   using Grammar = DarkMaterial::Detail::Grammar;
   using Lexer = DarkMaterial::Detail::Lexer;
   std::string value;
   bool first = false;
   switch (lexer.Next(value, first))
   {
   case Lexer::Token::End:
      return Grammar::make_YYEOF();
   case Lexer::Token::EndLine:
      return Grammar::make_EOL();
   case Lexer::Token::Invalid:
      return Grammar::make_INVALID();
   case Lexer::Token::Word:
      break;
   }
   if (!first)
      return Grammar::make_ARGUMENT(std::move(value));

   const std::string key = DarkMaterial::Lower(value);
   if (key == ".ifdef")
      return Grammar::make_IFDEF();
   if (key == ".ifndef")
      return Grammar::make_IFNDEF();
   if (key == ".else")
      return Grammar::make_ELSE();
   if (key == ".endif")
      return Grammar::make_ENDIF();
   if (key == "render_pass")
      return Grammar::make_RENDER_PASS();
   if (key == "include")
      return Grammar::make_INCLUDE();
   if (key == "terrain_scale")
      return Grammar::make_TERRAIN_SCALE();
   if (key == "tile_factor")
      return Grammar::make_TILE_FACTOR();
   if (key == "ani_rate")
      return Grammar::make_ANI_RATE();
   if (key == "ani_frames")
      return Grammar::make_ANI_FRAMES();
   if (key == "ani_mode")
      return Grammar::make_ANI_MODE();
   if (key == "illum_map")
      return Grammar::make_ILLUM_MAP();
   if (key == "force_opaque")
      return Grammar::make_FORCE_OPAQUE();
   if (key == "force_full_alpha")
      return Grammar::make_FORCE_FULL_ALPHA();
   if (key == "force_alpha_key")
      return Grammar::make_FORCE_ALPHA_KEY();
   if (key == "no_mipmap")
      return Grammar::make_NO_MIPMAP();
   if (key == "uv_clamp")
      return Grammar::make_UV_CLAMP();
   if (key == "force_local_pal")
      return Grammar::make_FORCE_LOCAL_PAL();
   if (key == "force_ani_settings")
      return Grammar::make_FORCE_ANI_SETTINGS();
   return Grammar::make_UNKNOWN(std::move(value));
}
}

%token EOL INVALID
%token <std::string> ARGUMENT UNKNOWN
%token IFDEF
%token IFNDEF
%token ELSE
%token ENDIF
%token RENDER_PASS
%token INCLUDE
%token TERRAIN_SCALE
%token TILE_FACTOR
%token ANI_RATE
%token ANI_FRAMES
%token ANI_MODE
%token ILLUM_MAP
%token FORCE_OPAQUE
%token FORCE_FULL_ALPHA
%token FORCE_ALPHA_KEY
%token NO_MIPMAP
%token UV_CLAMP
%token FORCE_LOCAL_PAL
%token FORCE_ANI_SETTINGS
%type <DarkMaterial::Detail::Command> command
%type <std::vector<std::string>> arguments

%%

document:
   %empty
 | document line
 ;

line:
   EOL
 | command arguments EOL
   {
      $2.insert($2.begin(), std::move($1.name));
      driver.Apply($1.directive, $2);
   }
 | error EOL { yyerrok; }
 ;

command:
   IFDEF { $$ = {Directive::Ifdef, ".ifdef"}; }
 | IFNDEF { $$ = {Directive::Ifndef, ".ifndef"}; }
 | ELSE { $$ = {Directive::Else, ".else"}; }
 | ENDIF { $$ = {Directive::Endif, ".endif"}; }
 | RENDER_PASS { $$ = {Directive::RenderPass, "render_pass"}; }
 | INCLUDE { $$ = {Directive::Include, "include"}; }
 | TERRAIN_SCALE { $$ = {Directive::TerrainScale, "terrain_scale"}; }
 | TILE_FACTOR { $$ = {Directive::TileFactor, "tile_factor"}; }
 | ANI_RATE { $$ = {Directive::AnimationRate, "ani_rate"}; }
 | ANI_FRAMES { $$ = {Directive::AnimationFrames, "ani_frames"}; }
 | ANI_MODE { $$ = {Directive::AnimationMode, "ani_mode"}; }
 | ILLUM_MAP { $$ = {Directive::IlluminationMap, "illum_map"}; }
 | FORCE_OPAQUE { $$ = {Directive::ForceOpaque, "force_opaque"}; }
 | FORCE_FULL_ALPHA { $$ = {Directive::FullAlpha, "force_full_alpha"}; }
 | FORCE_ALPHA_KEY { $$ = {Directive::AlphaKey, "force_alpha_key"}; }
 | NO_MIPMAP { $$ = {Directive::NoMipmap, "no_mipmap"}; }
 | UV_CLAMP { $$ = {Directive::UvClamp, "uv_clamp"}; }
 | FORCE_LOCAL_PAL { $$ = {Directive::LocalPalette, "force_local_pal"}; }
 | FORCE_ANI_SETTINGS { $$ = {Directive::ForceAnimation, "force_ani_settings"}; }
 | UNKNOWN { $$ = {Directive::Unknown, std::move($1)}; }
 ;

arguments:
   %empty { $$ = {}; }
 | arguments ARGUMENT
   {
      $$ = std::move($1);
      $$.push_back(std::move($2));
   }
 ;

%%

void DarkMaterial::Detail::Grammar::error(const std::string &)
{
   // As before, malformed lines do not replace valid/default material settings.
}
