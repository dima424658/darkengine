%{
#include <stdlib.h>
#include <schtok.h>
#include <cfgdbg.h>

#ifdef DEBUG
#include <memall.h>
#include <dbmem.h>

static char * local_strdup(const char * s)
{
   char * retVal = malloc(strlen(s) + 1);
   strcpy(retVal, s);
   return retVal;
}
#else
#define local_strdup(s) strdup(s)
#endif
%}

alpha  [a-zA-Z]
alphanum [a-zA-Z0-9\.]
digit    [0-9]
firstchar [a-zA-Z_]
followchar [a-zA-Z0-9_]
quote [\"]
string [^\"]
langle "<"
rangle ">"
astring [^<^>^\n]

%s PREPROC

%%

[\t ]+        /* ignore whitespace */;

^"#include"     {ConfigSpew("lexspew", ("schlex: include\n"));BEGIN PREPROC; return INCLUDE;};
^"#define"      {ConfigSpew("lexspew", ("schlex: define\n"));BEGIN PREPROC; return DEFINE;};

<PREPROC>\n     {ConfigSpew("lexspew", ("schlex: return\n"));BEGIN INITIAL;};

<PREPROC>{firstchar}{followchar}* {ConfigSpew("lexspew", ("schlex: ident\n"));yylval.strval = local_strdup(yytext); return IDENT;}
<PREPROC>-?{digit}+ {ConfigSpew("lexspew", ("schlex: int\n"));yylval.ival = atoi(yytext); return INT;}
<PREPROC>{quote}{string}*{quote} {ConfigSpew("lexspew", ("schlex: string\n"));yylval.strval = local_strdup(yytext); return STRING;}
<PREPROC>{langle}{astring}*{rangle} {ConfigSpew("lexspew", ("schlex: string\n"));yylval.strval = local_strdup(yytext); return STRING;}

schema          {ConfigSpew("lexspew", ("schlex: schema\n")); return SCHEMA;};
type            return TYPE;
gain            return GAIN;
message         return MESSAGE;
voice           return VOICE;
freq            return FREQ;

{alpha}{alphanum}* {ConfigSpew("lexspew", ("schlex: ident\n")); yylval.strval = local_strdup(yytext); return IDENT;}
{quote}{string}*{quote} {ConfigSpew("lexspew", ("schlex: string\n")); yylval.strval = local_strdup(yytext); return STRING;}
-?{digit}+ {ConfigSpew("lexspew", ("schlex: int\n")); yylval.ival = atoi(yytext); return INT;}

.               /* ignore */;

%%

int yywrap(void) { return 1;}



