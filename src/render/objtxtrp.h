/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/objtxtrp.h,v 1.1 1998/08/03 18:58:41 dc Exp $
// control for doing replacable textures on objects

#pragma once

#ifndef __OBJTXTRP_CPP
#define __OBJTXTRP_CPP

#include <objtype.h>

#define MAX_REPL_TXT 4

// call at runtime - return a MAX_REPL_TXT array of IRes *'s
EXTERN IRes **ObjTxtReplaceForObj(ObjID obj);

// for dropping textures refered to by TxtReplace
EXTERN void ObjTxtReplaceDropIRes(IRes *drop_me);
EXTERN void ObjTxtReplaceDropAll(void);

// setup/closedown
EXTERN void ObjTxtReplaceInit(void);
EXTERN void ObjTxtReplaceTerm(void);

#endif  // __OBJTXTRP_CPP
