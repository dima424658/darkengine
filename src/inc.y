/*
  Yacc file for parsing included c header files
*/

%union 
{
   char *strval;
   int  ival;
}

%token INCLUDE
%token DEFINE
%token <ival> INT
%token <strval> STRING
%token <strval> IDENT

%{
#include <stdio.h>
#include <config.h>
#include <cfgdbg.h>
#include <mprintf.h>
#include <string.h>
#include <inctab.h>

extern BOOL IncludeFileOpen(char *string, BOOL push);
extern BOOL IncludeFilePop();

extern FILE *incin;

#define incerror printf

%}

%%

file: opt_statements;

opt_statements: null | statements;

statements: statement | statement statements;

statement: include | define | define_empty | define_complex;

include: INCLUDE STRING
{
   IncludeFileOpen($2, TRUE);
   ConfigSpew("yaccspew", ("Include %s\n", $2));
};

define: DEFINE IDENT INT opt_other
{
   ConfigSpew("yaccspew", ("Define %s %d\n", $2, $3));
   IncTabAddInt($2, $3);
};

define_empty: DEFINE IDENT
{
   ConfigSpew("yaccspew", ("Define %s\n", $2));
};

define_complex: DEFINE IDENT not_int opt_other
{
   ConfigSpew("yaccspew", ("Define Complex\n"));
};

not_int: IDENT | STRING;

opt_other: null | other;

other: not_newline | not_newline other;

not_newline: IDENT | INT | STRING;

null: ;

%%
