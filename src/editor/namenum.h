/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/namenum.h,v 1.3 2000/01/29 13:12:48 adurant Exp $
// library for numname association for string tables
#pragma once

#ifndef __NAMENUM_H
#define __NAMENUM_H

#include <tagfile.h>

// forward declare it
typedef struct _namenum sNameNum;

typedef int str_hnd;

// couldnt find a handle
#define NAMENUM_NO_HND (-1)

// well, tags have to have some "no" value, in the current impl
#define NAMENUM_NO_TAG (-1)

// initializes a namenum table size cnt named "sys_name"
EXTERN sNameNum *NamenumInit(char *sys_name, int cnt);

// Free the NameNum and all its data
EXTERN void      NamenumFree(sNameNum *ss);
   
// saves ss to current TagFile (tagname "nn_"sys_name)
EXTERN BOOL      NamenumSave(sNameNum *ss, ITagFile *file);

// loads and initializes sys_name as a namenum system, return NULL if not found
EXTERN sNameNum *NamenumLoad(char *sys_name, ITagFile *file);
   
// store "str" in the system with "tag", return the handle
EXTERN str_hnd   NamenumStore(sNameNum *ss, char *str, int tag);

// clear hnd from the system
EXTERN void      NamenumClear(sNameNum *ss, str_hnd hnd);

// clear tag from the system
EXTERN void      NamenumClearByTag(sNameNum *ss, int tag);

// retrieve the string using the system handle given by store
EXTERN char     *NamenumFetch(sNameNum *ss, str_hnd hnd);

// retrieve the string using the tag given with creation
EXTERN char     *NamenumFetchByTag(sNameNum *ss, int tag);

// get the tag for this string
EXTERN int       NamenumFetchTagForString(sNameNum *ss, char *name);

// returns number of entries found - _newly mallocs_ *str_list
// if tags isn't NULL, it also fills it with a malloced list of tags
// NOTE: this seemed better to me than an iterator?  maybe not
EXTERN int       NamenumAll(sNameNum *ss, char ***str_list, int **tags);

// applys mapping to all tags in ss
EXTERN void      NamenumRetag(sNameNum *ss, int *mapping);

// return -1 if abort, else tag
EXTERN int       NamenumSimpleMenuChooseTag(sNameNum *ss);

#endif  // __NAMENUM_H
