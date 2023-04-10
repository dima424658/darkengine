/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/prompts.h,v 1.2 2000/01/29 13:13:01 adurant Exp $
// basic prompts for asking for data from the user
#pragma once

#ifndef __PROMPTS_H
#define __PROMPTS_H

// basic "get this value for me" prompts - all modal
EXTERN BOOL   prompt_bool(char *def);
EXTERN int    prompt_int(char *def);
EXTERN double prompt_double(char *def);
EXTERN char  *prompt_string(char *def, char *buf);

// for prompt strings
#define PROMPT_STR_LEN 128

#endif  // __PROMPTS_H
