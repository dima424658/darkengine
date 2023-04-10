/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

/* n:\bin\yacc -p inc -l -D inctok.h -o inc.c inc.y */
#ifdef INCTRACE
#define INCDEBUG 1
#else
#ifndef INCDEBUG
#define INCDEBUG 0
#endif
#endif
/*
 * Portable way of defining ANSI C prototypes
 */
#ifndef INC_ARGS
#if __STDC__
#define INC_ARGS(x)	x
#else
#define INC_ARGS(x)	()
#endif
#endif

#if INCDEBUG
typedef struct incNamedType_tag {	/* Tokens */
	char	* name;		/* printable name */
	short	token;		/* token # */
	short	type;		/* token type */
} incNamedType;
typedef struct incTypedRules_tag {	/* Typed rule table */
	char	* name;		/* compressed rule string */
	short	type;		/* rule result type */
} incTypedRules;

#endif

typedef union {
   char *strval;
   int  ival;
} INCSTYPE;
#define INCLUDE	257
#define DEFINE	258
#define INT	259
#define STRING	260
#define IDENT	261

#include <stdio.h>
#include <config.h>
#include <cfgdbg.h>
#include <mprintf.h>
#include <string.h>
#include <inctab.h>

extern BOOL IncludeFileOpen(char *string, BOOL push);
extern BOOL IncludeFilePop();

extern FILE *incin;

#define incerror printf

extern int incchar, incerrflag;
extern INCSTYPE inclval;
#if INCDEBUG
enum INC_Types { INC_t_NoneDefined, INC_t_ival, INC_t_strval
};
#endif
#if INCDEBUG
yyTypedRules incRules[] = {
	{ "&00: %01 &00",  0},
	{ "%01: %02",  0},
	{ "%02: %03",  0},
	{ "%02: %04",  0},
	{ "%04: %05",  0},
	{ "%04: %05 %04",  0},
	{ "%05: %06",  0},
	{ "%05: %07",  0},
	{ "%05: %08",  0},
	{ "%05: %09",  0},
	{ "%06: &02 &05",  0},
	{ "%07: &03 &06 &04 %10",  0},
	{ "%08: &03 &06",  0},
	{ "%09: &03 &06 %11 %10",  0},
	{ "%11: &06",  0},
	{ "%11: &05",  0},
	{ "%10: %03",  0},
	{ "%10: %12",  0},
	{ "%12: %13",  0},
	{ "%12: %13 %12",  0},
	{ "%13: &06",  0},
	{ "%13: &04",  0},
	{ "%13: &05",  0},
	{ "%03:",  0},
{ "$accept",  0},{ "error",  0}
};
yyNamedType incTokenTypes[] = {
	{ "$end",  0,  0},
	{ "error",  256,  0},
	{ "INCLUDE",  257,  0},
	{ "DEFINE",  258,  0},
	{ "INT",  259,  1},
	{ "STRING",  260,  2},
	{ "IDENT",  261,  2}

};
#endif
static short incdef[] = {

	  -1,   25,   -5,    3,   -9,  -17,   24
};
static short incex[] = {

	   0,   23,   -1,    1,    0,    0,   -1,    1,    0,   23, 
	 257,   23,  258,   23,   -1,    1,    0,   23,  257,   23, 
	 258,   23,   -1,    1
};
static short incact[] = {

	  -8,   -9,  258,  257,   -4,  261,  -12,  260,   -6,  -22, 
	 -21,  261,  260,  259,  -27,  -28,  -26,  261,  260,  259,   -1
};
static short incpact[] = {

	   2,    2,    8,   11,   17,   17,   17,    7,    5
};
static short incgo[] = {

	  -3,  -13,  -14,  -23,    0,  -16,  -15,    1,   -2,  -17, 
	 -18,  -19,  -20,  -11,  -10,    5,   -5,  -25,  -24,    6, 
	  -7,   -1
};
static short incpgo[] = {

	   0,    0,    0,   11,   12,   10,    9,    0,    1,    1, 
	   6,    8,    8,    8,    8,   16,   16,   14,   14,   18, 
	  20,   20,   20,    3,   18,    6,    0
};
static short incrlen[] = {

	   0,    0,    0,    2,    4,    4,    2,    1,    1,    1, 
	   2,    1,    1,    1,    1,    1,    1,    1,    1,    2, 
	   1,    1,    1,    0,    1,    1,    2
};
#define INCS0	0
#define INCDELTA	5
#define INCNPACT	9
#define INCNDEF	7

#define INCr24	0
#define INCr25	1
#define INCr26	2
#define INCr12	3
#define INCr13	4
#define INCr11	5
#define INCr10	6
#define INCrACCEPT	INCr24
#define INCrERROR	INCr25
#define INCrLR2	INCr26
#if INCDEBUG
char * incsvar[] = {
	"$accept",
	"file",
	"opt_statements",
	"null",
	"statements",
	"statement",
	"include",
	"define",
	"define_empty",
	"define_complex",
	"opt_other",
	"not_int",
	"other",
	"not_newline",
	0
};
short incrmap[] = {

	  24,   25,   26,   12,   13,   11,   10,    1,    2,    3, 
	   5,    6,    7,    8,    9,   14,   15,   16,   17,   19, 
	  20,   21,   22,   23,   18,    4,    0
};
short incsmap[] = {

	   0,    7,   11,   12,   17,   18,   22,    2,    1,   25, 
	  26,   13,   10,    9,    8,   14,    6,    5,    4,    3, 
	  16,   15,   24,   23,   27,   21,   20,   19
};
int incntoken = 7;
int incnvar = 14;
int incnstate = 28;
int incnrule = 27;
#endif

#if INCDEBUG
/*
 * Package up YACC context for tracing
 */
typedef struct incTraceItems_tag {
	int	state, lookahead, errflag, done;
	int	rule, npop;
	short	* states;
	int	nstates;
	INCSTYPE * values;
	int	nvalues;
	short	* types;
} incTraceItems;
#endif


/*
 * Copyright 1985, 1990 by Mortice Kern Systems Inc.  All rights reserved.
 * 
 * Automaton to interpret LALR(1) tables.
 *
 * Macros:
 *	incclearin - clear the lookahead token.
 *	incerrok - forgive a pending error
 *	INCERROR - simulate an error
 *	INCACCEPT - halt and return 0
 *	INCABORT - halt and return 1
 *	INCRETURN(value) - halt and return value.  You should use this
 *		instead of return(value).
 *	INCREAD - ensure incchar contains a lookahead token by reading
 *		one if it does not.  See also INCSYNC.
 *	INCRECOVERING - 1 if syntax error detected and not recovered
 *		yet; otherwise, 0.
 *
 * Preprocessor flags:
 *	INCDEBUG - includes debug code if 1.  The parser will print
 *		 a travelogue of the parse if this is defined as 1
 *		 and incdebug is non-zero.
 *		yacc -t sets INCDEBUG to 1, but not incdebug.
 *	INCTRACE - turn on INCDEBUG, and undefine default trace functions
 *		so that the interactive functions in 'ytrack.c' will
 *		be used.
 *	INCSSIZE - size of state and value stacks (default 150).
 *	INCSTATIC - By default, the state stack is an automatic array.
 *		If this is defined, the stack will be static.
 *		In either case, the value stack is static.
 *	INCALLOC - Dynamically allocate both the state and value stacks
 *		by calling malloc() and free().
 *	INCSYNC - if defined, yacc guarantees to fetch a lookahead token
 *		before any action, even if it doesnt need it for a decision.
 *		If INCSYNC is defined, INCREAD will never be necessary unless
 *		the user explicitly sets incchar = -1
 *
 * Copyright (c) 1983, by the University of Waterloo
 */
/*
 * Prototypes
 */

extern int inclex INC_ARGS((void));

#if INCDEBUG

#include <stdlib.h>		/* common prototypes */
#include <string.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

extern char *	incValue INC_ARGS((INCSTYPE, int));	/* print inclval */
extern void incShowState INC_ARGS((incTraceItems *));
extern void incShowReduce INC_ARGS((incTraceItems *));
extern void incShowGoto INC_ARGS((incTraceItems *));
extern void incShowShift INC_ARGS((incTraceItems *));
extern void incShowErrRecovery INC_ARGS((incTraceItems *));
extern void incShowErrDiscard INC_ARGS((incTraceItems *));

extern void incShowRead INC_ARGS((int));
#endif

/*
 * If INCDEBUG defined and incdebug set,
 * tracing functions will be called at appropriate times in incparse()
 * Pass state of YACC parse, as filled into incTraceItems incx
 * If incx.done is set by the tracing function, incparse() will terminate
 * with a return value of -1
 */
#define INC_TRACE(fn) { \
	incx.state = incstate; incx.lookahead = incchar; incx.errflag =incerrflag; \
	incx.states = incs+1; incx.nstates = incps-incs; \
	incx.values = incv+1; incx.nvalues = incpv-incv; \
	incx.types = inctypev+1; incx.done = 0; \
	incx.rule = inci; incx.npop = incj; \
	fn(&incx); \
	if (incx.done) INCRETURN(-1); }

#ifndef I18N
#define	gettext(x)	x
#endif

#ifndef INCSSIZE
# define INCSSIZE	150
#endif

#define INCERROR		goto incerrlabel
#define incerrok		incerrflag = 0
#if INCDEBUG
#define incclearin	{ if (incdebug) incShowRead(-1); incchar = -1; }
#else
#define incclearin	incchar = -1
#endif
#define INCACCEPT	INCRETURN(0)
#define INCABORT		INCRETURN(1)
#define INCRECOVERING()	(incerrflag != 0)
#ifdef INCALLOC
# define INCRETURN(val)	{ retval = (val); goto incReturn; }
#else
# define INCRETURN(val)	return(val)
#endif
#if INCDEBUG
/* The if..else makes this macro behave exactly like a statement */
# define INCREAD	if (incchar < 0) {					\
			if ((incchar = inclex()) < 0)			\
				incchar = 0;				\
			if (incdebug)					\
				incShowRead(incchar);			\
		} else
#else
# define INCREAD	if (incchar < 0) {					\
			if ((incchar = inclex()) < 0)			\
				incchar = 0;				\
		} else
#endif

#define INCERRCODE	256		/* value of `error' */
#define	INCQINCP	incq[incq-incp]

INCSTYPE	incval;				/* $ */
INCSTYPE	*incpvt;				/* $n */
INCSTYPE	inclval;				/* inclex() sets this */

int	incchar,				/* current token */
	incerrflag,			/* error flag */
	incnerrs;			/* error count */

#if INCDEBUG
int incdebug = 0;		/* debug if this flag is set */
extern char	*incsvar[];	/* table of non-terminals (aka 'variables') */
extern incNamedType incTokenTypes[];	/* table of terminals & their types */
extern short	incrmap[], incsmap[];	/* map internal rule/states */
extern int	incnstate, incnvar, incntoken, incnrule;

extern int	incGetType INC_ARGS((int));	/* token type */
extern char	*incptok INC_ARGS((int));	/* printable token string */
extern int	incExpandName INC_ARGS((int, int, char *, int));
				  /* expand incRules[] or incStates[] */
static char *	incgetState INC_ARGS((int));

#define incassert(condition, msg, arg) \
	if (!(condition)) { \
		printf(gettext("\nyacc bug: ")); \
		printf(msg, arg); \
		INCABORT; }
#else /* !INCDEBUG */
#define incassert(condition, msg, arg)
#endif


incparse()
{
	register short		inci, *incp;	/* for table lookup */
	register short		*incps;		/* top of state stack */
	register short		incstate;	/* current state */
	register INCSTYPE	*incpv;		/* top of value stack */
	register short		*incq;
	register int		incj;
#if INCDEBUG
	incTraceItems	incx;			/* trace block */
	short	* inctp;
	int	incruletype = 0;
#endif
#ifdef INCSTATIC
	static short	incs[INCSSIZE + 1];
	static INCSTYPE	incv[INCSSIZE + 1];
#if INCDEBUG
	static short	inctypev[INCSSIZE+1];	/* type assignments */
#endif
#else /* ! INCSTATIC */
#ifdef INCALLOC
	INCSTYPE *incv;
	short	*incs;
#if INCDEBUG
	short	*inctypev;
#endif
	INCSTYPE save_inclval;
	INCSTYPE save_incval;
	INCSTYPE *save_incpvt;
	int save_incchar, save_incerrflag, save_incnerrs;
	int retval;
#else
	short		incs[INCSSIZE + 1];
	static INCSTYPE	incv[INCSSIZE + 1];	/* historically static */
#if INCDEBUG
	short	inctypev[INCSSIZE+1];		/* mirror type table */
#endif
#endif /* ! INCALLOC */
#endif /* ! INCSTATIC */


#ifdef INCALLOC
	incs = (short *) malloc((INCSSIZE + 1) * sizeof(short));
	incv = (INCSTYPE *) malloc((INCSSIZE + 1) * sizeof(INCSTYPE));
#if INCDEBUG
	inctypev = (short *) malloc((INCSSIZE+1) * sizeof(short));
#endif
	if (incs == (short *)0 || incv == (INCSTYPE *)0
#if INCDEBUG
		|| inctypev == (short *) 0
#endif
	) {
		incerror("Not enough space for parser stacks");
		return 1;
	}
	save_inclval = inclval;
	save_incval = incval;
	save_incpvt = incpvt;
	save_incchar = incchar;
	save_incerrflag = incerrflag;
	save_incnerrs = incnerrs;
#endif

	incnerrs = 0;
	incerrflag = 0;
	incclearin;
	incps = incs;
	incpv = incv;
	*incps = incstate = INCS0;		/* start state */
#if INCDEBUG
	inctp = inctypev;
	inci = incj = 0;			/* silence compiler warnings */
#endif

incStack:
	incassert((unsigned)incstate < incnstate, gettext("state %d\n"), incstate);
	if (++incps > &incs[INCSSIZE]) {
		incerror("Parser stack overflow");
		INCABORT;
	}
	*incps = incstate;	/* stack current state */
	*++incpv = incval;	/* ... and value */
#if INCDEBUG
	*++inctp = incruletype;	/* ... and type */

	if (incdebug)
		INC_TRACE(incShowState)
#endif

	/*
	 *	Look up next action in action table.
	 */
incEncore:
#ifdef INCSYNC
	INCREAD;
#endif
	if (incstate >= sizeof incpact/sizeof incpact[0]) 	/* simple state */
		inci = incstate - INCDELTA;	/* reduce in any case */
	else {
		if(*(incp = &incact[incpact[incstate]]) >= 0) {
			/* Look for a shift on incchar */
#ifndef INCSYNC
			INCREAD;
#endif
			incq = incp;
			inci = incchar;
			while (inci < *incp++)
				;
			if (inci == incp[-1]) {
				incstate = ~INCQINCP;
#if INCDEBUG
				if (incdebug) {
					incruletype = incGetType(incchar);
					INC_TRACE(incShowShift)
				}
#endif
				incval = inclval;	/* stack what inclex() set */
				incclearin;		/* clear token */
				if (incerrflag)
					incerrflag--;	/* successful shift */
				goto incStack;
			}
		}

		/*
	 	 *	Fell through - take default action
	 	 */

		if (incstate >= sizeof incdef /sizeof incdef[0])
			goto incError;
		if ((inci = incdef[incstate]) < 0)	 { /* default == reduce? */
			/* Search exception table */
			incassert((unsigned)~inci < sizeof incex/sizeof incex[0],
				gettext("exception %d\n"), incstate);
			incp = &incex[~inci];
#ifndef INCSYNC
			INCREAD;
#endif
			while((inci = *incp) >= 0 && inci != incchar)
				incp += 2;
			inci = incp[1];
			incassert(inci >= 0,
				 gettext("Ex table not reduce %d\n"), inci);
		}
	}

	incassert((unsigned)inci < incnrule, gettext("reduce %d\n"), inci);
	incj = incrlen[inci];
#if INCDEBUG
	if (incdebug)
		INC_TRACE(incShowReduce)
	inctp -= incj;
#endif
	incps -= incj;		/* pop stacks */
	incpvt = incpv;		/* save top */
	incpv -= incj;
	incval = incpv[1];	/* default action $ = $1 */
#if INCDEBUG
	incruletype = incRules[incrmap[inci]].type;
#endif

	switch (inci) {		/* perform semantic action */
		
case INCr10: {	/* include :  INCLUDE STRING */

   IncludeFileOpen(incpvt[0].strval, TRUE);
   ConfigSpew("yaccspew", ("Include %s\n", incpvt[0].strval));

} break;

case INCr11: {	/* define :  DEFINE IDENT INT opt_other */

   ConfigSpew("yaccspew", ("Define %s %d\n", incpvt[-2].strval, incpvt[-1].ival));
   IncTabAddInt(incpvt[-2].strval, incpvt[-1].ival);

} break;

case INCr12: {	/* define_empty :  DEFINE IDENT */

   ConfigSpew("yaccspew", ("Define %s\n", incpvt[0].strval));

} break;

case INCr13: {	/* define_complex :  DEFINE IDENT not_int opt_other */

   ConfigSpew("yaccspew", ("Define Complex\n"));

} break;
	case INCrACCEPT:
		INCACCEPT;
	case INCrERROR:
		goto incError;
	}

	/*
	 *	Look up next state in goto table.
	 */

	incp = &incgo[incpgo[inci]];
	incq = incp++;
	inci = *incps;
	while (inci < *incp++)
		;

	incstate = ~(inci == *--incp? INCQINCP: *incq);
#if INCDEBUG
	if (incdebug)
		INC_TRACE(incShowGoto)
#endif
	goto incStack;

incerrlabel:	;		/* come here from INCERROR	*/
/*
#pragma used incerrlabel
 */
	incerrflag = 1;
	if (inci == INCrERROR) {
		incps--;
		incpv--;
#if INCDEBUG
		inctp--;
#endif
	}

incError:
	switch (incerrflag) {

	case 0:		/* new error */
		incnerrs++;
		inci = incchar;
		incerror("Syntax error");
		if (inci != incchar) {
			/* user has changed the current token */
			/* try again */
			incerrflag++;	/* avoid loops */
			goto incEncore;
		}

	case 1:		/* partially recovered */
	case 2:
		incerrflag = 3;	/* need 3 valid shifts to recover */
			
		/*
		 *	Pop states, looking for a
		 *	shift on `error'.
		 */

		for ( ; incps > incs; incps--, incpv--
#if INCDEBUG
					, inctp--
#endif
		) {
			if (*incps >= sizeof incpact/sizeof incpact[0])
				continue;
			incp = &incact[incpact[*incps]];
			incq = incp;
			do
				;
			while (INCERRCODE < *incp++);

			if (INCERRCODE == incp[-1]) {
				incstate = ~INCQINCP;
				goto incStack;
			}
				
			/* no shift in this state */
#if INCDEBUG
			if (incdebug && incps > incs+1)
				INC_TRACE(incShowErrRecovery)
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

		if (incchar == 0)  /* but not EOF */
			break;
#if INCDEBUG
		if (incdebug)
			INC_TRACE(incShowErrDiscard)
#endif
		incclearin;
		goto incEncore;	/* try again in same state */
	}
	INCABORT;

#ifdef INCALLOC
incReturn:
	inclval = save_inclval;
	incval = save_incval;
	incpvt = save_incpvt;
	incchar = save_incchar;
	incerrflag = save_incerrflag;
	incnerrs = save_incnerrs;
	free((char *)incs);
	free((char *)incv);
	return(retval);
#endif
}

		
#if INCDEBUG
/*
 * Return type of token
 */
int
incGetType(tok)
int tok;
{
	incNamedType * tp;
	for (tp = &incTokenTypes[incntoken-1]; tp > incTokenTypes; tp--)
		if (tp->token == tok)
			return tp->type;
	return 0;
}
/*
 * Print a token legibly.
 */
char *
incptok(tok)
int tok;
{
	incNamedType * tp;
	for (tp = &incTokenTypes[incntoken-1]; tp > incTokenTypes; tp--)
		if (tp->token == tok)
			return tp->name;
	return "";
}

/*
 * Read state 'num' from INCStatesFile
 */
#ifdef INCTRACE
static FILE *incStatesFile = (FILE *) 0;
static char incReadBuf[INCMAX_READ+1];

static char *
incgetState(num)
int num;
{
	int	size;

	if (incStatesFile == (FILE *) 0
	 && (incStatesFile = fopen(INCStatesFile, "r")) == (FILE *) 0)
		return "incExpandName: cannot open states file";

	if (num < incnstate - 1)
		size = (int)(incStates[num+1] - incStates[num]);
	else {
		/* length of last item is length of file - ptr(last-1) */
		if (fseek(incStatesFile, 0L, 2) < 0)
			goto cannot_seek;
		size = (int) (ftell(incStatesFile) - incStates[num]);
	}
	if (size < 0 || size > INCMAX_READ)
		return "incExpandName: bad read size";
	if (fseek(incStatesFile, incStates[num], 0) < 0) {
	cannot_seek:
		return "incExpandName: cannot seek in states file";
	}

	(void) fread(incReadBuf, 1, size, incStatesFile);
	incReadBuf[size] = '\0';
	return incReadBuf;
}
#endif /* INCTRACE */
/*
 * Expand encoded string into printable representation
 * Used to decode incStates and incRules strings.
 * If the expansion of 's' fits in 'buf', return 1; otherwise, 0.
 */
int
incExpandName(num, isrule, buf, len)
int num, isrule;
char * buf;
int len;
{
	int	i, n, cnt, type;
	char	* endp, * cp;
	char	*s;

	if (isrule)
		s = incRules[num].name;
	else
#ifdef INCTRACE
		s = incgetState(num);
#else
		s = "*no states*";
#endif

	for (endp = buf + len - 8; *s; s++) {
		if (buf >= endp) {		/* too large: return 0 */
		full:	(void) strcpy(buf, " ...\n");
			return 0;
		} else if (*s == '%') {		/* nonterminal */
			type = 0;
			cnt = incnvar;
			goto getN;
		} else if (*s == '&') {		/* terminal */
			type = 1;
			cnt = incntoken;
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
				if (n >= incnvar)
					goto too_big;
				cp = incsvar[n];
			} else if (n >= incntoken) {
			    too_big:
				cp = "<range err>";
			} else
				cp = incTokenTypes[n].name;

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
#ifndef INCTRACE
/*
 * Show current state of incparse
 */
void
incShowState(tp)
incTraceItems * tp;
{
	short * p;
	INCSTYPE * q;

	printf(
	    gettext("state %d (%d), char %s (%d)\n"),
	      incsmap[tp->state], tp->state,
	      incptok(tp->lookahead), tp->lookahead);
}
/*
 * show results of reduction
 */
void
incShowReduce(tp)
incTraceItems * tp;
{
	printf("reduce %d (%d), pops %d (%d)\n",
		incrmap[tp->rule], tp->rule,
		tp->states[tp->nstates - tp->npop],
		incsmap[tp->states[tp->nstates - tp->npop]]);
}
void
incShowRead(val)
int val;
{
	printf(gettext("read %s (%d)\n"), incptok(val), val);
}
void
incShowGoto(tp)
incTraceItems * tp;
{
	printf(gettext("goto %d (%d)\n"), incsmap[tp->state], tp->state);
}
void
incShowShift(tp)
incTraceItems * tp;
{
	printf(gettext("shift %d (%d)\n"), incsmap[tp->state], tp->state);
}
void
incShowErrRecovery(tp)
incTraceItems * tp;
{
	short	* top = tp->states + tp->nstates - 1;

	printf(
	gettext("Error recovery pops state %d (%d), uncovers %d (%d)\n"),
		incsmap[*top], *top, incsmap[*(top-1)], *(top-1));
}
void
incShowErrDiscard(tp)
incTraceItems * tp;
{
	printf(gettext("Error recovery discards %s (%d), "),
		incptok(tp->lookahead), tp->lookahead);
}
#endif	/* ! INCTRACE */
#endif	/* INCDEBUG */
