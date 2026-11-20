%{
#include <stdlib.h>
#include <schtok.h>
%}

alpha  [a-zA-Z]
alphanum [a-zA-Z0-9]
digit    [0-9]
quote [\"]
string [^\"]

%%

[\n\t ]+        /* ignore whitespace */;

schema          return SCHEMA;
type            return TYPE;
gain            return GAIN;
message         return MESSAGE;
voice           return VOICE;
freq            return FREQ;

{alpha}{alphanum}*      return IDENT;                         
{quote}{string}*{quote} return STRING;
-?{digit}+              return INTEGER_LITERAL;

%%

main()
{
   do
   {
      tok = yyparse();
      printf("%s", token_name[tok]);  
      if ((tok == IDENT) || (tok == STRING))
         printf(": %s\n", yytext);
      else if (tok == INTEGER_LITERAL)
         printf(": %d\n", atoi(yytext));
      else 
         printf("\n");
   } while (!feof(yyin));
}
