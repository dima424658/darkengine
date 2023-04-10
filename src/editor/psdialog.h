/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once

#include <aipsdscr.h>
#include <isdescty.h>
#include <isdescst.h>
#include <isdesced.h>
#include <sdesbase.h>

IStructEditor* NewPseudoScriptDialog (char* title, int maxSteps, sStructDesc* headerStruct, void* data, sAIPsdScrAct* steps);

