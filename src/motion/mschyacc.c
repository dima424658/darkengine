/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

/* n:\bin\yacc -p msch -l -D mschtok.h -o mschyacc.c c:\b\cam\src\mschema.y */

#ifdef MSCHTRACE
#define MSCHDEBUG 1
#else
#ifndef MSCHDEBUG
#define MSCHDEBUG 0
#endif
#endif

/*
 * Portable way of defining ANSI C prototypes
 */
#ifndef MSCH_ARGS
#if __STDC__
#define MSCH_ARGS(x)	x
#else
#define MSCH_ARGS(x)	()
#endif
#endif


#if MSCHDEBUG
typedef struct mschNamedType_tag {	/* Tokens */
	char	* name;		/* printable name */
	short	token;		/* token # */
	short	type;		/* token type */
} mschNamedType;
typedef struct mschTypedRules_tag {	/* Typed rule table */
	char	* name;		/* compressed rule string */
	short	type;		/* rule result type */
} mschTypedRules;

#endif

typedef union {
   char *strval;
   int  ival;
   float fval;
   BOOL  bval;
} MSCHSTYPE;
#define IDENT	257
#define STRING	258
#define INT	259
#define FLOAT	260
#define BOOLEAN	261
#define INCLUDE	262
#define DEFINE	263
#define LPAREN	264
#define RPAREN	265
#define EQUAL	266
#define COLON	267
#define SCHEMA	268
#define TAG	269
#define TAG_OPT	270
#define ACTOR	271
#define ACTORS_DECLARE	272
#define ARCHETYPE	273
#define MOTION	274
#define TAGLIST_HEADER	275
#define MOTLIST_HEADER	276
#define STRETCH	277
#define TIMEWARP	278
#define DURATION	279
#define DISTANCE	280
#define MOT_NECK_FIXED	281
#define MOT_NECK_NOT_FIXED	282
#define MOT_BLEND_NONE	283
#define MOT_BLEND_DEFAULT	284
#define MOT_IS_TURN	285
#define MOT_BLEND_LENGTH	286
#define MOT_IN_PLACE	287
#define MOT_IS_LOCO	288

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


extern int mschchar, mscherrflag;
extern MSCHSTYPE mschlval;
#if MSCHDEBUG
enum MSCH_Types { MSCH_t_NoneDefined, MSCH_t_strval, MSCH_t_ival, MSCH_t_fval, MSCH_t_bval
};
#endif
#if MSCHDEBUG
mschTypedRules mschRules[] = {
	{ "&00: %01 &00",  0},
	{ "%01: %02",  0},
	{ "%02: %03",  0},
	{ "%02: %03 %02",  0},
	{ "%03: %04",  0},
	{ "%03: %05",  0},
	{ "%03: %06",  0},
	{ "%03: %07",  0},
	{ "%04: &07 &03",  0},
	{ "%08: %09",  0},
	{ "%08: %10",  0},
	{ "%09: &15 &05",  0},
	{ "%06: &14 &02 %08",  0},
	{ "%07: &17 &04",  0},
	{ "%11: %12",  0},
	{ "%11: %13",  0},
	{ "%12: &18 &02",  0},
	{ "%13: &18 &04",  0},
	{ "%05: &13 &02 &16 &04 %11 %14 %15 %16",  0},
	{ "%14: %17",  0},
	{ "%14: %10",  0},
	{ "%17: %17 %18",  0},
	{ "%17: %18",  0},
	{ "%18: %19",  0},
	{ "%18: %20",  0},
	{ "%18: %21",  0},
	{ "%18: %22",  0},
	{ "%19: &23 &05",  0},
	{ "%20: &24 &05",  0},
	{ "%21: &22 &05",  0},
	{ "%22: &25 &05",  0},
	{ "%15: &20 %23",  0},
	{ "%23: %24",  0},
	{ "%23: %10",  0},
	{ "%24: %24 %25",  0},
	{ "%24: %25",  0},
	{ "%26: %27",  0},
	{ "%26: %10",  0},
	{ "%27: &04",  0},
	{ "%25: &02 %26",  0},
	{ "%16: &21 %28",  0},
	{ "%28: %29",  0},
	{ "%28: %10",  0},
	{ "%29: %29 %30",  0},
	{ "%29: %30",  0},
	{ "%31: %32",  0},
	{ "%31: %10",  0},
	{ "%32: %33 %32",  0},
	{ "%32: %33",  0},
	{ "%33: %34",  0},
	{ "%33: %35",  0},
	{ "%33: %36",  0},
	{ "%33: &27",  0},
	{ "%33: &29",  0},
	{ "%33: %37",  0},
	{ "%33: %38",  0},
	{ "%33: %39",  0},
	{ "%34: &26",  0},
	{ "%37: &28",  0},
	{ "%35: &31 &04",  0},
	{ "%39: &32",  0},
	{ "%38: &33",  0},
	{ "%36: &30",  0},
	{ "%30: &02 %31",  0},
	{ "%10:",  0},
	{ "%16:",  0},
{ "$accept",  0},{ "error",  0}
};
mschNamedType mschTokenTypes[] = {
	{ "$end",  0,  0},
	{ "error",  256,  0},
	{ "IDENT",  257,  1},
	{ "STRING",  258,  1},
	{ "INT",  259,  2},
	{ "FLOAT",  260,  3},
	{ "BOOLEAN",  261,  4},
	{ "INCLUDE",  262,  0},
	{ "DEFINE",  263,  0},
	{ "LPAREN",  264,  0},
	{ "RPAREN",  265,  0},
	{ "EQUAL",  266,  0},
	{ "COLON",  267,  0},
	{ "SCHEMA",  268,  0},
	{ "TAG",  269,  0},
	{ "TAG_OPT",  270,  0},
	{ "ACTOR",  271,  0},
	{ "ACTORS_DECLARE",  272,  0},
	{ "ARCHETYPE",  273,  0},
	{ "MOTION",  274,  0},
	{ "TAGLIST_HEADER",  275,  0},
	{ "MOTLIST_HEADER",  276,  0},
	{ "STRETCH",  277,  0},
	{ "TIMEWARP",  278,  0},
	{ "DURATION",  279,  0},
	{ "DISTANCE",  280,  0},
	{ "MOT_NECK_FIXED",  281,  0},
	{ "MOT_NECK_NOT_FIXED",  282,  0},
	{ "MOT_BLEND_NONE",  283,  0},
	{ "MOT_BLEND_DEFAULT",  284,  0},
	{ "MOT_IS_TURN",  285,  0},
	{ "MOT_BLEND_LENGTH",  286,  0},
	{ "MOT_IN_PLACE",  287,  0},
	{ "MOT_IS_LOCO",  288,  0}

};
#endif
static short mschdef[] = {

	  67,   -1,   62,   -5,   66,   62,    3,   62,   65,   62, 
	  62,   64,   63
};
static short mschex[] = {

	   0,    0,   -1,    1,  275,   62,   -1,    1
};
static short mschact[] = {

	 -25,  -28,  -26,  -27,  272,  269,  268,  262,  -24,  257, 
	 -46,  259,   -3,  257,  -49,  258,  -23,  271,  -22,  270, 
	 -21,  259,  -48,  260,  -20,  273,  -18,  -16,  -17,  -19, 
	 280,  279,  278,  277,  -45,  -44,  259,  257,  -39,  260, 
	 -40,  260,  -41,  260,  -42,  260,   -6,  275,   -8,  257, 
	 -10,  276,  -38,  259,  -11,  257,  -36,  -83,  -35,  -84, 
	 -31,  -14,  -33,  -32,  288,  287,  286,  285,  284,  283, 
	 282,  281,  -34,  259,   -1
};
static short mschpact[] = {

	   4,   16,   19,   30,   30,   49,   51,   53,   49,   55, 
	  64,   55,   64,   73,   47,   45,   43,   41,   39,   36, 
	  25,   23,   21,   17,   15,   13,   11,    9,    4
};
static short mschgo[] = {

	  -2,  -51,  -50,    0,   -1,  -52,  -53,  -54,  -55,  -47, 
	 -56,  -60,  -68,  -72,  -74,  -78,  -57,   10,    9,    7, 
	   5,    3,   -4,  -58,  -59,  -15,   -7,  -43,   -5,  -61, 
	 -62,    4,  -63,  -64,  -65,  -66,  -67,   -9,  -69,  -70, 
	   8,  -37,  -71,  -73,  -12,  -75,  -76,   11,  -30,  -79, 
	 -77,   12,  -13,  -80,  -81,  -82,  -85,  -86,  -87,   -1
};
static short mschpgo[] = {

	   0,    0,    0,   27,   46,   55,   57,   58,   54,   56, 
	  53,   39,   42,   35,   34,   33,   32,    6,   24,   23, 
	   8,    7,   10,    5,    0,    2,    4,    4,    4,    4, 
	   9,    9,   22,   22,   25,   28,   28,   30,   30,   30, 
	  30,   26,   36,   37,   37,   41,   41,   27,   43,   44, 
	  44,   48,   48,   50,   52,   52,   52,   52,   52,   52, 
	  52,   52,   16,   50,   43,   36,   25,    2,    0
};
static short mschrlen[] = {

	   0,    0,    0,    0,    2,    1,    1,    1,    2,    1, 
	   1,    2,    1,    2,    2,    2,    2,    8,    2,    2, 
	   2,    3,    2,    2,    1,    2,    1,    1,    1,    1, 
	   1,    1,    1,    1,    1,    2,    1,    1,    1,    1, 
	   1,    2,    1,    2,    1,    1,    1,    2,    1,    2, 
	   1,    1,    1,    2,    1,    1,    1,    1,    1,    1, 
	   1,    1,    0,    1,    1,    1,    1,    1,    2
};
#define MSCHS0	28
#define MSCHDELTA	25
#define MSCHNPACT	29
#define MSCHNDEF	13

#define MSCHr66	0
#define MSCHr67	1
#define MSCHr68	2
#define MSCHr65	3
#define MSCHr63	4
#define MSCHr62	5
#define MSCHr61	6
#define MSCHr60	7
#define MSCHr59	8
#define MSCHr58	9
#define MSCHr57	10
#define MSCHr39	11
#define MSCHr38	12
#define MSCHr30	13
#define MSCHr29	14
#define MSCHr28	15
#define MSCHr27	16
#define MSCHr18	17
#define MSCHr17	18
#define MSCHr16	19
#define MSCHr13	20
#define MSCHr12	21
#define MSCHr11	22
#define MSCHr8	23
#define MSCHrACCEPT	MSCHr66
#define MSCHrERROR	MSCHr67
#define MSCHrLR2	MSCHr68
#if MSCHDEBUG
char * mschsvar[] = {
	"$accept",
	"file",
	"statements",
	"statement",
	"include",
	"schema",
	"tag",
	"actors_declare",
	"opt_tag_spec",
	"tag_spec",
	"null",
	"archetype",
	"arch_string",
	"arch_int",
	"optparams",
	"taglist",
	"motlist",
	"paraminsts",
	"paraminst",
	"timewarp",
	"duration",
	"stretch",
	"distance",
	"opttaginsts",
	"taginsts",
	"taginst",
	"opt_tag_val",
	"tag_val",
	"optmotions",
	"motinsts",
	"motinst",
	"optmotparamlist",
	"motparamlist",
	"motparam",
	"neck_fixed",
	"blend_length",
	"is_turn",
	"blend_none",
	"is_loco",
	"in_place",
	0
};
short mschrmap[] = {

	  66,   67,   68,   65,   63,   62,   61,   60,   59,   58, 
	  57,   39,   38,   30,   29,   28,   27,   18,   17,   16, 
	  13,   12,   11,    8,    1,    3,    4,    5,    6,    7, 
	   9,   10,   14,   15,   20,   21,   22,   23,   24,   25, 
	  26,   31,   33,   34,   35,   36,   37,   40,   42,   43, 
	  44,   45,   46,   47,   49,   50,   51,   52,   53,   54, 
	  55,   56,   64,   48,   41,   32,   19,    2,    0
};
short mschsmap[] = {

	   9,   11,   14,   24,   38,   47,   48,   49,   52,   54, 
	  61,   64,   81,   70,   39,   31,   30,   29,   28,   25, 
	  22,   19,   17,   12,    4,    3,    2,    1,    0,   66, 
	  67,   68,   69,   85,   71,   72,   56,   57,   42,   43, 
	  44,   45,   55,   40,   41,   13,   18,   23,   15,   10, 
	  16,    8,    7,    6,    5,   21,   20,   27,   26,   37, 
	  46,   36,   35,   34,   33,   32,   53,   51,   60,   50, 
	  59,   58,   65,   63,   84,   62,   83,   82,   86,   80, 
	  79,   78,   77,   76,   75,   74,   73
};
int mschntoken = 34;
int mschnvar = 40;
int mschnstate = 87;
int mschnrule = 69;
#endif

#if MSCHDEBUG
/*
 * Package up YACC context for tracing
 */
typedef struct mschTraceItems_tag {
	int	state, lookahead, errflag, done;
	int	rule, npop;
	short	* states;
	int	nstates;
	MSCHSTYPE * values;
	int	nvalues;
	short	* types;
} mschTraceItems;
#endif


/*
 * Copyright 1985, 1990 by Mortice Kern Systems Inc.  All rights reserved.
 * 
 * Automaton to interpret LALR(1) tables.
 *
 * Macros:
 *	mschclearin - clear the lookahead token.
 *	mscherrok - forgive a pending error
 *	MSCHERROR - simulate an error
 *	MSCHACCEPT - halt and return 0
 *	MSCHABORT - halt and return 1
 *	MSCHRETURN(value) - halt and return value.  You should use this
 *		instead of return(value).
 *	MSCHREAD - ensure mschchar contains a lookahead token by reading
 *		one if it does not.  See also MSCHSYNC.
 *	MSCHRECOVERING - 1 if syntax error detected and not recovered
 *		yet; otherwise, 0.
 *
 * Preprocessor flags:
 *	MSCHDEBUG - includes debug code if 1.  The parser will print
 *		 a travelogue of the parse if this is defined as 1
 *		 and mschdebug is non-zero.
 *		yacc -t sets MSCHDEBUG to 1, but not mschdebug.
 *	MSCHTRACE - turn on MSCHDEBUG, and undefine default trace functions
 *		so that the interactive functions in 'ytrack.c' will
 *		be used.
 *	MSCHSSIZE - size of state and value stacks (default 150).
 *	MSCHSTATIC - By default, the state stack is an automatic array.
 *		If this is defined, the stack will be static.
 *		In either case, the value stack is static.
 *	MSCHALLOC - Dynamically allocate both the state and value stacks
 *		by calling malloc() and free().
 *	MSCHSYNC - if defined, yacc guarantees to fetch a lookahead token
 *		before any action, even if it doesnt need it for a decision.
 *		If MSCHSYNC is defined, MSCHREAD will never be necessary unless
 *		the user explicitly sets mschchar = -1
 *
 * Copyright (c) 1983, by the University of Waterloo
 */
/*
 * Prototypes
 */

extern int mschlex MSCH_ARGS((void));

#if MSCHDEBUG

#include <stdlib.h>		/* common prototypes */
#include <string.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

extern char *	mschValue MSCH_ARGS((MSCHSTYPE, int));	/* print mschlval */
extern void mschShowState MSCH_ARGS((mschTraceItems *));
extern void mschShowReduce MSCH_ARGS((mschTraceItems *));
extern void mschShowGoto MSCH_ARGS((mschTraceItems *));
extern void mschShowShift MSCH_ARGS((mschTraceItems *));
extern void mschShowErrRecovery MSCH_ARGS((mschTraceItems *));
extern void mschShowErrDiscard MSCH_ARGS((mschTraceItems *));

extern void mschShowRead MSCH_ARGS((int));
#endif

/*
 * If MSCHDEBUG defined and mschdebug set,
 * tracing functions will be called at appropriate times in mschparse()
 * Pass state of YACC parse, as filled into mschTraceItems mschx
 * If mschx.done is set by the tracing function, mschparse() will terminate
 * with a return value of -1
 */
#define MSCH_TRACE(fn) { \
	mschx.state = mschstate; mschx.lookahead = mschchar; mschx.errflag =mscherrflag; \
	mschx.states = mschs+1; mschx.nstates = mschps-mschs; \
	mschx.values = mschv+1; mschx.nvalues = mschpv-mschv; \
	mschx.types = mschtypev+1; mschx.done = 0; \
	mschx.rule = mschi; mschx.npop = mschj; \
	fn(&mschx); \
	if (mschx.done) MSCHRETURN(-1); }

#ifndef I18N
#define	gettext(x)	x
#endif

#ifndef MSCHSSIZE
# define MSCHSSIZE	150
#endif

#define MSCHERROR		goto mscherrlabel
#define mscherrok		mscherrflag = 0
#if MSCHDEBUG
#define mschclearin	{ if (mschdebug) mschShowRead(-1); mschchar = -1; }
#else
#define mschclearin	mschchar = -1
#endif
#define MSCHACCEPT	MSCHRETURN(0)
#define MSCHABORT		MSCHRETURN(1)
#define MSCHRECOVERING()	(mscherrflag != 0)
#ifdef MSCHALLOC
# define MSCHRETURN(val)	{ retval = (val); goto mschReturn; }
#else
# define MSCHRETURN(val)	return(val)
#endif
#if MSCHDEBUG
/* The if..else makes this macro behave exactly like a statement */
# define MSCHREAD	if (mschchar < 0) {					\
			if ((mschchar = mschlex()) < 0)			\
				mschchar = 0;				\
			if (mschdebug)					\
				mschShowRead(mschchar);			\
		} else
#else
# define MSCHREAD	if (mschchar < 0) {					\
			if ((mschchar = mschlex()) < 0)			\
				mschchar = 0;				\
		} else
#endif

#define MSCHERRCODE	256		/* value of `error' */
#define	MSCHQMSCHP	mschq[mschq-mschp]

MSCHSTYPE	mschval;				/* $ */
MSCHSTYPE	*mschpvt;				/* $n */
MSCHSTYPE	mschlval;				/* mschlex() sets this */

int	mschchar,				/* current token */
	mscherrflag,			/* error flag */
	mschnerrs;			/* error count */

#if MSCHDEBUG
int mschdebug = 0;		/* debug if this flag is set */
extern char	*mschsvar[];	/* table of non-terminals (aka 'variables') */
extern mschNamedType mschTokenTypes[];	/* table of terminals & their types */
extern short	mschrmap[], mschsmap[];	/* map internal rule/states */
extern int	mschnstate, mschnvar, mschntoken, mschnrule;

extern int	mschGetType MSCH_ARGS((int));	/* token type */
extern char	*mschptok MSCH_ARGS((int));	/* printable token string */
extern int	mschExpandName MSCH_ARGS((int, int, char *, int));
				  /* expand mschRules[] or mschStates[] */
static char *	mschgetState MSCH_ARGS((int));

#define mschassert(condition, msg, arg) \
	if (!(condition)) { \
		printf(gettext("\nyacc bug: ")); \
		printf(msg, arg); \
		MSCHABORT; }
#else /* !MSCHDEBUG */
#define mschassert(condition, msg, arg)
#endif

void MotSchemaYaccParse(char *schemaFile)
{
   // init globals
        
   IncTabsInit();

   mprintf("parsing file %s!\n",schemaFile);
   MschParseFile(schemaFile);

   IncTabsShutdown();

}



mschparse()
{
	register short		mschi, *mschp;	/* for table lookup */
	register short		*mschps;		/* top of state stack */
	register short		mschstate;	/* current state */
	register MSCHSTYPE	*mschpv;		/* top of value stack */
	register short		*mschq;
	register int		mschj;
#if MSCHDEBUG
	mschTraceItems	mschx;			/* trace block */
	short	* mschtp;
	int	mschruletype = 0;
#endif
#ifdef MSCHSTATIC
	static short	mschs[MSCHSSIZE + 1];
	static MSCHSTYPE	mschv[MSCHSSIZE + 1];
#if MSCHDEBUG
	static short	mschtypev[MSCHSSIZE+1];	/* type assignments */
#endif
#else /* ! MSCHSTATIC */
#ifdef MSCHALLOC
	MSCHSTYPE *mschv;
	short	*mschs;
#if MSCHDEBUG
	short	*mschtypev;
#endif
	MSCHSTYPE save_mschlval;
	MSCHSTYPE save_mschval;
	MSCHSTYPE *save_mschpvt;
	int save_mschchar, save_mscherrflag, save_mschnerrs;
	int retval;
#else
	short		mschs[MSCHSSIZE + 1];
	static MSCHSTYPE	mschv[MSCHSSIZE + 1];	/* historically static */
#if MSCHDEBUG
	short	mschtypev[MSCHSSIZE+1];		/* mirror type table */
#endif
#endif /* ! MSCHALLOC */
#endif /* ! MSCHSTATIC */


#ifdef MSCHALLOC
	mschs = (short *) malloc((MSCHSSIZE + 1) * sizeof(short));
	mschv = (MSCHSTYPE *) malloc((MSCHSSIZE + 1) * sizeof(MSCHSTYPE));
#if MSCHDEBUG
	mschtypev = (short *) malloc((MSCHSSIZE+1) * sizeof(short));
#endif
	if (mschs == (short *)0 || mschv == (MSCHSTYPE *)0
#if MSCHDEBUG
		|| mschtypev == (short *) 0
#endif
	) {
		mscherror("Not enough space for parser stacks");
		return 1;
	}
	save_mschlval = mschlval;
	save_mschval = mschval;
	save_mschpvt = mschpvt;
	save_mschchar = mschchar;
	save_mscherrflag = mscherrflag;
	save_mschnerrs = mschnerrs;
#endif

	mschnerrs = 0;
	mscherrflag = 0;
	mschclearin;
	mschps = mschs;
	mschpv = mschv;
	*mschps = mschstate = MSCHS0;		/* start state */
#if MSCHDEBUG
	mschtp = mschtypev;
	mschi = mschj = 0;			/* silence compiler warnings */
#endif

mschStack:
	mschassert((unsigned)mschstate < mschnstate, gettext("state %d\n"), mschstate);
	if (++mschps > &mschs[MSCHSSIZE]) {
		mscherror("Parser stack overflow");
		MSCHABORT;
	}
	*mschps = mschstate;	/* stack current state */
	*++mschpv = mschval;	/* ... and value */
#if MSCHDEBUG
	*++mschtp = mschruletype;	/* ... and type */

	if (mschdebug)
		MSCH_TRACE(mschShowState)
#endif

	/*
	 *	Look up next action in action table.
	 */
mschEncore:
#ifdef MSCHSYNC
	MSCHREAD;
#endif
	if (mschstate >= sizeof mschpact/sizeof mschpact[0]) 	/* simple state */
		mschi = mschstate - MSCHDELTA;	/* reduce in any case */
	else {
		if(*(mschp = &mschact[mschpact[mschstate]]) >= 0) {
			/* Look for a shift on mschchar */
#ifndef MSCHSYNC
			MSCHREAD;
#endif
			mschq = mschp;
			mschi = mschchar;
			while (mschi < *mschp++)
				;
			if (mschi == mschp[-1]) {
				mschstate = ~MSCHQMSCHP;
#if MSCHDEBUG
				if (mschdebug) {
					mschruletype = mschGetType(mschchar);
					MSCH_TRACE(mschShowShift)
				}
#endif
				mschval = mschlval;	/* stack what mschlex() set */
				mschclearin;		/* clear token */
				if (mscherrflag)
					mscherrflag--;	/* successful shift */
				goto mschStack;
			}
		}

		/*
	 	 *	Fell through - take default action
	 	 */

		if (mschstate >= sizeof mschdef /sizeof mschdef[0])
      {
//         mscherror("mshyacc.c:  (%s) Unknown state transition %d out of possible %d\n", mschpv->strval, mschstate, sizeof mschdef /sizeof mschdef[0]);
         mscherror("mshyacc.c:  Unknown state transition %d out of possible %d\n", mschstate, sizeof mschdef /sizeof mschdef[0]);
         goto mschError;
      }
			
		if ((mschi = mschdef[mschstate]) < 0)	 { /* default == reduce? */
			/* Search exception table */
			mschassert((unsigned)~mschi < sizeof mschex/sizeof mschex[0],
				gettext("exception %d\n"), mschstate);
			mschp = &mschex[~mschi];
#ifndef MSCHSYNC
			MSCHREAD;
#endif
			while((mschi = *mschp) >= 0 && mschi != mschchar)
				mschp += 2;
			mschi = mschp[1];
			mschassert(mschi >= 0,
				 gettext("Ex table not reduce %d\n"), mschi);
		}
	}

	mschassert((unsigned)mschi < mschnrule, gettext("reduce %d\n"), mschi);
	mschj = mschrlen[mschi];
#if MSCHDEBUG
	if (mschdebug)
		MSCH_TRACE(mschShowReduce)
	mschtp -= mschj;
#endif
	mschps -= mschj;		/* pop stacks */
	mschpvt = mschpv;		/* save top */
	mschpv -= mschj;
	mschval = mschpv[1];	/* default action $ = $1 */
#if MSCHDEBUG
	mschruletype = mschRules[mschrmap[mschi]].type;
#endif

	switch (mschi) {		/* perform semantic action */
		
case MSCHr8: {	/* include :  INCLUDE STRING */

   ConfigSpew("yakspew",("include %s\n",mschpvt[0].strval));       

   IncParseFile(mschpvt[0].strval);
   FreeString(mschpvt[0].strval);

} break;

case MSCHr11: {	/* tag_spec :  TAG_OPT FLOAT */

   g_TagIsMandatory=FALSE;
   g_TagWeight=mschpvt[0].fval;

} break;

case MSCHr12: {	/* tag :  TAG IDENT opt_tag_spec */

   sTagInfo info;

   if(config_is_defined("yakspew"))
   {
      mprintf("tag %s mand %d",mschpvt[-1].strval,g_TagIsMandatory);
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
   MotDBRegisterTag((Label *)(mschpvt[-1].strval),&info); 
   // reset globals
   g_TagIsMandatory=TRUE;
   g_TagWeight=0;

   FreeString(mschpvt[-1].strval);

} break;

case MSCHr13: {	/* actors_declare :  ACTORS_DECLARE INT */

   MotDBCreate(mschpvt[0].ival);

} break;

case MSCHr16: {	/* arch_string :  ARCHETYPE IDENT */

   g_ArchIsString=TRUE;
   strncpy(g_ArchName.text,mschpvt[0].strval,sizeof(g_ArchName));
   FreeString(mschpvt[0].strval);

} break;

case MSCHr17: {	/* arch_int :  ARCHETYPE INT */

   g_ArchIsString=FALSE;
   g_ArchInt=mschpvt[0].ival;

} break;

case MSCHr18: {	/* schema :  SCHEMA IDENT ACTOR INT archetype optparams taglist motlist */

   int i;
   sMotDesc *pMotDesc;
   sTagDesc *pTagDesc;
   sMotSchemaDesc schema;

   if(config_is_defined("yakspew"))     
   {
      if(g_ArchIsString)
         mprintf("schema %s\n  with actor %d\n  with archetype %s\n",mschpvt[-6].strval,mschpvt[-4].ival,g_ArchName.text);       
      else
         mprintf("schema %s\n  with actor %d\n  with archetype %d\n",mschpvt[-6].strval,mschpvt[-4].ival,g_ArchInt);       

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
   schema.actor=mschpvt[-4].ival;
   strncpy(schema.name.text,mschpvt[-6].strval,15);

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

   FreeString(mschpvt[-6].strval);

} break;

case MSCHr27: {	/* timewarp :  TIMEWARP FLOAT */

   g_TimeWarp=mschpvt[0].fval;

} break;

case MSCHr28: {	/* duration :  DURATION FLOAT */

   g_Duration=mschpvt[0].fval;

} break;

case MSCHr29: {	/* stretch :  STRETCH FLOAT */

   g_Stretch=mschpvt[0].fval;

} break;

case MSCHr30: {	/* distance :  DISTANCE FLOAT */

   g_Distance=mschpvt[0].fval;

} break;

case MSCHr38: {	/* tag_val :  INT */

   g_TagVal=mschpvt[0].ival;

} break;

case MSCHr39: {	/* taginst :  IDENT opt_tag_val */

   int size;

   if(g_nSchemaTags<kMaxTagsPerSchema)
   {
      size=sizeof(g_aSchemaTags[0].type);     
      strncpy(g_aSchemaTags[g_nSchemaTags].type.text,mschpvt[-1].strval,size);
      g_aSchemaTags[g_nSchemaTags].value=g_TagVal;
      g_nSchemaTags++;  
   } else
   {
      Warning(("Cannot add tag %s to schema - too many tags\n",mschpvt[-1].strval));
   }
   // reset globals 
   g_TagVal=0;
   FreeString(mschpvt[-1].strval);

} break;

case MSCHr57: {	/* neck_fixed :  MOT_NECK_FIXED */

   g_NeckIsFixed=TRUE;

} break;

case MSCHr58: {	/* blend_none :  MOT_BLEND_NONE */

   g_BlendLength=0;             

} break;

case MSCHr59: {	/* blend_length :  MOT_BLEND_LENGTH INT */

   g_BlendLength=mschpvt[0].ival;             

} break;

case MSCHr60: {	/* in_place :  MOT_IN_PLACE */

   g_InPlace=TRUE;        

} break;

case MSCHr61: {	/* is_loco :  MOT_IS_LOCO */

   g_IsLoco=TRUE;        

} break;

case MSCHr62: {	/* is_turn :  MOT_IS_TURN */

   g_IsTurn=TRUE;

} break;

case MSCHr63: {	/* motinst :  IDENT optmotparamlist */

   int size;
     
   if(g_nSchemaMotions<kMaxMotionsPerSchema)
   {
      size=sizeof(g_aSchemaMotions[0].name);     
      strncpy(g_aSchemaMotions[g_nSchemaMotions].name.text,mschpvt[-1].strval,size);
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
      Warning(("Cannot add motion %s to schema - too many motions\n",mschpvt[-1].strval));
   }

   // reset motion params
   g_NeckIsFixed=FALSE;
   g_BlendLength=kDefaultBlendLength;
   g_IsTurn=FALSE;
   g_IsLoco=FALSE;
   g_InPlace=FALSE;

   FreeString(mschpvt[-1].strval);

} break;

case MSCHr65: {	/* motlist :  */


} break;
	case MSCHrACCEPT:
		MSCHACCEPT;
	case MSCHrERROR:
		goto mschError;
	}

	/*
	 *	Look up next state in goto table.
	 */

	mschp = &mschgo[mschpgo[mschi]];
	mschq = mschp++;
	mschi = *mschps;
	while (mschi < *mschp++)
		;

	mschstate = ~(mschi == *--mschp? MSCHQMSCHP: *mschq);
#if MSCHDEBUG
	if (mschdebug)
		MSCH_TRACE(mschShowGoto)
#endif
	goto mschStack;

mscherrlabel:	;		/* come here from MSCHERROR	*/
/*
#pragma used mscherrlabel
 */
	mscherrflag = 1;
	if (mschi == MSCHrERROR) {
		mschps--;
		mschpv--;
#if MSCHDEBUG
		mschtp--;
#endif
	}

mschError:
	switch (mscherrflag) {

	case 0:		/* new error */
		mschnerrs++;
		mschi = mschchar;
// mschpvt		
// pMotDesc->name
      mscherror("mshyacc.c:  Syntax error on char '%c'\n", mschchar);
		if (mschi != mschchar) {
			/* user has changed the current token */
			/* try again */
			mscherrflag++;	/* avoid loops */
			goto mschEncore;
		}

	case 1:		/* partially recovered */
	case 2:
		mscherrflag = 3;	/* need 3 valid shifts to recover */
			
		/*
		 *	Pop states, looking for a
		 *	shift on `error'.
		 */

		for ( ; mschps > mschs; mschps--, mschpv--
#if MSCHDEBUG
					, mschtp--
#endif
		) {
			if (*mschps >= sizeof mschpact/sizeof mschpact[0])
				continue;
			mschp = &mschact[mschpact[*mschps]];
			mschq = mschp;
			do
				;
			while (MSCHERRCODE < *mschp++);

			if (MSCHERRCODE == mschp[-1]) {
				mschstate = ~MSCHQMSCHP;
				goto mschStack;
			}
				
			/* no shift in this state */
#if MSCHDEBUG
			if (mschdebug && mschps > mschs+1)
				MSCH_TRACE(mschShowErrRecovery)
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

		if (mschchar == 0)  /* but not EOF */
			break;
#if MSCHDEBUG
		if (mschdebug)
			MSCH_TRACE(mschShowErrDiscard)
#endif
		mschclearin;
		goto mschEncore;	/* try again in same state */
	}
	MSCHABORT;

#ifdef MSCHALLOC
mschReturn:
	mschlval = save_mschlval;
	mschval = save_mschval;
	mschpvt = save_mschpvt;
	mschchar = save_mschchar;
	mscherrflag = save_mscherrflag;
	mschnerrs = save_mschnerrs;
	free((char *)mschs);
	free((char *)mschv);
	return(retval);
#endif
}

		
#if MSCHDEBUG
/*
 * Return type of token
 */
int
mschGetType(tok)
int tok;
{
	mschNamedType * tp;
	for (tp = &mschTokenTypes[mschntoken-1]; tp > mschTokenTypes; tp--)
		if (tp->token == tok)
			return tp->type;
	return 0;
}
/*
 * Print a token legibly.
 */
char *
mschptok(tok)
int tok;
{
	mschNamedType * tp;
	for (tp = &mschTokenTypes[mschntoken-1]; tp > mschTokenTypes; tp--)
		if (tp->token == tok)
			return tp->name;
	return "";
}

/*
 * Read state 'num' from mschStatesFile
 */
#ifdef MSCHTRACE
// Bodisafa:  Who knows what a good MSCHMAX_READ size is.
#define MSCHMAX_READ 256
static FILE *mschStatesFile = (FILE *) 0;
static char mschReadBuf[MSCHMAX_READ+1];

static char *
mschgetState(num)
int num;
{
	int	size;

	if (mschStatesFile == (FILE *) 0
	 && (mschStatesFile = fopen(mschStatesFile, "r")) == (FILE *) 0)
		return "mschExpandName: cannot open states file";

	if (num < mschnstate - 1)
		size = (int)(mschStates[num+1] - mschStates[num]);
	else {
		/* length of last item is length of file - ptr(last-1) */
		if (fseek(mschStatesFile, 0L, 2) < 0)
			goto cannot_seek;
		size = (int) (ftell(mschStatesFile) - mschStates[num]);
	}
	if (size < 0 || size > MSCHMAX_READ)
		return "mschExpandName: bad read size";
	if (fseek(mschStatesFile, mschStates[num], 0) < 0) {
	cannot_seek:
		return "mschExpandName: cannot seek in states file";
	}

	(void) fread(mschReadBuf, 1, size, mschStatesFile);
	mschReadBuf[size] = '\0';
	return mschReadBuf;
}
#endif /* MSCHTRACE */
/*
 * Expand encoded string into printable representation
 * Used to decode mschStates and mschRules strings.
 * If the expansion of 's' fits in 'buf', return 1; otherwise, 0.
 */
int
mschExpandName(num, isrule, buf, len)
int num, isrule;
char * buf;
int len;
{
	int	i, n, cnt, type;
	char	* endp, * cp;
	char	*s;

	if (isrule)
		s = mschRules[num].name;
	else
#ifdef MSCHTRACE
		s = mschgetState(num);
#else
		s = "*no states*";
#endif

	for (endp = buf + len - 8; *s; s++) {
		if (buf >= endp) {		/* too large: return 0 */
		full:	(void) strcpy(buf, " ...\n");
			return 0;
		} else if (*s == '%') {		/* nonterminal */
			type = 0;
			cnt = mschnvar;
			goto getN;
		} else if (*s == '&') {		/* terminal */
			type = 1;
			cnt = mschntoken;
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
				if (n >= mschnvar)
					goto too_big;
				cp = mschsvar[n];
			} else if (n >= mschntoken) {
			    too_big:
				cp = "<range err>";
			} else
				cp = mschTokenTypes[n].name;

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
#ifndef MSCHTRACE
/*
 * Show current state of mschparse
 */
void
mschShowState(tp)
mschTraceItems * tp;
{
	short * p;
	MSCHSTYPE * q;

	printf(
	    gettext("state %d (%d), char %s (%d)\n"),
	      mschsmap[tp->state], tp->state,
	      mschptok(tp->lookahead), tp->lookahead);
}
/*
 * show results of reduction
 */
void
mschShowReduce(tp)
mschTraceItems * tp;
{
	printf("reduce %d (%d), pops %d (%d)\n",
		mschrmap[tp->rule], tp->rule,
		tp->states[tp->nstates - tp->npop],
		mschsmap[tp->states[tp->nstates - tp->npop]]);
}
void
mschShowRead(val)
int val;
{
	printf(gettext("read %s (%d)\n"), mschptok(val), val);
}
void
mschShowGoto(tp)
mschTraceItems * tp;
{
	printf(gettext("goto %d (%d)\n"), mschsmap[tp->state], tp->state);
}
void
mschShowShift(tp)
mschTraceItems * tp;
{
	printf(gettext("shift %d (%d)\n"), mschsmap[tp->state], tp->state);
}
void
mschShowErrRecovery(tp)
mschTraceItems * tp;
{
	short	* top = tp->states + tp->nstates - 1;

	printf(
	gettext("Error recovery pops state %d (%d), uncovers %d (%d)\n"),
		mschsmap[*top], *top, mschsmap[*(top-1)], *(top-1));
}
void
mschShowErrDiscard(tp)
mschTraceItems * tp;
{
	printf(gettext("Error recovery discards %s (%d), "),
		mschptok(tp->lookahead), tp->lookahead);
}
#endif	/* ! MSCHTRACE */
#endif	/* MSCHDEBUG */
