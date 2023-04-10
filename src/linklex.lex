/* Test lexer for hashtable.  */
%{
#include <lg.h>
#include <stdlib.h>
#include <string.h>
#include <linktok.h>
char identbuffer[2048];
char* nextident = identbuffer;
%}

alpha  [a-zA-Z]
alphanum [a-zA-Z0-9]
delete (D|d)(E|e)(L|l)(E|e)(T|t)(E|e)
insert (I|i)(N|n)(S|s)(E|e)(R|r)(T|t)
flavor (F|f)(L|l)(A|a)(V|v)(O|o)(R|r)

%%

[ \n]                  {}

[0-9]*                 { yylval = atoi(yytext); return(INTLIT);} 

";"                     {  nextident = identbuffer;  return(SEMI); }        

"*"			{ return(STAR); }

"~" 			{ return(NOT); }

{delete}               { return(DELETE);}


/* 
":="                   {return (ASSIGN);}

"("                    { return (OPEN); }
")"                    { return (CLOSE); } 

","                    { return (COMMA); }

"?"                   { return(VAL);} 
"{"                    {return(IN);}
"}"                    {return(OUT);}




{insert}               {return (INSERT);} 

{list}                 {return(LIST);} 

*/ 
{flavor}	       {return(FLAVOR);}
"query" 	       {return(QUERY); } 

{alpha}{alphanum}*     { 
                         strcpy(nextident,yytext);
                         yylval = (int)nextident;
                         nextident += strlen(nextident)+1;
                         return(IDENT); 
                       } 




%% 

int yywrap(void) { return 1;} 

