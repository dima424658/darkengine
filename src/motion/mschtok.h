/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
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
extern MSCHSTYPE mschlval;
