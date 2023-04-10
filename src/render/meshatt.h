/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/meshatt.h,v 1.3 1998/05/22 19:32:06 dc Exp $
#pragma once
#ifndef _MESHATT_H_
#define _MESHATT_H_

/* --<<= --/-/-/-/-/-/-/ <<< ((( ((( /\ ))) ))) >>> \-\-\-\-\-\-\-- =>>-- *\
   meshatt.h
   attaching things to mesh objects--interface/property business
\* --<<= --\-\-\-\-\-\-\ <<< ((( ((( \/ ))) ))) >>> /-/-/-/-/-/-/-- =>>-- */

#include <mm.h>
#include <propdef.h>
#include <meshatts.h>

#undef INTERFACE
#define INTERFACE IMeshAttachProperty
DECLARE_PROPERTY_INTERFACE(IMeshAttachProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sMeshAttach*); 
}; 

#define PROP_MESH_ATTACH_DATA_NAME "MeshAttach"

// init the property itself and such
EXTERN BOOL MeshAttachPropInit(void);

// call this to say "im about to render a mesh model, check attaches"
EXTERN mms_custom_data *MeshAttachRenderSetup(ObjID obj, mms_model *pModel, uchar *clut);

// @TODO: What else do we want in this interface?
EXTERN BOOL ObjMeshAttachGet(ObjID obj, sMeshAttach **ppAttach);
EXTERN BOOL ObjMeshAttachSet(ObjID obj, sMeshAttach *pAttach);

// attach and remove from a mesh object... attached_obj
//   is the objid from the meshattachinstance
EXTERN BOOL AttachObjToMeshObj(ObjID mesh_obj, sMeshAttachInstance *mData);
EXTERN BOOL RemoveObjFromMeshObj(ObjID mesh_obj, ObjID attached_obj);


#endif // ~_MESHATT_H_
