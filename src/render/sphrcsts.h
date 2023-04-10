/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/sphrcsts.h,v 1.6 2000/01/31 09:53:25 adurant Exp $
#pragma once

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   sphrcsts.h

   structure/enum definitions for spherecaster

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */


#ifndef _SPHRCSTS_H_
#define _SPHRCSTS_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <sphrcstt.h>

// This is a secondary structure, reached through sSphrContact, below.
// Every polygon collision has one of these.  Every edge has two, and
// every vertex, any number.  They are organized in linked lists,
// using indices into their array rather than pointers.
#define NO_NEXT_CONTACT_DATA -1
struct sSphrContactData {
   ushort cell_index;
   ushort polygon_index;
   ushort vertex_offset;
   uchar vertex_in_poly;
   char next;
};


// Here's our main structure.  Each contact can yield any number of
// these, though most will yield zero or one.
struct sSphrContact {
   sSphrContactKind kind;
   int first_contact_data;      // index into array of sSphrContactData
   mxs_vector point_in_world;
   mxs_vector point_on_ray;
   mxs_real time;               // time along cast
};

#ifdef __cplusplus
};
#endif

#endif
