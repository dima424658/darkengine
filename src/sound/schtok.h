/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
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
extern YYSTYPE yylval;
