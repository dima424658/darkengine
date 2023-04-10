/*
  Yacc file for parsing audio schemas.
*/

%union 
{
   char *strval;
   int  ival;
}

%token SCHEMA 
%token TYPE 
%token GAIN 
%token MESSAGE 
%token VOICE 
%token NO_REP 
%token <ival> FREQ 
%token <strval> IDENT 
%token <strval> STRING 
%token <ival> INT
%token INCLUDE
%token DEFINE

%{
#include <stdio.h>
#include <schbase.h>
#include <string.h>
#include <mprintf.h>
#include <config.h>
#include <cfgdbg.h>
#include <inctab.h>

#define FILE_NAME_LEN 100

extern FILE *yyin;
FILE *out_file = NULL;
FILE *list_file = NULL;

struct sAudioSampleRaw sample;
struct sAudioSchemaRaw schema;

/*
  Include file stuff 
*/
extern FILE *incin;

extern BOOL IncludeFilePop();
extern BOOL IncludeFileOpen(char *string, BOOL push);
extern incparse();

#define STRING_TABLE_SIZE 1000

char *string_table = NULL;
char *str_ptr = NULL;  // current loc in string table
int string_table_size = 0;

// Grow the string table
StringTableGrow()
{
   char *new_string_table;

   new_string_table = (char*) realloc(string_table, 
                                      string_table_size+STRING_TABLE_SIZE);
   if (new_string_table == NULL)
   {
      Warning(("StringTableGrow: can't grow string table\n"));
      string_table = str_ptr = NULL;
      string_table_size = 0;
   }
   else
   {
      str_ptr = new_string_table+(str_ptr-string_table);
      string_table = new_string_table;
      string_table_size += STRING_TABLE_SIZE;
   }
}

// Add a string to the string table, growing if necessary
int StringTableAdd(char *string)
{
   int offset;

   if (str_ptr+strlen(string)+1>string_table+string_table_size)
      StringTableGrow();
   if (str_ptr+strlen(string)+1>string_table+string_table_size)
   {
      Warning(("StringTableAdd: can't grow string table big enough\n"));
      return -1;
   }
   strcpy(str_ptr, string);
   offset = str_ptr-string_table;
   str_ptr += strlen(string)+1;
   return offset;
}

// Write the string table to the output file
void StringTableDump(FILE *out_file)
{
   fwrite(string_table, str_ptr-string_table, 1, out_file);
}

#define yyerror printf

%}

%%

file: statements;

statements: statement | statement statements;

statement: include | schema;

include: INCLUDE STRING
{
   if (IncludeFileOpen($2, FALSE))
      while (!feof(incin) || IncludeFilePop())
         incparse();
};

schema: SCHEMA IDENT opt_schema_params samples
{
   fwrite(&schema, sizeof(struct sAudioSchemaRaw), 1, out_file);
   memset(&schema, 0, sizeof(struct sAudioSchemaRaw));
};

opt_schema_params: null | schema_params;

schema_params: schema_param | schema_param schema_params;

schema_param: TYPE INT | GAIN INT | MESSAGE IDENT;

samples: sample | sample samples;

sample: IDENT opt_text opt_sample_params
{
   sample.file_name = StringTableAdd($1);
   if (list_file)
      fprintf(list_file, "File: %s, Text: %s\n", 
              &string_table[sample.file_name], 
              &string_table[sample.text]);
   fwrite(&sample, sizeof(struct sAudioSampleRaw), 1, out_file);
   memset(&sample, 0, sizeof(struct sAudioSampleRaw));
};

opt_text: text | null;

text: STRING
{
   sample.text = StringTableAdd($1);
};

opt_sample_params: null | sample_params;

sample_params: sample_param | sample_param sample_params;

sample_param: voice | no_rep | freq;

voice: VOICE INT
{
   sample.voice_id = $2;
};

no_rep: NO_REP
{
};

freq: FREQ INT
{
   sample.frequency = $2;
};

null: ;

%%

void OpenYyin(char *file_name)
{
   FILE *in_file;

   if (yyin != NULL)
      fclose(yyin);
   if (!(in_file = fopen(file_name, "r")))
      Warning(("schtool: can't open file %s for reading\n", 
               file_name));
   yyin = in_file;
}

void ParseArgs(int argc, char **argv)
{
   if (argc<3)
   {
      printf("Usage: schtool <schema_file> <out_file> [list_file]\n");
      return;
   }
   OpenYyin(argv[1]);
   if (!(out_file = fopen(argv[2], "w")))
      Warning(("schtool: can't open file %s for writing\n",
               argv[2]));
   if (argc>=3)
      if (!(list_file = fopen(argv[3], "w")))
         Warning(("schtool: can't open file %s for writing\n",
                  argv[3]));
}

int main(int argc, char **argv)
{
   config_init();
   config_parse_commandline(argc, argv, NULL);
   mono_init();
   mono_clear();
   if (config_is_defined("monolog"))
      mono_logon("monolog.txt", MONO_LOG_NEW, 0);
   ParseArgs(argc, argv);
   if ((yyin == NULL) || (out_file == NULL))
      return 0;
   mono_clear();
   OpenYyin(argv[1]);
   if (yyin == NULL)
      return 0;
   IncTabsInit();
   while (!feof(yyin))  
      yyparse();
   StringTableDump(out_file);
   fclose(yyin);
   fclose(out_file);
   if (list_file)
      fclose(list_file);
   return 0;
}







