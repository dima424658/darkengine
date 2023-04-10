/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sound/vocprop.h,v 1.2 2000/01/31 10:02:58 adurant Exp $
#pragma once

/* <<--- /-/-/-/-/-/-/-/ <<< (( / (( /\ )) \ )) >>> \-\-\-\-\-\-\-\ --->> *\
   vocprop.h

   A voice is an abstract object with two propeties: SynName and this.
   All this gives us is a handle into spch.

\* <<--- \-\-\-\-\-\-\-\ <<< (( \ (( \/ )) / )) >>> /-/-/-/-/-/-/-/ --->> */


#ifndef _VOCPROP_H_
#define _VOCPROP_H_

#include <lg.h>
#include <objtype.h>


#define PROP_VOICE_INDEX_NAME "VoiceIndex"

EXTERN BOOL ObjGetVoiceIndex(ObjID obj, int* iIndex);
EXTERN void ObjSetVoiceIndex(ObjID obj, int iIndex);

#endif // ~_VOCPROP_H_
