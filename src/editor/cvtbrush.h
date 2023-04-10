/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/cvtbrush.h,v 1.5 2000/01/29 13:11:23 adurant Exp $
#pragma once

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\
   cvtbrush.h

   export file

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */


#ifndef _CVTBRUSH_H_
#define _CVTBRUSH_H_

#include <wr.h>

EXTERN uchar ConvertFindFinalMedium(uchar medium);
EXTERN void ConvertRemovePersistance(PortalCell *cell, uchar final_medium);
EXTERN void ConvertPersistantCells(uchar added_flags);

#endif
