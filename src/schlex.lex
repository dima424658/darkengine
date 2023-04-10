%{
#include <stdlib.h>
#include <schtok.h>
#include <dbg.h>
#include <cfgdbg.h>

#ifdef DEBUG
#include <memall.h>
#include <dbmem.h>

#define local_strdup(s) (strcpy((char *)(malloc(strlen(s) + 1)), (s)))
#else
#define local_strdup(s) strdup(s)
#endif

#ifdef CNTLEX
#define yylval cntlval
#define yytext cnttext
#define yyin cntin
#define yyparse cntparse
#define OpenYyin OpenCntIn
#define YyParseFile CntParseFile
#define yywrap cntwrap
#define yy_reset cnt_reset
#endif

%}

alpha  [a-zA-Z]
alphanum [a-zA-Z0-9\.]
digit    [0-9]
firstchar [a-zA-Z_]
followchar [a-zA-Z0-9_]
quote [\"]
string [^\"]
lparen "("
rparen ")"
colon ":"
astring [^<^>^\n]
comment "//"
dot "."

%x PREPROC
%x COMMENT

%%

[\t ]+        /* ignore whitespace */;

/* inline comments */
{comment} {
   ConfigSpew("lexspew", ("schlex: comment\n"));
   BEGIN COMMENT;
}

<COMMENT>\n {
   ConfigSpew("lexspew", ("schlex: end comment\n"));
   BEGIN INITIAL;
}

^"#include"     {
   ConfigSpew("lexspew", ("schlex: include\n"));
   BEGIN PREPROC;
   return INCLUDE;
}
^"#define"      {
   ConfigSpew("lexspew", ("schlex: define\n"));
   BEGIN PREPROC;
   return DEFINE;
}

<PREPROC>\n     {
   ConfigSpew("lexspew", ("schlex: return\n"));
   BEGIN INITIAL;
}

<PREPROC>{firstchar}{followchar}* {
   ConfigSpew("lexspew", ("schlex: ident\n"));
   yylval.strval = local_strdup(yytext);
   return IDENT;
}
<PREPROC>-?{digit}+ {
   ConfigSpew("lexspew", ("schlex: int\n"));
   yylval.ival = atoi(yytext);
   return INT;
}
<PREPROC>{quote}{string}*{quote} {
   ConfigSpew("lexspew", ("schlex: string\n"));
   yylval.strval = local_strdup(yytext);
   return STRING;
}
<PREPROC>{lparen}{astring}*{rparen} {
   ConfigSpew("lexspew", ("schlex: string\n"));
   yylval.strval = local_strdup(yytext);
   return STRING;
}

schema          {ConfigSpew("lexspew", ("schlex: schema\n")); return SCHEMA;};
flags           return FLAGS;
volume          return VOLUME;
delay           return DELAY;
pan             return PAN;
pan_range       return PAN_RANGE;
priority        return PRIORITY;
fade            return FADE;
archetype       return ARCHETYPE;
freq            return FREQ;
mono_loop       return MONO_LOOP;
poly_loop       return POLY_LOOP;
concept         return CONCEPT;
tag             return TAG;
tag_int         return TAG_INT;
voice           return VOICE;
schema_voice    return SCHEMA_VOICE;
message         return MESSAGE;
no_repeat       return NO_REPEAT;
no_cache        return NO_CACHE;
audio_class     return AUDIO_CLASS;
stream          return STREAM;
loop_count      return LOOP_COUNT;
env_tag_required return ENV_TAG_REQUIRED;
env_tag         return ENV_TAG;
play_once       return PLAY_ONCE;
no_combat       return NO_COMBAT; 
net_ambient     return NET_AMBIENT;
local_spatial   return LOCAL_SPATIAL;

{firstchar}{followchar}* {
   int val;

   ConfigSpew("lexspew", ("schlex: ident\n"));
   if (!IncTabLookup(yytext, &val))
   {
      yylval.strval = local_strdup(yytext);
      return IDENT;
   }
   else
   {
      yylval.ival = val;
      return INT;
   }
}

{quote}{string}*{quote} {
   ConfigSpew("lexspew", ("schlex: string\n"));
   yylval.strval = local_strdup(yytext);
   return STRING;
}

-?{digit}+ {
   ConfigSpew("lexspew", ("schlex: int\n"));
   yylval.ival = atoi(yytext);
   return INT;
}

-?({digit}+)|({digit}*{dot}{digit}+)|({digit}+{dot}{digit}*) {
   ConfigSpew("lexspew", ("schlex: int\n"));
   yylval.fval = atof(yytext);
   return FLOAT;
}

= return EQUAL;
{colon} return COLON;
{lparen} return LPAREN;
{rparen} return RPAREN;



.               /* ignore */;

%%

BOOL OpenYyin(char *file_name)
{
   FILE *in_file;

   if (yyin != NULL)
      fclose(yyin);
   if (!(in_file = fopen(file_name, "r")))
   {
      Warning(("OpenYYIn: can't open file %s for reading\n",
               file_name));
      return FALSE;
   }
   yyin = in_file;
   return TRUE;
}

extern int yyparse();

void YyParseFile(char *schemaFile)
{
   yyin = NULL;
   yy_reset();
   if (!OpenYyin(schemaFile))
      return;
   while (!feof(yyin))
      yyparse();
   fclose(yyin);
}

int yywrap(void) { return 1;}



