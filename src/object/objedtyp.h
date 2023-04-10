/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/objedtyp.h,v 1.4 1997/12/04 15:03:36 mahk Exp $
#pragma once  
#ifndef __OBJEDTYP_H
#define __OBJEDTYP_H

#include <comtools.h>
F_DECLARE_INTERFACE(IObjEditor);
F_DECLARE_INTERFACE(IObjEditors); 

//
// Object editor types
//

enum eObjEdModality_
{
   kObjEdModal,
   kObjEdModeless, 
};

typedef ulong eObjEdModality; 

typedef struct sObjEditorDesc sObjEditorDesc;
typedef struct sObjEditorEvent sObjEditorEvent; 

typedef void* ObjEditCBData;
typedef void (LGAPI* ObjEditCB)(sObjEditorEvent* event, ObjEditCBData data);

typedef struct sEditTraitIter sEditTraitIter; 


#endif // __OBJEDTYP_H

