/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
typedef union {
   char *strval;
   int  ival;
} INCSTYPE;
#define INCLUDE	257
#define DEFINE	258
#define INT	259
#define STRING	260
#define IDENT	261
extern INCSTYPE inclval;
