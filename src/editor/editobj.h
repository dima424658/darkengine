/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/editobj.h,v 1.20 2000/01/29 13:11:44 adurant Exp $
// editor object code
// specifically for render/creation
#pragma once

#ifndef __EDITOBJ_H
#define __EDITOBJ_H

#include <osystype.h>
#include <editbr.h>

////////////////////////////////////////////////////////////
// EDITOR-SPECIFIC OBJECT LAYER
//

//------------------------------------------------------------
// PROPERTY-SAVVY STUFF
//


EXTERN void editObjInit(void);
EXTERN void editObjTerm(void);

// 
// Set the archetype that will be used for subsequent object creation
//
EXTERN void  editObjSetDefaultArchetype(ObjID archetype);
EXTERN ObjID editObjGetDefaultArchetype(void);

// place and object from scratch, OBJ_NULL archetype means default
EXTERN BOOL editObjCreateObjFromBrush(ObjID archetype, editBrush *us);

// set an object's brush without syncronizing.  
EXTERN BOOL editObjSetObjectBrush(ObjID id, editBrush* us);

// Looks up an object's brush
EXTERN editBrush *editObjGetBrushFromObj(ObjID usID);


EXTERN void editObjDeleteObjBrush(ObjID obj);

//------------------------------------------------------------
// THE OLD STUFF: Yet to be propertized.
//

// Icky nasty global for if you want an edit brush to be created on obj create
EXTERN BOOL gCreateEditBrush;

// Create a brush from an object.  Should ideally be called by a property callback.
EXTERN bool editobjCreateBrushfromObj(int usID);

// update an existing obj
EXTERN bool editobjUpdateObjfromBrush(editBrush *usbr);
EXTERN bool editobjUpdateBrushfromObj(int usID);
EXTERN void editobjUpdateBrushFromObjScale(int brID, mxs_vector *scale);


// fully deref or replace all objs (for portalization)
EXTERN void editobjFullDeref(void);
EXTERN void editobjFullReref(void);

// Set whether an object can have a brush
EXTERN BOOL ObjCanHaveBrush(ObjID obj);
EXTERN void ObjSetCanHaveBrush(ObjID obj, BOOL can_have); 
EXTERN void ObjUnsetCanHaveBrush(ObjID obj); 



#endif // __EDITOBJ_H



