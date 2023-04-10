/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/script/scrptst.h,v 1.1 1998/03/05 08:47:52 MAT Exp $

/* --<<= --/-/-/-/-/-/-/ <<< ((( ((( /\ ))) ))) >>> \-\-\-\-\-\-\-- =>>-- *\

   scrptst.h

   script saving and loading

\* --<<= --\-\-\-\-\-\-\ <<< ((( ((( \/ ))) ))) >>> /-/-/-/-/-/-/-- =>>-- */

#pragma once
#ifndef _SCRPTST_H_
#define _SCRPTST_H_

#include <tagfile.h>


extern void ScriptStateSave(ITagFile *pFile);
extern void ScriptStateLoad(ITagFile *pFile);

#endif // ~_SCRPTST_H_

