/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

/* n:\bin\lex -p inc -l -o r:\prj\cam\src\win32\inclex.c r:\prj\cam\src\inclex.lex */
#define YYNEWLINE 10
#define INITIAL 0
#define PREPROC 2
#define inc_endst 40
#define inc_nxtmax 626
#define YY_LA_SIZE 5

static unsigned short inc_la_act[] = {
 0, 9, 9, 9, 9, 2, 1, 0, 9, 10, 3, 9, 4, 9, 10, 9,
 10, 5, 9, 10, 5, 9, 10, 9, 10, 9, 10, 9, 10, 8, 7, 5,
 6, 4, 9, 10,
};

static unsigned char inc_look[] = {
 0
};

static short inc_final[] = {
 0, 0, 2, 3, 4, 4, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6,
 6, 6, 6, 7, 7, 10, 12, 15, 17, 20, 23, 25, 27, 29, 29, 30,
 30, 31, 32, 32, 32, 33, 34, 34, 36
};
#ifndef inc_state_t
#define inc_state_t unsigned char
#endif

static inc_state_t inc_begin[] = {
 0, 4, 19, 38, 0
};

static inc_state_t inc_next[] = {
 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 3, 2, 2, 2, 2, 2,
 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
 5, 7, 8, 9, 10, 11, 6, 12, 13, 14, 15, 16, 17, 18, 28, 28,
 28, 28, 28, 28, 28, 28, 28, 20, 21, 28, 28, 28, 28, 28, 28, 28,
 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 20, 28,
 26, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 23, 28, 28, 24, 25,
 25, 25, 25, 25, 25, 25, 25, 25, 28, 28, 27, 28, 28, 28, 28, 22,
 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
 22, 22, 22, 22, 22, 22, 22, 22, 22, 28, 28, 28, 28, 22, 28, 22,
 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
 22, 22, 22, 22, 22, 22, 22, 22, 22, 28, 28, 28, 28, 28, 29, 29,
 29, 29, 29, 29, 29, 29, 29, 29, 40, 29, 29, 29, 29, 29, 29, 29,
 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 40, 29, 30, 29, 29, 29,
 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 39, 29, 29, 29,
 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 31, 31,
 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
 32, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 33, 33,
 33, 33, 33, 33, 33, 33, 33, 33, 34, 35, 40, 40, 36, 36, 36, 36,
 36, 36, 36, 36, 36, 36, 40, 40, 37, 37, 37, 37, 37, 37, 37, 37,
 37, 37, 40, 40, 40, 40, 40, 40, 34, 37, 37, 37, 37, 37, 37, 37,
 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37,
 37, 37, 37, 40, 40, 40, 40, 37, 40, 37, 37, 37, 37, 37, 37, 37,
 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37,
 37, 37, 37,
};

static inc_state_t inc_check[] = {
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 4, 5, 7, 8, 9, 10, 5, 11, 6, 13, 14, 15, 16, 17, 19, 19,
 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 27, 27,
 27, 27, 27, 27, 27, 27, 27, 27, 36, 27, 27, 27, 27, 27, 27, 27,
 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 35, 27, 27, 27, 27, 27,
 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 38, 27, 27, 27,
 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 26, 26,
 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 25, 25,
 25, 25, 25, 25, 25, 25, 25, 25, 24, 34, ~0, ~0, 34, 34, 34, 34,
 34, 34, 34, 34, 34, 34, ~0, ~0, 22, 22, 22, 22, 22, 22, 22, 22,
 22, 22, ~0, ~0, ~0, ~0, ~0, ~0, 24, 22, 22, 22, 22, 22, 22, 22,
 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
 22, 22, 22, ~0, ~0, ~0, ~0, 22, ~0, 22, 22, 22, 22, 22, 22, 22,
 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
 22, 22, 22,
};

static inc_state_t inc_default[] = {
 40, 40, 40, 40, 0, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40,
 40, 40, 40, 40, 40, 40, 40, 25, 25, 40, 40, 40, 40, 27, 40, 26,
 40, 25, 40, 34, 34, 22, 19, 5,
};

static short inc_base[] = {
 0, 627, 627, 627, 93, 29, 26, 29, 29, 27, 23, 34, 627, 38, 30, 22,
 40, 40, 627, 142, 627, 627, 504, 627, 448, 478, 398, 270, 627, 627, 627, 627,
 627, 627, 492, 285, 235, 627, 329, 627, 627
};


/*
 * Copyright 1988, 1990 by Mortice Kern Systems Inc.  All rights reserved.
 * All rights reserved.
 */
#include <stdlib.h>
#include <stdio.h>
/*
 * Define gettext() to an appropriate function for internationalized messages
 * or custom processing.
 */
#if	__STDC__
#define YY_ARGS(args)	args
#else
#define YY_ARGS(args)	()
#endif

#ifndef I18N
#define gettext(s)	(s)
#endif
/*
 * Include string.h to get definition of memmove() and size_t.
 * If you do not have string.h or it does not declare memmove
 * or size_t, you will have to declare them here.
 */
#include <string.h>
/* Uncomment next line if memmove() is not declared in string.h */
/*extern char * memmove();*/
/* Uncomment next line if size_t is not available in stdio.h or string.h */
/*typedef unsigned size_t;*/
/* Drop this when LATTICE provides memmove */
#ifdef LATTICE
#define memmove	memcopy
#endif

/*
 * YY_STATIC determines the scope of variables and functions
 * declared by the lex scanner. It must be set with a -DYY_STATIC
 * option to the compiler (it cannot be defined in the lex program).
 */
#ifdef	YY_STATIC
/* define all variables as static to allow more than one lex scanner */
#define	YY_DECL	static
#else
/* define all variables as global to allow other modules to access them */
#define	YY_DECL	
#endif

/*
 * You can redefine incgetc. For YACC Tracing, compile this code
 * with -DYYTRACE to get input from yt_getc
 */
#ifdef YYTRACE
extern int	yt_getc YY_ARGS((void));
#define incgetc()	yt_getc()
#else
#define	incgetc()	getc(incin) 	/* inclex input source */
#endif

/*
 * the following can be redefined by the user.
 */
#define	ECHO		fputs(inctext, incout)
#define	output(c)	putc((c), incout) /* inclex sink for unmatched chars */
#define	YY_FATAL(msg)	{ fprintf(stderr, "inclex: %s\n", msg); exit(1); }
#define	YY_INTERACTIVE	1		/* save micro-seconds if 0 */
#define	YYLMAX		100		/* token and pushback buffer size */

/*
 * the following must not be redefined.
 */
#define	inc_tbuf	inctext		/* token string */

#define	BEGIN		inc_start =
#define	REJECT		goto inc_reject
#define	NLSTATE		(inc_lastc = YYNEWLINE)
#define	YY_INIT \
	(inc_start = incleng = inc_end = 0, inc_lastc = YYNEWLINE)
#define	incmore()	goto inc_more
#define	incless(n)	if ((n) < 0 || (n) > inc_end) ; \
			else { YY_SCANNER; incleng = (n); YY_USER; }

YY_DECL	void	inc_reset YY_ARGS((void));
YY_DECL	int	input	YY_ARGS((void));
YY_DECL	int	unput	YY_ARGS((int c));

/* functions defined in libl.lib */
extern	int	incwrap	YY_ARGS((void));
extern	void	incerror	YY_ARGS((char *fmt, ...));
extern	void	inccomment	YY_ARGS((char *term));
extern	int	incmapch	YY_ARGS((int delim, int escape));


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




#ifdef	YY_DEBUG
#undef	YY_DEBUG
#define	YY_DEBUG(fmt, a1, a2)	fprintf(stderr, fmt, a1, a2)
#else
#define	YY_DEBUG(fmt, a1, a2)
#endif

/*
 * The declaration for the lex scanner can be changed by
 * redefining YYLEX or YYDECL. This must be done if you have
 * more than one scanner in a program.
 */
#ifndef	YYLEX
#define	YYLEX inclex			/* name of lex scanner */
#endif

#ifndef YYDECL
#define	YYDECL	int YYLEX YY_ARGS((void))	/* declaration for lex scanner */
#endif

/* stdin and stdout may not neccessarily be constants */
YY_DECL	FILE   *incin = stdin;
YY_DECL	FILE   *incout = stdout;
YY_DECL	int	inclineno = 1;		/* line number */

/*
 * inc_tbuf is an alias for inctext.
 * inc_sbuf[0:incleng-1] contains the states corresponding to inc_tbuf.
 * inc_tbuf[0:incleng-1] contains the current token.
 * inc_tbuf[incleng:inc_end-1] contains pushed-back characters.
 * When the user action routine is active,
 * inc_save contains inc_tbuf[incleng], which is set to '\0'.
 * Things are different when YY_PRESERVE is defined. 
 */

YY_DECL	char inc_tbuf [YYLMAX+1]; /* text buffer (really inctext) */
static	inc_state_t inc_sbuf [YYLMAX+1];	/* state buffer */

static	int	inc_end = 0;		/* end of pushback */
static	int	inc_start = 0;		/* start state */
static	int	inc_lastc = YYNEWLINE;	/* previous char */
YY_DECL	int	incleng = 0;		/* inctext token length */

#ifndef YY_PRESERVE	/* the efficient default push-back scheme */

static	char inc_save;	/* saved inctext[incleng] */

#define	YY_USER	{ /* set up inctext for user */ \
		inc_save = inctext[incleng]; \
		inctext[incleng] = 0; \
	}
#define	YY_SCANNER { /* set up inctext for scanner */ \
		inctext[incleng] = inc_save; \
	}

#else		/* not-so efficient push-back for inctext mungers */

static	char inc_save [YYLMAX];
static	char *inc_push = inc_save+YYLMAX;

#define	YY_USER { \
		size_t n = inc_end - incleng; \
		inc_push = inc_save+YYLMAX - n; \
		if (n > 0) \
			memmove(inc_push, inctext+incleng, n); \
		inctext[incleng] = 0; \
	}
#define	YY_SCANNER { \
		size_t n = inc_save+YYLMAX - inc_push; \
		if (n > 0) \
			memmove(inctext+incleng, inc_push, n); \
		inc_end = incleng + n; \
	}

#endif

/*
 * The actual lex scanner (usually inclex(void)).
 * NOTE: you should invoke inc_init() if you are calling inclex()
 * with new input; otherwise old lookaside will get in your way
 * and inclex() will die horribly.
 */
YYDECL {
	register int c, i, incst, incbase;
	int incfmin, incfmax;	/* inc_la_act indices of final states */
	int incoldi, incoleng;	/* base i, incleng before look-ahead */
	int inceof;		/* 1 if eof has already been read */



	inceof = 0;
	i = incleng;
	YY_SCANNER;

  inc_again:
	incleng = i;
	/* determine previous char. */
	if (i > 0)
		inc_lastc = inctext[i-1];
	/* scan previously accepted token adjusting inclineno */
	while (i > 0)
		if (inctext[--i] == YYNEWLINE)
			inclineno++;
	/* adjust pushback */
	inc_end -= incleng;
	memmove(inctext, inctext+incleng, (size_t) inc_end);
	i = 0;

  inc_contin:
	incoldi = i;

	/* run the state machine until it jams */
	inc_sbuf[i] = incst = inc_begin[inc_start + (inc_lastc == YYNEWLINE)];
	do {
		YY_DEBUG(gettext("<state %d, i = %d>\n"), incst, i);
		if (i >= YYLMAX)
			YY_FATAL(gettext("Token buffer overflow"));

		/* get input char */
		if (i < inc_end)
			c = inc_tbuf[i];		/* get pushback char */
		else if (!inceof && (c = incgetc()) != EOF) {
			inc_end = i+1;
			inc_tbuf[i] = c;
		} else /* c == EOF */ {
			c = EOF;		/* just to make sure... */
			if (i == incoldi) {	/* no token */
				inceof = 0;
				if (incwrap())
					return 0;
				else
					goto inc_again;
			} else {
				inceof = 1;	/* don't re-read EOF */
				break;
			}
		}
		YY_DEBUG(gettext("<input %d = 0x%02x>\n"), c, c);

		/* look up next state */
		while ((incbase = inc_base[incst]+c) > inc_nxtmax || inc_check[incbase] != incst) {
			if (incst == inc_endst)
				goto inc_jammed;
			incst = inc_default[incst];
		}
		incst = inc_next[incbase];
	  inc_jammed: ;
	  inc_sbuf[++i] = incst;
	} while (!(incst == inc_endst || YY_INTERACTIVE && inc_base[incst] > inc_nxtmax && inc_default[incst] == inc_endst));
	YY_DEBUG(gettext("<stopped %d, i = %d>\n"), incst, i);
	if (incst != inc_endst)
		++i;

  inc_search:
	/* search backward for a final state */
	while (--i > incoldi) {
		incst = inc_sbuf[i];
		if ((incfmin = inc_final[incst]) < (incfmax = inc_final[incst+1]))
			goto inc_found;	/* found final state(s) */
	}
	/* no match, default action */
	i = incoldi + 1;
	output(inc_tbuf[incoldi]);
	goto inc_again;

  inc_found:
	YY_DEBUG(gettext("<final state %d, i = %d>\n"), incst, i);
	incoleng = i;		/* save length for REJECT */
	
	/* pushback look-ahead RHS */
	if ((c = (int)(inc_la_act[incfmin]>>9) - 1) >= 0) { /* trailing context? */
		unsigned char *bv = inc_look + c*YY_LA_SIZE;
		static unsigned char bits [8] = {
			1<<0, 1<<1, 1<<2, 1<<3, 1<<4, 1<<5, 1<<6, 1<<7
		};
		while (1) {
			if (--i < incoldi) {	/* no / */
				i = incoleng;
				break;
			}
			incst = inc_sbuf[i];
			if (bv[(unsigned)incst/8] & bits[(unsigned)incst%8])
				break;
		}
	}

	/* perform action */
	incleng = i;
	YY_USER;
	switch (inc_la_act[incfmin] & 0777) {
	case 0:
	;
	break;
	case 1:
	{
   ConfigSpew("lexspew", ("include\n"));
   BEGIN PREPROC;
   return INCLUDE;
};
	break;
	case 2:
	{
   ConfigSpew("lexspew", ("define\n"));
   BEGIN PREPROC;
   return DEFINE;
};
	break;
	case 3:
	{
   ConfigSpew("lexspew", ("return\n"));
   BEGIN INITIAL;
};
	break;
	case 4:
	{
   ConfigSpew("lexspew", ("ident\n"));
   inclval.strval = local_strdup(inctext);
   return IDENT;
}
	break;
	case 5:
	{
   ConfigSpew("lexspew", ("int\n"));
   inclval.ival = atoi(inctext);
   return INT;
}
	break;
	case 6:
	{
   ConfigSpew("lexspew", ("hex\n"));
   inclval.ival = strtol(inctext, NULL, 16);
   return INT;
}
	break;
	case 7:
	{
   ConfigSpew("lexspew", ("string\n"));
   inclval.strval = local_strdup(inctext);
   return STRING;
}
	break;
	case 8:
	{
   ConfigSpew("lexspew", ("string\n"));
   inclval.strval = local_strdup(inctext);
   return STRING;
}
	break;
	case 9:
	;
	break;
	case 10:
	;
	break;


	}
	YY_SCANNER;
	i = incleng;
	goto inc_again;			/* action fell though */

  inc_reject:
	YY_SCANNER;
	i = incoleng;			/* restore original inctext */
	if (++incfmin < incfmax)
		goto inc_found;		/* another final state, same length */
	else
		goto inc_search;		/* try shorter inctext */

  inc_more:
	YY_SCANNER;
	i = incleng;
	if (i > 0)
		inc_lastc = inctext[i-1];
	goto inc_contin;
}
/*
 * Safely switch input stream underneath LEX
 */
typedef struct inc_save_block_tag {
	FILE	* oldfp;
	int	oldline;
	int	oldend;
	int	oldstart;
	int	oldlastc;
	int	oldleng;
	char	savetext[YYLMAX+1];
	inc_state_t	savestate[YYLMAX+1];
} YY_SAVED;

YY_SAVED *
incSaveScan(fp)
FILE * fp;
{
	YY_SAVED * p;

	if ((p = (YY_SAVED *) malloc(sizeof(*p))) == NULL)
		return p;

	p->oldfp = incin;
	p->oldline = inclineno;
	p->oldend = inc_end;
	p->oldstart = inc_start;
	p->oldlastc = inc_lastc;
	p->oldleng = incleng;
	(void) memcpy(p->savetext, inctext, sizeof inctext);
	(void) memcpy((char *) p->savestate, (char *) inc_sbuf,
		sizeof inc_sbuf);

	incin = fp;
	inclineno = 1;
	YY_INIT;

	return p;
}
/*f
 * Restore previous LEX state
 */
void
incRestoreScan(p)
YY_SAVED * p;
{
	if (p == NULL)
		return;
	incin = p->oldfp;
	inclineno = p->oldline;
	inc_end = p->oldend;
	inc_start = p->oldstart;
	inc_lastc = p->oldlastc;
	incleng = p->oldleng;

	(void) memcpy(inctext, p->savetext, sizeof inctext);
	(void) memcpy((char *) inc_sbuf, (char *) p->savestate,
		sizeof inc_sbuf);
	free(p);
}
/*
 * User-callable re-initialization of inclex()
 */
void
inc_reset()
{
	YY_INIT;
	inclineno = 1;		/* line number */
}
/* get input char with pushback */
YY_DECL int
input()
{
	int c;
#ifndef YY_PRESERVE
	if (inc_end > incleng) {
		inc_end--;
		memmove(inctext+incleng, inctext+incleng+1,
			(size_t) (inc_end-incleng));
		c = inc_save;
		YY_USER;
#else
	if (inc_push < inc_save+YYLMAX) {
		c = *inc_push++;
#endif
	} else
		c = incgetc();
	inc_lastc = c;
	if (c == YYNEWLINE)
		inclineno++;
	return c;
}

/*f
 * pushback char
 */
YY_DECL int
unput(c)
	int c;
{
#ifndef YY_PRESERVE
	if (inc_end >= YYLMAX)
		YY_FATAL(gettext("Push-back buffer overflow"));
	if (inc_end > incleng) {
		inctext[incleng] = inc_save;
		memmove(inctext+incleng+1, inctext+incleng,
			(size_t) (inc_end-incleng));
		inctext[incleng] = 0;
	}
	inc_end++;
	inc_save = c;
#else
	if (inc_push <= inc_save)
		YY_FATAL(gettext("Push-back buffer overflow"));
	*--inc_push = c;
#endif
	if (c == YYNEWLINE)
		inclineno--;
	return c;
}


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


