/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sound/esndglue.h,v 1.4 2000/01/31 10:03:02 adurant Exp $
#pragma once

/* <<--- /-/-/-/-/-/-/-/ <<< (( / (( /\ )) \ )) >>> \-\-\-\-\-\-\-\ --->> *\
   esndglue.h

   These are the elements of the environmental sound system which need
   to be exported as C.

\* <<--- \-\-\-\-\-\-\-\ <<< (( \ (( \/ )) / )) >>> /-/-/-/-/-/-/-/ --->> */


#ifndef _ESNDGLUE_H_
#define _ESNDGLUE_H_


#include <objtype.h>
#include <dispatch.h>


/* <<--- /-/-/-/-/-/-/-/ <<< (( / (( /\ )) \ )) >>> \-\-\-\-\-\-\-\ --->> *\

   For each tag we're working with we need to keep track of where we
   got it: the current room, our first object, our second object, the
   app proper.  We stuff this into the upper byte of each tag type
   field in our database.

\* <<--- \-\-\-\-\-\-\-\ <<< (( \ (( \/ )) / )) >>> /-/-/-/-/-/-/-/ --->> */


// app-level flow control
EXTERN void ESndDatabaseNotify(DispatchData *msg);
EXTERN void ESndInit(void);
EXTERN void ESndTerm(void);

EXTERN void ESndSetTagRequired(const Label *pTagName);

// These functions amount to an iterator.
EXTERN void ESndSchemaNewStart(const Label *pSchemaName, ObjID SchemaObjID);
EXTERN void ESndSchemaNewAddEnumTag(const Label *pTagName,
                                    const Label *pEnum);
EXTERN void ESndSchemaNewAddIntTag(const Label *pTagName,
                                   int iMin, int iMax);
EXTERN void ESndSchemaNewFinish(void);

// destroy all databases
EXTERN void ESndDestroy(void);

#ifndef SHIP
EXTERN void ESndDump();
#endif

#endif // ~_ESNDGLUE_H_
