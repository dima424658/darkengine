/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/sphrcstt.h,v 1.5 2000/01/31 09:53:26 adurant Exp $
#pragma once

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   sphrcstt.h

   type declaratons for spherecaster

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */

#ifndef _SPHRCSTT_H_
#define _SPHRCSTT_H_

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct sSphrContactData sSphrContactData;
typedef struct sSphrContact sSphrContact;

// moved here for watcom compilation

// We can bump into the terrain in three different ways.  Some casts
// can even result in combinations of these if we're bouncing around
// complicated corners.
typedef enum sSphrContactKind {
   kSphrPolygon = 65536, kSphrEdge, kSphrVertex
} sSphrContactKind;

#ifdef __cplusplus
};
#endif

#endif
