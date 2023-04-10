/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

/* n:\bin\yacc -l -D schtok.h -o sound\schyacc.c schema.y */
#ifdef YYTRACE
#define YYDEBUG 1
#else
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#endif
/*
 * Portable way of defining ANSI C prototypes
 */
#ifndef YY_ARGS
#if __STDC__
#define YY_ARGS(x)	x
#else
#define YY_ARGS(x)	()
#endif
#endif

#if YYDEBUG
typedef struct yyNamedType_tag {	/* Tokens */
	char	* name;		/* printable name */
	short	token;		/* token # */
	short	type;		/* token type */
} yyNamedType;
typedef struct yyTypedRules_tag {	/* Typed rule table */
	char	* name;		/* compressed rule string */
	short	type;		/* rule result type */
} yyTypedRules;

#endif

typedef union {
   char *strval;
   int  ival;
   float fval;
} YYSTYPE;
#define SCHEMA	257
#define FLAGS	258
#define VOLUME	259
#define DELAY	260
#define PAN	261
#define PRIORITY	262
#define FADE	263
#define ARCHETYPE	264
#define FREQ	265
#define IDENT	266
#define STRING	267
#define INT	268
#define FLOAT	269
#define INCLUDE	270
#define DEFINE	271
#define MONO_LOOP	272
#define POLY_LOOP	273
#define CONCEPT	274
#define VOICE	275
#define LPAREN	276
#define RPAREN	277
#define EQUAL	278
#define COLON	279
#define TAG	280
#define TAG_INT	281
#define SCHEMA_VOICE	282
#define MESSAGE	283
#define AUDIO_CLASS	284
#define PAN_RANGE	285
#define NO_REPEAT	286
#define NO_CACHE	287
#define STREAM	288
#define LOOP_COUNT	289
#define ENV_TAG_REQUIRED	290
#define ENV_TAG	291
#define NO_COMBAT	292
#define PLAY_ONCE	293
#define NET_AMBIENT	294
#define LOCAL_SPATIAL	295

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

extern int yychar, yyerrflag;
extern YYSTYPE yylval;
#if YYDEBUG
enum YY_Types { YY_t_NoneDefined, YY_t_strval, YY_t_ival, YY_t_fval
};
#endif
#if YYDEBUG
yyTypedRules yyRules[] = {
	{ "&00: %01 &00",  0},
	{ "%01: %02",  0},
	{ "%02: %03",  0},
	{ "%02: %02 %03",  0},
	{ "%03: %04",  0},
	{ "%03: %05",  0},
	{ "%03: %06",  0},
	{ "%03: %07",  0},
	{ "%03: %08",  0},
	{ "%03: %09",  0},
	{ "%03: %10",  0},
	{ "%03: %11",  0},
	{ "%03: %12",  0},
	{ "%04: &15 &12",  0},
	{ "%07: &19 &11 &13",  0},
	{ "%06: &20 &11 %13",  0},
	{ "%13: %14",  0},
	{ "%13: %15",  0},
	{ "%14: %16",  0},
	{ "%14: %16 %14",  0},
	{ "%16: %17",  0},
	{ "%05: &02 &11 %18 %19",  0},
	{ "%18: %15",  0},
	{ "%18: %20",  0},
	{ "%20: %21",  0},
	{ "%20: %20 %21",  0},
	{ "%21: %22",  0},
	{ "%21: %23",  0},
	{ "%21: %24",  0},
	{ "%21: %25",  0},
	{ "%21: %26",  0},
	{ "%21: %27",  0},
	{ "%21: %28",  0},
	{ "%21: %29",  0},
	{ "%21: %17",  0},
	{ "%21: %30",  0},
	{ "%21: %31",  0},
	{ "%21: %32",  0},
	{ "%21: %33",  0},
	{ "%21: %34",  0},
	{ "%21: %35",  0},
	{ "%21: %36",  0},
	{ "%21: %37",  0},
	{ "%21: %38",  0},
	{ "%21: %39",  0},
	{ "%21: %40",  0},
	{ "%22: &03 &13",  0},
	{ "%34: &31",  0},
	{ "%37: &37",  0},
	{ "%38: &38",  0},
	{ "%39: &39",  0},
	{ "%40: &40",  0},
	{ "%33: &32",  0},
	{ "%35: &33",  0},
	{ "%23: &04 &13",  0},
	{ "%24: &05 &13",  0},
	{ "%25: &06 &13",  0},
	{ "%26: &30 &13",  0},
	{ "%27: &07 &13",  0},
	{ "%28: &08 &13",  0},
	{ "%17: &09 &11",  0},
	{ "%30: &17 &13 &13",  0},
	{ "%31: &18 &13 &13 &13",  0},
	{ "%36: &34 &13",  0},
	{ "%29: &29 &11",  0},
	{ "%32: &28 &11",  0},
	{ "%10: &25 &11 %41",  0},
	{ "%41: %15",  0},
	{ "%41: %42",  0},
	{ "%42: %43",  0},
	{ "%42: %43 %42",  0},
	{ "%43: &11",  0},
	{ "%11: &26 &11",  0},
	{ "%12: &35 &11",  0},
	{ "%08: &27 &11 &13 &11 %44",  0},
	{ "%44: %15",  0},
	{ "%44: %45",  0},
	{ "%45: %46",  0},
	{ "%45: %46 %45",  0},
	{ "%46: %47",  0},
	{ "%46: %48",  0},
	{ "%46: %49",  0},
	{ "%47: &21 &11 &22",  0},
	{ "%48: &21 &11 &13 &13 &22",  0},
	{ "%49: &21 &11 %50 %51 &22",  0},
	{ "%51: %15",  0},
	{ "%51: %52",  0},
	{ "%52: %50",  0},
	{ "%52: %50 %52",  0},
	{ "%50: &11",  0},
	{ "%09: &36 %44",  0},
	{ "%19: %15",  0},
	{ "%19: %53",  0},
	{ "%53: %54",  0},
	{ "%53: %54 %53",  0},
	{ "%54: &11 %55 %56",  0},
	{ "%55: %57",  0},
	{ "%55: %15",  0},
	{ "%57: &12",  0},
	{ "%56: %58",  0},
	{ "%56: %15",  0},
	{ "%58: &10 &13",  0},
	{ "%15:",  0},
{ "$accept",  0},{ "error",  0}
};
yyNamedType yyTokenTypes[] = {
	{ "$end",  0,  0},
	{ "error",  256,  0},
	{ "SCHEMA",  257,  0},
	{ "FLAGS",  258,  0},
	{ "VOLUME",  259,  0},
	{ "DELAY",  260,  0},
	{ "PAN",  261,  0},
	{ "PRIORITY",  262,  0},
	{ "FADE",  263,  0},
	{ "ARCHETYPE",  264,  0},
	{ "FREQ",  265,  0},
	{ "IDENT",  266,  1},
	{ "STRING",  267,  1},
	{ "INT",  268,  2},
	{ "FLOAT",  269,  3},
	{ "INCLUDE",  270,  0},
	{ "DEFINE",  271,  0},
	{ "MONO_LOOP",  272,  0},
	{ "POLY_LOOP",  273,  0},
	{ "CONCEPT",  274,  0},
	{ "VOICE",  275,  0},
	{ "LPAREN",  276,  0},
	{ "RPAREN",  277,  0},
	{ "EQUAL",  278,  0},
	{ "COLON",  279,  0},
	{ "TAG",  280,  0},
	{ "TAG_INT",  281,  0},
	{ "SCHEMA_VOICE",  282,  0},
	{ "MESSAGE",  283,  0},
	{ "AUDIO_CLASS",  284,  0},
	{ "PAN_RANGE",  285,  0},
	{ "NO_REPEAT",  286,  0},
	{ "NO_CACHE",  287,  0},
	{ "STREAM",  288,  0},
	{ "LOOP_COUNT",  289,  0},
	{ "ENV_TAG_REQUIRED",  290,  0},
	{ "ENV_TAG",  291,  0},
	{ "NO_COMBAT",  292,  0},
	{ "PLAY_ONCE",  293,  0},
	{ "NET_AMBIENT",  294,  0},
	{ "LOCAL_SPATIAL",  295,  0}

};
#endif
static short yydef[] = {

	  97,  104,   -1,  100,   97,   97,   97,  101,  102,   97, 
	 103,   -5,   97,   97,   98,   99,   97
};
static short yyex[] = {

	   0,    0,   -1,    1,  277,   97,   -1,    1
};
static short yyact[] = {

	 -46,  -43,  -44,  -45,  -47,  -48,  -50,  -49,   -1,  291, 
	 290,  282,  281,  280,  275,  274,  270,  257,  -42,  276, 
	 -41,  266,  -61,  266,  -62,  266,   -5,  266,   -6,  266, 
	  -7,  266,  -40,  266,  -88,  267,  -39,  266,  -38,  268, 
	 -63,  266,  -25,  -26,  -27,  -28,  -30,  -31,  -32,  -33, 
	 -34,  -37,  -36,  -29,  -83,  -78,  -77,  -35,  -82,  -81, 
	 -80,  -79,  295,  294,  293,  292,  289,  288,  287,  286, 
	 285,  284,  283,  273,  272,  264,  263,  262,  261,  260, 
	 259,  258,  -32,  264,  -87,  268,  -56,  -24,  -59,  277, 
	 268,  266,  -13,  266,  -65,  266,  -66,  266,  -67,  268, 
	 -23,  268,  -22,  268,  -70,  266,  -71,  268,  -72,  268, 
	 -73,  268,  -74,  268,  -75,  268,  -76,  268,  -84,  268, 
	 -14,  266,  -56,  266,  -20,  268,  -19,  268,  -69,  268, 
	 -53,  267,  -57,  277,  -58,  277,  -68,  268,  -18,  265, 
	 -52,  268,   -1
};
static short yypact[] = {

	  19,    9,   36,   19,   41,   62,   83,   41,   62,  121, 
	  83,  123,   19,  131,  121,  123,  139,  141,  137,  135, 
	 133,  129,  127,  125,  119,  117,  115,  113,  111,  109, 
	 107,  105,  103,  101,   99,   97,   95,   93,   89,   85, 
	  39,   37,   35,   33,   31,   29,   27,   25,   23,   21,    9
};
static short yygo[] = {

	  -3,   -2,  -90,  -89,    1,  -91,  -92,  -93,  -94,  -95, 
	 -96,  -97,  -98,  -99,  -86, -102, -100,   10, -127, -104, 
	-101, -139, -136, -143, -145, -130,   16,   13,   11,    9, 
	   6,    5,    4,  -11, -103, -103, -115,   10,    6,  -10, 
	 -85,   -9, -106, -105,    8, -107, -108, -109, -110, -111, 
	-112, -113, -114, -116, -117, -118, -119, -120, -121, -122, 
	-123, -124, -125, -126,  -64, -129, -128,    7,   -8,  -60, 
	 -55,   12, -132, -131,    3,   -4, -133, -134, -135,  -12, 
	 -16,   38,  -21, -138, -137,   15, -141, -140,   14,  -15, 
	 -17,  -54, -142, -144,   -1
};
static short yypgo[] = {

	   0,    0,    0,   93,   92,   89,   10,   80,   78,   77, 
	  76,    9,   13,   12,   68,   11,   55,   52,   59,   54, 
	  53,   36,   51,   50,   49,   48,   47,   46,   58,   56, 
	  63,   62,   61,   60,   57,   45,    6,    7,    8,    5, 
	   1,    1,    3,    3,    3,    3,    3,    3,    3,    3, 
	   3,   14,   14,   16,   33,   39,   41,   41,   43,   43, 
	  43,   43,   43,   43,   43,   43,   43,   43,   43,   43, 
	  43,   43,   43,   43,   43,   43,   43,   43,   64,   64, 
	  66,   70,   70,   73,   75,   75,   75,   82,   82,   84, 
	  40,   40,   87,   90,   90,   91,   91,   25,   87,   84, 
	  73,   66,   39,   16,    0,    0
};
static short yyrlen[] = {

	   0,    0,    0,    2,    1,    3,    2,    1,    5,    5, 
	   3,    5,    2,    2,    1,    3,    2,    2,    2,    4, 
	   3,    2,    2,    2,    2,    2,    2,    2,    1,    1, 
	   1,    1,    1,    1,    1,    2,    4,    3,    3,    2, 
	   1,    2,    1,    1,    1,    1,    1,    1,    1,    1, 
	   1,    1,    1,    2,    1,    1,    1,    2,    1,    1, 
	   1,    1,    1,    1,    1,    1,    1,    1,    1,    1, 
	   1,    1,    1,    1,    1,    1,    1,    1,    1,    1, 
	   2,    1,    1,    2,    1,    1,    1,    1,    1,    2, 
	   1,    1,    2,    1,    1,    1,    1,    0,    1,    1, 
	   1,    1,    1,    1,    1,    2
};
#define YYS0	50
#define YYDELTA	48
#define YYNPACT	51
#define YYNDEF	17

#define YYr103	0
#define YYr104	1
#define YYr105	2
#define YYr101	3
#define YYr98	4
#define YYr95	5
#define YYr90	6
#define YYr89	7
#define YYr84	8
#define YYr83	9
#define YYr82	10
#define YYr74	11
#define YYr73	12
#define YYr72	13
#define YYr71	14
#define YYr66	15
#define YYr65	16
#define YYr64	17
#define YYr63	18
#define YYr62	19
#define YYr61	20
#define YYr60	21
#define YYr59	22
#define YYr58	23
#define YYr57	24
#define YYr56	25
#define YYr55	26
#define YYr54	27
#define YYr53	28
#define YYr52	29
#define YYr51	30
#define YYr50	31
#define YYr49	32
#define YYr48	33
#define YYr47	34
#define YYr46	35
#define YYr21	36
#define YYr15	37
#define YYr14	38
#define YYr13	39
#define YYrACCEPT	YYr103
#define YYrERROR	YYr104
#define YYrLR2	YYr105
#if YYDEBUG
char * yysvar[] = {
	"$accept",
	"file",
	"statements",
	"statement",
	"include",
	"schema",
	"voice",
	"concept",
	"schema_voice",
	"env_tag",
	"tag",
	"tag_int",
	"env_tag_required",
	"opt_voice_params",
	"voice_params",
	"null",
	"voice_param",
	"archetype",
	"opt_schema_params",
	"opt_samples",
	"schema_params",
	"schema_param",
	"flags",
	"volume",
	"delay",
	"pan",
	"pan_range",
	"priority",
	"fade",
	"audioclass",
	"monoloop",
	"polyloop",
	"message",
	"no_cache",
	"no_repeat",
	"stream",
	"loopcount",
	"no_combat",
	"play_once",
	"net_ambient",
	"local_spatial",
	"opt_tag_states",
	"tag_states",
	"tag_state",
	"opt_schema_tags",
	"schema_tags",
	"schema_tag",
	"schema_tag_empty",
	"schema_tag_int",
	"schema_tag_enum",
	"schema_arg",
	"schema_args",
	"schema_arg_list",
	"samples",
	"sample",
	"opt_text",
	"opt_freq",
	"text",
	"freq",
	0
};
short yyrmap[] = {

	 103,  104,  105,  101,   98,   95,   90,   89,   84,   83, 
	  82,   74,   73,   72,   71,   66,   65,   64,   63,   62, 
	  61,   60,   59,   58,   57,   56,   55,   54,   53,   52, 
	  51,   50,   49,   48,   47,   46,   21,   15,   14,   13, 
	   2,    3,    4,    5,    6,    7,    8,    9,   10,   11, 
	  12,   16,   17,   19,   20,   22,   24,   25,   26,   27, 
	  28,   29,   30,   31,   32,   33,   34,   35,   36,   37, 
	  38,   39,   40,   41,   42,   43,   44,   45,   67,   68, 
	  70,   75,   76,   78,   79,   80,   81,   85,   86,   88, 
	  91,   92,   94,   96,   97,   99,  100,  102,   93,   87, 
	  77,   69,   23,   18,    1,    0
};
short yysmap[] = {

	   1,   20,   21,   27,   33,   34,   35,   43,   88,   90, 
	  92,   98,  101,  117,  118,  123,  134,  140,  129,  127, 
	 126,  107,  106,   99,   66,   58,   57,   56,   55,   54, 
	  53,   52,   51,   50,   49,   48,   47,   41,   39,   36, 
	  30,   23,    9,    8,    7,    6,    5,    4,    3,    2, 
	   0,  144,  131,  143,   22,   97,  137,  138,  100,  128, 
	  31,   32,   42,   46,  103,  104,  105,  139,  130,  108, 
	 109,  110,  111,  112,  113,  114,   59,   60,   61,   62, 
	  63,   64,   65,  115,  121,   95,   96,   37,   19,   38, 
	  18,   17,   16,   15,   14,   13,   12,   11,   10,   94, 
	  93,  122,   91,   89,   87,  116,   86,   85,   84,   83, 
	  82,   81,   80,   79,   78,   77,   76,   75,   74,   73, 
	  72,   71,   70,   69,   68,   67,   45,   44,  102,   29, 
	  28,   40,   26,   25,   24,  125,  124,  136,  120,  119, 
	 135,  133,  132,  142,  141
};
int yyntoken = 41;
int yynvar = 59;
int yynstate = 145;
int yynrule = 106;
#endif

#if YYDEBUG
/*
 * Package up YACC context for tracing
 */
typedef struct yyTraceItems_tag {
	int	state, lookahead, errflag, done;
	int	rule, npop;
	short	* states;
	int	nstates;
	YYSTYPE * values;
	int	nvalues;
	short	* types;
} yyTraceItems;
#endif


/*
 * Copyright 1985, 1990 by Mortice Kern Systems Inc.  All rights reserved.
 * 
 * Automaton to interpret LALR(1) tables.
 *
 * Macros:
 *	yyclearin - clear the lookahead token.
 *	yyerrok - forgive a pending error
 *	YYERROR - simulate an error
 *	YYACCEPT - halt and return 0
 *	YYABORT - halt and return 1
 *	YYRETURN(value) - halt and return value.  You should use this
 *		instead of return(value).
 *	YYREAD - ensure yychar contains a lookahead token by reading
 *		one if it does not.  See also YYSYNC.
 *	YYRECOVERING - 1 if syntax error detected and not recovered
 *		yet; otherwise, 0.
 *
 * Preprocessor flags:
 *	YYDEBUG - includes debug code if 1.  The parser will print
 *		 a travelogue of the parse if this is defined as 1
 *		 and yydebug is non-zero.
 *		yacc -t sets YYDEBUG to 1, but not yydebug.
 *	YYTRACE - turn on YYDEBUG, and undefine default trace functions
 *		so that the interactive functions in 'ytrack.c' will
 *		be used.
 *	YYSSIZE - size of state and value stacks (default 150).
 *	YYSTATIC - By default, the state stack is an automatic array.
 *		If this is defined, the stack will be static.
 *		In either case, the value stack is static.
 *	YYALLOC - Dynamically allocate both the state and value stacks
 *		by calling malloc() and free().
 *	YYSYNC - if defined, yacc guarantees to fetch a lookahead token
 *		before any action, even if it doesnt need it for a decision.
 *		If YYSYNC is defined, YYREAD will never be necessary unless
 *		the user explicitly sets yychar = -1
 *
 * Copyright (c) 1983, by the University of Waterloo
 */
/*
 * Prototypes
 */

extern int yylex YY_ARGS((void));

#if YYDEBUG

#include <stdlib.h>		/* common prototypes */
#include <string.h>

extern char *	yyValue YY_ARGS((YYSTYPE, int));	/* print yylval */
extern void yyShowState YY_ARGS((yyTraceItems *));
extern void yyShowReduce YY_ARGS((yyTraceItems *));
extern void yyShowGoto YY_ARGS((yyTraceItems *));
extern void yyShowShift YY_ARGS((yyTraceItems *));
extern void yyShowErrRecovery YY_ARGS((yyTraceItems *));
extern void yyShowErrDiscard YY_ARGS((yyTraceItems *));

extern void yyShowRead YY_ARGS((int));
#endif

/*
 * If YYDEBUG defined and yydebug set,
 * tracing functions will be called at appropriate times in yyparse()
 * Pass state of YACC parse, as filled into yyTraceItems yyx
 * If yyx.done is set by the tracing function, yyparse() will terminate
 * with a return value of -1
 */
#define YY_TRACE(fn) { \
	yyx.state = yystate; yyx.lookahead = yychar; yyx.errflag =yyerrflag; \
	yyx.states = yys+1; yyx.nstates = yyps-yys; \
	yyx.values = yyv+1; yyx.nvalues = yypv-yyv; \
	yyx.types = yytypev+1; yyx.done = 0; \
	yyx.rule = yyi; yyx.npop = yyj; \
	fn(&yyx); \
	if (yyx.done) YYRETURN(-1); }

#ifndef I18N
#define	gettext(x)	x
#endif

#ifndef YYSSIZE
# define YYSSIZE	150
#endif

#define YYERROR		goto yyerrlabel
#define yyerrok		yyerrflag = 0
#if YYDEBUG
#define yyclearin	{ if (yydebug) yyShowRead(-1); yychar = -1; }
#else
#define yyclearin	yychar = -1
#endif
#define YYACCEPT	YYRETURN(0)
#define YYABORT		YYRETURN(1)
#define YYRECOVERING()	(yyerrflag != 0)
#ifdef YYALLOC
# define YYRETURN(val)	{ retval = (val); goto yyReturn; }
#else
# define YYRETURN(val)	return(val)
#endif
#if YYDEBUG
/* The if..else makes this macro behave exactly like a statement */
# define YYREAD	if (yychar < 0) {					\
			if ((yychar = yylex()) < 0)			\
				yychar = 0;				\
			if (yydebug)					\
				yyShowRead(yychar);			\
		} else
#else
# define YYREAD	if (yychar < 0) {					\
			if ((yychar = yylex()) < 0)			\
				yychar = 0;				\
		} else
#endif

#define YYERRCODE	256		/* value of `error' */
#define	YYQYYP	yyq[yyq-yyp]

YYSTYPE	yyval;				/* $ */
YYSTYPE	*yypvt;				/* $n */
YYSTYPE	yylval;				/* yylex() sets this */

int	yychar,				/* current token */
	yyerrflag,			/* error flag */
	yynerrs;			/* error count */

#if YYDEBUG
int yydebug = 0;		/* debug if this flag is set */
extern char	*yysvar[];	/* table of non-terminals (aka 'variables') */
extern yyNamedType yyTokenTypes[];	/* table of terminals & their types */
extern short	yyrmap[], yysmap[];	/* map internal rule/states */
extern int	yynstate, yynvar, yyntoken, yynrule;

extern int	yyGetType YY_ARGS((int));	/* token type */
extern char	*yyptok YY_ARGS((int));	/* printable token string */
extern int	yyExpandName YY_ARGS((int, int, char *, int));
				  /* expand yyRules[] or yyStates[] */
static char *	yygetState YY_ARGS((int));

#define yyassert(condition, msg, arg) \
	if (!(condition)) { \
		printf(gettext("\nyacc bug: ")); \
		printf(msg, arg); \
		YYABORT; }
#else /* !YYDEBUG */
#define yyassert(condition, msg, arg)
#endif

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

yyparse()
{
	register short		yyi, *yyp;	/* for table lookup */
	register short		*yyps;		/* top of state stack */
	register short		yystate;	/* current state */
	register YYSTYPE	*yypv;		/* top of value stack */
	register short		*yyq;
	register int		yyj;
#if YYDEBUG
	yyTraceItems	yyx;			/* trace block */
	short	* yytp;
	int	yyruletype = 0;
#endif
#ifdef YYSTATIC
	static short	yys[YYSSIZE + 1];
	static YYSTYPE	yyv[YYSSIZE + 1];
#if YYDEBUG
	static short	yytypev[YYSSIZE+1];	/* type assignments */
#endif
#else /* ! YYSTATIC */
#ifdef YYALLOC
	YYSTYPE *yyv;
	short	*yys;
#if YYDEBUG
	short	*yytypev;
#endif
	YYSTYPE save_yylval;
	YYSTYPE save_yyval;
	YYSTYPE *save_yypvt;
	int save_yychar, save_yyerrflag, save_yynerrs;
	int retval;
#else
	short		yys[YYSSIZE + 1];
	static YYSTYPE	yyv[YYSSIZE + 1];	/* historically static */
#if YYDEBUG
	short	yytypev[YYSSIZE+1];		/* mirror type table */
#endif
#endif /* ! YYALLOC */
#endif /* ! YYSTATIC */


#ifdef YYALLOC
	yys = (short *) malloc((YYSSIZE + 1) * sizeof(short));
	yyv = (YYSTYPE *) malloc((YYSSIZE + 1) * sizeof(YYSTYPE));
#if YYDEBUG
	yytypev = (short *) malloc((YYSSIZE+1) * sizeof(short));
#endif
	if (yys == (short *)0 || yyv == (YYSTYPE *)0
#if YYDEBUG
		|| yytypev == (short *) 0
#endif
	) {
		yyerror("Not enough space for parser stacks");
		return 1;
	}
	save_yylval = yylval;
	save_yyval = yyval;
	save_yypvt = yypvt;
	save_yychar = yychar;
	save_yyerrflag = yyerrflag;
	save_yynerrs = yynerrs;
#endif

	yynerrs = 0;
	yyerrflag = 0;
	yyclearin;
	yyps = yys;
	yypv = yyv;
	*yyps = yystate = YYS0;		/* start state */
#if YYDEBUG
	yytp = yytypev;
	yyi = yyj = 0;			/* silence compiler warnings */
#endif

yyStack:
	yyassert((unsigned)yystate < yynstate, gettext("state %d\n"), yystate);
	if (++yyps > &yys[YYSSIZE]) {
		yyerror("Parser stack overflow");
		YYABORT;
	}
	*yyps = yystate;	/* stack current state */
	*++yypv = yyval;	/* ... and value */
#if YYDEBUG
	*++yytp = yyruletype;	/* ... and type */

	if (yydebug)
		YY_TRACE(yyShowState)
#endif

	/*
	 *	Look up next action in action table.
	 */
yyEncore:
#ifdef YYSYNC
	YYREAD;
#endif
	if (yystate >= sizeof yypact/sizeof yypact[0]) 	/* simple state */
		yyi = yystate - YYDELTA;	/* reduce in any case */
	else {
		if(*(yyp = &yyact[yypact[yystate]]) >= 0) {
			/* Look for a shift on yychar */
#ifndef YYSYNC
			YYREAD;
#endif
			yyq = yyp;
			yyi = yychar;
			while (yyi < *yyp++)
				;
			if (yyi == yyp[-1]) {
				yystate = ~YYQYYP;
#if YYDEBUG
				if (yydebug) {
					yyruletype = yyGetType(yychar);
					YY_TRACE(yyShowShift)
				}
#endif
				yyval = yylval;	/* stack what yylex() set */
				yyclearin;		/* clear token */
				if (yyerrflag)
					yyerrflag--;	/* successful shift */
				goto yyStack;
			}
		}

		/*
	 	 *	Fell through - take default action
	 	 */

		if (yystate >= sizeof yydef /sizeof yydef[0])
			goto yyError;
		if ((yyi = yydef[yystate]) < 0)	 { /* default == reduce? */
			/* Search exception table */
			yyassert((unsigned)~yyi < sizeof yyex/sizeof yyex[0],
				gettext("exception %d\n"), yystate);
			yyp = &yyex[~yyi];
#ifndef YYSYNC
			YYREAD;
#endif
			while((yyi = *yyp) >= 0 && yyi != yychar)
				yyp += 2;
			yyi = yyp[1];
			yyassert(yyi >= 0,
				 gettext("Ex table not reduce %d\n"), yyi);
		}
	}

	yyassert((unsigned)yyi < yynrule, gettext("reduce %d\n"), yyi);
	yyj = yyrlen[yyi];
#if YYDEBUG
	if (yydebug)
		YY_TRACE(yyShowReduce)
	yytp -= yyj;
#endif
	yyps -= yyj;		/* pop stacks */
	yypvt = yypv;		/* save top */
	yypv -= yyj;
	yyval = yypv[1];	/* default action $ = $1 */
#if YYDEBUG
	yyruletype = yyRules[yyrmap[yyi]].type;
#endif

	switch (yyi) {		/* perform semantic action */
		
case YYr13: {	/* include :  INCLUDE STRING */

   IncParseFile(yypvt[0].strval);
   FreeString(yypvt[0].strval);

} break;

case YYr14: {	/* concept :  CONCEPT IDENT INT */

   Label ConceptLabel;

   MakeLabel(&ConceptLabel, yypvt[-1].strval);
   SpeechAddConcept(&ConceptLabel, yypvt[0].ival);
   FreeString(yypvt[-1].strval);

} break;

case YYr15: {	/* voice :  VOICE IDENT opt_voice_params */

   ObjID voiceID;
   const char* name = yypvt[-1].strval; 

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
   FreeString(yypvt[-1].strval);

} break;

case YYr21: {	/* schema :  SCHEMA IDENT opt_schema_params opt_samples */

   ObjID objID;
   int i, j;

   MakeLabel(&schemaLabel, yypvt[-2].strval);
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
   FreeString(yypvt[-2].strval);

} break;

case YYr46: {	/* flags :  FLAGS INT */

   schemaPlayParams.flags = yypvt[0].ival;
   schemaParamsSet = TRUE;

} break;

case YYr47: {	/* no_repeat :  NO_REPEAT */

   schemaPlayParams.flags |= SCH_NO_REPEAT;
   schemaParamsSet = TRUE;

} break;

case YYr48: {	/* no_combat :  NO_COMBAT */

   schemaPlayParams.flags |= SCH_NO_COMBAT;
   schemaParamsSet = TRUE;

} break;

case YYr49: {	/* play_once :  PLAY_ONCE */

   schemaPlayParams.flags |= SCH_PLAY_ONCE;
   schemaParamsSet = TRUE;

} break;

case YYr50: {	/* net_ambient :  NET_AMBIENT */

   schemaPlayParams.flags |= SCH_NET_AMBIENT;
   schemaParamsSet = TRUE;

} break;

case YYr51: {	/* local_spatial :  LOCAL_SPATIAL */

   schemaPlayParams.flags |= SCH_LOC_SPATIAL;
   schemaParamsSet = TRUE;

} break;

case YYr52: {	/* no_cache :  NO_CACHE */

   schemaPlayParams.flags |= SCH_NO_CACHE;
   schemaParamsSet = TRUE;

} break;

case YYr53: {	/* stream :  STREAM */

   schemaPlayParams.flags |= SCH_STREAM;
   schemaParamsSet = TRUE;

} break;

case YYr54: {	/* volume :  VOLUME INT */

   schemaPlayParams.volume = yypvt[0].ival;
   schemaParamsSet = TRUE;

} break;

case YYr55: {	/* delay :  DELAY INT */

   schemaPlayParams.initialDelay = yypvt[0].ival;
   schemaParamsSet = TRUE;

} break;

case YYr56: {	/* pan :  PAN INT */

   schemaPlayParams.flags &= ~SCH_PAN_RANGE;
   schemaPlayParams.flags |= SCH_PAN_POS;
   schemaPlayParams.pan = yypvt[0].ival;
   schemaParamsSet = TRUE;

} break;

case YYr57: {	/* pan_range :  PAN_RANGE INT */

   schemaPlayParams.flags &= ~SCH_PAN_POS;
   schemaPlayParams.flags |= SCH_PAN_RANGE;
   schemaPlayParams.pan = yypvt[0].ival;
   schemaParamsSet = TRUE;

} break;

case YYr58: {	/* priority :  PRIORITY INT */

   schemaPriority = yypvt[0].ival;
   schemaPrioritySet = TRUE;

} break;

case YYr59: {	/* fade :  FADE INT */

   schemaPlayParams.fade = yypvt[0].ival;
   schemaParamsSet = TRUE;

} break;

case YYr60: {	/* archetype :  ARCHETYPE IDENT */

   Label label;
   sSchemaPlayParams *pArchParams, *pDefaultParams;

   MakeLabel(&label, yypvt[0].strval);
   archID = IObjectSystem_GetObjectNamed(pObjSys, &label);
   if (archID == OBJ_NULL)
      Warning(("Unknown archetype %s\n", yypvt[0].strval));
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
   FreeString(yypvt[0].strval);

} break;

case YYr61: {	/* monoloop :  MONO_LOOP INT INT */

   loopSet = TRUE;
   loopParams.flags &= ~SCHEMA_LOOP_POLY;
   loopParams.maxSamples = 1;
   loopParams.intervalMin = yypvt[-1].ival;
   loopParams.intervalMax = yypvt[0].ival;

} break;

case YYr62: {	/* polyloop :  POLY_LOOP INT INT INT */

   loopSet = TRUE;
   loopParams.flags |= SCHEMA_LOOP_POLY;
   loopParams.maxSamples = yypvt[-2].ival;
   loopParams.intervalMin = yypvt[-1].ival;
   loopParams.intervalMax = yypvt[0].ival;

} break;

case YYr63: {	/* loopcount :  LOOP_COUNT INT */

   loopParams.flags |= SCHEMA_LOOP_COUNT;
   loopParams.count = yypvt[0].ival;

} break;

case YYr64: {	/* audioclass :  AUDIO_CLASS IDENT */

   // @NOTE: this must be in the same order as in schbase.h
   // @NOTE: Max is 16.
   static char *classes[]={"noise","speech","ambient","music","metaui",                   // Originial Thief classes.
                           "player_feet","other_feet","collisions","weapons","monsters"}; // Additional Shock classes.
   bool ok=FALSE;
   int i;

   if (isdigit(yypvt[0].strval[0]))
   {
      int val=yypvt[0].strval[0]-'0';
      schemaPlayParams.flags |= ((val+1)<<SCH_CLASS_SHIFT);
      schemaParamsSet = TRUE;
      ok=TRUE;
   }
   else
      for (i=0; i<sizeof(classes)/sizeof(classes[0]); i++)
      {
         if (stricmp(yypvt[0].strval,classes[i])==0)
         {
            schemaPlayParams.flags |= ((i+1)<<SCH_CLASS_SHIFT);
            schemaParamsSet = TRUE;
            ok=TRUE;
            break;
         }
      }
   if (!ok)
      Warning(("Hey! %s isnt a valid audio class\n",yypvt[0].strval));
   FreeString(yypvt[0].strval);

} break;

case YYr65: {	/* message :  MESSAGE IDENT */

   msgSet = TRUE;
   strncpy(&(msgType.text[0]), yypvt[0].strval, 15);
   msgType.text[15] = '\0';
   FreeString(yypvt[0].strval);

} break;

case YYr66: {	/* tag :  TAG IDENT opt_tag_states */

   int i;
   Label TagNameLabel;

   MakeLabel(&TagNameLabel, yypvt[-1].strval);
   SpeechAddTag(&TagNameLabel);
   for (i = 0; i < g_iNumTagValues; ++i)
      SpeechAddTagValue(&g_aTagValueLabel[i]);

   g_iNumTagValues = 0;
   FreeString(yypvt[-1].strval);

} break;

case YYr71: {	/* tag_state :  IDENT */

   if (g_iNumTagValues == (kMaxTagValues - 1)) {
      mprintf("Hey!  Trying to have more than %d states for one tag.\n",
              kMaxTagValues);
      return 1;
   }

   MakeLabel(&g_aTagValueLabel[g_iNumTagValues], yypvt[0].strval);
   ++g_iNumTagValues;

   FreeString(yypvt[0].strval);

} break;

case YYr72: {	/* tag_int :  TAG_INT IDENT */

   Label TagNameLabel;

   MakeLabel(&TagNameLabel, yypvt[0].strval);
   SpeechAddIntTag(&TagNameLabel);
   FreeString(yypvt[0].strval);

} break;

case YYr73: {	/* env_tag_required :  ENV_TAG_REQUIRED IDENT */

   int i;
   Label TagNameLabel;

   MakeLabel(&TagNameLabel, yypvt[0].strval);
   ESndSetTagRequired(&TagNameLabel);

   FreeString(yypvt[0].strval);

} break;

case YYr74: {	/* schema_voice :  SCHEMA_VOICE IDENT INT IDENT opt_schema_tags */

   Label VoiceLabel, ConceptLabel;
   int i;
   int iWeight = yypvt[-2].ival;

   MakeLabel(&VoiceLabel, yypvt[-3].strval);
   MakeLabel(&ConceptLabel, yypvt[-1].strval);

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

   FreeString(yypvt[-3].strval);
   FreeString(yypvt[-1].strval);

} break;

case YYr82: {	/* schema_tag_empty :  LPAREN IDENT RPAREN */

   MakeLabel(&g_aTagNameLabel[g_iNumTagsAdded], yypvt[-1].strval);
   g_aTagMinInt[g_iNumTagsAdded] = 0;
   g_aTagMaxInt[g_iNumTagsAdded] = 0xffffffff;
   ++g_iNumTagsAdded;

   FreeString(yypvt[-1].strval);

} break;

case YYr83: {	/* schema_tag_int :  LPAREN IDENT INT INT RPAREN */

   MakeLabel(&g_aTagNameLabel[g_iNumTagsAdded], yypvt[-3].strval);
   g_aTagMinInt[g_iNumTagsAdded] = yypvt[-2].ival;
   g_aTagMaxInt[g_iNumTagsAdded] = yypvt[-1].ival;
   ++g_iNumTagsAdded;

   FreeString(yypvt[-3].strval);

} break;

case YYr84: {	/* schema_tag_enum :  LPAREN IDENT schema_arg schema_args RPAREN */

   MakeLabel(&g_aTagNameLabel[g_iNumTagsAdded], yypvt[-3].strval);
   ++g_iNumTagsAdded;

   FreeString(yypvt[-3].strval);

} break;

case YYr89: {	/* schema_arg :  IDENT */

   if (g_aTagNumEnums[g_iNumTagsAdded] == 8) {
      Warning(("More than eight enums for a schema tag: %s.\n", yypvt[0].strval));
   } else {
      MakeLabel(&g_aTagEnumLabel[g_iNumTagsAdded]
                                [g_aTagNumEnums[g_iNumTagsAdded]], yypvt[0].strval);
      ++g_aTagNumEnums[g_iNumTagsAdded];
   }

   FreeString(yypvt[0].strval);

} break;

case YYr90: {	/* env_tag :  ENV_TAG opt_schema_tags */

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

} break;

case YYr95: {	/* sample :  IDENT opt_text opt_freq */

   if (samplesNum<SCHEMA_SAMPLES_MAX)
   {
      int nameLen = min(strlen(yypvt[-2].strval), SAMPLE_NAME_LEN-1);
      strncpy(sampleNames[samplesNum], yypvt[-2].strval, nameLen);
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
      Warning(("Sample %s, too many samples for schema\n", yypvt[-2].strval));

   FreeString(yypvt[-2].strval);

} break;

case YYr98: {	/* text :  STRING */


} break;

case YYr101: {	/* freq :  FREQ INT */

   sampleFreq = (uchar)yypvt[0].ival;
   freqSet = TRUE;

} break;
	case YYrACCEPT:
		YYACCEPT;
	case YYrERROR:
		goto yyError;
	}

	/*
	 *	Look up next state in goto table.
	 */

	yyp = &yygo[yypgo[yyi]];
	yyq = yyp++;
	yyi = *yyps;
	while (yyi < *yyp++)
		;

	yystate = ~(yyi == *--yyp? YYQYYP: *yyq);
#if YYDEBUG
	if (yydebug)
		YY_TRACE(yyShowGoto)
#endif
	goto yyStack;

yyerrlabel:	;		/* come here from YYERROR	*/
/*
#pragma used yyerrlabel
 */
	yyerrflag = 1;
	if (yyi == YYrERROR) {
		yyps--;
		yypv--;
#if YYDEBUG
		yytp--;
#endif
	}

yyError:
	switch (yyerrflag) {

	case 0:		/* new error */
		yynerrs++;
		yyi = yychar;
		yyerror("Syntax error");
		if (yyi != yychar) {
			/* user has changed the current token */
			/* try again */
			yyerrflag++;	/* avoid loops */
			goto yyEncore;
		}

	case 1:		/* partially recovered */
	case 2:
		yyerrflag = 3;	/* need 3 valid shifts to recover */
			
		/*
		 *	Pop states, looking for a
		 *	shift on `error'.
		 */

		for ( ; yyps > yys; yyps--, yypv--
#if YYDEBUG
					, yytp--
#endif
		) {
			if (*yyps >= sizeof yypact/sizeof yypact[0])
				continue;
			yyp = &yyact[yypact[*yyps]];
			yyq = yyp;
			do
				;
			while (YYERRCODE < *yyp++);

			if (YYERRCODE == yyp[-1]) {
				yystate = ~YYQYYP;
				goto yyStack;
			}
				
			/* no shift in this state */
#if YYDEBUG
			if (yydebug && yyps > yys+1)
				YY_TRACE(yyShowErrRecovery)
#endif
			/* pop stacks; try again */
		}
		/* no shift on error - abort */
		break;

	case 3:
		/*
		 *	Erroneous token after
		 *	an error - discard it.
		 */

		if (yychar == 0)  /* but not EOF */
			break;
#if YYDEBUG
		if (yydebug)
			YY_TRACE(yyShowErrDiscard)
#endif
		yyclearin;
		goto yyEncore;	/* try again in same state */
	}
	YYABORT;

#ifdef YYALLOC
yyReturn:
	yylval = save_yylval;
	yyval = save_yyval;
	yypvt = save_yypvt;
	yychar = save_yychar;
	yyerrflag = save_yyerrflag;
	yynerrs = save_yynerrs;
	free((char *)yys);
	free((char *)yyv);
	return(retval);
#endif
}

		
#if YYDEBUG
/*
 * Return type of token
 */
int
yyGetType(tok)
int tok;
{
	yyNamedType * tp;
	for (tp = &yyTokenTypes[yyntoken-1]; tp > yyTokenTypes; tp--)
		if (tp->token == tok)
			return tp->type;
	return 0;
}
/*
 * Print a token legibly.
 */
char *
yyptok(tok)
int tok;
{
	yyNamedType * tp;
	for (tp = &yyTokenTypes[yyntoken-1]; tp > yyTokenTypes; tp--)
		if (tp->token == tok)
			return tp->name;
	return "";
}

/*
 * Read state 'num' from YYStatesFile
 */
#ifdef YYTRACE
static FILE *yyStatesFile = (FILE *) 0;
static char yyReadBuf[YYMAX_READ+1];

static char *
yygetState(num)
int num;
{
	int	size;

	if (yyStatesFile == (FILE *) 0
	 && (yyStatesFile = fopen(YYStatesFile, "r")) == (FILE *) 0)
		return "yyExpandName: cannot open states file";

	if (num < yynstate - 1)
		size = (int)(yyStates[num+1] - yyStates[num]);
	else {
		/* length of last item is length of file - ptr(last-1) */
		if (fseek(yyStatesFile, 0L, 2) < 0)
			goto cannot_seek;
		size = (int) (ftell(yyStatesFile) - yyStates[num]);
	}
	if (size < 0 || size > YYMAX_READ)
		return "yyExpandName: bad read size";
	if (fseek(yyStatesFile, yyStates[num], 0) < 0) {
	cannot_seek:
		return "yyExpandName: cannot seek in states file";
	}

	(void) fread(yyReadBuf, 1, size, yyStatesFile);
	yyReadBuf[size] = '\0';
	return yyReadBuf;
}
#endif /* YYTRACE */
/*
 * Expand encoded string into printable representation
 * Used to decode yyStates and yyRules strings.
 * If the expansion of 's' fits in 'buf', return 1; otherwise, 0.
 */
int
yyExpandName(num, isrule, buf, len)
int num, isrule;
char * buf;
int len;
{
	int	i, n, cnt, type;
	char	* endp, * cp;
	char	*s;

	if (isrule)
		s = yyRules[num].name;
	else
#ifdef YYTRACE
		s = yygetState(num);
#else
		s = "*no states*";
#endif

	for (endp = buf + len - 8; *s; s++) {
		if (buf >= endp) {		/* too large: return 0 */
		full:	(void) strcpy(buf, " ...\n");
			return 0;
		} else if (*s == '%') {		/* nonterminal */
			type = 0;
			cnt = yynvar;
			goto getN;
		} else if (*s == '&') {		/* terminal */
			type = 1;
			cnt = yyntoken;
		getN:
			if (cnt < 100)
				i = 2;
			else if (cnt < 1000)
				i = 3;
			else
				i = 4;
			for (n = 0; i-- > 0; )
				n = (n * 10) + *++s - '0';
			if (type == 0) {
				if (n >= yynvar)
					goto too_big;
				cp = yysvar[n];
			} else if (n >= yyntoken) {
			    too_big:
				cp = "<range err>";
			} else
				cp = yyTokenTypes[n].name;

			if ((i = strlen(cp)) + buf > endp)
				goto full;
			(void) strcpy(buf, cp);
			buf += i;
		} else
			*buf++ = *s;
	}
	*buf = '\0';
	return 1;
}
#ifndef YYTRACE
/*
 * Show current state of yyparse
 */
void
yyShowState(tp)
yyTraceItems * tp;
{
	short * p;
	YYSTYPE * q;

	printf(
	    gettext("state %d (%d), char %s (%d)\n"),
	      yysmap[tp->state], tp->state,
	      yyptok(tp->lookahead), tp->lookahead);
}
/*
 * show results of reduction
 */
void
yyShowReduce(tp)
yyTraceItems * tp;
{
	printf("reduce %d (%d), pops %d (%d)\n",
		yyrmap[tp->rule], tp->rule,
		tp->states[tp->nstates - tp->npop],
		yysmap[tp->states[tp->nstates - tp->npop]]);
}
void
yyShowRead(val)
int val;
{
	printf(gettext("read %s (%d)\n"), yyptok(val), val);
}
void
yyShowGoto(tp)
yyTraceItems * tp;
{
	printf(gettext("goto %d (%d)\n"), yysmap[tp->state], tp->state);
}
void
yyShowShift(tp)
yyTraceItems * tp;
{
	printf(gettext("shift %d (%d)\n"), yysmap[tp->state], tp->state);
}
void
yyShowErrRecovery(tp)
yyTraceItems * tp;
{
	short	* top = tp->states + tp->nstates - 1;

	printf(
	gettext("Error recovery pops state %d (%d), uncovers %d (%d)\n"),
		yysmap[*top], *top, yysmap[*(top-1)], *(top-1));
}
void
yyShowErrDiscard(tp)
yyTraceItems * tp;
{
	printf(gettext("Error recovery discards %s (%d), "),
		yyptok(tp->lookahead), tp->lookahead);
}
#endif	/* ! YYTRACE */
#endif	/* YYDEBUG */
