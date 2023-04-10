/*
  Yacc file for parsing motion schemas.
*/

%union
{
   char *strval;
   int  ival;
   float fval;
   BOOL  bval;
}

/* standard tokens */
%token <strval> IDENT
%token <strval> STRING
%token <ival> INT
%token <fval> FLOAT
%token <bval> BOOLEAN
%token INCLUDE
%token DEFINE
%token LPAREN
%token RPAREN
%token EQUAL
%token COLON

/* motion schema specific tokens */
%token SCHEMA
%token TAG
%token TAG_OPT
%token ACTOR
%token ACTORS_DECLARE
%token ARCHETYPE
%token MOTION
%token TAGLIST_HEADER
%token MOTLIST_HEADER
%token STRETCH
%token TIMEWARP
%token DURATION
%token DISTANCE
%token MOT_NECK_FIXED
%token MOT_NECK_NOT_FIXED
%token MOT_BLEND_NONE
%token MOT_BLEND_DEFAULT
%token MOT_IS_TURN
%token MOT_BLEND_LENGTH
%token MOT_IN_PLACE
%token MOT_IS_LOCO



%{
#include <stdio.h>
#include <string.h>
#include <mprintf.h>
#include <inctab.h>
#include <cfgdbg.h>
#include <config.h>
#include <label.h>
#include <motdb_.h>
#include <motdbase.h>


#define FILE_NAME_LEN 100

/*
  Include file stuff
*/
extern void IncParseFile(char *fileName);
extern void SchemaParseFile(char *fileName, int (*parseFn)());

#define mscherror mprintf

#define kDefaultBlendLength 500 // msec

#define FreeString(ident) { free(ident); ident = NULL; }

/* globals */
#define kMaxTagsPerSchema       16
#define kMaxMotionsPerSchema    128

static sTagDesc g_aSchemaTags[kMaxTagsPerSchema];
static sMotDesc g_aSchemaMotions[kMaxMotionsPerSchema];

static int g_nSchemaTags=0;
static int g_nSchemaMotions=0;

static float g_TimeWarp=0;
static float g_Stretch=0;
static float g_Duration=0;
static float g_Distance=0;

static BOOL g_TagIsMandatory=TRUE;
static float g_TagWeight=0;

static int g_TagVal=0;

static BOOL g_ArchIsString=FALSE;
static int g_ArchInt=0;
static Label g_ArchName;

static BOOL g_NeckIsFixed=FALSE;
static int  g_BlendLength=kDefaultBlendLength;
static BOOL g_IsTurn=FALSE;
static BOOL g_IsLoco=FALSE;
static BOOL g_InPlace=FALSE;


%}

%%

file: statements;

statements: statement | statement statements;
statement: include | schema | tag | actors_declare;

include: INCLUDE STRING
{
   ConfigSpew("yakspew",("include %s\n",$2));       

   IncParseFile($2);
   FreeString($2);
};

opt_tag_spec: tag_spec | null;

tag_spec: TAG_OPT FLOAT
{
   g_TagIsMandatory=FALSE;
   g_TagWeight=$2;
};

tag: TAG IDENT opt_tag_spec
{
   sTagInfo info;

   if(config_is_defined("yakspew"))
   {
      mprintf("tag %s mand %d",$2,g_TagIsMandatory);
      if(g_TagIsMandatory)
      {
         mprintf("\n");
      } else
      {
         mprintf(" weight %g\n",g_TagWeight);
      }
   }
   info.isMandatory=g_TagIsMandatory;
   info.weight = g_TagWeight;
   MotDBRegisterTag((Label *)($2),&info); 
   // reset globals
   g_TagIsMandatory=TRUE;
   g_TagWeight=0;

   FreeString($2);
};

actors_declare: ACTORS_DECLARE INT
{
   MotDBCreate($2);
};

archetype: arch_string | arch_int;

arch_string: ARCHETYPE IDENT
{
   g_ArchIsString=TRUE;
   strncpy(g_ArchName.text,$2,sizeof(g_ArchName));
   FreeString($2);
};

arch_int: ARCHETYPE INT
{
   g_ArchIsString=FALSE;
   g_ArchInt=$2;
};

schema: SCHEMA IDENT ACTOR INT archetype optparams taglist motlist
{
   int i;
   sMotDesc *pMotDesc;
   sTagDesc *pTagDesc;
   sMotSchemaDesc schema;

   if(config_is_defined("yakspew"))     
   {
      if(g_ArchIsString)
         mprintf("schema %s\n  with actor %d\n  with archetype %s\n",$2,$4,g_ArchName.text);       
      else
         mprintf("schema %s\n  with actor %d\n  with archetype %d\n",$2,$4,g_ArchInt);       

      // look at params
      if(g_Duration)
      {
         mprintf("  duration %g\n",g_Duration);
      }
      if(g_Distance)
      {
         mprintf("  distance %g\n",g_Distance);
      }
      if(g_TimeWarp)
      {
         mprintf("  timewarp %g\n",g_TimeWarp);
      }
      if(g_Stretch)
      {
         mprintf("  stretch %g\n",g_Stretch);
      }

      // look at tag globals
      pTagDesc=g_aSchemaTags;
      for(i=0;i<g_nSchemaTags;i++,pTagDesc++)
      {
         mprintf("  with tag %s value %d\n",pTagDesc->type.text,pTagDesc->value);
      }
        
      // look at motion globals     
      pMotDesc=g_aSchemaMotions;
      for(i=0;i<g_nSchemaMotions;i++,pMotDesc++)
      {
         mprintf("  with motion %s,",pMotDesc->name.text);
         if(pMotDesc->stuff.flags&kMSF_NECK_IS_FIXED)
         {
            mprintf(" neck_fixed");
         }
         if(pMotDesc->stuff.blendLength!=kDefaultBlendLength)
         {
            mprintf(" blend %d",pMotDesc->stuff.blendLength);
         }
         if(pMotDesc->stuff.flags&kMSF_IS_TURN)
         {
            mprintf(" is_turn");
         }
         if(pMotDesc->stuff.flags&kMSF_IS_LOCO)
         {
            mprintf(" is_loco");
         }
         if(pMotDesc->stuff.flags&kMSF_WANT_NO_XLAT)
         {
            mprintf(" in_place");
         }
         mprintf("\n");
      }
   }

   schema.nTags=g_nSchemaTags;
   schema.pTags=g_aSchemaTags;
   schema.nMots=g_nSchemaMotions;
   schema.pMots=g_aSchemaMotions;
   schema.duration=g_Duration;
   schema.distance=g_Distance;
   schema.timeWarp=g_TimeWarp;
   schema.stretch=g_Stretch;
   schema.actor=$4;
   strncpy(schema.name.text,$2,15);

   if(g_ArchIsString)
   {
      strncpy(schema.archName.text,g_ArchName.text,sizeof(schema.archName));
      schema.archIsString=TRUE;
   } else
   {
      schema.archInt=g_ArchInt;  
      schema.archIsString=FALSE;
   }
   MotDBAddSchema(&schema);

   // reset globals
   g_nSchemaTags=0;
   g_nSchemaMotions=0;
   g_TimeWarp=0;
   g_Stretch=0;
   g_Duration=0;
   g_Distance=0;

   FreeString($2);
};

optparams: paraminsts | null;

paraminsts: paraminsts paraminst | paraminst;

paraminst: timewarp | duration | stretch | distance;
 
timewarp: TIMEWARP FLOAT
{
   g_TimeWarp=$2;
};

duration: DURATION FLOAT
{
   g_Duration=$2;
};

stretch: STRETCH FLOAT
{
   g_Stretch=$2;
};

distance: DISTANCE FLOAT
{
   g_Distance=$2;
};


taglist: TAGLIST_HEADER opttaginsts;

opttaginsts: taginsts | null;

taginsts: taginsts taginst | taginst;

opt_tag_val: tag_val | null;

tag_val: INT
{
   g_TagVal=$1;
};

taginst: IDENT opt_tag_val
{
   int size;

   if(g_nSchemaTags<kMaxTagsPerSchema)
   {
      size=sizeof(g_aSchemaTags[0].type);     
      strncpy(g_aSchemaTags[g_nSchemaTags].type.text,$1,size);
      g_aSchemaTags[g_nSchemaTags].value=g_TagVal;
      g_nSchemaTags++;  
   } else
   {
      Warning(("Cannot add tag %s to schema - too many tags\n",$1));
   }
   // reset globals 
   g_TagVal=0;
   FreeString($1);
};

motlist: MOTLIST_HEADER optmotions;

optmotions: motinsts | null;

motinsts: motinsts motinst | motinst;

optmotparamlist: motparamlist | null;

motparamlist: motparam motparamlist | motparam;

motparam: neck_fixed | blend_length | is_turn | MOT_NECK_NOT_FIXED | 
        MOT_BLEND_DEFAULT | blend_none | is_loco | in_place;

neck_fixed: MOT_NECK_FIXED 
{
   g_NeckIsFixed=TRUE;
};

blend_none: MOT_BLEND_NONE
{
   g_BlendLength=0;             
};

blend_length: MOT_BLEND_LENGTH INT
{
   g_BlendLength=$2;             
};

in_place: MOT_IN_PLACE
{
   g_InPlace=TRUE;        
};

is_loco: MOT_IS_LOCO
{
   g_IsLoco=TRUE;        
};

is_turn: MOT_IS_TURN
{
   g_IsTurn=TRUE;
};

motinst: IDENT optmotparamlist
{
   int size;
     
   if(g_nSchemaMotions<kMaxMotionsPerSchema)
   {
      size=sizeof(g_aSchemaMotions[0].name);     
      strncpy(g_aSchemaMotions[g_nSchemaMotions].name.text,$1,size);
      g_aSchemaMotions[g_nSchemaMotions].stuff.flags=NULL;
      if(g_NeckIsFixed)
         g_aSchemaMotions[g_nSchemaMotions].stuff.flags|=kMSF_NECK_IS_FIXED;
      g_aSchemaMotions[g_nSchemaMotions].stuff.blendLength=g_BlendLength;
      if(g_IsTurn)
         g_aSchemaMotions[g_nSchemaMotions].stuff.flags|=kMSF_IS_TURN;
      if(g_IsLoco)
         g_aSchemaMotions[g_nSchemaMotions].stuff.flags|=kMSF_IS_LOCO;
      if(g_InPlace)
         g_aSchemaMotions[g_nSchemaMotions].stuff.flags|=kMSF_WANT_NO_XLAT;
      g_nSchemaMotions++;  
   } else
   {
      Warning(("Cannot add motion %s to schema - too many motions\n",$1));
   }

   // reset motion params
   g_NeckIsFixed=FALSE;
   g_BlendLength=kDefaultBlendLength;
   g_IsTurn=FALSE;
   g_IsLoco=FALSE;
   g_InPlace=FALSE;

   FreeString($1);
};

null: ;

motlist:
{
};


%%

void MotSchemaYaccParse(char *schemaFile)
{
   // init globals
        
   IncTabsInit();

   mprintf("parsing file %s!\n",schemaFile);
   MschParseFile(schemaFile);

   IncTabsShutdown();

}


