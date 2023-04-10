/*
  $Header: r:/t2repos/thief2/src/schema.y,v 1.28 2000/01/20 23:15:12 PATMAC Exp $
  Yacc file for parsing audio schemas.
*/

%union
{
   char *strval;
   int  ival;
   float fval;
}

%token SCHEMA
%token FLAGS
%token VOLUME
%token DELAY
%token PAN
%token PRIORITY
%token FADE
%token ARCHETYPE
%token FREQ
%token <strval> IDENT
%token <strval> STRING
%token <ival> INT
%token <fval> FLOAT
%token INCLUDE
%token DEFINE
%token MONO_LOOP
%token POLY_LOOP
%token CONCEPT
%token VOICE
%token LPAREN
%token RPAREN
%token EQUAL
%token COLON
%token TAG
%token TAG_INT
%token SCHEMA_VOICE
%token MESSAGE
%token AUDIO_CLASS
%token PAN_RANGE
%token NO_REPEAT
%token NO_CACHE
%token STREAM
%token LOOP_COUNT
%token ENV_TAG_REQUIRED
%token ENV_TAG
%token NO_COMBAT
%token PLAY_ONCE
%token NET_AMBIENT
%token LOCAL_SPATIAL

%{
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <mprintf.h>

#include <appagg.h>
#include <iobjsys.h>
#include <property.h>
#include <traitman.h>
#include <propman.h>
#include <linkman.h>
#include <linktype.h>
#include <relation.h>
#include <label.h>

#include <schbase.h>
#include <inctab.h>
#include <schprop.h>
#include <spchprop.h>
#include <schdb.h>
#include <schsamps.h>
#include <spchglue.h>
#include <esndglue.h>


#define FILE_NAME_LEN 100

/*
  Include file stuff
*/
extern void IncParseFile(char *fileName);
extern void SchemaParseFile(char *fileName, int (*parseFn)());

// current schema label
static struct Label schemaLabel;

// current schema ObjID
ObjID g_SchemaID;

// schema archetype
static ObjID baseSchemaID;

// schema parameters
static schemaParamsSet;
static sSchemaPlayParams schemaPlayParams;
static ISchemaPlayParamsProperty *pPropSchemaPlayParams;

// priority
static bool schemaPrioritySet;
static int schemaPriority;

static IObjectSystem *pObjSys;
static ITraitManager *pTraitMan;

// current archetype
static ObjID archID;

// sample information
#define SCHEMA_SAMPLES_MAX     20      // max samples/schema
#define SAMPLE_NAME_LEN 20      // max sample file name length
static char sampleNames[SCHEMA_SAMPLES_MAX][SAMPLE_NAME_LEN];
static uchar sampleFreqs[SCHEMA_SAMPLES_MAX];
static char *pSampleNames[SCHEMA_SAMPLES_MAX];
static uchar sampleFreq;
static int samplesNum = 0;
static BOOL freqSet;

// loop info
static sSchemaLoopParams loopParams;
static BOOL loopSet;

// voice
static ObjID baseVoiceID;

static int voicesNum;
#define VOICES_MAX 20
static ObjID voices[VOICES_MAX];

// messages
static struct Label msgType;
static BOOL msgSet;

// tag creation
#define kMaxTagValues 32
static int g_iNumTagValues;
static Label g_aTagValueLabel[kMaxTagValues];

// adding tags to schemas
static int g_iNumTagsAdded;
static Label g_aTagNameLabel[kMaxTagValues];
static int g_aTagMinInt[kMaxTagValues];
static int g_aTagMaxInt[kMaxTagValues];
static Label g_aTagEnumLabel[kMaxTagValues][8]; // 8 is a hard limit in tagdb
static BOOL g_aTagNumEnums[kMaxTagValues];      // 0 indicates an int tag


#define min(x,y) ((x)<(y)?(x):(y))

#define yyerror printf

// zero all the stuff telling us whether things have been set
void SchemaDataReset(void)
{
   sSchemaPlayParams *pSchemaPlayParams;
   int i, j, k;

   samplesNum = 0;
   schemaParamsSet = FALSE;
   schemaPrioritySet = FALSE;
   loopSet = FALSE;
   loopParams.flags = 0;
   msgSet = FALSE;
   voicesNum = 0;

   // get and copy the default parameters from the base schema
   pSchemaPlayParams = SchemaPlayParamsGet(baseSchemaID);
   schemaPlayParams = *pSchemaPlayParams;
}


void TagReset(void)
{
   int i, j;

   g_iNumTagsAdded = 0;
   for (i = 0; i < kMaxTagValues; ++i) {
      g_aTagNumEnums[i] = 0;
      for (j = 0; j < 8; ++j)
         g_aTagEnumLabel[i][j].text[0] = '\0';
   }
}


void VoiceDataReset(void)
{
   archID = OBJ_NULL;
}


// This copies pString into pLabel, truncating to 16 characters
// including the terminal 0.  If the string is longer than that we
// never tell anyone.
static void MakeLabel(Label *pLabel, char *pString)
{
   strncpy((char*)pLabel, pString, 15);
   pLabel->text[15] = '\0';
}


#define FreeString(ident) { free(ident); ident = NULL; }

%}

%%

file: statements;

statements: statement | statements statement ;
statement: include | schema | voice | concept | schema_voice | env_tag
         | tag | tag_int | env_tag_required;


include: INCLUDE STRING
{
   IncParseFile($2);
   FreeString($2);
};


concept: CONCEPT IDENT INT
{
   Label ConceptLabel;

   MakeLabel(&ConceptLabel, $2);
   SpeechAddConcept(&ConceptLabel, $3);
   FreeString($2);
};


voice: VOICE IDENT opt_voice_params
{
   ObjID voiceID;
   const char* name = $2; 

   voiceID = IObjectSystem_GetObjectNamed(pObjSys, name);
   if (voiceID == OBJ_NULL)
   {
      if (archID == OBJ_NULL)
         voiceID = ITraitManager_CreateArchetype(pTraitMan, name,
                                                 baseVoiceID);
      else
         voiceID = ITraitManager_CreateArchetype(pTraitMan, name,
                                                 archID);
   } else
      mprintf("Voice has existing object name: %s\n", name);

   // @TODO: This creates voice objects even for voices which are
   // just placeholders in the object hierarchy.  Should we create
   // these structures on demand instead?
   SpeechVoiceNew(voiceID);

   VoiceDataReset();
   FreeString($2);
};

opt_voice_params: voice_params | null;

voice_params: voice_param | voice_param voice_params;

voice_param: archetype;

schema: SCHEMA IDENT opt_schema_params opt_samples
{
   ObjID objID;
   int i, j;

   MakeLabel(&schemaLabel, $2);
   if (archID == OBJ_NULL)
      objID = SchemaCreate(&schemaLabel, baseSchemaID);
   else {
      objID = SchemaCreate(&schemaLabel, archID);
      archID = OBJ_NULL;
   }

   if (objID != OBJ_NULL) {
      g_SchemaID = objID;

      if (samplesNum > 0) {
         // make sure we clear existing samples if were replacing the schema
         SchemaSamplesDestroy(objID);
         SchemaSamplesCreate(objID, samplesNum, pSampleNames, sampleFreqs);
      }
      // set the playback parameters if defined
      if (schemaParamsSet)
         SCHEMA_PLAY_PARAMS_SET(objID, &schemaPlayParams);
      if (schemaPrioritySet)
         SCHEMA_PRIORITY_SET(objID, schemaPriority);
      if (loopSet)
         SCHEMA_LOOP_PARAMS_SET(objID, &loopParams);
      if (msgSet)
         SCHEMA_MESSAGE_SET(objID, &msgType);
   }

   SchemaDataReset();
   FreeString($2);
};

opt_schema_params: null | schema_params;
schema_params: schema_param | schema_params schema_param;

schema_param: flags | volume | delay | pan | pan_range | priority | fade |
         audioclass | archetype | monoloop | polyloop | message | no_cache |
         no_repeat | stream | loopcount | no_combat | play_once |
         net_ambient | local_spatial;

flags: FLAGS INT
{
   schemaPlayParams.flags = $2;
   schemaParamsSet = TRUE;
};

no_repeat: NO_REPEAT
{
   schemaPlayParams.flags |= SCH_NO_REPEAT;
   schemaParamsSet = TRUE;
};

no_combat: NO_COMBAT
{
   schemaPlayParams.flags |= SCH_NO_COMBAT;
   schemaParamsSet = TRUE;
};


play_once: PLAY_ONCE
{
   schemaPlayParams.flags |= SCH_PLAY_ONCE;
   schemaParamsSet = TRUE;
};


net_ambient: NET_AMBIENT
{
   schemaPlayParams.flags |= SCH_NET_AMBIENT;
   schemaParamsSet = TRUE;
};


local_spatial: LOCAL_SPATIAL
{
   schemaPlayParams.flags |= SCH_LOC_SPATIAL;
   schemaParamsSet = TRUE;
};


no_cache: NO_CACHE
{
   schemaPlayParams.flags |= SCH_NO_CACHE;
   schemaParamsSet = TRUE;
};

stream: STREAM
{
   schemaPlayParams.flags |= SCH_STREAM;
   schemaParamsSet = TRUE;
};

volume: VOLUME INT
{
   schemaPlayParams.volume = $2;
   schemaParamsSet = TRUE;
};

delay: DELAY INT
{
   schemaPlayParams.initialDelay = $2;
   schemaParamsSet = TRUE;
};

pan: PAN INT
{
   schemaPlayParams.flags &= ~SCH_PAN_RANGE;
   schemaPlayParams.flags |= SCH_PAN_POS;
   schemaPlayParams.pan = $2;
   schemaParamsSet = TRUE;
};

pan_range: PAN_RANGE INT
{
   schemaPlayParams.flags &= ~SCH_PAN_POS;
   schemaPlayParams.flags |= SCH_PAN_RANGE;
   schemaPlayParams.pan = $2;
   schemaParamsSet = TRUE;
};

priority: PRIORITY INT
{
   schemaPriority = $2;
   schemaPrioritySet = TRUE;
};

fade: FADE INT
{
   schemaPlayParams.fade = $2;
   schemaParamsSet = TRUE;
};

archetype: ARCHETYPE IDENT
{
   Label label;
   sSchemaPlayParams *pArchParams, *pDefaultParams;

   MakeLabel(&label, $2);
   archID = IObjectSystem_GetObjectNamed(pObjSys, &label);
   if (archID == OBJ_NULL)
      Warning(("Unknown archetype %s\n", $2));
   if (((pArchParams = SchemaPlayParamsGet(archID)) != NULL) &&
      ((pDefaultParams = SchemaPlayParamsGet(baseSchemaID)) != NULL))
   {
      if (schemaPlayParams.flags == pDefaultParams->flags)
         schemaPlayParams.flags = pArchParams->flags;
      if (schemaPlayParams.volume == pDefaultParams->volume)
         schemaPlayParams.volume = pArchParams->volume;
      if (schemaPlayParams.pan == pDefaultParams->pan)
         schemaPlayParams.pan = pArchParams->pan;
      if (schemaPlayParams.initialDelay == pDefaultParams->initialDelay)
         schemaPlayParams.initialDelay = pArchParams->initialDelay;
      if (schemaPlayParams.fade == pDefaultParams->fade)
         schemaPlayParams.fade = pArchParams->fade;
   }
   FreeString($2);
};

monoloop: MONO_LOOP INT INT
{
   loopSet = TRUE;
   loopParams.flags &= ~SCHEMA_LOOP_POLY;
   loopParams.maxSamples = 1;
   loopParams.intervalMin = $2;
   loopParams.intervalMax = $3;
};

polyloop: POLY_LOOP INT INT INT
{
   loopSet = TRUE;
   loopParams.flags |= SCHEMA_LOOP_POLY;
   loopParams.maxSamples = $2;
   loopParams.intervalMin = $3;
   loopParams.intervalMax = $4;
};

loopcount: LOOP_COUNT INT
{
   loopParams.flags |= SCHEMA_LOOP_COUNT;
   loopParams.count = $2;
};

audioclass: AUDIO_CLASS IDENT
{
   // @NOTE: this must be in the same order as in schbase.h
   // @NOTE: Max is 16.
   static char *classes[]={"noise","speech","ambient","music","metaui",                   // Originial Thief classes.
                           "player_feet","other_feet","collisions","weapons","monsters"}; // Additional Shock classes.
   bool ok=FALSE;
   int i;

   if (isdigit($2[0]))
   {
      int val=$2[0]-'0';
      schemaPlayParams.flags |= ((val+1)<<SCH_CLASS_SHIFT);
      schemaParamsSet = TRUE;
      ok=TRUE;
   }
   else
      for (i=0; i<sizeof(classes)/sizeof(classes[0]); i++)
      {
         if (stricmp($2,classes[i])==0)
         {
            schemaPlayParams.flags |= ((i+1)<<SCH_CLASS_SHIFT);
            schemaParamsSet = TRUE;
            ok=TRUE;
            break;
         }
      }
   if (!ok)
      Warning(("Hey! %s isnt a valid audio class\n",$2));
   FreeString($2);
};

message: MESSAGE IDENT
{
   msgSet = TRUE;
   strncpy(&(msgType.text[0]), $2, 15);
   msgType.text[15] = '\0';
   FreeString($2);
};

tag: TAG IDENT opt_tag_states
{
   int i;
   Label TagNameLabel;

   MakeLabel(&TagNameLabel, $2);
   SpeechAddTag(&TagNameLabel);
   for (i = 0; i < g_iNumTagValues; ++i)
      SpeechAddTagValue(&g_aTagValueLabel[i]);

   g_iNumTagValues = 0;
   FreeString($2);
};

opt_tag_states: null | tag_states;
tag_states: tag_state | tag_state tag_states;


tag_state: IDENT
{
   if (g_iNumTagValues == (kMaxTagValues - 1)) {
      mprintf("Hey!  Trying to have more than %d states for one tag.\n",
              kMaxTagValues);
      return 1;
   }

   MakeLabel(&g_aTagValueLabel[g_iNumTagValues], $1);
   ++g_iNumTagValues;

   FreeString($1);
};


tag_int: TAG_INT IDENT
{
   Label TagNameLabel;

   MakeLabel(&TagNameLabel, $2);
   SpeechAddIntTag(&TagNameLabel);
   FreeString($2);
};


env_tag_required: ENV_TAG_REQUIRED IDENT
{
   int i;
   Label TagNameLabel;

   MakeLabel(&TagNameLabel, $2);
   ESndSetTagRequired(&TagNameLabel);

   FreeString($2);
};


schema_voice: SCHEMA_VOICE IDENT INT IDENT opt_schema_tags
{
   Label VoiceLabel, ConceptLabel;
   int i;
   int iWeight = $3;

   MakeLabel(&VoiceLabel, $2);
   MakeLabel(&ConceptLabel, $4);

   SpeechSchemaNewStart(&schemaLabel, g_SchemaID, &VoiceLabel, iWeight,
                        &ConceptLabel);

   for (i = 0; i < g_iNumTagsAdded; ++i) {
      if (g_aTagNumEnums[i])
         SpeechSchemaNewAddEnumTag(&g_aTagNameLabel[i],
                                   &g_aTagEnumLabel[i]);
      else
         SpeechSchemaNewAddIntTag(&g_aTagNameLabel[i],
                                  g_aTagMinInt[i],
                                  g_aTagMaxInt[i]);
   }
   SpeechSchemaNewFinish();

   TagReset();

   FreeString($2);
   FreeString($4);
};


opt_schema_tags: null | schema_tags;

schema_tags: schema_tag | schema_tag schema_tags;

schema_tag:
   schema_tag_empty
 | schema_tag_int
 | schema_tag_enum;

schema_tag_empty: LPAREN IDENT RPAREN
{
   MakeLabel(&g_aTagNameLabel[g_iNumTagsAdded], $2);
   g_aTagMinInt[g_iNumTagsAdded] = 0;
   g_aTagMaxInt[g_iNumTagsAdded] = 0xffffffff;
   ++g_iNumTagsAdded;

   FreeString($2);
};

schema_tag_int: LPAREN IDENT INT INT RPAREN
{
   MakeLabel(&g_aTagNameLabel[g_iNumTagsAdded], $2);
   g_aTagMinInt[g_iNumTagsAdded] = $3;
   g_aTagMaxInt[g_iNumTagsAdded] = $4;
   ++g_iNumTagsAdded;

   FreeString($2);
};

schema_tag_enum: LPAREN IDENT schema_arg schema_args RPAREN
{
   MakeLabel(&g_aTagNameLabel[g_iNumTagsAdded], $2);
   ++g_iNumTagsAdded;

   FreeString($2);
};


schema_args: null | schema_arg_list;

schema_arg_list: schema_arg | schema_arg schema_arg_list;

schema_arg: IDENT
{
   if (g_aTagNumEnums[g_iNumTagsAdded] == 8) {
      Warning(("More than eight enums for a schema tag: %s.\n", $1));
   } else {
      MakeLabel(&g_aTagEnumLabel[g_iNumTagsAdded]
                                [g_aTagNumEnums[g_iNumTagsAdded]], $1);
      ++g_aTagNumEnums[g_iNumTagsAdded];
   }

   FreeString($1);
};


env_tag: ENV_TAG opt_schema_tags
{
   int i;

   ESndSchemaNewStart(&schemaLabel, g_SchemaID);

   for (i = 0; i < g_iNumTagsAdded; ++i) {
      if (g_aTagNumEnums[i])
         ESndSchemaNewAddEnumTag(&g_aTagNameLabel[i],
                                 &g_aTagEnumLabel[i]);
      else
         ESndSchemaNewAddIntTag(&g_aTagNameLabel[i],
                                g_aTagMinInt[i],
                                g_aTagMaxInt[i]);
   }
   ESndSchemaNewFinish();

   TagReset();
};


opt_samples: null | samples;

samples: sample | sample samples;

sample: IDENT opt_text opt_freq
{
   if (samplesNum<SCHEMA_SAMPLES_MAX)
   {
      int nameLen = min(strlen($1), SAMPLE_NAME_LEN-1);
      strncpy(sampleNames[samplesNum], $1, nameLen);
      sampleNames[samplesNum][nameLen] = '\0';
      if (freqSet)
      {
         sampleFreqs[samplesNum] = sampleFreq;
         freqSet = FALSE;
      }
      else
         sampleFreqs[samplesNum] = SAMPLE_DEFAULT_FREQ;
      samplesNum++;
   }
   else
      Warning(("Sample %s, too many samples for schema\n", $1));

   FreeString($1);
};

opt_text: text | null;

text: STRING
{
};

opt_freq: freq | null;

freq: FREQ INT
{
   sampleFreq = (uchar)$2;
   freqSet = TRUE;
};

null: ;

%%

extern FILE *yyin;

extern void YyParseFile(char *schemaFile);

void SchemaYaccParse(char *schemaFile)
{
   int i;

   Label baseSchemaLabel = {BASE_SCHEMA_OBJ};
   Label baseVoiceLabel = {BASE_VOICE_OBJ};
   IPropertyManager *pPropMan = AppGetObj(IPropertyManager);
   ILinkManager *pLinkMan = AppGetObj(ILinkManager);
   sSchemaPlayParams *pSchemaPlayParams;

   if (!pObjSys)
      pObjSys = AppGetObj(IObjectSystem);
   if (!pTraitMan)
      pTraitMan = AppGetObj(ITraitManager);
   baseSchemaID = IObjectSystem_GetObjectNamed(pObjSys, &baseSchemaLabel);
   baseVoiceID = IObjectSystem_GetObjectNamed(pObjSys, &baseVoiceLabel);

   if (pSchemaPlayParams = SchemaPlayParamsGet(baseSchemaID))
      schemaPlayParams = *pSchemaPlayParams;

   SchemaDataReset();
   VoiceDataReset();
   freqSet = FALSE;

   // setup names table for sample creation
   for (i=0; i<SCHEMA_SAMPLES_MAX; i++)
      pSampleNames[i] = &sampleNames[i][0];

   IncTabsInit();

//   SpeechExpressionDatabaseBuild();

   YyParseFile(schemaFile);

   IncTabsShutdown();

//   SpeechExpressionDatabaseDestroy();

   SafeRelease(pPropMan);
   SafeRelease(pLinkMan);
   SafeRelease(pObjSys);
   SafeRelease(pTraitMan);
}
