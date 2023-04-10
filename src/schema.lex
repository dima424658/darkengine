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
-?{digit}+              return INT;

%%

main()
{
   do
   {
      tok = yyparse();
      printf("%s", token_name[tok]);  
      if ((tok == IDENT) || (tok == STRING))
         printf(": %s\n", yytext);
      else if (tok == INT)
         printf(": %d\n", atoi(yytext));
      else 
         printf("\n");
   } while (!feof(yyin));
}
