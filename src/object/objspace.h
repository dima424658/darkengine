/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/object/objspace.h,v 1.1 2000/02/25 00:03:47 adurant Exp $
//
#include <comtools.h>

#ifndef INCLUDED_OBJSPACE_H
#define INCLUDED_OBJSPACE_H

#pragma once

///////////////////////////////////////////////////////////////////////////////
//
// Object Space Agg Member
//
// Handles changes to the min and max of the object system
//
// @NOTE: This is totally a retrofit for Thief 2, hence the new, tiny interface.
//

typedef long tObjIdx;  // Fancy way of saying ObjID

struct sObjBounds
{
   tObjIdx min;  // lowest abstract
   tObjIdx max;  // highest concrete
}; 

F_DECLARE_INTERFACE(IObjIDManager); 
F_DECLARE_INTERFACE(IObjIDSink); 


////////////////////////////////////////////////////////////
//
// INTERFACE: IObjIDManager
//

interface IObjIDManager: IUnknown
{
   //
   // Get the bounds of the ObjID space
   //
   virtual sObjBounds GetObjIDBounds() = 0; 

   //
   // Change the size of the local ObjID space.  Possibly expensive.
   //
   virtual tResult ResizeObjIDSpace(const sObjBounds& bounds) = 0; 

   //
   // Connect/Disconnect a sink
   //
   virtual tResult Connect(IObjIDSink* pSink) = 0; 
   virtual tResult Disconnect(IObjIDSink* pSink) = 0; 



}; 

////////////////////////////////////////////////////////////
//
// INTERFACE: IObjIDSink
//

interface IObjIDSink: IUnknown
{
   //
   // The ObjID space has been resized
   //
   virtual void OnObjIDSpaceResize(const sObjBounds& bounds) = 0; 
}; 

//------------------------------------------------------------
// CLASS: cObjIDSink
//
// Base implementation of IObjIDSink
//

class cObjIDSink: public cCTUnaggregated<IObjIDSink,&IID_IObjIDSink,kCTU_Default>
{
   virtual void OnObjIDSpaceResize(const sObjBounds& bounds) {}; 
}; 





///////////////////////////////////////////////////////////////////////////////

#endif /* !INCLUDED_OBJSPACE_H */
