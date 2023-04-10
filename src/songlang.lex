/* $Header: r:/t2repos/thief2/src/songlang.lex,v 1.3 1999/03/18 14:32:18 mwhite Exp $ */
%{

#include <songtok.h>
#include <string.h>
#include <mprintf.h>

static int sgSongLineNum;
static char sgSongFilename[64];

%}

ws                [ \t]+
comment           "//".*\n
qstring           \"[^\"\n]*[\"\n]
id                [a-zA-Z_][a-zA-Z0-9_]*
int_num           -?[0-9]+
percent_num       [0-9]+\%
float_num         [0-9]+\.[0-9]+
nl                \n

%%

{ws}              ;
{comment}         { sgSongLineNum++; }
{qstring}         {
                     songyylval.strval = strdup (songyytext+1);
                     if (songyylval.strval[songyyleng-2] != '"')
                        // TBD : Better error handling.  Possibly return an "ERROR" token
                        // with a "strval" value indicating the error string?
                        mprintf ("Unterminated Character String\n");
                     else
                        songyylval.strval[songyyleng-2] = '\0'; /* Remove close quote. */
                     return QSTRING;
                  }
Song              { return SONG; }
DeclareSections   { return DECLARESECTIONS; }
/*
DeclareEvents     { return DECLAREEVENTS; }
*/
Section           { return SECTION; }
Sample            { return SAMPLE; }
Silence           { return SILENCE; }
Seconds           { return SECONDS; }
Samples           { return SAMPLES; }
LoopCount         { return LOOPCOUNT; }
Immediate         { return IMMEDIATE; }
Probability       { return PROBABILITY; }
OnEvent           { return ONEVENT; }
GotoSection       { return GOTOSECTION; }
GotoEnd           { return GOTOEND; }
Volume            { return VOLUME; }
Pan               { return PAN; }
/*
To                { return TO; }
*/
{id}              {
                     songyylval.strval = strdup(songyytext);
                     return ID;
                  }
{int_num}         {
                     songyylval.intval = atoi(songyytext);
                     return INT_NUM;
                  }
{percent_num}     {
                     songyylval.strval = strdup (songyytext);
                     songyylval.strval[songyyleng-1] = '\0'; /* Remove '%' */
                     songyylval.intval = atoi(songyylval.strval);
                     return PERCENT_NUM;
                  }
{float_num}       {
                     songyylval.floatval = atof(songyytext);
                     return FLOAT_NUM;
                  }
{nl}              { sgSongLineNum++; }

%%

static BOOL OpenSongYyin(char *file_name)
{
   FILE *in_file;

   if (songyyin != NULL)
      fclose(songyyin);

   if (!(in_file = fopen(file_name, "r")))
   {
      /* TBD : Error message. */
      /* Warning(("OpenSongYyIn: can't open file %s for reading\n", file_name));*/
      return FALSE;
   }

   songyyin = in_file;

   return TRUE;
}

extern int songyyparse();

// Returns TRUE if successful.
BOOL SongYyParseFile(char *songFile)
{
   int parseFailed;

   songyy_reset();
   sgSongLineNum = 1;

   strcpy (sgSongFilename, songFile);

   parseFailed = 0;
   if (!OpenSongYyin(songFile))
      return;

   while (
          (!feof(songyyin)) &&
          (!parseFailed)
          )
      parseFailed = songyyparse();

   fclose(songyyin);

   return (! parseFailed);
}


int songyywrap(void)
{
   return 1;
}

void songyyerror (char* errmsg)
{
   mprintf ("Error: Parsing song file %s at line %d on word \"%s\"\n--- Reason: %s\n",
            sgSongFilename, sgSongLineNum, songyytext, errmsg);
}
