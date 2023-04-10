/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/actreact/reactype.h,v 1.3 2000/01/29 12:44:37 adurant Exp $
#pragma once

#ifndef __REACTYPE_H
#define __REACTYPE_H

#include <senstype.h>

//
// Scalars and cookies
//
typedef ulong ReactionID; // Unique effect ID
#define REACTION_NULL 0xFFFFFFFF

typedef ulong eReactionResult; 
typedef void* tReactionFuncData; 
typedef struct sReactionParam sReactionParam;
typedef ulong eReactObjParam;

//
// Structures
//

typedef struct sReactionDesc sReactionDesc;
typedef struct sReactionEvent sReactionEvent;

//
// Functions
//

typedef eReactionResult (LGAPI * ReactionFunc)(sReactionEvent* event, const sReactionParam* param, tReactionFuncData data); 




#endif // __REACTYPE_H





