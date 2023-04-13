/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: x:/prj/tech/libsrc/sdesc/RCS/isdesced.h 1.5 1999/01/27 17:33:43 mahk Exp $
#pragma once  
#ifndef __ISDESCED_H
#define __ISDESCED_H

#include <comtools.h>
#include <sdestype.h>
#include <isdescty.h>

////////////////////////////////////////////////////////////
// Struct Desc Editor API
//
// Mostly COM for interoperability
//

F_DECLARE_INTERFACE(IStructEditor);

enum eSdescModality_
{
   kStructEdModal,
   kStructEdModeless, 
};

typedef ulong eSdescModality;


#undef INTERFACE
#define INTERFACE IStructEditor
DECLARE_INTERFACE_(IStructEditor,IUnknown)
{
   DECLARE_UNKNOWN_PURE();

   //
   // Start the editor.  Call this one.
   // If Modal, returns TRUE iff the ok button was pushed. 
   // If Modeless, returns FALSE immediately
   //
   STDMETHOD_(BOOL,Go)(THIS_ eSdescModality modal) PURE;

#define IStructEditor_Go(p, a) COMCall1(p, Go, a) 

   //
   // Set the callback to be called when buttons get pushed
   //
   STDMETHOD(SetCallback)(THIS_ StructEditCB cb, StructEditCBData data) PURE;

#define IStructEditor_SetCallback(p, a, b) COMCall2(p, SetCallback, a, b)

   //
   // Accessors
   //
   STDMETHOD_(const sStructEditorDesc*,Describe)(THIS) PURE;
   STDMETHOD_(const sStructDesc*,DescribeStruct)(THIS) PURE;
   STDMETHOD_(void*,Struct)(THIS) PURE;

};
 
#define IStructEditor_Describe(p)         COMCall0(p, Describe)
#define IStructEditor_DescribeStruct(p)   COMCall0(p, DescribeStruct)
#define IStructEditor_Struct(p)           COMCall0(p, Struct)

#undef INTERFACE

EXTERN IStructEditor* CreateStructEditor(const sStructEditorDesc* eddesc, const sStructDesc* sdesc, void* editme); 

#endif // __ISDESCED_H
 
