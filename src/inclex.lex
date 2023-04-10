%{
#include <stdlib.h>
#include <inctok.h>
#include <dbg.h>
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

whitespace [\t ]
alpha  [a-zA-Z]
digit  [0-9]
firstchar [a-zA-Z_]
followchar [a-zA-Z0-9_]
quote [\"]
string [^\"]
langle "<"
rangle ">"
astring [^<^>^\n]
hex [xX]

%s PREPROC

%%

{whitespace}    /* ignore */;

^"#include" {
   ConfigSpew("lexspew", ("include\n"));
   BEGIN PREPROC;
   return INCLUDE;
};
^"#define" {
   ConfigSpew("lexspew", ("define\n"));
   BEGIN PREPROC;
   return DEFINE;
};

<PREPROC>\n {
   ConfigSpew("lexspew", ("return\n"));
   BEGIN INITIAL;
};

<PREPROC>{firstchar}{followchar}* {
   ConfigSpew("lexspew", ("ident\n"));
   inclval.strval = local_strdup(inctext);
   return IDENT;
}
<PREPROC>-?{digit}+ {
   ConfigSpew("lexspew", ("int\n"));
   inclval.ival = atoi(inctext);
   return INT;
}
<PREPROC>0{hex}-?{digit}+ {
   ConfigSpew("lexspew", ("hex\n"));
   inclval.ival = strtol(inctext, NULL, 16);
   return INT;
}

<PREPROC>{quote}{string}*{quote} {
   ConfigSpew("lexspew", ("string\n"));
   inclval.strval = local_strdup(inctext);
   return STRING;
}
<PREPROC>{langle}{astring}*{rangle} {
   ConfigSpew("lexspew", ("string\n"));
   inclval.strval = local_strdup(inctext);
   return STRING;
}

.|\n            /* ignore */;
<PREPROC>.      /* ignore */;

%%

#define MAX_FILES_OPEN 10
YY_SAVED *file_stack[MAX_FILES_OPEN];
int files_in_stack = 0;

BOOL IncludeFileOpen(char *string, BOOL push)
{
   FILE *file;
   char file_name[100];

   strncpy(file_name, &string[1], strlen(string)-2);
   file_name[strlen(string)-2] = '\0';
   if (push && (files_in_stack >= MAX_FILES_OPEN))
   {
      Warning(("Max open files exceeded\n"));
      return FALSE;
   }
   if ((file = fopen(file_name, "r")) == NULL)
   {
      Warning(("Can't open file %s\n", file_name));
      return FALSE;
   }
   if (push)
   {
      file_stack[files_in_stack] = incSaveScan(incin);
      files_in_stack++;
   }
   incin = file;
   BEGIN INITIAL;
   return TRUE;
}

BOOL IncludeFilePop()
{
   BEGIN INITIAL;
   if (files_in_stack>0)
   {
      if (incin)
         fclose(incin);
      --files_in_stack;
      incRestoreScan(file_stack[files_in_stack]);
      return TRUE;
   }
   return FALSE;
}

int incwrap(void)
{
   if (IncludeFilePop())
      return 0;
   return 1;
}

int incparse();

void IncParseFile(char *fileName)
{
   files_in_stack = 0;
   YY_INIT;
   if (IncludeFileOpen(fileName, FALSE))
      while (!feof(incin))
         incparse();
   fclose(incin);
}


