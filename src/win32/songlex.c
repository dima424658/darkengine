/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

/* n:\bin\lex -p songyy -l -o win32\songlex.c r:\prj\cam\src\songlang.lex */
#define YYNEWLINE 10
#define INITIAL 0
#define songyy_endst 117
#define songyy_nxtmax 561
#define YY_LA_SIZE 15

static unsigned short songyy_la_act[] = {
 0, 18, 18, 18, 18, 18, 18, 18, 18, 18, 19, 22, 20, 21, 19, 18,
 18, 18, 18, 18, 16, 18, 18, 18, 18, 18, 18, 18, 15, 18, 18, 18,
 18, 18, 18, 14, 18, 18, 18, 18, 18, 18, 13, 18, 18, 18, 17, 18,
 18, 18, 18, 18, 18, 18, 18, 18, 12, 18, 18, 18, 18, 18, 18, 18,
 18, 11, 18, 18, 18, 18, 18, 18, 18, 18, 10, 18, 18, 18, 18, 18,
 18, 18, 18, 18, 18, 18, 18, 18, 18, 4, 18, 18, 18, 18, 18, 18,
 18, 18, 18, 7, 18, 18, 18, 18, 6, 18, 9, 18, 18, 18, 18, 18,
 18, 8, 18, 18, 18, 5, 18, 18, 3, 18, 2, 1,
};

static unsigned char songyy_look[] = {
 0
};

static short songyy_final[] = {
 0, 0, 1, 1, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 10, 11,
 12, 13, 13, 14, 15, 16, 17, 18, 19, 20, 22, 23, 24, 25, 26, 27,
 28, 30, 31, 32, 33, 34, 35, 37, 38, 39, 40, 41, 42, 44, 45, 46,
 48, 49, 50, 51, 52, 53, 54, 55, 56, 58, 59, 60, 61, 62, 63, 64,
 65, 67, 68, 69, 70, 71, 72, 73, 74, 76, 77, 78, 79, 80, 81, 82,
 83, 84, 85, 86, 87, 88, 89, 91, 92, 93, 94, 95, 96, 97, 98, 99,
 101, 102, 103, 104, 106, 108, 109, 110, 111, 112, 113, 115, 116, 117, 119, 120,
 122, 122, 123, 123, 123, 124
};
#ifndef songyy_state_t
#define songyy_state_t unsigned char
#endif

static songyy_state_t songyy_begin[] = {
 0, 0, 0
};

static songyy_state_t songyy_next[] = {
 117, 117, 117, 117, 117, 117, 117, 117, 117, 1, 15, 117, 1, 117, 117, 117,
 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117,
 1, 117, 3, 1, 117, 117, 117, 117, 117, 117, 117, 117, 117, 13, 117, 2,
 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 114, 117, 117, 117, 117, 117,
 117, 12, 12, 12, 5, 12, 12, 10, 12, 7, 12, 12, 6, 12, 12, 9,
 8, 12, 12, 4, 12, 12, 11, 12, 12, 12, 12, 40, 68, 79, 117, 12,
 117, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 16, 21, 22, 23, 24,
 25, 26, 27, 28, 17, 31, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 19, 19, 19, 19, 19, 19,
 19, 19, 19, 19, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 32, 33,
 34, 35, 36, 37, 38, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 39,
 41, 42, 43, 20, 44, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 30,
 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 29, 59, 60,
 61, 45, 62, 63, 64, 65, 66, 67, 69, 70, 71, 72, 73, 74, 75, 76,
 77, 78, 80, 81, 82, 83, 84, 85, 86, 89, 91, 92, 93, 88, 94, 95,
 96, 90, 97, 98, 99, 100, 101, 87, 103, 104, 105, 106, 107, 102, 108, 109,
 110, 111, 112, 112, 112, 112, 112, 112, 112, 112, 112, 112, 113, 112, 112, 112,
 112, 112, 112, 112, 112, 112, 112, 112, 112, 112, 112, 112, 112, 112, 112, 112,
 112, 112, 112, 112, 113, 112, 112, 112, 112, 112, 112, 112, 112, 112, 112, 112,
 112, 112, 112, 112, 112, 112, 112, 112, 112, 112, 112, 112, 112, 112, 112, 112,
 112, 112, 112, 112, 112, 112, 112, 112, 112, 112, 112, 112, 112, 112, 112, 112,
 112, 112, 112, 112, 112, 112, 112, 112, 112, 112, 112, 112, 112, 112, 112, 112,
 112, 112, 112, 112, 112, 112, 112, 112, 112, 112, 112, 112, 112, 112, 112, 112,
 112, 112, 112, 112, 112, 112, 112, 112, 112, 112, 112, 112, 112, 112, 112, 112,
 112, 112, 115, 115, 115, 115, 115, 115, 115, 115, 115, 115, 116, 115, 115, 115,
 115, 115, 115, 115, 115, 115, 115, 115, 115, 115, 115, 115, 115, 115, 115, 115,
 115, 115, 115, 115, 115, 115, 115, 115, 115, 115, 115, 115, 115, 115, 115, 115,
 115, 115, 115, 115, 115, 115, 115, 115, 115, 115, 115, 115, 115, 115, 115, 115,
 115, 115, 115, 115, 115, 115, 115, 115, 115, 115, 115, 115, 115, 115, 115, 115,
 115, 115, 115, 115, 115, 115, 115, 115, 115, 115, 115, 115, 115, 115, 115, 115,
 115, 115, 115, 115, 115, 115, 115, 115, 115, 115, 115, 115, 115, 115, 115, 115,
 115, 115, 115, 115, 115, 115, 115, 115, 115, 115, 115, 115, 115, 115, 115, 115,
 115, 115,
};

static songyy_state_t songyy_check[] = {
 ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, 0, 0, ~0, 1, ~0, ~0, ~0,
 ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0,
 0, ~0, 0, 1, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, 0, ~0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, ~0, ~0, ~0, ~0, ~0,
 ~0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 39, 67, 78, ~0, 0,
 ~0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 14, 11, 21, 22, 23,
 24, 10, 26, 27, 14, 30, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 13, 13, 13, 13, 13, 13,
 13, 13, 13, 13, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 31, 29,
 33, 34, 35, 36, 37, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 9,
 40, 41, 42, 12, 43, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 28,
 8, 46, 45, 48, 49, 50, 51, 52, 53, 54, 55, 7, 57, 28, 58, 59,
 60, 8, 61, 62, 63, 6, 65, 66, 68, 69, 70, 71, 5, 73, 74, 75,
 76, 77, 79, 80, 81, 82, 83, 84, 85, 4, 90, 91, 92, 4, 93, 94,
 89, 4, 96, 97, 98, 99, 88, 4, 101, 103, 104, 105, 102, 101, 107, 108,
 87, 110, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
 3, 3, 114, 114, 114, 114, 114, 114, 114, 114, 114, 114, 114, 114, 114, 114,
 114, 114, 114, 114, 114, 114, 114, 114, 114, 114, 114, 114, 114, 114, 114, 114,
 114, 114, 114, 114, 114, 114, 114, 114, 114, 114, 114, 114, 114, 114, 114, 114,
 114, 114, 114, 114, 114, 114, 114, 114, 114, 114, 114, 114, 114, 114, 114, 114,
 114, 114, 114, 114, 114, 114, 114, 114, 114, 114, 114, 114, 114, 114, 114, 114,
 114, 114, 114, 114, 114, 114, 114, 114, 114, 114, 114, 114, 114, 114, 114, 114,
 114, 114, 114, 114, 114, 114, 114, 114, 114, 114, 114, 114, 114, 114, 114, 114,
 114, 114, 114, 114, 114, 114, 114, 114, 114, 114, 114, 114, 114, 114, 114, 114,
 114, 114,
};

static songyy_state_t songyy_default[] = {
 117, 117, 117, 117, 12, 12, 12, 12, 12, 12, 12, 12, 117, 117, 117, 117,
 117, 117, 17, 13, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
 3, 117, 117, 114, 117,
};

static short songyy_base[] = {
 0, 3, 11, 306, 184, 167, 150, 142, 143, 97, 18, 13, 116, 106, 86, 562,
 562, 96, 562, 562, 562, 17, 9, 18, 27, 562, 14, 20, 170, 74, 23, 74,
 562, 77, 61, 73, 68, 70, 562, 22, 90, 108, 100, 96, 562, 131, 131, 562,
 145, 147, 147, 141, 139, 143, 133, 129, 562, 143, 153, 155, 151, 161, 143, 159,
 562, 151, 151, 25, 153, 148, 156, 151, 562, 170, 162, 174, 158, 172, 10, 173,
 176, 160, 172, 167, 169, 165, 562, 194, 195, 179, 174, 182, 174, 187, 186, 562,
 178, 183, 191, 178, 562, 185, 195, 187, 198, 184, 562, 191, 193, 562, 202, 562,
 562, 562, 434, 562, 562, 562
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
 * You can redefine songyygetc. For YACC Tracing, compile this code
 * with -DYYTRACE to get input from yt_getc
 */
#ifdef YYTRACE
extern int	yt_getc YY_ARGS((void));
#define songyygetc()	yt_getc()
#else
#define	songyygetc()	getc(songyyin) 	/* songyylex input source */
#endif

/*
 * the following can be redefined by the user.
 */
#define	ECHO		fputs(songyytext, songyyout)
#define	output(c)	putc((c), songyyout) /* songyylex sink for unmatched chars */
#define	YY_FATAL(msg)	{ fprintf(stderr, "songyylex: %s\n", msg); exit(1); }
#define	YY_INTERACTIVE	1		/* save micro-seconds if 0 */
#define	YYLMAX		100		/* token and pushback buffer size */

/*
 * the following must not be redefined.
 */
#define	songyy_tbuf	songyytext		/* token string */

#define	BEGIN		songyy_start =
#define	REJECT		goto songyy_reject
#define	NLSTATE		(songyy_lastc = YYNEWLINE)
#define	YY_INIT \
	(songyy_start = songyyleng = songyy_end = 0, songyy_lastc = YYNEWLINE)
#define	songyymore()	goto songyy_more
#define	songyyless(n)	if ((n) < 0 || (n) > songyy_end) ; \
			else { YY_SCANNER; songyyleng = (n); YY_USER; }

YY_DECL	void	songyy_reset YY_ARGS((void));
YY_DECL	int	input	YY_ARGS((void));
YY_DECL	int	unput	YY_ARGS((int c));

/* functions defined in libl.lib */
extern	int	songyywrap	YY_ARGS((void));
extern	void	songyyerror	YY_ARGS((char *fmt, ...));
extern	void	songyycomment	YY_ARGS((char *term));
extern	int	songyymapch	YY_ARGS((int delim, int escape));



#include <songtok.h>
#include <string.h>
#include <mprintf.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

static int sgSongLineNum;
static char sgSongFilename[64];




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
#define	YYLEX songyylex			/* name of lex scanner */
#endif

#ifndef YYDECL
#define	YYDECL	int YYLEX YY_ARGS((void))	/* declaration for lex scanner */
#endif

/* stdin and stdout may not neccessarily be constants */
YY_DECL	FILE* songyyin = NULL;
YY_DECL	FILE* songyyout = NULL;
YY_DECL	int	songyylineno = 1;		/* line number */

/*
 * songyy_tbuf is an alias for songyytext.
 * songyy_sbuf[0:songyyleng-1] contains the states corresponding to songyy_tbuf.
 * songyy_tbuf[0:songyyleng-1] contains the current token.
 * songyy_tbuf[songyyleng:songyy_end-1] contains pushed-back characters.
 * When the user action routine is active,
 * songyy_save contains songyy_tbuf[songyyleng], which is set to '\0'.
 * Things are different when YY_PRESERVE is defined. 
 */

YY_DECL	char songyy_tbuf [YYLMAX+1]; /* text buffer (really songyytext) */
static	songyy_state_t songyy_sbuf [YYLMAX+1];	/* state buffer */

static	int	songyy_end = 0;		/* end of pushback */
static	int	songyy_start = 0;		/* start state */
static	int	songyy_lastc = YYNEWLINE;	/* previous char */
YY_DECL	int	songyyleng = 0;		/* songyytext token length */

#ifndef YY_PRESERVE	/* the efficient default push-back scheme */

static	char songyy_save;	/* saved songyytext[songyyleng] */

#define	YY_USER	{ /* set up songyytext for user */ \
		songyy_save = songyytext[songyyleng]; \
		songyytext[songyyleng] = 0; \
	}
#define	YY_SCANNER { /* set up songyytext for scanner */ \
		songyytext[songyyleng] = songyy_save; \
	}

#else		/* not-so efficient push-back for songyytext mungers */

static	char songyy_save [YYLMAX];
static	char *songyy_push = songyy_save+YYLMAX;

#define	YY_USER { \
		size_t n = songyy_end - songyyleng; \
		songyy_push = songyy_save+YYLMAX - n; \
		if (n > 0) \
			memmove(songyy_push, songyytext+songyyleng, n); \
		songyytext[songyyleng] = 0; \
	}
#define	YY_SCANNER { \
		size_t n = songyy_save+YYLMAX - songyy_push; \
		if (n > 0) \
			memmove(songyytext+songyyleng, songyy_push, n); \
		songyy_end = songyyleng + n; \
	}

#endif

/*
 * The actual lex scanner (usually songyylex(void)).
 * NOTE: you should invoke songyy_init() if you are calling songyylex()
 * with new input; otherwise old lookaside will get in your way
 * and songyylex() will die horribly.
 */
YYDECL {
	register int c, i, songyyst, songyybase;
	int songyyfmin, songyyfmax;	/* songyy_la_act indices of final states */
	int songyyoldi, songyyoleng;	/* base i, songyyleng before look-ahead */
	int songyyeof;		/* 1 if eof has already been read */



	songyyeof = 0;
	i = songyyleng;
	YY_SCANNER;

  songyy_again:
	songyyleng = i;
	/* determine previous char. */
	if (i > 0)
		songyy_lastc = songyytext[i-1];
	/* scan previously accepted token adjusting songyylineno */
	while (i > 0)
		if (songyytext[--i] == YYNEWLINE)
			songyylineno++;
	/* adjust pushback */
	songyy_end -= songyyleng;
	memmove(songyytext, songyytext+songyyleng, (size_t) songyy_end);
	i = 0;

  songyy_contin:
	songyyoldi = i;

	/* run the state machine until it jams */
	songyy_sbuf[i] = songyyst = songyy_begin[songyy_start + (songyy_lastc == YYNEWLINE)];
	do {
		YY_DEBUG(gettext("<state %d, i = %d>\n"), songyyst, i);
		if (i >= YYLMAX)
			YY_FATAL(gettext("Token buffer overflow"));

		/* get input char */
		if (i < songyy_end)
			c = songyy_tbuf[i];		/* get pushback char */
		else if (!songyyeof && (c = songyygetc()) != EOF) {
			songyy_end = i+1;
			songyy_tbuf[i] = c;
		} else /* c == EOF */ {
			c = EOF;		/* just to make sure... */
			if (i == songyyoldi) {	/* no token */
				songyyeof = 0;
				if (songyywrap())
					return 0;
				else
					goto songyy_again;
			} else {
				songyyeof = 1;	/* don't re-read EOF */
				break;
			}
		}
		YY_DEBUG(gettext("<input %d = 0x%02x>\n"), c, c);

		/* look up next state */
		while ((songyybase = songyy_base[songyyst]+c) > songyy_nxtmax || songyy_check[songyybase] != songyyst) {
			if (songyyst == songyy_endst)
				goto songyy_jammed;
			songyyst = songyy_default[songyyst];
		}
		songyyst = songyy_next[songyybase];
	  songyy_jammed: ;
	  songyy_sbuf[++i] = songyyst;
	} while (!(songyyst == songyy_endst || YY_INTERACTIVE && songyy_base[songyyst] > songyy_nxtmax && songyy_default[songyyst] == songyy_endst));
	YY_DEBUG(gettext("<stopped %d, i = %d>\n"), songyyst, i);
	if (songyyst != songyy_endst)
		++i;

  songyy_search:
	/* search backward for a final state */
	while (--i > songyyoldi) {
		songyyst = songyy_sbuf[i];
		if ((songyyfmin = songyy_final[songyyst]) < (songyyfmax = songyy_final[songyyst+1]))
			goto songyy_found;	/* found final state(s) */
	}
	/* no match, default action */
	i = songyyoldi + 1;
	output(songyy_tbuf[songyyoldi]);
	goto songyy_again;

  songyy_found:
	YY_DEBUG(gettext("<final state %d, i = %d>\n"), songyyst, i);
	songyyoleng = i;		/* save length for REJECT */
	
	/* pushback look-ahead RHS */
	if ((c = (int)(songyy_la_act[songyyfmin]>>9) - 1) >= 0) { /* trailing context? */
		unsigned char *bv = songyy_look + c*YY_LA_SIZE;
		static unsigned char bits [8] = {
			1<<0, 1<<1, 1<<2, 1<<3, 1<<4, 1<<5, 1<<6, 1<<7
		};
		while (1) {
			if (--i < songyyoldi) {	/* no / */
				i = songyyoleng;
				break;
			}
			songyyst = songyy_sbuf[i];
			if (bv[(unsigned)songyyst/8] & bits[(unsigned)songyyst%8])
				break;
		}
	}

	/* perform action */
	songyyleng = i;
	YY_USER;
	switch (songyy_la_act[songyyfmin] & 0777) {
	case 0:
	;
	break;
	case 1:
	{ sgSongLineNum++; }
	break;
	case 2:
	{
                     songyylval.strval = strdup (songyytext+1);
                     if (songyylval.strval[songyyleng-2] != '"')
                        // TBD : Better error handling.  Possibly return an "ERROR" token
                        // with a "strval" value indicating the error string?
                        mprintf ("Unterminated Character String\n");
                     else
                        songyylval.strval[songyyleng-2] = '\0'; /* Remove close quote. */
                     return QSTRING;
                  }
	break;
	case 3:
	{ return SONG; }
	break;
	case 4:
	{ return DECLARESECTIONS; }
	break;
	case 5:
	{ return SECTION; }
	break;
	case 6:
	{ return SAMPLE; }
	break;
	case 7:
	{ return SILENCE; }
	break;
	case 8:
	{ return SECONDS; }
	break;
	case 9:
	{ return SAMPLES; }
	break;
	case 10:
	{ return LOOPCOUNT; }
	break;
	case 11:
	{ return IMMEDIATE; }
	break;
	case 12:
	{ return PROBABILITY; }
	break;
	case 13:
	{ return ONEVENT; }
	break;
	case 14:
	{ return GOTOSECTION; }
	break;
	case 15:
	{ return GOTOEND; }
	break;
	case 16:
	{ return VOLUME; }
	break;
	case 17:
	{ return PAN; }
	break;
	case 18:
	{
                     songyylval.strval = strdup(songyytext);
                     return ID;
                  }
	break;
	case 19:
	{
                     songyylval.intval = atoi(songyytext);
                     return INT_NUM;
                  }
	break;
	case 20:
	{
                     songyylval.strval = strdup (songyytext);
                     songyylval.strval[songyyleng-1] = '\0'; /* Remove '%' */
                     songyylval.intval = atoi(songyylval.strval);
                     return PERCENT_NUM;
                  }
	break;
	case 21:
	{
                     songyylval.floatval = atof(songyytext);
                     return FLOAT_NUM;
                  }
	break;
	case 22:
	{ sgSongLineNum++; }
	break;


	}
	YY_SCANNER;
	i = songyyleng;
	goto songyy_again;			/* action fell though */

  songyy_reject:
	YY_SCANNER;
	i = songyyoleng;			/* restore original songyytext */
	if (++songyyfmin < songyyfmax)
		goto songyy_found;		/* another final state, same length */
	else
		goto songyy_search;		/* try shorter songyytext */

  songyy_more:
	YY_SCANNER;
	i = songyyleng;
	if (i > 0)
		songyy_lastc = songyytext[i-1];
	goto songyy_contin;
}
/*
 * Safely switch input stream underneath LEX
 */
typedef struct songyy_save_block_tag {
	FILE	* oldfp;
	int	oldline;
	int	oldend;
	int	oldstart;
	int	oldlastc;
	int	oldleng;
	char	savetext[YYLMAX+1];
	songyy_state_t	savestate[YYLMAX+1];
} YY_SAVED;

YY_SAVED *
songyySaveScan(fp)
FILE * fp;
{
	YY_SAVED * p;

	if ((p = (YY_SAVED *) malloc(sizeof(*p))) == NULL)
		return p;

	p->oldfp = songyyin;
	p->oldline = songyylineno;
	p->oldend = songyy_end;
	p->oldstart = songyy_start;
	p->oldlastc = songyy_lastc;
	p->oldleng = songyyleng;
	(void) memcpy(p->savetext, songyytext, sizeof songyytext);
	(void) memcpy((char *) p->savestate, (char *) songyy_sbuf,
		sizeof songyy_sbuf);

	songyyin = fp;
	songyylineno = 1;
	YY_INIT;

	return p;
}
/*f
 * Restore previous LEX state
 */
void
songyyRestoreScan(p)
YY_SAVED * p;
{
	if (p == NULL)
		return;
	songyyin = p->oldfp;
	songyylineno = p->oldline;
	songyy_end = p->oldend;
	songyy_start = p->oldstart;
	songyy_lastc = p->oldlastc;
	songyyleng = p->oldleng;

	(void) memcpy(songyytext, p->savetext, sizeof songyytext);
	(void) memcpy((char *) songyy_sbuf, (char *) p->savestate,
		sizeof songyy_sbuf);
	free(p);
}
/*
 * User-callable re-initialization of songyylex()
 */
void
songyy_reset()
{
	YY_INIT;
	songyylineno = 1;		/* line number */
}
/* get input char with pushback */
YY_DECL int
input()
{
	int c;
#ifndef YY_PRESERVE
	if (songyy_end > songyyleng) {
		songyy_end--;
		memmove(songyytext+songyyleng, songyytext+songyyleng+1,
			(size_t) (songyy_end-songyyleng));
		c = songyy_save;
		YY_USER;
#else
	if (songyy_push < songyy_save+YYLMAX) {
		c = *songyy_push++;
#endif
	} else
		c = songyygetc();
	songyy_lastc = c;
	if (c == YYNEWLINE)
		songyylineno++;
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
	if (songyy_end >= YYLMAX)
		YY_FATAL(gettext("Push-back buffer overflow"));
	if (songyy_end > songyyleng) {
		songyytext[songyyleng] = songyy_save;
		memmove(songyytext+songyyleng+1, songyytext+songyyleng,
			(size_t) (songyy_end-songyyleng));
		songyytext[songyyleng] = 0;
	}
	songyy_end++;
	songyy_save = c;
#else
	if (songyy_push <= songyy_save)
		YY_FATAL(gettext("Push-back buffer overflow"));
	*--songyy_push = c;
#endif
	if (c == YYNEWLINE)
		songyylineno--;
	return c;
}


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
