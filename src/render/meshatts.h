/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/meshatts.h,v 1.3 2000/01/29 13:39:09 adurant Exp $
#pragma once

/* --<<= --/-/-/-/-/-/-/ <<< ((( ((( /\ ))) ))) >>> \-\-\-\-\-\-\-- =>>-- *\
   meshatt.h

   attaching things to mesh objects--data structure

\* --<<= --\-\-\-\-\-\-\ <<< ((( ((( \/ ))) ))) >>> /-/-/-/-/-/-/-- =>>-- */


#ifndef _MESHATTS_H_
#define _MESHATTS_H_

#include <matrixs.h>


typedef struct sMeshAttachInstance
{
   ObjID m_Object;      // what's being attached
   int m_iJoint;        // what part of the mesh it's stuck to
   mxs_angvec m_Angles; // orientation in segment space
   ushort m_Flags;      // none yet, but we had 16 bits free
   mxs_vector m_Offset; // position in segment space
} sMeshAttachInstance;


// This is not part of the property itself; it's allocated when the
// property is created.
typedef struct sMeshAttachAccessory
{
   BOOL m_bAttachmentCurrent;
} sMeshAttachAccessory;


// If you change this, you'll also have to fiddle with the SDESC in
// meshatt.cpp.
#define kMeshAttachMax 4

typedef struct sMeshAttach
{
   mms_custom_data *m_pCustom;          // invisible to user
   sMeshAttachAccessory *m_pAccessory;
   sMeshAttachInstance m_Attachment[kMeshAttachMax];
} sMeshAttach;


#endif // ~_MESHATTS_H_
