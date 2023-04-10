/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

/* n:\bin\yacc -p songyy -l -D songtok.h -o songyacc.c r:\prj\cam\src\songlang.y */
#define SONG	257
#define DECLARESECTIONS	258
#define SECTION	259
#define SAMPLE	260
#define SILENCE	261
#define SECONDS	262
#define SAMPLES	263
#define LOOPCOUNT	264
#define IMMEDIATE	265
#define PROBABILITY	266
#define ONEVENT	267
#define GOTOSECTION	268
#define GOTOEND	269
#define VOLUME	270
#define PAN	271
#define INT_NUM	272
#define PERCENT_NUM	273
#define FLOAT_NUM	274
#define QSTRING	275
#define ID	276
#ifdef SONGYYTRACE
#define SONGYYDEBUG 1
#else
#ifndef SONGYYDEBUG
#define SONGYYDEBUG 0
#endif
#endif
/*
 * Portable way of defining ANSI C prototypes
 */
#ifndef SONGYY_ARGS
#if __STDC__
#define SONGYY_ARGS(x)	x
#else
#define SONGYY_ARGS(x)	()
#endif
#endif

#if SONGYYDEBUG
typedef struct songyyNamedType_tag {	/* Tokens */
	char	* name;		/* printable name */
	short	token;		/* token # */
	short	type;		/* token type */
} songyyNamedType;
typedef struct songyyTypedRules_tag {	/* Typed rule table */
	char	* name;		/* compressed rule string */
	short	type;		/* rule result type */
} songyyTypedRules;

#endif

typedef union {
   char *strval;
   int  intval;
   float floatval;
} SONGYYSTYPE;


#include <mprintf.h>
#include <song.h>

static ISong* sgpCurrSong;
static ISongSection* sgpCurrSection; 
static ISongSample* sgpCurrSample;
static ISongEvent* sgpCurrEvent;
static ISongGoto* sgpCurrGoto;

static BOOL sgInSections;

static unsigned _FindSectionIndexFromID (ISong* pSong, char* id)
{
   ISongSection* pSection;
   sSongSectionInfo sectionInfo;
   int index;
   unsigned i;
   unsigned count;

   /* Do linear search to match ID. */
   index = -1;
   count = ISong_CountSections (sgpCurrSong);
   for (i = 0; i < count; i++)
   {
      ISong_GetSection (pSong, i, &pSection);
      ISongSection_GetSectionInfo (pSection, &sectionInfo);
      ISongSection_Release (pSection);

      if (!strcmp (&(sectionInfo.id), id))
      {
         index = i;
         break;
      }
   }

   return (unsigned) index;
}

static BOOL _StringLengthErrorCheck (char* pStr)
{
   char tmpStr[100];

   if (strlen (pStr) >= kSONG_MaxStringLen)
   {
      sprintf (tmpStr, "String must be less than %d characters long.", kSONG_MaxStringLen);
      songyyerror (tmpStr);
      return TRUE;
   }

   return FALSE;
}

extern int songyychar, songyyerrflag;
extern SONGYYSTYPE songyylval;
#if SONGYYDEBUG
enum SONGYY_Types { SONGYY_t_NoneDefined, SONGYY_t_intval, SONGYY_t_floatval, SONGYY_t_strval
};
#endif
#if SONGYYDEBUG
yyTypedRules songyyRules[] = {
	{ "&00: %04 &00",  0},
	{ "%04: &02 &21 %05 %06 %07",  0},
	{ "%04: &02 &01",  0},
	{ "%04: &01",  0},
	{ "%05: %08",  0},
	{ "%08: &03 %09",  0},
	{ "%08: &03 &01",  0},
	{ "%09: %10",  0},
	{ "%09: %09 %10",  0},
	{ "%10: &21",  0},
	{ "%07: %11",  0},
	{ "%07: %07 %11",  0},
	{ "%11: &04 &21 %12 %13 %06",  0},
	{ "%11: &04 &01",  0},
	{ "%13: %14",  0},
	{ "%13: %15",  0},
	{ "%12:",  0},
	{ "%12: %12 %03",  0},
	{ "%12: %12 %01",  0},
	{ "%03: &15 &17",  1},
	{ "%14: %16",  0},
	{ "%14: %14 %16",  0},
	{ "%16: &05 &20",  0},
	{ "%16: &05 &01",  0},
	{ "%15: &06 &19 &07",  0},
	{ "%01: &09 &17",  1},
	{ "%02: &11 &18",  1},
	{ "%06:",  0},
	{ "%06: %06 %17",  0},
	{ "%17: &12 &20 %18 %19",  0},
	{ "%17: &12 &01",  0},
	{ "%18:",  0},
	{ "%18: %18 &10",  0},
	{ "%19: %20",  0},
	{ "%19: %19 %20",  0},
	{ "%20: &13 &21 %02",  0},
	{ "%20: &14 %02",  0},
	{ "%20: &13 &01",  0},
	{ "%20: &14 &01",  0},
{ "$accept",  0},{ "error",  0}
};
yyNamedType songyyTokenTypes[] = {
	{ "$end",  0,  0},
	{ "error",  256,  0},
	{ "SONG",  257,  0},
	{ "DECLARESECTIONS",  258,  0},
	{ "SECTION",  259,  0},
	{ "SAMPLE",  260,  0},
	{ "SILENCE",  261,  0},
	{ "SECONDS",  262,  0},
	{ "SAMPLES",  263,  0},
	{ "LOOPCOUNT",  264,  0},
	{ "IMMEDIATE",  265,  0},
	{ "PROBABILITY",  266,  0},
	{ "ONEVENT",  267,  0},
	{ "GOTOSECTION",  268,  0},
	{ "GOTOEND",  269,  0},
	{ "VOLUME",  270,  0},
	{ "PAN",  271,  0},
	{ "INT_NUM",  272,  1},
	{ "PERCENT_NUM",  273,  1},
	{ "FLOAT_NUM",  274,  2},
	{ "QSTRING",  275,  3},
	{ "ID",  276,  3}

};
#endif
static short songyydef[] = {

	  -1,   40,    3,    5,   39,   34,    4
};
static short songyyex[] = {

	   0,    0,   -1,    1
};
static short songyyact[] = {

	 -44,  -24,  257,  256,  -45,  -23,  276,  256,  -22,  258, 
	 -43,  -42,  276,  256,  -42,  276,  -19,  -20,  267,  259, 
	 -31,  -56,  275,  256,  -41,  -40,  276,  256,  -19,  259, 
	 -30,  -15,  -16,  269,  268,  265,  -12,  -13,  -14,  -11, 
	 270,  264,  261,  260,  -26,  -10,  266,  256,  -27,   -9, 
	 276,  256,  -15,  -16,  269,  268,  -33,  272,   -8,  274, 
	 -35,  -36,  275,  256,  -37,  272,  -12,  260,  -32,  273, 
	 -10,  266,  -34,  262,  -20,  267,   -1
};
static short songyypact[] = {

	   8,   15,   29,   54,   67,   68,   75,   73,   71,   69, 
	  65,   62,   59,   57,   50,   46,   40,   33,   26,   22, 
	  18,   12,    9,    6,    2
};
static short songyygo[] = {

	 -38,  -29,  -28,    8,  -39,   -1,  -54,   -7,  -21,    5, 
	  -3,  -46,   -2,  -48,  -47,    1,  -50,  -49,    2,  -17, 
	  -6,   -5,  -51,  -53,  -52,    4,  -55,  -18,   -4,  -58, 
	 -57,    3,   -1
};
static short songyypgo[] = {

	   0,    0,    0,    5,   17,   26,   30,   30,   30,   30, 
	  27,   26,    2,    0,   22,   24,   24,    4,   19,   19, 
	  19,   17,   14,   11,    5,    5,    6,   12,   12,   10, 
	  10,   20,   21,   21,    8,    8,   27,   28,   28,   20, 
	  11,    0
};
static short songyyrlen[] = {

	   0,    0,    0,    5,    5,    4,    2,    2,    2,    3, 
	   2,    2,    2,    2,    3,    2,    2,    2,    2,    2, 
	   0,    2,    1,    2,    1,    2,    1,    1,    2,    1, 
	   2,    1,    1,    2,    0,    2,    0,    1,    2,    1, 
	   2,    2
};
#define SONGYYS0	24
#define SONGYYDELTA	19
#define SONGYYNPACT	25
#define SONGYYNDEF	7

#define SONGYYr39	0
#define SONGYYr40	1
#define SONGYYr41	2
#define SONGYYr1	3
#define SONGYYr12	4
#define SONGYYr29	5
#define SONGYYr38	6
#define SONGYYr37	7
#define SONGYYr36	8
#define SONGYYr35	9
#define SONGYYr32	10
#define SONGYYr30	11
#define SONGYYr26	12
#define SONGYYr25	13
#define SONGYYr24	14
#define SONGYYr23	15
#define SONGYYr22	16
#define SONGYYr19	17
#define SONGYYr18	18
#define SONGYYr17	19
#define SONGYYr16	20
#define SONGYYr13	21
#define SONGYYr9	22
#define SONGYYr6	23
#define SONGYYr3	24
#define SONGYYr2	25
#define SONGYYrACCEPT	SONGYYr39
#define SONGYYrERROR	SONGYYr40
#define SONGYYrLR2	SONGYYr41
#if SONGYYDEBUG
char * songyysvar[] = {
	"$accept",
	"loopcount",
	"probability",
	"volume",
	"song",
	"declarations",
	"onevents",
	"sections",
	"declaresections",
	"sectiondeclarationlist",
	"sectiondeclaration",
	"section",
	"sectionoptions",
	"samplesorsilence",
	"samples",
	"silence",
	"sample",
	"onevent",
	"oneventoptions",
	"gotosections",
	"gotosection",
	0
};
short songyyrmap[] = {

	  39,   40,   41,    1,   12,   29,   38,   37,   36,   35, 
	  32,   30,   26,   25,   24,   23,   22,   19,   18,   17, 
	  16,   13,    9,    6,    3,    2,    4,    7,    8,   10, 
	  11,   15,   20,   21,   27,   28,   31,   33,   34,   14, 
	   5,    0
};
short songyysmap[] = {

	   3,   12,   19,   31,   40,   41,   54,   49,   46,   44, 
	  36,   34,   33,   32,   28,   27,   26,   25,   17,   15, 
	  13,    6,    5,    2,    0,   42,   45,   43,   56,   30, 
	  20,   55,   48,   57,   50,   51,   52,   37,   38,   23, 
	  22,    9,   11,    1,    4,    7,   10,   14,   18,   24, 
	  39,   35,   53,    8,   16,   21,   29,   47
};
int songyyntoken = 22;
int songyynvar = 21;
int songyynstate = 58;
int songyynrule = 42;
#endif

#if SONGYYDEBUG
/*
 * Package up YACC context for tracing
 */
typedef struct songyyTraceItems_tag {
	int	state, lookahead, errflag, done;
	int	rule, npop;
	short	* states;
	int	nstates;
	SONGYYSTYPE * values;
	int	nvalues;
	short	* types;
} songyyTraceItems;
#endif


/*
 * Copyright 1985, 1990 by Mortice Kern Systems Inc.  All rights reserved.
 * 
 * Automaton to interpret LALR(1) tables.
 *
 * Macros:
 *	songyyclearin - clear the lookahead token.
 *	songyyerrok - forgive a pending error
 *	SONGYYERROR - simulate an error
 *	SONGYYACCEPT - halt and return 0
 *	SONGYYABORT - halt and return 1
 *	SONGYYRETURN(value) - halt and return value.  You should use this
 *		instead of return(value).
 *	SONGYYREAD - ensure songyychar contains a lookahead token by reading
 *		one if it does not.  See also SONGYYSYNC.
 *	SONGYYRECOVERING - 1 if syntax error detected and not recovered
 *		yet; otherwise, 0.
 *
 * Preprocessor flags:
 *	SONGYYDEBUG - includes debug code if 1.  The parser will print
 *		 a travelogue of the parse if this is defined as 1
 *		 and songyydebug is non-zero.
 *		yacc -t sets SONGYYDEBUG to 1, but not songyydebug.
 *	SONGYYTRACE - turn on SONGYYDEBUG, and undefine default trace functions
 *		so that the interactive functions in 'ytrack.c' will
 *		be used.
 *	SONGYYSSIZE - size of state and value stacks (default 150).
 *	SONGYYSTATIC - By default, the state stack is an automatic array.
 *		If this is defined, the stack will be static.
 *		In either case, the value stack is static.
 *	SONGYYALLOC - Dynamically allocate both the state and value stacks
 *		by calling malloc() and free().
 *	SONGYYSYNC - if defined, yacc guarantees to fetch a lookahead token
 *		before any action, even if it doesnt need it for a decision.
 *		If SONGYYSYNC is defined, SONGYYREAD will never be necessary unless
 *		the user explicitly sets songyychar = -1
 *
 * Copyright (c) 1983, by the University of Waterloo
 */
/*
 * Prototypes
 */

extern int songyylex SONGYY_ARGS((void));

#if SONGYYDEBUG

#include <stdlib.h>		/* common prototypes */
#include <string.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

extern char *	songyyValue SONGYY_ARGS((SONGYYSTYPE, int));	/* print songyylval */
extern void songyyShowState SONGYY_ARGS((songyyTraceItems *));
extern void songyyShowReduce SONGYY_ARGS((songyyTraceItems *));
extern void songyyShowGoto SONGYY_ARGS((songyyTraceItems *));
extern void songyyShowShift SONGYY_ARGS((songyyTraceItems *));
extern void songyyShowErrRecovery SONGYY_ARGS((songyyTraceItems *));
extern void songyyShowErrDiscard SONGYY_ARGS((songyyTraceItems *));

extern void songyyShowRead SONGYY_ARGS((int));
#endif

/*
 * If SONGYYDEBUG defined and songyydebug set,
 * tracing functions will be called at appropriate times in songyyparse()
 * Pass state of YACC parse, as filled into songyyTraceItems songyyx
 * If songyyx.done is set by the tracing function, songyyparse() will terminate
 * with a return value of -1
 */
#define SONGYY_TRACE(fn) { \
	songyyx.state = songyystate; songyyx.lookahead = songyychar; songyyx.errflag =songyyerrflag; \
	songyyx.states = songyys+1; songyyx.nstates = songyyps-songyys; \
	songyyx.values = songyyv+1; songyyx.nvalues = songyypv-songyyv; \
	songyyx.types = songyytypev+1; songyyx.done = 0; \
	songyyx.rule = songyyi; songyyx.npop = songyyj; \
	fn(&songyyx); \
	if (songyyx.done) SONGYYRETURN(-1); }

#ifndef I18N
#define	gettext(x)	x
#endif

#ifndef SONGYYSSIZE
# define SONGYYSSIZE	150
#endif

#define SONGYYERROR		goto songyyerrlabel
#define songyyerrok		songyyerrflag = 0
#if SONGYYDEBUG
#define songyyclearin	{ if (songyydebug) songyyShowRead(-1); songyychar = -1; }
#else
#define songyyclearin	songyychar = -1
#endif
#define SONGYYACCEPT	SONGYYRETURN(0)
#define SONGYYABORT		SONGYYRETURN(1)
#define SONGYYRECOVERING()	(songyyerrflag != 0)
#ifdef SONGYYALLOC
# define SONGYYRETURN(val)	{ retval = (val); goto songyyReturn; }
#else
# define SONGYYRETURN(val)	return(val)
#endif
#if SONGYYDEBUG
/* The if..else makes this macro behave exactly like a statement */
# define SONGYYREAD	if (songyychar < 0) {					\
			if ((songyychar = songyylex()) < 0)			\
				songyychar = 0;				\
			if (songyydebug)					\
				songyyShowRead(songyychar);			\
		} else
#else
# define SONGYYREAD	if (songyychar < 0) {					\
			if ((songyychar = songyylex()) < 0)			\
				songyychar = 0;				\
		} else
#endif

#define SONGYYERRCODE	256		/* value of `error' */
#define	SONGYYQSONGYYP	songyyq[songyyq-songyyp]

SONGYYSTYPE	songyyval;				/* $ */
SONGYYSTYPE	*songyypvt;				/* $n */
SONGYYSTYPE	songyylval;				/* songyylex() sets this */

int	songyychar,				/* current token */
	songyyerrflag,			/* error flag */
	songyynerrs;			/* error count */

#if SONGYYDEBUG
int songyydebug = 0;		/* debug if this flag is set */
extern char	*songyysvar[];	/* table of non-terminals (aka 'variables') */
extern songyyNamedType songyyTokenTypes[];	/* table of terminals & their types */
extern short	songyyrmap[], songyysmap[];	/* map internal rule/states */
extern int	songyynstate, songyynvar, songyyntoken, songyynrule;

extern int	songyyGetType SONGYY_ARGS((int));	/* token type */
extern char	*songyyptok SONGYY_ARGS((int));	/* printable token string */
extern int	songyyExpandName SONGYY_ARGS((int, int, char *, int));
				  /* expand songyyRules[] or songyyStates[] */
static char *	songyygetState SONGYY_ARGS((int));

#define songyyassert(condition, msg, arg) \
	if (!(condition)) { \
		printf(gettext("\nyacc bug: ")); \
		printf(msg, arg); \
		SONGYYABORT; }
#else /* !SONGYYDEBUG */
#define songyyassert(condition, msg, arg)
#endif

// From the lexer... 
extern BOOL SongYyParseFile(char *songFile);

// Returns TRUE if successful and sets *ppSong to the song that was parsed.
// Returns FALSE upon failure and sets *ppSong to NULL.
BOOL SongYaccParse (char* songFile, ISong **ppSong)
{
   BOOL parseOK;

   /* Initialize static globals. */
   CreateSong (&sgpCurrSong, NULL);
   CreateSongSection (&sgpCurrSection, NULL);
   CreateSongSample (&sgpCurrSample, NULL);
   CreateSongEvent (&sgpCurrEvent, NULL);
   CreateSongGoto (&sgpCurrGoto, NULL);

   /* Need to know if parsing a section to determine where to add events to:
    * the song or the section.  This is to support default OnEvents.
    */
   sgInSections = FALSE;

   /* Parse the file. */
   parseOK = SongYyParseFile(songFile);

   /* Clean up. */
   SafeRelease (sgpCurrGoto);
   SafeRelease (sgpCurrEvent);
   SafeRelease (sgpCurrSample);
   SafeRelease (sgpCurrSection);

   if (parseOK)
   {
      /* Don't release the song... return it. */
      *ppSong = sgpCurrSong;
   }
   else
   {
      /* We failed, so release the song and give back a NULL song. */
      SafeRelease (sgpCurrSong);
      *ppSong = NULL;
   }

   return parseOK;
}

songyyparse()
{
	register short		songyyi, *songyyp;	/* for table lookup */
	register short		*songyyps;		/* top of state stack */
	register short		songyystate;	/* current state */
	register SONGYYSTYPE	*songyypv;		/* top of value stack */
	register short		*songyyq;
	register int		songyyj;
#if SONGYYDEBUG
	songyyTraceItems	songyyx;			/* trace block */
	short	* songyytp;
	int	songyyruletype = 0;
#endif
#ifdef SONGYYSTATIC
	static short	songyys[SONGYYSSIZE + 1];
	static SONGYYSTYPE	songyyv[SONGYYSSIZE + 1];
#if SONGYYDEBUG
	static short	songyytypev[SONGYYSSIZE+1];	/* type assignments */
#endif
#else /* ! SONGYYSTATIC */
#ifdef SONGYYALLOC
	SONGYYSTYPE *songyyv;
	short	*songyys;
#if SONGYYDEBUG
	short	*songyytypev;
#endif
	SONGYYSTYPE save_songyylval;
	SONGYYSTYPE save_songyyval;
	SONGYYSTYPE *save_songyypvt;
	int save_songyychar, save_songyyerrflag, save_songyynerrs;
	int retval;
#else
	short		songyys[SONGYYSSIZE + 1];
	static SONGYYSTYPE	songyyv[SONGYYSSIZE + 1];	/* historically static */
#if SONGYYDEBUG
	short	songyytypev[SONGYYSSIZE+1];		/* mirror type table */
#endif
#endif /* ! SONGYYALLOC */
#endif /* ! SONGYYSTATIC */


#ifdef SONGYYALLOC
	songyys = (short *) malloc((SONGYYSSIZE + 1) * sizeof(short));
	songyyv = (SONGYYSTYPE *) malloc((SONGYYSSIZE + 1) * sizeof(SONGYYSTYPE));
#if SONGYYDEBUG
	songyytypev = (short *) malloc((SONGYYSSIZE+1) * sizeof(short));
#endif
	if (songyys == (short *)0 || songyyv == (SONGYYSTYPE *)0
#if SONGYYDEBUG
		|| songyytypev == (short *) 0
#endif
	) {
		songyyerror("Not enough space for parser stacks");
		return 1;
	}
	save_songyylval = songyylval;
	save_songyyval = songyyval;
	save_songyypvt = songyypvt;
	save_songyychar = songyychar;
	save_songyyerrflag = songyyerrflag;
	save_songyynerrs = songyynerrs;
#endif

	songyynerrs = 0;
	songyyerrflag = 0;
	songyyclearin;
	songyyps = songyys;
	songyypv = songyyv;
	*songyyps = songyystate = SONGYYS0;		/* start state */
#if SONGYYDEBUG
	songyytp = songyytypev;
	songyyi = songyyj = 0;			/* silence compiler warnings */
#endif

songyyStack:
	songyyassert((unsigned)songyystate < songyynstate, gettext("state %d\n"), songyystate);
	if (++songyyps > &songyys[SONGYYSSIZE]) {
		songyyerror("Parser stack overflow");
		SONGYYABORT;
	}
	*songyyps = songyystate;	/* stack current state */
	*++songyypv = songyyval;	/* ... and value */
#if SONGYYDEBUG
	*++songyytp = songyyruletype;	/* ... and type */

	if (songyydebug)
		SONGYY_TRACE(songyyShowState)
#endif

	/*
	 *	Look up next action in action table.
	 */
songyyEncore:
#ifdef SONGYYSYNC
	SONGYYREAD;
#endif
	if (songyystate >= sizeof songyypact/sizeof songyypact[0]) 	/* simple state */
		songyyi = songyystate - SONGYYDELTA;	/* reduce in any case */
	else {
		if(*(songyyp = &songyyact[songyypact[songyystate]]) >= 0) {
			/* Look for a shift on songyychar */
#ifndef SONGYYSYNC
			SONGYYREAD;
#endif
			songyyq = songyyp;
			songyyi = songyychar;
			while (songyyi < *songyyp++)
				;
			if (songyyi == songyyp[-1]) {
				songyystate = ~SONGYYQSONGYYP;
#if SONGYYDEBUG
				if (songyydebug) {
					songyyruletype = songyyGetType(songyychar);
					SONGYY_TRACE(songyyShowShift)
				}
#endif
				songyyval = songyylval;	/* stack what songyylex() set */
				songyyclearin;		/* clear token */
				if (songyyerrflag)
					songyyerrflag--;	/* successful shift */
				goto songyyStack;
			}
		}

		/*
	 	 *	Fell through - take default action
	 	 */

		if (songyystate >= sizeof songyydef /sizeof songyydef[0])
			goto songyyError;
		if ((songyyi = songyydef[songyystate]) < 0)	 { /* default == reduce? */
			/* Search exception table */
			songyyassert((unsigned)~songyyi < sizeof songyyex/sizeof songyyex[0],
				gettext("exception %d\n"), songyystate);
			songyyp = &songyyex[~songyyi];
#ifndef SONGYYSYNC
			SONGYYREAD;
#endif
			while((songyyi = *songyyp) >= 0 && songyyi != songyychar)
				songyyp += 2;
			songyyi = songyyp[1];
			songyyassert(songyyi >= 0,
				 gettext("Ex table not reduce %d\n"), songyyi);
		}
	}

	songyyassert((unsigned)songyyi < songyynrule, gettext("reduce %d\n"), songyyi);
	songyyj = songyyrlen[songyyi];
#if SONGYYDEBUG
	if (songyydebug)
		SONGYY_TRACE(songyyShowReduce)
	songyytp -= songyyj;
#endif
	songyyps -= songyyj;		/* pop stacks */
	songyypvt = songyypv;		/* save top */
	songyypv -= songyyj;
	songyyval = songyypv[1];	/* default action $ = $1 */
#if SONGYYDEBUG
	songyyruletype = songyyRules[songyyrmap[songyyi]].type;
#endif

	switch (songyyi) {		/* perform semantic action */
		
case SONGYYr1: {	/* song :  SONG ID declarations onevents sections */

         sSongInfo songInfo;

         
         if (_StringLengthErrorCheck (songyypvt[-3].strval))
            SONGYYABORT;

         ISong_GetSongInfo (sgpCurrSong, &songInfo);
         strcpy (songInfo.id, songyypvt[-3].strval);
         ISong_SetSongInfo (sgpCurrSong, &songInfo);
      
} break;

case SONGYYr2: {	/* song :  SONG error */

         songyyerror ("Song parsing failure.  Aborting.");
         SONGYYABORT;
      
} break;

case SONGYYr3: {	/* song :  error */

         songyyerror ("File parsing failure.  Aborting.");
         SONGYYABORT;
      
} break;

case SONGYYr6: {	/* declaresections :  DECLARESECTIONS error */

         songyyerror ("DeclareSections parsing failure.  Aborting.");
         SONGYYABORT;
      
} break;

case SONGYYr9: {	/* sectiondeclaration :  ID */

         sSongSectionInfo sectionInfo;
         ISongSection* pSection;

         
         if (_StringLengthErrorCheck (songyypvt[0].strval))
            SONGYYABORT;

         
         memset (&sectionInfo, 0, sizeof (sectionInfo));
         strcpy (sectionInfo.id, songyypvt[0].strval);

         
         CreateSongSection (&pSection, NULL);
         ISongSection_SetSectionInfo (pSection, &sectionInfo);
         ISong_AddSection (sgpCurrSong, pSection);
         ISongSection_Release (pSection);
      
} break;

case SONGYYr12: {	/* section :  SECTION ID sectionoptions samplesorsilence onevents */

         int index;
         sSongSectionInfo sectionInfo;

         
         
         if (_StringLengthErrorCheck (songyypvt[-3].strval))
            SONGYYABORT;

         ISongSection_GetSectionInfo (sgpCurrSection, &sectionInfo);
         strcpy (sectionInfo.id, songyypvt[-3].strval);
         ISongSection_SetSectionInfo (sgpCurrSection, &sectionInfo);

         
         index = _FindSectionIndexFromID (sgpCurrSong, songyypvt[-3].strval);
         if (index == -1)
         {
            songyyerror("Section not declared.");
            SONGYYABORT;
         }

         
         ISong_SetSection (sgpCurrSong, index, sgpCurrSection);

         
         ISongSection_Release (sgpCurrSection);
         CreateSongSection (&sgpCurrSection, NULL);
      
} break;

case SONGYYr13: {	/* section :  SECTION error */

         songyyerror ("Section parsing failure.  Aborting.");
         SONGYYABORT;
      
} break;

case SONGYYr16: {	/* sectionoptions :  */

         
         sgInSections = TRUE;
      
} break;

case SONGYYr17: {	/* sectionoptions :  sectionoptions volume */

         
         sSongSectionInfo sectionInfo;
         ISongSection_GetSectionInfo (sgpCurrSection, &sectionInfo);
         sectionInfo.volume = songyypvt[0].intval;
         ISongSection_SetSectionInfo (sgpCurrSection, &sectionInfo);
      
} break;

case SONGYYr18: {	/* sectionoptions :  sectionoptions loopcount */

         
         sSongSectionInfo sectionInfo;
         ISongSection_GetSectionInfo (sgpCurrSection, &sectionInfo);
         sectionInfo.loopCount = songyypvt[0].intval;
         ISongSection_SetSectionInfo (sgpCurrSection, &sectionInfo);
      
} break;

case SONGYYr19: {	/* volume :  VOLUME INT_NUM */
  songyyval.intval = songyypvt[0].intval; 
} break;

case SONGYYr22: {	/* sample :  SAMPLE QSTRING */

         sSongSampleInfo sampleInfo;

         
         if (_StringLengthErrorCheck (songyypvt[0].strval))
            SONGYYABORT;

         ISongSample_GetSampleInfo (sgpCurrSample, &sampleInfo);
         strcpy (sampleInfo.name, songyypvt[0].strval);
         ISongSample_SetSampleInfo (sgpCurrSample, &sampleInfo);

         
         ISongSection_AddSample (sgpCurrSection, sgpCurrSample);

         
         ISongSample_Release (sgpCurrSample);
         CreateSongSample (&sgpCurrSample, NULL);
      
} break;

case SONGYYr23: {	/* sample :  SAMPLE error */

         songyyerror ("Sample parsing error.  Aborting.");
         SONGYYABORT;
      
} break;

case SONGYYr24: {	/* silence :  SILENCE FLOAT_NUM SECONDS */

         songyyerror ("Silence not implemented.");
      
} break;

case SONGYYr25: {	/* loopcount :  LOOPCOUNT INT_NUM */
  songyyval.intval = songyypvt[0].intval; 
} break;

case SONGYYr26: {	/* probability :  PROBABILITY PERCENT_NUM */
  songyyval.intval = songyypvt[0].intval; 
} break;

case SONGYYr29: {	/* onevent :  ONEVENT QSTRING oneventoptions gotosections */

         sSongEventInfo eventInfo;

         
         if (_StringLengthErrorCheck (songyypvt[-2].strval))
            SONGYYABORT;

         
         ISongEvent_GetEventInfo (sgpCurrEvent, &eventInfo);
         strcpy (eventInfo.eventString, songyypvt[-2].strval);
         ISongEvent_SetEventInfo (sgpCurrEvent, &eventInfo);

         if (sgInSections)
            
            ISongSection_AddEvent (sgpCurrSection, sgpCurrEvent);
         else
            ISong_AddEvent (sgpCurrSong, sgpCurrEvent);
         
         
         ISongEvent_Release (sgpCurrEvent);
         CreateSongEvent (&sgpCurrEvent, NULL);
      
} break;

case SONGYYr30: {	/* onevent :  ONEVENT error */

         songyyerror ("Event parsing failure.  Aborting.");
         SONGYYABORT;
      
} break;

case SONGYYr32: {	/* oneventoptions :  oneventoptions IMMEDIATE */

         sSongEventInfo eventInfo;

         ISongEvent_GetEventInfo (sgpCurrEvent, &eventInfo);
         eventInfo.flags |= kSONG_EventFlagImmediate;
         ISongEvent_SetEventInfo (sgpCurrEvent, &eventInfo);
      
} break;

case SONGYYr35: {	/* gotosection :  GOTOSECTION ID probability */

         int index;
         sSongGotoInfo gotoInfo;

         
         if (_StringLengthErrorCheck (songyypvt[-1].strval))
            SONGYYABORT;
         
         
         index = _FindSectionIndexFromID (sgpCurrSong, songyypvt[-1].strval);
         if (index == -1)
         {
            songyyerror ("Section undeclared.");
            SONGYYABORT;
         }

         
         ISongGoto_GetGotoInfo (sgpCurrGoto, &gotoInfo);
         gotoInfo.probability       = songyypvt[0].intval;
         gotoInfo.sectionIndex      = index;
         ISongGoto_SetGotoInfo (sgpCurrGoto, &gotoInfo);

         
         ISongEvent_AddGoto (sgpCurrEvent, sgpCurrGoto);

         
         ISongGoto_Release (sgpCurrGoto);
         CreateSongGoto (&sgpCurrGoto, NULL);
      
} break;

case SONGYYr36: {	/* gotosection :  GOTOEND probability */

         int index;
         sSongGotoInfo gotoInfo;
         
         
         index = ISong_CountSections (sgpCurrSong);

         
         ISongGoto_GetGotoInfo (sgpCurrGoto, &gotoInfo);
         gotoInfo.probability       = songyypvt[0].intval;
         gotoInfo.sectionIndex      = index;
         ISongGoto_SetGotoInfo (sgpCurrGoto, &gotoInfo);

         
         ISongEvent_AddGoto (sgpCurrEvent, sgpCurrGoto);

         
         ISongGoto_Release (sgpCurrGoto);
         CreateSongGoto (&sgpCurrGoto, NULL);
      
} break;

case SONGYYr37: {	/* gotosection :  GOTOSECTION error */

         songyyerror ("GotoSection parsing failure.  Aborting.");
         SONGYYABORT;
      
} break;

case SONGYYr38: {	/* gotosection :  GOTOEND error */

         songyyerror ("GotoEnd parsing failure. Aborting.");
         SONGYYABORT;
      
} break;
	case SONGYYrACCEPT:
		SONGYYACCEPT;
	case SONGYYrERROR:
		goto songyyError;
	}

	/*
	 *	Look up next state in goto table.
	 */

	songyyp = &songyygo[songyypgo[songyyi]];
	songyyq = songyyp++;
	songyyi = *songyyps;
	while (songyyi < *songyyp++)
		;

	songyystate = ~(songyyi == *--songyyp? SONGYYQSONGYYP: *songyyq);
#if SONGYYDEBUG
	if (songyydebug)
		SONGYY_TRACE(songyyShowGoto)
#endif
	goto songyyStack;

songyyerrlabel:	;		/* come here from SONGYYERROR	*/
/*
#pragma used songyyerrlabel
 */
	songyyerrflag = 1;
	if (songyyi == SONGYYrERROR) {
		songyyps--;
		songyypv--;
#if SONGYYDEBUG
		songyytp--;
#endif
	}

songyyError:
	switch (songyyerrflag) {

	case 0:		/* new error */
		songyynerrs++;
		songyyi = songyychar;
		songyyerror("Syntax error");
		if (songyyi != songyychar) {
			/* user has changed the current token */
			/* try again */
			songyyerrflag++;	/* avoid loops */
			goto songyyEncore;
		}

	case 1:		/* partially recovered */
	case 2:
		songyyerrflag = 3;	/* need 3 valid shifts to recover */
			
		/*
		 *	Pop states, looking for a
		 *	shift on `error'.
		 */

		for ( ; songyyps > songyys; songyyps--, songyypv--
#if SONGYYDEBUG
					, songyytp--
#endif
		) {
			if (*songyyps >= sizeof songyypact/sizeof songyypact[0])
				continue;
			songyyp = &songyyact[songyypact[*songyyps]];
			songyyq = songyyp;
			do
				;
			while (SONGYYERRCODE < *songyyp++);

			if (SONGYYERRCODE == songyyp[-1]) {
				songyystate = ~SONGYYQSONGYYP;
				goto songyyStack;
			}
				
			/* no shift in this state */
#if SONGYYDEBUG
			if (songyydebug && songyyps > songyys+1)
				SONGYY_TRACE(songyyShowErrRecovery)
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

		if (songyychar == 0)  /* but not EOF */
			break;
#if SONGYYDEBUG
		if (songyydebug)
			SONGYY_TRACE(songyyShowErrDiscard)
#endif
		songyyclearin;
		goto songyyEncore;	/* try again in same state */
	}
	SONGYYABORT;

#ifdef SONGYYALLOC
songyyReturn:
	songyylval = save_songyylval;
	songyyval = save_songyyval;
	songyypvt = save_songyypvt;
	songyychar = save_songyychar;
	songyyerrflag = save_songyyerrflag;
	songyynerrs = save_songyynerrs;
	free((char *)songyys);
	free((char *)songyyv);
	return(retval);
#endif
}

		
#if SONGYYDEBUG
/*
 * Return type of token
 */
int
songyyGetType(tok)
int tok;
{
	songyyNamedType * tp;
	for (tp = &songyyTokenTypes[songyyntoken-1]; tp > songyyTokenTypes; tp--)
		if (tp->token == tok)
			return tp->type;
	return 0;
}
/*
 * Print a token legibly.
 */
char *
songyyptok(tok)
int tok;
{
	songyyNamedType * tp;
	for (tp = &songyyTokenTypes[songyyntoken-1]; tp > songyyTokenTypes; tp--)
		if (tp->token == tok)
			return tp->name;
	return "";
}

/*
 * Read state 'num' from SONGYYStatesFile
 */
#ifdef SONGYYTRACE
static FILE *songyyStatesFile = (FILE *) 0;
static char songyyReadBuf[SONGYYMAX_READ+1];

static char *
songyygetState(num)
int num;
{
	int	size;

	if (songyyStatesFile == (FILE *) 0
	 && (songyyStatesFile = fopen(SONGYYStatesFile, "r")) == (FILE *) 0)
		return "songyyExpandName: cannot open states file";

	if (num < songyynstate - 1)
		size = (int)(songyyStates[num+1] - songyyStates[num]);
	else {
		/* length of last item is length of file - ptr(last-1) */
		if (fseek(songyyStatesFile, 0L, 2) < 0)
			goto cannot_seek;
		size = (int) (ftell(songyyStatesFile) - songyyStates[num]);
	}
	if (size < 0 || size > SONGYYMAX_READ)
		return "songyyExpandName: bad read size";
	if (fseek(songyyStatesFile, songyyStates[num], 0) < 0) {
	cannot_seek:
		return "songyyExpandName: cannot seek in states file";
	}

	(void) fread(songyyReadBuf, 1, size, songyyStatesFile);
	songyyReadBuf[size] = '\0';
	return songyyReadBuf;
}
#endif /* SONGYYTRACE */
/*
 * Expand encoded string into printable representation
 * Used to decode songyyStates and songyyRules strings.
 * If the expansion of 's' fits in 'buf', return 1; otherwise, 0.
 */
int
songyyExpandName(num, isrule, buf, len)
int num, isrule;
char * buf;
int len;
{
	int	i, n, cnt, type;
	char	* endp, * cp;
	char	*s;

	if (isrule)
		s = songyyRules[num].name;
	else
#ifdef SONGYYTRACE
		s = songyygetState(num);
#else
		s = "*no states*";
#endif

	for (endp = buf + len - 8; *s; s++) {
		if (buf >= endp) {		/* too large: return 0 */
		full:	(void) strcpy(buf, " ...\n");
			return 0;
		} else if (*s == '%') {		/* nonterminal */
			type = 0;
			cnt = songyynvar;
			goto getN;
		} else if (*s == '&') {		/* terminal */
			type = 1;
			cnt = songyyntoken;
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
				if (n >= songyynvar)
					goto too_big;
				cp = songyysvar[n];
			} else if (n >= songyyntoken) {
			    too_big:
				cp = "<range err>";
			} else
				cp = songyyTokenTypes[n].name;

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
#ifndef SONGYYTRACE
/*
 * Show current state of songyyparse
 */
void
songyyShowState(tp)
songyyTraceItems * tp;
{
	short * p;
	SONGYYSTYPE * q;

	printf(
	    gettext("state %d (%d), char %s (%d)\n"),
	      songyysmap[tp->state], tp->state,
	      songyyptok(tp->lookahead), tp->lookahead);
}
/*
 * show results of reduction
 */
void
songyyShowReduce(tp)
songyyTraceItems * tp;
{
	printf("reduce %d (%d), pops %d (%d)\n",
		songyyrmap[tp->rule], tp->rule,
		tp->states[tp->nstates - tp->npop],
		songyysmap[tp->states[tp->nstates - tp->npop]]);
}
void
songyyShowRead(val)
int val;
{
	printf(gettext("read %s (%d)\n"), songyyptok(val), val);
}
void
songyyShowGoto(tp)
songyyTraceItems * tp;
{
	printf(gettext("goto %d (%d)\n"), songyysmap[tp->state], tp->state);
}
void
songyyShowShift(tp)
songyyTraceItems * tp;
{
	printf(gettext("shift %d (%d)\n"), songyysmap[tp->state], tp->state);
}
void
songyyShowErrRecovery(tp)
songyyTraceItems * tp;
{
	short	* top = tp->states + tp->nstates - 1;

	printf(
	gettext("Error recovery pops state %d (%d), uncovers %d (%d)\n"),
		songyysmap[*top], *top, songyysmap[*(top-1)], *(top-1));
}
void
songyyShowErrDiscard(tp)
songyyTraceItems * tp;
{
	printf(gettext("Error recovery discards %s (%d), "),
		songyyptok(tp->lookahead), tp->lookahead);
}
#endif	/* ! SONGYYTRACE */
#endif	/* SONGYYDEBUG */
