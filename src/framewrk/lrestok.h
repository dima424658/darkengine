/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/lrestok.h,v 1.2 2000/01/29 13:21:15 adurant Exp $
// lresname token setup/support defines
// the actual declarations are in lresname.h
#pragma once

#ifndef __LRESTOK_H
#define __LRESTOK_H

// Tokenized path support
// note: token idx's start at 1, so 0 can mean no token, so memset 0 works
#define LRES_MAX_TOKENS       64 
#define LRES_MAX_TOK_LEN      16

#define LRES_NO_TOKEN         0

EXTERN char  lres_rn_tokens[LRES_MAX_TOKENS][LRES_MAX_TOK_LEN];
EXTERN short lres_rn_tok_lock[LRES_MAX_TOKENS];
#define rn_GetTokenStr(i)  (lres_rn_tokens[i-1])
#define rn_GetTokenLock(i) (lres_rn_tok_lock[i-1])

#define _rn_UseTokIdx(i)   (rn_GetTokenLock(i)++)

// both in lresname, used by lresdisk and lresname
// looks in current token list for this string
char _rn_FindTokenStr(char *token);

// say that i need to use a "token".  returns the tok idx for it.
// if it isnt already a token, makes it one
// if it is, incs its lock cnt, returns it
char _rn_UseTokenStr(char *token);

// release a usage of this idx, if this makes the count 0, free it
void _rn_RelTokIdx(char idx);

#endif //  __LRESTOK_H
