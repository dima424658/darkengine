/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
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
typedef union {
   char *strval;
   int  intval;
   float floatval;
} SONGYYSTYPE;
extern SONGYYSTYPE songyylval;
